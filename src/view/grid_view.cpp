#include "biwaytoolkit/view/grid_view.hpp"
#include "biwaytoolkit/view/card_view.hpp"
#include "biwaytoolkit/view/image_view.hpp"
#include "biwaytoolkit/core/color_scheme.hpp"
#include <pango/pangocairo.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <algorithm>
#include <iostream>
#include <cmath>

namespace biway {

static std::string str_to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

GridView::GridView() {
}

void GridView::set_adapter(std::vector<AppInfo> items) {
    m_all_items = std::move(items);
    set_filter_query(m_filter_query);
}

void GridView::set_filter_query(const std::string& query) {
    m_filter_query = query;
    m_filtered_items.clear();

    if (m_filter_query.empty()) {
        m_filtered_items = m_all_items;
    } else {
        std::string lower_query = str_to_lower(m_filter_query);
        for (const auto& item : m_all_items) {
            std::string lower_title = str_to_lower(item.title);
            std::string lower_sub = str_to_lower(item.subtitle);
            std::string lower_cmd = str_to_lower(item.exec_cmd);
            std::string lower_id = str_to_lower(item.id);

            if (lower_title.find(lower_query) != std::string::npos ||
                lower_sub.find(lower_query) != std::string::npos ||
                lower_cmd.find(lower_query) != std::string::npos ||
                lower_id.find(lower_query) != std::string::npos) {
                m_filtered_items.push_back(item);
            }
        }
    }

    m_selected_index = m_filtered_items.empty() ? -1 : 0;
    m_scroll_y = 0.0;
}

const AppInfo* GridView::get_selected_item() const {
    if (m_selected_index >= 0 && m_selected_index < static_cast<int>(m_filtered_items.size())) {
        return &m_filtered_items[m_selected_index];
    }
    return nullptr;
}

int GridView::compute_columns(int bounds_w, int& out_cell_w) const {
    if (bounds_w <= 0) {
        out_cell_w = m_cell_w;
        return 1;
    }

    int cols = m_cols;
    if (m_auto_fit) {
        cols = std::max(1, (bounds_w + m_space_x) / (m_min_col_w + m_space_x));
    }

    if (m_stretch_mode == StretchMode::ColumnWidth && cols > 0) {
        int total_spacing = (cols - 1) * m_space_x;
        out_cell_w = std::max(20, (bounds_w - total_spacing) / cols);
    } else {
        out_cell_w = m_cell_w;
    }

    m_effective_cols = cols;
    m_effective_cell_w = out_cell_w;
    return cols;
}

void GridView::ensure_visible(int viewport_height, int cols, int row_stride) {
    if (m_selected_index < 0 || viewport_height <= 0 || cols <= 0) return;

    int row = m_selected_index / cols;
    double item_top = row * row_stride;
    double item_bottom = item_top + m_cell_h;

    if (item_top < m_scroll_y) {
        m_scroll_y = item_top;
    } else if (item_bottom > m_scroll_y + viewport_height) {
        m_scroll_y = item_bottom - viewport_height;
    }

    int total_rows = (static_cast<int>(m_filtered_items.size()) + cols - 1) / cols;
    double content_h = total_rows * row_stride - m_space_y;
    double max_scroll = std::max(0.0, content_h - viewport_height);
    m_scroll_y = std::clamp(m_scroll_y, 0.0, max_scroll);
}

int GridView::item_at(int lx, int ly, const Rect& bounds) const {
    int cell_w = m_effective_cell_w;
    int cols = compute_columns(bounds.width, cell_w);

    int rel_x = lx - bounds.x;
    int rel_y = ly - bounds.y + static_cast<int>(m_scroll_y);

    if (rel_x < 0 || rel_x >= bounds.width || rel_y < 0) {
        return -1;
    }

    int col = rel_x / (cell_w + m_space_x);
    int row = rel_y / (m_cell_h + m_space_y);

    if (col >= cols) return -1;

    int idx = row * cols + col;
    if (idx >= 0 && idx < static_cast<int>(m_filtered_items.size())) {
        return idx;
    }
    return -1;
}

void GridView::draw(cairo_t* cr, const Rect& bounds) {
    if (!is_visible() || !cr || bounds.width <= 0 || bounds.height <= 0) return;

    m_last_width = bounds.width;
    m_last_height = bounds.height;
    auto theme = ColorScheme::get();

    int cell_w = 0;
    int cols = compute_columns(bounds.width, cell_w);

    int total_rows = (static_cast<int>(m_filtered_items.size()) + cols - 1) / cols;
    int row_stride = m_cell_h + m_space_y;
    double content_h = std::max(0, total_rows * row_stride - m_space_y);
    double max_scroll = std::max(0.0, content_h - bounds.height);

    m_scroll_y = std::clamp(m_scroll_y, 0.0, max_scroll);

    cairo_save(cr);
    cairo_rectangle(cr, bounds.x, bounds.y, bounds.width, bounds.height);
    cairo_clip(cr);

    int start_row = std::max(0, static_cast<int>(m_scroll_y / row_stride));
    int end_row = std::min(total_rows, static_cast<int>((m_scroll_y + bounds.height) / row_stride) + 1);

    int start_idx = start_row * cols;
    int end_idx = std::min(end_row * cols, static_cast<int>(m_filtered_items.size()));

    for (int i = start_idx; i < end_idx; ++i) {
        const auto& item = m_filtered_items[i];
        int grid_row = i / cols;
        int grid_col = i % cols;

        int cell_x = bounds.x + grid_col * (cell_w + m_space_x);
        double cell_y = bounds.y + grid_row * row_stride - m_scroll_y;
        Rect cell_rect(cell_x, static_cast<int>(cell_y), cell_w, m_cell_h);

        bool is_selected = (i == m_selected_index);
        bool is_hovered = (i == m_hovered_index);

        // Background
        if (is_selected) {
            CardView::draw_rounded_rect(cr, cell_x, cell_y, cell_w, m_cell_h, 8.0);
            cairo_set_source_rgba(cr, theme->colors.primary_container.r,
                                      theme->colors.primary_container.g,
                                      theme->colors.primary_container.b,
                                      0.6f);
            cairo_fill(cr);

            CardView::draw_rounded_rect(cr, cell_x + 0.5, cell_y + 0.5, cell_w - 1.0, m_cell_h - 1.0, 8.0);
            cairo_set_source_rgba(cr, theme->colors.primary.r,
                                      theme->colors.primary.g,
                                      theme->colors.primary.b,
                                      0.8f);
            cairo_set_line_width(cr, 1.0);
            cairo_stroke(cr);
        } else if (is_hovered) {
            CardView::draw_rounded_rect(cr, cell_x, cell_y, cell_w, m_cell_h, 8.0);
            cairo_set_source_rgba(cr, theme->colors.surface_variant.r,
                                      theme->colors.surface_variant.g,
                                      theme->colors.surface_variant.b,
                                      0.4f);
            cairo_fill(cr);
        }

        // Draw App Icon
        Rect icon_rect(cell_x + (cell_w - 48) / 2, static_cast<int>(cell_y) + 12, 48, 48);
        ImageView icon_view(item.icon_path.empty() ? item.icon_name : item.icon_path);
        icon_view.set_target_size(48);
        icon_view.draw(cr, icon_rect);

        // Draw App Title
        PangoLayout* layout = pango_cairo_create_layout(cr);
        pango_layout_set_text(layout, item.title.c_str(), -1);

        PangoFontDescription* desc = pango_font_description_from_string("Sans 10");
        pango_layout_set_font_description(layout, desc);
        pango_font_description_free(desc);

        pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
        pango_layout_set_width(layout, std::max(0, cell_w - 8) * PANGO_SCALE);
        pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);

        cairo_move_to(cr, cell_x + 4, cell_y + 66);
        cairo_set_source_rgba(cr, theme->colors.on_surface.r,
                                  theme->colors.on_surface.g,
                                  theme->colors.on_surface.b,
                                  theme->colors.on_surface.a);
        pango_cairo_show_layout(cr, layout);

        g_object_unref(layout);
    }

