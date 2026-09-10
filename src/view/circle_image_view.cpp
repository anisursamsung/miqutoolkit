#include "miqutoolkit/view/circle_image_view.hpp"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace miqu {

void CircleImageView::draw(cairo_t* cr, const Rect& bounds) {
    if (!is_visible() || !cr || bounds.width <= 0 || bounds.height <= 0) return;

    int draw_x = bounds.x + m_margin.left;
    int draw_y = bounds.y + m_margin.top;
    int draw_w = std::max(0, bounds.width - m_margin.left - m_margin.right);
    int draw_h = std::max(0, bounds.height - m_margin.top - m_margin.bottom);

    if (draw_w <= 0 || draw_h <= 0) return;

    double cx = draw_x + draw_w / 2.0;
    double cy = draw_y + draw_h / 2.0;
    double radius = std::min(draw_w, draw_h) / 2.0;

    cairo_save(cr);

    // Circular clip path
    cairo_arc(cr, cx, cy, radius, 0, 2.0 * M_PI);
    cairo_clip(cr);

    // Optional rotation around center
    if (std::abs(m_rotation_degrees) > 0.001) {
        double rad = m_rotation_degrees * (M_PI / 180.0);
        cairo_translate(cr, cx, cy);
        cairo_rotate(cr, rad);
        cairo_translate(cr, -cx, -cy);
    }

    // Call base ImageView draw inside clipped & rotated context
    ImageView::draw(cr, Rect(draw_x, draw_y, draw_w, draw_h));

    cairo_restore(cr);

    // Optional circular border stroke
    if (m_border_width > 0 && m_border_color.a > 0.0f) {
        cairo_save(cr);
        double stroke_offset = m_border_width / 2.0;
        cairo_arc(cr, cx, cy, std::max(0.0, radius - stroke_offset), 0, 2.0 * M_PI);
        cairo_set_source_rgba(cr, m_border_color.r, m_border_color.g, m_border_color.b, m_border_color.a);
        cairo_set_line_width(cr, m_border_width);
        cairo_stroke(cr);
        cairo_restore(cr);
    }
}

} // namespace miqu
