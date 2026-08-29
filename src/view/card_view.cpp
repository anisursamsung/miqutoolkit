#include "miqutoolkit/view/card_view.hpp"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace miqu {

void CardView::draw_rounded_rect(cairo_t* cr, double x, double y, double w, double h, double r) {
    if (r <= 0.0) {
        cairo_rectangle(cr, x, y, w, h);
        return;
    }
    double deg = M_PI / 180.0;
    cairo_new_sub_path(cr);
    cairo_arc(cr, x + w - r, y + r, r, -90 * deg, 0 * deg);
    cairo_arc(cr, x + w - r, y + h - r, r, 0 * deg, 90 * deg);
    cairo_arc(cr, x + r, y + h - r, r, 90 * deg, 180 * deg);
    cairo_arc(cr, x + r, y + r, r, 180 * deg, 270 * deg);
    cairo_close_path(cr);
}

void CardView::draw(cairo_t* cr, const Rect& bounds) {
    if (!is_visible() || !cr || bounds.width <= 0 || bounds.height <= 0) return;

    // Apply outer margin
    int draw_x = bounds.x + m_margin.left;
    int draw_y = bounds.y + m_margin.top;
    int draw_w = std::max(0, bounds.width - m_margin.left - m_margin.right);
    int draw_h = std::max(0, bounds.height - m_margin.top - m_margin.bottom);

    if (draw_w <= 0 || draw_h <= 0) return;

    cairo_save(cr);

    // 1. Draw Card Surface Fill
    draw_rounded_rect(cr, draw_x, draw_y, draw_w, draw_h, m_corner_radius);
    cairo_set_source_rgba(cr, m_background_color.r, m_background_color.g, m_background_color.b, m_background_color.a);
    cairo_fill(cr);

    // 2. Draw Card Border Stroke
    if (m_stroke_width > 0 && m_stroke_color.a > 0.0f) {
        double offset = m_stroke_width / 2.0;
        draw_rounded_rect(cr, draw_x + offset, draw_y + offset, draw_w - m_stroke_width, draw_h - m_stroke_width, std::max(0.0, m_corner_radius - offset));
        cairo_set_source_rgba(cr, m_stroke_color.r, m_stroke_color.g, m_stroke_color.b, m_stroke_color.a);
        cairo_set_line_width(cr, m_stroke_width);
        cairo_stroke(cr);
    }

    cairo_restore(cr);

    // 3. Draw Children inside the card if any exist
    if (!m_children.empty()) {
        FrameLayout::draw(cr, Rect(draw_x, draw_y, draw_w, draw_h));
    }
}

} // namespace miqu