    // Draw Visual Scrollbar Thumb Indicator if content overflows
    if (max_scroll > 0.0) {
        double track_h = bounds.height;
        double thumb_h = std::max(24.0, (static_cast<double>(bounds.height) / content_h) * track_h);
        double scroll_ratio = m_scroll_y / max_scroll;
        double thumb_y = bounds.y + scroll_ratio * (track_h - thumb_h);
        double thumb_x = bounds.x + bounds.width - 4.0;

        cairo_save(cr);
        CardView::draw_rounded_rect(cr, thumb_x, thumb_y, 4.0, thumb_h, 2.0);
        cairo_set_source_rgba(cr, theme->colors.outline.r,
                                  theme->colors.outline.g,
                                  theme->colors.outline.b,
                                  0.6f);
        cairo_fill(cr);
        cairo_restore(cr);
    }

    cairo_restore(cr);
}

bool GridView::on_key(const KeyPressEvent& event) {
    if (!event.pressed) return false;
    if (m_filtered_items.empty()) return false;

    int cell_w = 0;
    int cols = compute_columns(m_last_width > 0 ? m_last_width : 800, cell_w);
    int total = static_cast<int>(m_filtered_items.size());
    int row_stride = m_cell_h + m_space_y;

    if (event.keysym == XKB_KEY_Return || event.keysym == XKB_KEY_KP_Enter) {
        if (m_selected_index >= 0 && m_selected_index < total) {
            if (m_on_item_click) {
                m_on_item_click(m_filtered_items[m_selected_index]);
            }
            return true;
        }
        return false;
    }

    if (event.keysym == XKB_KEY_Right) {
        m_selected_index = (m_selected_index + 1) % total;
        if (m_last_height > 0) ensure_visible(m_last_height, cols, row_stride);
        return true;
    }

    if (event.keysym == XKB_KEY_Left) {
        m_selected_index = (m_selected_index - 1 + total) % total;
        if (m_last_height > 0) ensure_visible(m_last_height, cols, row_stride);
        return true;
    }

    if (event.keysym == XKB_KEY_Down) {
        if (m_selected_index + cols < total) {
            m_selected_index += cols;
        } else {
            m_selected_index = (m_selected_index % cols);
        }
        if (m_last_height > 0) ensure_visible(m_last_height, cols, row_stride);
        return true;
    }

    if (event.keysym == XKB_KEY_Up) {
        if (m_selected_index - cols >= 0) {
            m_selected_index -= cols;
        } else {
            int last_row_start = (total / cols) * cols;
            int candidate = last_row_start + (m_selected_index % cols);
            if (candidate >= total) candidate -= cols;
            m_selected_index = std::max(0, candidate);
        }
        if (m_last_height > 0) ensure_visible(m_last_height, cols, row_stride);
        return true;
    }

    if (event.keysym == XKB_KEY_Page_Down) {
        m_selected_index = std::min(total - 1, m_selected_index + cols * 3);
        if (m_last_height > 0) ensure_visible(m_last_height, cols, row_stride);
        return true;
    }

    if (event.keysym == XKB_KEY_Page_Up) {
        m_selected_index = std::max(0, m_selected_index - cols * 3);
        if (m_last_height > 0) ensure_visible(m_last_height, cols, row_stride);
        return true;
    }

    return false;
}

