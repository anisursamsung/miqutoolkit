#include "miqutoolkit/view/bottom_navigation_view.hpp"
#include "miqutoolkit/view/card_view.hpp"
#include "miqutoolkit/core/config.hpp"
#include "miqutoolkit/core/window.hpp"
#include <pango/pangocairo.h>
#include <algorithm>
#include <iostream>

namespace miqu {

BottomNavigationView::BottomNavigationView() {
    m_padding = Padding(0, 0, 0, 0);
}

void BottomNavigationView::set_items(const std::vector<BottomNavItem>& items) {
    m_items = items;
    if (m_selected_index >= static_cast<int>(m_items.size())) {
        m_selected_index = m_items.empty() ? 0 : static_cast<int>(m_items.size()) - 1;
    }
    if (m_window) m_window->schedule_redraw();
}

void BottomNavigationView::add_item(const BottomNavItem& item) {
    m_items.push_back(item);
    if (m_window) m_window->schedule_redraw();
}

void BottomNavigationView::add_item(const std::string& label, const std::string& icon, const std::string& badge) {
    m_items.emplace_back(label, icon, badge);
    if (m_window) m_window->schedule_redraw();
}

void BottomNavigationView::set_selected_index(int index) {
    if (index >= 0 && index < static_cast<int>(m_items.size()) && index != m_selected_index) {
        m_selected_index = index;
        if (m_on_item_selected) {
            m_on_item_selected(m_selected_index);
        }
        if (m_window) m_window->schedule_redraw();
    }
}

int BottomNavigationView::get_item_index_at(int lx, const Rect& bounds) const {
    if (m_items.empty() || bounds.width <= 0) return -1;
    if (lx < bounds.x || lx >= bounds.x + bounds.width) return -1;

    int item_w = bounds.width / static_cast<int>(m_items.size());
    if (item_w <= 0) return -1;

    int idx = (lx - bounds.x) / item_w;
    return std::clamp(idx, 0, static_cast<int>(m_items.size()) - 1);
}

Size BottomNavigationView::measure_size() const {
    return Size(160, m_bar_height);
}

void BottomNavigationView::draw(cairo_t* cr, const Rect& bounds) {
    if (!is_visible() || !cr || bounds.width <= 0 || bounds.height <= 0 || m_items.empty()) return;

    auto config = Config::get();
    cairo_save(cr);

    // 1. Background
    cairo_new_path(cr);
    cairo_rectangle(cr, bounds.x, bounds.y, bounds.width, bounds.height);
    cairo_set_source_rgba(cr, config->colors.surface.r,
                              config->colors.surface.g,
                              config->colors.surface.b,
                              config->colors.surface.a);
    cairo_fill(cr);

    // 2. Top Hairline Divider
    if (m_show_divider) {
        cairo_new_path(cr);
        cairo_move_to(cr, bounds.x, bounds.y + 0.5);
        cairo_line_to(cr, bounds.x + bounds.width, bounds.y + 0.5);
        cairo_set_source_rgba(cr, config->colors.outline_variant.r,
                                  config->colors.outline_variant.g,
                                  config->colors.outline_variant.b,
                                  0.35f);
        cairo_set_line_width(cr, 1.0);
        cairo_stroke(cr);
    }

    size_t count = m_items.size();
    int item_w = bounds.width / static_cast<int>(count);
    std::string font_family = !config->metrics.font_family.empty() ? config->metrics.font_family : "Sans";

    for (size_t i = 0; i < count; ++i) {
        const auto& item = m_items[i];
        int ix = bounds.x + static_cast<int>(i) * item_w;
        int iw = (i == count - 1) ? (bounds.x + bounds.width - ix) : item_w;
        int iy = bounds.y;

        bool is_selected = (static_cast<int>(i) == m_selected_index);
        bool is_hovered = (static_cast<int>(i) == m_hovered_index);

        // Material 3 Active Pill Indicator
        int pw = std::min(m_pill_w, std::max(20, iw - 12));
        int ph = m_pill_h;
        int px = ix + (iw - pw) / 2;
        int py = iy + 6;
        int pill_radius = ph / 2;

        if (is_selected) {
            CardView::draw_rounded_rect(cr, px, py, pw, ph, pill_radius);
            cairo_set_source_rgba(cr, config->colors.primary_container.r,
                                      config->colors.primary_container.g,
                                      config->colors.primary_container.b,
                                      config->colors.primary_container.a);
            cairo_fill(cr);

            CardView::draw_rounded_rect(cr, px + 0.5, py + 0.5, pw - 1.0, ph - 1.0, std::max(0, pill_radius - 1));
            cairo_set_source_rgba(cr, config->colors.primary.r,
                                      config->colors.primary.g,
                                      config->colors.primary.b,
                                      0.30f);
            cairo_set_line_width(cr, 1.0);
            cairo_stroke(cr);
        } else if (is_hovered) {
            CardView::draw_rounded_rect(cr, px, py, pw, ph, pill_radius);
            cairo_set_source_rgba(cr, config->colors.surface_variant.r,
                                      config->colors.surface_variant.g,
                                      config->colors.surface_variant.b,
                                      0.40f);
            cairo_fill(cr);
        }

        // Icon Rendering (centered inside the pill)
        if (!item.icon.empty()) {
            PangoLayout* icon_layout = pango_cairo_create_layout(cr);
            pango_layout_set_text(icon_layout, item.icon.c_str(), -1);

            std::string icon_font = font_family + " 15";
            PangoFontDescription* icon_desc = pango_font_description_from_string(icon_font.c_str());
            pango_layout_set_font_description(icon_layout, icon_desc);
            pango_font_description_free(icon_desc);

            int icon_w = 0, icon_h = 0;
            pango_layout_get_pixel_size(icon_layout, &icon_w, &icon_h);

            double icon_x = px + (pw - icon_w) / 2.0;
            double icon_y = py + (ph - icon_h) / 2.0;

            cairo_move_to(cr, icon_x, icon_y);
            Color icon_fg = is_selected ? config->colors.on_primary_container : config->colors.on_surface_variant;
            cairo_set_source_rgba(cr, icon_fg.r, icon_fg.g, icon_fg.b, icon_fg.a);
            pango_cairo_show_layout(cr, icon_layout);
            g_object_unref(icon_layout);
        }

        // Label Rendering (centered beneath the pill)
        if (!item.label.empty()) {
            PangoLayout* lbl_layout = pango_cairo_create_layout(cr);
            pango_layout_set_text(lbl_layout, item.label.c_str(), -1);

            std::string lbl_font = font_family + " 10";
            if (is_selected) lbl_font += " Bold";
            PangoFontDescription* lbl_desc = pango_font_description_from_string(lbl_font.c_str());
            pango_layout_set_font_description(lbl_layout, lbl_desc);
            pango_font_description_free(lbl_desc);

            pango_layout_set_width(lbl_layout, (iw - 4) * PANGO_SCALE);
            pango_layout_set_alignment(lbl_layout, PANGO_ALIGN_CENTER);
            pango_layout_set_ellipsize(lbl_layout, PANGO_ELLIPSIZE_END);

            int lbl_w = 0, lbl_h = 0;
            pango_layout_get_pixel_size(lbl_layout, &lbl_w, &lbl_h);

            double lbl_x = ix + 2.0;
            double lbl_y = py + ph + 3.0;

            cairo_move_to(cr, lbl_x, lbl_y);
            Color lbl_fg = is_selected ? config->colors.primary : config->colors.on_surface_variant;
            cairo_set_source_rgba(cr, lbl_fg.r, lbl_fg.g, lbl_fg.b, lbl_fg.a);
            pango_cairo_show_layout(cr, lbl_layout);
            g_object_unref(lbl_layout);
        }

        // Optional Badge (top-right of pill)
        if (!item.badge.empty()) {
            PangoLayout* badge_layout = pango_cairo_create_layout(cr);
            pango_layout_set_text(badge_layout, item.badge.c_str(), -1);

            std::string b_font = font_family + " 9 Bold";
            PangoFontDescription* b_desc = pango_font_description_from_string(b_font.c_str());
            pango_layout_set_font_description(badge_layout, b_desc);
            pango_font_description_free(b_desc);

            int bw = 0, bh = 0;
            pango_layout_get_pixel_size(badge_layout, &bw, &bh);

            int b_pad = 4;
            int b_rect_w = std::max(bh, bw + b_pad * 2);
            int b_rect_h = bh + 2;
            int bx = px + pw - b_rect_w / 2;
            int by = py - 2;

            CardView::draw_rounded_rect(cr, bx, by, b_rect_w, b_rect_h, b_rect_h / 2);
            cairo_set_source_rgba(cr, 0.88f, 0.24f, 0.24f, 1.0f);
            cairo_fill(cr);

            cairo_move_to(cr, bx + (b_rect_w - bw) / 2.0, by + (b_rect_h - bh) / 2.0);
            cairo_set_source_rgba(cr, 1.0f, 1.0f, 1.0f, 1.0f);
            pango_cairo_show_layout(cr, badge_layout);
            g_object_unref(badge_layout);
        }
    }

    cairo_restore(cr);
}

bool BottomNavigationView::on_mouse_move(int lx, int ly, const Rect& bounds) {
    (void)ly;
    int prev = m_hovered_index;
    m_hovered_index = get_item_index_at(lx, bounds);
    if (prev != m_hovered_index) {
        if (m_window) m_window->schedule_redraw();
    }
    return true;
}

bool BottomNavigationView::on_mouse_button(int lx, int ly, MouseButton button, bool pressed, const Rect& bounds) {
    (void)ly;
    if (button == MouseButton::Left && pressed) {
        int idx = get_item_index_at(lx, bounds);
        if (idx >= 0) {
            set_selected_index(idx);
            return true;
        }
    }
    return false;
}

// -----------------------------------------------------------------------------
// BottomNavigationViewBuilder
// -----------------------------------------------------------------------------

BottomNavigationViewBuilder::BottomNavigationViewBuilder() {
    m_view = std::make_shared<BottomNavigationView>();
}

} // namespace miqu
