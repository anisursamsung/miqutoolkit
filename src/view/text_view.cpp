#include "miqutoolkit/view/text_view.hpp"
#include "miqutoolkit/core/config.hpp"
#include <pango/pangocairo.h>

namespace miqu {

Size TextView::measure_size() const {
    if (m_text.empty()) return Size(0, 0);

    cairo_surface_t* temp_surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
    cairo_t* cr = cairo_create(temp_surf);

    PangoLayout* layout = pango_cairo_create_layout(cr);
    pango_layout_set_text(layout, m_text.c_str(), -1);

    auto config = Config::get();
    std::string font_family = !m_font_family.empty() ? m_font_family : config->metrics.font_family;
    if (font_family.empty()) font_family = "Sans";
    int font_size = m_font_size > 0 ? m_font_size : config->metrics.font_size;
    if (font_size <= 0) font_size = 11;

    std::string font_desc_str = font_family + " " + std::to_string(font_size);
    if (m_bold) font_desc_str += " Bold";
    if (m_italic) font_desc_str += " Italic";

    PangoFontDescription* desc = pango_font_description_from_string(font_desc_str.c_str());
    pango_layout_set_font_description(layout, desc);
    pango_font_description_free(desc);

    int text_w = 0, text_h = 0;
    pango_layout_get_pixel_size(layout, &text_w, &text_h);

    g_object_unref(layout);
    cairo_destroy(cr);
    cairo_surface_destroy(temp_surf);

    int total_w = text_w + m_padding.left + m_padding.right + m_margin.left + m_margin.right;
    int total_h = text_h + m_padding.top + m_padding.bottom + m_margin.top + m_margin.bottom;
    return Size(total_w, total_h);
}

void TextView::draw(cairo_t* cr, const Rect& bounds) {
    if (!is_visible() || !cr || m_text.empty() || bounds.width <= 0 || bounds.height <= 0) return;

    Rect content_bounds = get_content_rect(bounds);
    int draw_x = content_bounds.x + m_margin.left;
    int draw_y = content_bounds.y + m_margin.top;
    int draw_w = std::max(0, content_bounds.width - m_margin.left - m_margin.right);
    int draw_h = std::max(0, content_bounds.height - m_margin.top - m_margin.bottom);

    if (draw_w <= 0 || draw_h <= 0) return;

    cairo_save(cr);

    PangoLayout* layout = pango_cairo_create_layout(cr);
    pango_layout_set_text(layout, m_text.c_str(), -1);

    auto config = Config::get();
    std::string font_family = !m_font_family.empty() ? m_font_family : config->metrics.font_family;
    if (font_family.empty()) font_family = "Sans";
    int font_size = m_font_size > 0 ? m_font_size : config->metrics.font_size;
    if (font_size <= 0) font_size = 11;

    std::string font_desc_str = font_family + " " + std::to_string(font_size);
    if (m_bold) font_desc_str += " Bold";
    if (m_italic) font_desc_str += " Italic";

    PangoFontDescription* desc = pango_font_description_from_string(font_desc_str.c_str());
    pango_layout_set_font_description(layout, desc);
    pango_font_description_free(desc);

    pango_layout_set_width(layout, draw_w * PANGO_SCALE);

    if (m_ellipsize) {
        pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
    }

    if (m_align == TextAlignment::Center) {
        pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
    } else if (m_align == TextAlignment::Right) {
        pango_layout_set_alignment(layout, PANGO_ALIGN_RIGHT);
    } else {
        pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);
    }

    int text_w = 0, text_h = 0;
    pango_layout_get_pixel_size(layout, &text_w, &text_h);

    double baseline_y = draw_y + (draw_h - text_h) / 2.0;

    cairo_move_to(cr, draw_x, baseline_y);
    Color text_col = m_has_custom_color ? m_color : config->colors.on_surface;
    cairo_set_source_rgba(cr, text_col.r, text_col.g, text_col.b, text_col.a);
    pango_cairo_show_layout(cr, layout);

    g_object_unref(layout);
    cairo_restore(cr);
}

} // namespace miqu