bool GridView::on_mouse_move(int lx, int ly, const Rect& bounds) {
    int idx = item_at(lx, ly, bounds);
    if (idx != m_hovered_index) {
        m_hovered_index = idx;
        return true;
    }
    return false;
}

bool GridView::on_mouse_button(int lx, int ly, MouseButton button, bool pressed, const Rect& bounds) {
    if (button != MouseButton::Left) return false;

    int idx = item_at(lx, ly, bounds);
    if (idx >= 0 && idx < static_cast<int>(m_filtered_items.size())) {
        if (pressed) {
            m_selected_index = idx;
            return true;
        } else {
            if (m_on_item_click) {
                m_on_item_click(m_filtered_items[idx]);
            }
            return true;
        }
    }
    return false;
}

bool GridView::on_scroll(double delta) {
    if (m_filtered_items.empty() || m_last_height <= 0) return false;

    int cell_w = 0;
    int cols = compute_columns(m_last_width > 0 ? m_last_width : 800, cell_w);

    int total_rows = (static_cast<int>(m_filtered_items.size()) + cols - 1) / cols;
    int row_stride = m_cell_h + m_space_y;
    double content_h = std::max(0, total_rows * row_stride - m_space_y);
    double max_scroll = std::max(0.0, content_h - m_last_height);

    if (max_scroll <= 0.0) return false;

    double prev_scroll = m_scroll_y;
    m_scroll_y = std::clamp(m_scroll_y + delta * 2.5, 0.0, max_scroll);

    return (m_scroll_y != prev_scroll);
}

} // namespace biway
