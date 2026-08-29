#include "miqutoolkit/view/relative_layout.hpp"
#include <map>

namespace miqu {

Size RelativeLayout::measure_size() const {
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

void RelativeLayout::draw(cairo_t* cr, const Rect& bounds) {
    if (!is_visible() || !cr || bounds.width <= 0 || bounds.height <= 0) return;

    m_child_entries.clear();

    Rect content_rect = get_content_rect(bounds);
    int avail_w = std::max(0, content_rect.width - m_margin.left - m_margin.right);
    int avail_h = std::max(0, content_rect.height - m_margin.top - m_margin.bottom);
    int origin_x = content_rect.x + m_margin.left;
    int origin_y = content_rect.y + m_margin.top;

    if (avail_w <= 0 || avail_h <= 0) return;

    std::map<View*, Rect> resolved_bounds;

    for (const auto& child : m_children) {
        if (!child || child->get_visibility() == Visibility::Gone) continue;

        const auto& params = child->get_layout_params();
        const auto& margin = child->get_margin();
        Size measured = child->measure_size();

        int child_w = (params.width >= 0) ? params.width : measured.width;
        int child_h = (params.height >= 0) ? params.height : measured.height;

        if (params.width == static_cast<int>(LayoutDimension::MatchParent)) {
            child_w = std::max(0, avail_w - margin.left - margin.right);
        }
        if (params.height == static_cast<int>(LayoutDimension::MatchParent)) {
            child_h = std::max(0, avail_h - margin.top - margin.bottom);
        }

        int child_x = origin_x + margin.left;
        int child_y = origin_y + margin.top;

        // Sibling horizontal positioning
        if (params.to_end_of && resolved_bounds.find(params.to_end_of.get()) != resolved_bounds.end()) {
            const auto& ref = resolved_bounds[params.to_end_of.get()];
            child_x = ref.x + ref.width + margin.left;
        } else if (params.to_start_of && resolved_bounds.find(params.to_start_of.get()) != resolved_bounds.end()) {
            const auto& ref = resolved_bounds[params.to_start_of.get()];
            child_x = ref.x - child_w - margin.right;
        } else if (params.align_parent_end) {
            child_x = origin_x + avail_w - child_w - margin.right;
        } else if (params.center_horizontal || params.center_in_parent) {
            child_x = origin_x + (avail_w - child_w) / 2;
        }

        // Sibling vertical positioning
        if (params.below && resolved_bounds.find(params.below.get()) != resolved_bounds.end()) {
            const auto& ref = resolved_bounds[params.below.get()];
            child_y = ref.y + ref.height + margin.top;
        } else if (params.above && resolved_bounds.find(params.above.get()) != resolved_bounds.end()) {
            const auto& ref = resolved_bounds[params.above.get()];
            child_y = ref.y - child_h - margin.bottom;
        } else if (params.align_parent_bottom) {
            child_y = origin_y + avail_h - child_h - margin.bottom;
        } else if (params.center_vertical || params.center_in_parent) {
            child_y = origin_y + (avail_h - child_h) / 2;
        }

        Rect child_rect(child_x, child_y, child_w, child_h);
        resolved_bounds[child.get()] = child_rect;
        m_child_entries.push_back({child, child_rect});

        child->draw(cr, child_rect);
    }
}

} // namespace miqu
