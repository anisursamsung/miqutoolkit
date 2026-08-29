#include "biwaytoolkit/view/linear_layout.hpp"
#include <numeric>
#include <algorithm>

namespace biway {

Size LinearLayout::measure_size() const {
    int total_w = 0;
    int total_h = 0;
    int visible_child_count = 0;

    for (const auto& child : m_children) {
        if (!child || child->get_visibility() == Visibility::Gone) continue;

        Size child_size = child->measure_size();
        const auto& margin = child->get_margin();
        int child_total_w = child_size.width + margin.left + margin.right;
        int child_total_h = child_size.height + margin.top + margin.bottom;

        if (m_orientation == Orientation::Horizontal) {
            total_w += child_total_w;
            total_h = std::max(total_h, child_total_h);
        } else {
            total_w = std::max(total_w, child_total_w);
            total_h += child_total_h;
        }
        visible_child_count++;
    }

    if (visible_child_count > 1) {
        if (m_orientation == Orientation::Horizontal) {
            total_w += (visible_child_count - 1) * m_spacing;
        } else {
            total_h += (visible_child_count - 1) * m_spacing;
        }
    }

    total_w += m_padding.left + m_padding.right + m_margin.left + m_margin.right;
    total_h += m_padding.top + m_padding.bottom + m_margin.top + m_margin.bottom;

    return Size(total_w, total_h);
}

void LinearLayout::draw(cairo_t* cr, const Rect& bounds) {
    if (!is_visible() || !cr || bounds.width <= 0 || bounds.height <= 0) return;

    m_child_entries.clear();

    Rect content_rect = get_content_rect(bounds);
    int avail_w = std::max(0, content_rect.width - m_margin.left - m_margin.right);
    int avail_h = std::max(0, content_rect.height - m_margin.top - m_margin.bottom);
    int origin_x = content_rect.x + m_margin.left;
    int origin_y = content_rect.y + m_margin.top;

    if (avail_w <= 0 || avail_h <= 0) return;

    // First pass: Calculate fixed space and total weight
    float total_weight = 0.0f;
    int used_fixed_space = 0;
    int visible_child_count = 0;

    for (const auto& child : m_children) {
        if (!child || child->get_visibility() == Visibility::Gone) continue;

        const auto& params = child->get_layout_params();
        const auto& margin = child->get_margin();

        if (params.weight > 0.0f) {
            total_weight += params.weight;
            if (m_orientation == Orientation::Horizontal) {
                used_fixed_space += margin.left + margin.right;
            } else {
                used_fixed_space += margin.top + margin.bottom;
            }
        } else {
            Size measured = child->measure_size();
            if (m_orientation == Orientation::Horizontal) {
                int child_w = (params.width >= 0) ? params.width : measured.width;
                used_fixed_space += child_w + margin.left + margin.right;
            } else {
                int child_h = (params.height >= 0) ? params.height : measured.height;
                used_fixed_space += child_h + margin.top + margin.bottom;
            }
        }
        visible_child_count++;
    }

    if (visible_child_count > 1) {
        used_fixed_space += (visible_child_count - 1) * m_spacing;
    }

    int total_avail_axis = (m_orientation == Orientation::Horizontal) ? avail_w : avail_h;
    int remaining_space = std::max(0, total_avail_axis - used_fixed_space);

    // Second pass: Allocate and draw
    int current_cursor = 0;

    for (const auto& child : m_children) {
        if (!child || child->get_visibility() == Visibility::Gone) continue;

        const auto& params = child->get_layout_params();
        const auto& margin = child->get_margin();
        Size measured = child->measure_size();

        int child_w = 0;
        int child_h = 0;

        if (m_orientation == Orientation::Horizontal) {
            if (params.weight > 0.0f && total_weight > 0.0f) {
                child_w = static_cast<int>((params.weight / total_weight) * remaining_space);
            } else if (params.width >= 0) {
                child_w = params.width;
            } else if (measured.width > 0) {
                child_w = measured.width;
            } else {
                child_w = std::max(0, avail_w - margin.left - margin.right);
            }

            if (params.height == static_cast<int>(LayoutDimension::MatchParent)) {
                child_h = std::max(0, avail_h - margin.top - margin.bottom);
            } else if (params.height >= 0) {
                child_h = params.height;
            } else if (measured.height > 0) {
                child_h = measured.height;
            } else {
                child_h = std::max(0, avail_h - margin.top - margin.bottom);
            }

            int child_x = origin_x + current_cursor + margin.left;
            int child_y = origin_y + margin.top;

            Gravity g = (params.gravity != Gravity::None) ? params.gravity : m_gravity;
            if (g & Gravity::CenterVertical) {
                child_y = origin_y + (avail_h - child_h) / 2;
            } else if (g & Gravity::Bottom) {
                child_y = origin_y + avail_h - child_h - margin.bottom;
            }

            Rect child_bounds(child_x, child_y, child_w, child_h);
            m_child_entries.push_back({child, child_bounds});
            child->draw(cr, child_bounds);

            current_cursor += child_w + margin.left + margin.right + m_spacing;
        } else {
            // Vertical Orientation
            if (params.weight > 0.0f && total_weight > 0.0f) {
                child_h = static_cast<int>((params.weight / total_weight) * remaining_space);
            } else if (params.height >= 0) {
                child_h = params.height;
            } else if (measured.height > 0) {
                child_h = measured.height;
            } else {
                child_h = std::max(0, avail_h - margin.top - margin.bottom);
            }

            if (params.width == static_cast<int>(LayoutDimension::MatchParent)) {
                child_w = std::max(0, avail_w - margin.left - margin.right);
            } else if (params.width >= 0) {
                child_w = params.width;
            } else if (measured.width > 0) {
                child_w = measured.width;
            } else {
                child_w = std::max(0, avail_w - margin.left - margin.right);
            }

            int child_x = origin_x + margin.left;
            int child_y = origin_y + current_cursor + margin.top;

            Gravity g = (params.gravity != Gravity::None) ? params.gravity : m_gravity;
            if (g & Gravity::CenterHorizontal) {
                child_x = origin_x + (avail_w - child_w) / 2;
            } else if (g & Gravity::Right) {
                child_x = origin_x + avail_w - child_w - margin.right;
            }

            Rect child_bounds(child_x, child_y, child_w, child_h);
            m_child_entries.push_back({child, child_bounds});
            child->draw(cr, child_bounds);

            current_cursor += child_h + margin.top + margin.bottom + m_spacing;
        }
    }
}

} // namespace biway
