#include "miqutoolkit/view/bottom_navigation_view.hpp"
#include "miqutoolkit/view/card_view.hpp"
#include "miqutoolkit/core/config.hpp"
#include <pango/pangocairo.h>
#include <algorithm>
#include <cmath>

namespace miqu {

BottomNavigationView::BottomNavigationView() {
    set_layout_params(LayoutParams(static_cast<int>(LayoutDimension::MatchParent), m_bar_height));
}

void BottomNavigationView::set_items(const std::vector<BottomNavItem>& items) {
    m_items = items;
    m_selected_index = std::clamp(m_selected_index, 0, std::max(0, static_cast<int>(m_items.size()) - 1));
    request_redraw();
}

void BottomNavigationView::add_item(const BottomNavItem& item) {
    m_items.push_back(item);
    request_redraw();
}

void BottomNavigationView::add_item(const std::string& label, const std::string& icon, const std::string& badge) {
    m_items.emplace_back(label, icon, badge);
    request_redraw();
}

void BottomNavigationView::set_selected_index(int index) {
    if (m_items.empty()) return;
    int new_index = std::clamp(index, 0, static_cast<int>(m_items.size()) - 1);
    if (m_selected_index != new_index) {
        m_selected_index = new_index;
        if (m_on_item_selected) {
            m_on_item_selected(m_selected_index);
        }
        request_redraw();
    }
}

int BottomNavigationView::get_item_index_at(int lx, int ly, const Rect& bounds) const {
    if (!bounds.contains(Point(lx, ly)) || m_items.empty()) return -1;

    int pad_side = (m_corner_radius > 0) ? 8 : 0;
    int rel_x = lx - (bounds.x + pad_side);
    int avail_w = bounds.width - pad_side * 2;
    if (rel_x < 0 || rel_x >= avail_w || avail_w <= 0) return -1;

    int item_w = avail_w / static_cast<int>(m_items.size());
    if (item_w <= 0) return -1;

    int idx = rel_x / item_w;
    return std::clamp(idx, 0, static_cast<int>(m_items.size()) - 1);
}

Size BottomNavigationView::measure_size() const {
    if (m_items.empty()) return Size(0, m_bar_height);
    int total_w = static_cast<int>(m_items.size()) * m_item_width + ((m_corner_radius > 0) ? 16 : 0);
    return Size(total_w, m_bar_height);
}

void BottomNavigationView::draw(cairo_t* cr, const Rect& bounds) {
    if (!is_visible() || !cr || bounds.width <= 0 || bounds.height <= 0 || m_items.empty()) return;

    auto config = Config::get();
    cairo_save(cr);

    // 1. Background
    if (m_corner_radius > 0) {
        double radius = static_cast<double>(m_corner_radius);
        CardView::draw_rounded_rect(cr, bounds.x, bounds.y, bounds.width, bounds.height, radius);
        cairo_set_source_rgba(cr, config->colors.surface_variant.r,
                                  config->colors.surface_variant.g,
                                  config->colors.surface_variant.b,
                                  0.90f);
        cairo_fill(cr);

        CardView::draw_rounded_rect(cr, bounds.x + 0.5, bounds.y + 0.5, bounds.width - 1.0, bounds.height - 1.0, radius);
        cairo_set_source_rgba(cr, config->colors.outline_variant.r,
                                  config->colors.outline_variant.g,
                                  config->colors.outline_variant.b,
                                  0.45f);
        cairo_set_line_width(cr, 1.0);
        cairo_stroke(cr);
    } else {
        cairo_new_path(cr);
        cairo_rectangle(cr, bounds.x, bounds.y, bounds.width, bounds.height);
        cairo_set_source_rgba(cr, config->colors.surface.r,
                                  config->colors.surface.g,
                                  config->colors.surface.b,
                                  config->colors.surface.a);
        cairo_fill(cr);

        // Top Hairline Divider
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
    }

    size_t count = m_items.size();
    int pad_side = (m_corner_radius > 0) ? 8 : 0;
    int avail_w = bounds.width - pad_side * 2;
    int item_w = std::max(1, avail_w / static_cast<int>(count));
    std::string font_family = !config->metrics.font_family.empty() ? config->metrics.font_family : "Sans";

    for (size_t i = 0; i < count; ++i) {
        const auto& item = m_items[i];
        int ix = bounds.x + pad_side + static_cast<int>(i) * item_w;
        int iw = (i == count - 1) ? (bounds.x + bounds.width - pad_side - ix) : item_w;
        int iy = bounds.y;

        bool is_selected = (static_cast<int>(i) == m_selected_index);
        bool is_hovered = (static_cast<int>(i) == m_hovered_index);

        // Material 3 Active Pill Indicator
        int pw = std::min(m_pill_w, std::max(20, iw - 12));
        int ph = m_pill_h;
        int px = ix + (iw - pw) / 2;
        int py = iy + 5;
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

        // Label Rendering (below the pill)
        if (!item.label.empty()) {
            PangoLayout* label_layout = pango_cairo_create_layout(cr);
            pango_layout_set_text(label_layout, item.label.c_str(), -1);

            int font_size = config->metrics.caption_size > 0 ? config->metrics.caption_size : 10;
            std::string label_font = font_family + (is_selected ? " Bold " : " ") + std::to_string(font_size);
            PangoFontDescription* label_desc = pango_font_description_from_string(label_font.c_str());
            pango_layout_set_font_description(label_layout, label_desc);
            pango_font_description_free(label_desc);

            pango_layout_set_alignment(label_layout, PANGO_ALIGN_CENTER);
            pango_layout_set_width(label_layout, iw * PANGO_SCALE);
            pango_layout_set_ellipsize(label_layout, PANGO_ELLIPSIZE_END);

            int text_w = 0, text_h = 0;
            pango_layout_get_pixel_size(label_layout, &text_w, &text_h);

            double text_x = ix + (iw - text_w) / 2.0;
            double text_y = py + ph + 2;

            cairo_move_to(cr, text_x, text_y);
            Color label_fg = is_selected ? config->colors.primary : config->colors.on_surface_variant;
            float alpha = is_selected ? 1.0f : 0.75f;
            cairo_set_source_rgba(cr, label_fg.r, label_fg.g, label_fg.b, alpha);
            pango_cairo_show_layout(cr, label_layout);
            g_object_unref(label_layout);
        }

        // Badge Pill Indicator (top-right of the item pill)
        if (!item.badge.empty()) {
            PangoLayout* badge_layout = pango_cairo_create_layout(cr);
            pango_layout_set_text(badge_layout, item.badge.c_str(), -1);

            std::string badge_font = font_family + " Bold 9";
            PangoFontDescription* badge_desc = pango_font_description_from_string(badge_font.c_str());
            pango_layout_set_font_description(badge_layout, badge_desc);
            pango_font_description_free(badge_desc);

            int bw = 0, bh = 0;
            pango_layout_get_pixel_size(badge_layout, &bw, &bh);

            int bp_pad = 4;
            int bp_w = std::max(16, bw + bp_pad * 2);
            int bp_h = 14;
            int bp_x = px + pw - bp_w / 2;
            int bp_y = py - 2;

            CardView::draw_rounded_rect(cr, bp_x, bp_y, bp_w, bp_h, bp_h / 2);
            cairo_set_source_rgba(cr, config->colors.primary.r,
                                      config->colors.primary.g,
                                      config->colors.primary.b,
                                      1.0f);
            cairo_fill(cr);

            double btext_x = bp_x + (bp_w - bw) / 2.0;
            double btext_y = bp_y + (bp_h - bh) / 2.0;
            cairo_move_to(cr, btext_x, btext_y);
            cairo_set_source_rgba(cr, config->colors.on_primary.r,
                                      config->colors.on_primary.g,
                                      config->colors.on_primary.b,
                                      1.0f);
            pango_cairo_show_layout(cr, badge_layout);
            g_object_unref(badge_layout);
        }
    }

    cairo_restore(cr);
}

bool BottomNavigationView::on_mouse_move(int lx, int ly, const Rect& bounds) {
    int old_hover = m_hovered_index;
    m_hovered_index = get_item_index_at(lx, ly, bounds);
    return m_hovered_index != old_hover;
}

bool BottomNavigationView::on_mouse_button(int lx, int ly, MouseButton button, bool pressed, const Rect& bounds) {
    if (button != MouseButton::Left) return false;

    if (pressed) {
        m_pressed_index = get_item_index_at(lx, ly, bounds);
        return m_pressed_index >= 0;
    } else {
        int release_idx = get_item_index_at(lx, ly, bounds);
        if (release_idx >= 0 && release_idx == m_pressed_index) {
            set_selected_index(release_idx);
        }
        m_pressed_index = -1;
        return true;
    }
}

BottomNavigationViewBuilder::BottomNavigationViewBuilder() {
    m_view = std::make_shared<BottomNavigationView>();
}

} // namespace miqu
