#include "miqutoolkit/view/button.hpp"
#include "miqutoolkit/view/card_view.hpp"
#include "miqutoolkit/core/color_scheme.hpp"
#include <pango/pangocairo.h>
#include <iostream>

namespace miqu {

Size Button::measure_size() const {
    cairo_surface_t* temp_surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
    cairo_t* cr = cairo_create(temp_surf);

    PangoLayout* layout = pango_cairo_create_layout(cr);
    pango_layout_set_text(layout, m_text.c_str(), -1);

    std::string font_desc_str = "Sans " + std::to_string(m_font_size);
    if (m_font_bold) font_desc_str += " Bold";

    PangoFontDescription* desc = pango_font_description_from_string(font_desc_str.c_str());
    pango_layout_set_font_description(layout, desc);
    pango_font_description_free(desc);

    int text_w = 0, text_h = 0;
    pango_layout_get_pixel_size(layout, &text_w, &text_h);

    g_object_unref(layout);
    cairo_destroy(cr);
    cairo_surface_destroy(temp_surf);

    int icon_space = m_icon.empty() ? 0 : 24;
    int pad_h = m_padding.left + m_padding.right > 0 ? (m_padding.left + m_padding.right) : 24;
    int pad_v = m_padding.top + m_padding.bottom > 0 ? (m_padding.top + m_padding.bottom) : 12;

    int total_w = text_w + icon_space + pad_h + m_margin.left + m_margin.right;
    int total_h = std::max(text_h, 20) + pad_v + m_margin.top + m_margin.bottom;

    return Size(total_w, total_h);
}

void Button::draw(cairo_t* cr, const Rect& bounds) {
    if (!is_visible() || !cr || bounds.width <= 0 || bounds.height <= 0) return;

    auto theme = ColorScheme::get();

    int draw_x = bounds.x + m_margin.left;
    int draw_y = bounds.y + m_margin.top;
    int draw_w = std::max(0, bounds.width - m_margin.left - m_margin.right);
    int draw_h = std::max(0, bounds.height - m_margin.top - m_margin.bottom);

    if (draw_w <= 0 || draw_h <= 0) return;

    Color bg_color = Color::transparent();
    Color fg_color = theme->colors.on_surface;

    if (m_use_custom_colors) {
        bg_color = m_custom_bg;
        fg_color = m_custom_fg;
    } else if (m_selected) {
        bg_color = theme->colors.primary_container;
        fg_color = theme->colors.on_primary_container;
    } else if (m_pressed) {
        bg_color = theme->colors.primary.with_alpha(0.35f);
        fg_color = theme->colors.primary;
    } else if (m_hovered) {
        bg_color = theme->colors.surface_variant.with_alpha(0.6f);
        fg_color = theme->colors.on_surface;
    }

    cairo_save(cr);

    // Background
    if (bg_color.a > 0.0f) {
        CardView::draw_rounded_rect(cr, draw_x, draw_y, draw_w, draw_h, m_corner_radius);
        cairo_set_source_rgba(cr, bg_color.r, bg_color.g, bg_color.b, bg_color.a);
        cairo_fill(cr);
    }

    // Border if selected
    if (m_selected) {
        CardView::draw_rounded_rect(cr, draw_x + 0.5, draw_y + 0.5, draw_w - 1.0, draw_h - 1.0, m_corner_radius);
        cairo_set_source_rgba(cr, theme->colors.primary.r, theme->colors.primary.g, theme->colors.primary.b, 0.8);
        cairo_set_line_width(cr, 1.0);
        cairo_stroke(cr);
    }

    // Text Label
    if (!m_text.empty()) {
        PangoLayout* layout = pango_cairo_create_layout(cr);
        pango_layout_set_text(layout, m_text.c_str(), -1);

        std::string font_desc_str = "Sans " + std::to_string(m_font_size);
        if (m_font_bold) font_desc_str += " Bold";

        PangoFontDescription* desc = pango_font_description_from_string(font_desc_str.c_str());
        pango_layout_set_font_description(layout, desc);
        pango_font_description_free(desc);

        int pad_l = m_padding.left > 0 ? m_padding.left : 12;
        int pad_r = m_padding.right > 0 ? m_padding.right : 12;

        pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
        pango_layout_set_width(layout, std::max(0, draw_w - pad_l - pad_r) * PANGO_SCALE);
        pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);

        int text_w = 0, text_h = 0;
        pango_layout_get_pixel_size(layout, &text_w, &text_h);

        double label_y = draw_y + (draw_h - text_h) / 2.0;

        cairo_move_to(cr, draw_x + pad_l, label_y);
        cairo_set_source_rgba(cr, fg_color.r, fg_color.g, fg_color.b, fg_color.a);
        pango_cairo_show_layout(cr, layout);

        g_object_unref(layout);
    }

    cairo_restore(cr);
}

bool Button::on_mouse_move(int lx, int ly, const Rect& bounds) {
    bool now_hovered = bounds.contains(lx, ly);
    if (now_hovered != m_hovered) {
        m_hovered = now_hovered;
        return true;
    }
    return false;
}

bool Button::on_mouse_button(int lx, int ly, MouseButton button, bool pressed, const Rect& bounds) {
    if (button != MouseButton::Left) return false;

    if (bounds.contains(lx, ly)) {
        if (pressed) {
            m_pressed = true;
            return true;
        } else if (m_pressed) {
            m_pressed = false;
            if (m_on_click) {
                m_on_click();
            }
            return true;
        }
    } else if (!pressed && m_pressed) {
        m_pressed = false;
        return true;
    }

    return false;
}

} // namespace miqu
