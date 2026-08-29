#include "miqutoolkit/view/frame_layout.hpp"

namespace miqu {

Size FrameLayout::measure_size() const {
    int max_w = 0;
    int max_h = 0;

    for (const auto& child : m_children) {
        if (!child || child->get_visibility() == Visibility::Gone) continue;

        Size child_size = child->measure_size();
        const auto& margin = child->get_margin();
        int child_total_w = child_size.width + margin.left + margin.right;
        int child_total_h = child_size.height + margin.top + margin.bottom;

        max_w = std::max(max_w, child_total_w);
        max_h = std::max(max_h, child_total_h);
    }

    max_w += m_padding.left + m_padding.right + m_margin.left + m_margin.right;
    max_h += m_padding.top + m_padding.bottom + m_margin.top + m_margin.bottom;

    return Size(max_w, max_h);
}

void FrameLayout::draw(cairo_t* cr, const Rect& bounds) {
    if (!is_visible() || !cr || bounds.width <= 0 || bounds.height <= 0) return;

    m_child_entries.clear();

    Rect content_rect = get_content_rect(bounds);
    int avail_w = std::max(0, content_rect.width - m_margin.left - m_margin.right);
    int avail_h = std::max(0, content_rect.height - m_margin.top - m_margin.bottom);
    int origin_x = content_rect.x + m_margin.left;
    int origin_y = content_rect.y + m_margin.top;

    if (avail_w <= 0 || avail_h <= 0) return;

    for (const auto& child : m_children) {
        if (!child || child->get_visibility() == Visibility::Gone) continue;

        const auto& params = child->get_layout_params();
        const auto& margin = child->get_margin();
        Size measured = child->measure_size();

        int child_w = 0;
        int child_h = 0;

        if (params.width == static_cast<int>(LayoutDimension::MatchParent)) {
            child_w = std::max(0, avail_w - margin.left - margin.right);
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

        int child_x = origin_x + margin.left;
        int child_y = origin_y + margin.top;

        Gravity g = params.gravity;
        if (g & Gravity::CenterHorizontal) {
            child_x = origin_x + (avail_w - child_w) / 2;
        } else if (g & Gravity::Right) {
            child_x = origin_x + avail_w - child_w - margin.right;
        }

        if (g & Gravity::CenterVertical) {
            child_y = origin_y + (avail_h - child_h) / 2;
        } else if (g & Gravity::Bottom) {
            child_y = origin_y + avail_h - child_h - margin.bottom;
        }

        Rect child_bounds(child_x, child_y, child_w, child_h);
        m_child_entries.push_back({child, child_bounds});
        child->draw(cr, child_bounds);
    }
}

} // namespace miqu
