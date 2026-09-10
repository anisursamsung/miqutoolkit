#include "miqutoolkit/view/tile_view.hpp"
#include "miqutoolkit/core/config.hpp"
#include <pango/pangocairo.h>
#include <algorithm>

namespace miqu {

TileView::TileView(std::string title, std::string icon_source, bool is_image)
    : m_title(std::move(title)), m_icon_source(std::move(icon_source)), m_is_image(is_image) {}

void TileView::draw(cairo_t* cr, const Rect& bounds) {
    if (!cr || bounds.width <= 0 || bounds.height <= 0) return;

    auto config = Config::get();
    std::string font_family = config->metrics.font_family.empty() ? "Sans" : config->metrics.font_family;
    int font_size = config->metrics.font_size > 0 ? config->metrics.font_size : 10;
    int horiz_padding = 8;
    int text_max_w = std::max(0, bounds.width - horiz_padding * 2);

    // 1. Prepare Title Layout & Measure Height
    PangoLayout* title_layout = pango_cairo_create_layout(cr);
    pango_layout_set_text(title_layout, m_title.c_str(), -1);
    std::string title_font_spec = font_family + " " + std::to_string(font_size);
    PangoFontDescription* title_desc = pango_font_description_from_string(title_font_spec.c_str());
    pango_layout_set_font_description(title_layout, title_desc);
    pango_font_description_free(title_desc);
    pango_layout_set_alignment(title_layout, PANGO_ALIGN_CENTER);
    pango_layout_set_width(title_layout, text_max_w * PANGO_SCALE);
    pango_layout_set_ellipsize(title_layout, PANGO_ELLIPSIZE_END);

    int title_w = 0, title_h = 0;
    pango_layout_get_pixel_size(title_layout, &title_w, &title_h);

    // 2. Prepare Subtitle Layout & Measure Height (if present and not image mode)
    bool has_subtitle = !m_subtitle.empty() && !m_is_image;
    PangoLayout* sub_layout = nullptr;
    int sub_w = 0, sub_h = 0;
    if (has_subtitle) {
        sub_layout = pango_cairo_create_layout(cr);
        pango_layout_set_text(sub_layout, m_subtitle.c_str(), -1);
        int sub_size = std::max(6, font_size - 2);
        std::string sub_font_spec = font_family + " " + std::to_string(sub_size);
        PangoFontDescription* sub_desc = pango_font_description_from_string(sub_font_spec.c_str());
        pango_layout_set_font_description(sub_layout, sub_desc);
        pango_font_description_free(sub_desc);
        pango_layout_set_alignment(sub_layout, PANGO_ALIGN_CENTER);
        pango_layout_set_width(sub_layout, text_max_w * PANGO_SCALE);
        pango_layout_set_ellipsize(sub_layout, PANGO_ELLIPSIZE_END);
        pango_layout_get_pixel_size(sub_layout, &sub_w, &sub_h);
    }

    int title_y = 0;
    int sub_y = 0;

    if (m_is_image) {
        // --- Image Thumbnail Mode ---
        int pad_x = 8;
        int pad_top = 8;
        int pad_bottom = 6;
        int gap = 6; // Breathing space between thumbnail and title
        int img_w = bounds.width - pad_x * 2;
        int img_h = std::max(20, bounds.height - pad_top - gap - title_h - pad_bottom);

        Rect img_rect(bounds.x + pad_x, bounds.y + pad_top, img_w, img_h);
        ImageView img_view(m_icon_source);
        img_view.set_target_size(std::max(img_w, img_h));
        img_view.set_fit_mode(FitMode::Cover);
        img_view.set_quality_mode(m_quality);
        img_view.set_corner_radius(m_corner_radius);
        img_view.draw(cr, img_rect);

        title_y = bounds.y + pad_top + img_h + gap;
    } else {
        // --- Standard App Icon Mode ---
        int icon_size = has_subtitle ? 42 : 48;
        int icon_title_gap = has_subtitle ? 6 : 10;
        int title_sub_gap = 4; // Clear breathing room between title and subtitle

        int total_content_h = icon_size + icon_title_gap + title_h + (has_subtitle ? (title_sub_gap + sub_h) : 0);
        int top_offset = std::max(6, (bounds.height - total_content_h) / 2);

        Rect icon_rect(bounds.x + (bounds.width - icon_size) / 2, bounds.y + top_offset, icon_size, icon_size);
        ImageView icon_view(m_icon_source);
        icon_view.set_target_size(icon_size);
        icon_view.set_quality_mode(m_quality);
        icon_view.draw(cr, icon_rect);

        title_y = bounds.y + top_offset + icon_size + icon_title_gap;
        if (has_subtitle) {
            sub_y = title_y + title_h + title_sub_gap;
        }
    }

    // 3. Render Title
    cairo_move_to(cr, bounds.x + horiz_padding, title_y);
    cairo_set_source_rgba(cr, config->colors.on_surface.r,
                              config->colors.on_surface.g,
                              config->colors.on_surface.b,
                              config->colors.on_surface.a);
    pango_cairo_show_layout(cr, title_layout);
    g_object_unref(title_layout);

    // 4. Render Subtitle
    if (has_subtitle && sub_layout) {
        cairo_move_to(cr, bounds.x + horiz_padding, sub_y);
        if (m_highlight_subtitle) {
            cairo_set_source_rgba(cr, config->colors.primary.r,
                                      config->colors.primary.g,
                                      config->colors.primary.b,
                                      1.0f);
        } else {
            cairo_set_source_rgba(cr, config->colors.on_surface_variant.r,
                                      config->colors.on_surface_variant.g,
                                      config->colors.on_surface_variant.b,
                                      0.75f);
        }
        pango_cairo_show_layout(cr, sub_layout);
        g_object_unref(sub_layout);
    }
}

} // namespace miqu
