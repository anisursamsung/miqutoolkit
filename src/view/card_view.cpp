#include "miqutoolkit/view/card_view.hpp"
#include "miqutoolkit/core/config.hpp"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace miqu {

void CardView::draw_rounded_rect(cairo_t* cr, double x, double y, double w, double h, double r) {
    ViewGroup::draw_rounded_rect(cr, x, y, w, h, r);
}

void CardView::draw(cairo_t* cr, const Rect& bounds) {
    if (!is_visible() || !cr || bounds.width <= 0 || bounds.height <= 0) return;

    auto config = Config::get();
    int old_r = m_corner_radius;
    Color old_bg = m_background_color;
    bool old_has_bg = m_has_custom_bg;
    double old_sw = m_stroke_width;
    Color old_sc = m_stroke_color;

    if (m_corner_radius < 0) {
        m_corner_radius = config->metrics.corner_radius;
    }
    if (!m_has_custom_bg) {
        m_background_color = config->colors.surface;
        m_has_custom_bg = true;
    }
    if (m_stroke_width == 0 && m_stroke_color.a <= 0.0f) {
        m_stroke_width = 1;
        m_stroke_color = config->colors.outline_variant;
    }

    FrameLayout::draw(cr, bounds);

    m_corner_radius = old_r;
    m_background_color = old_bg;
    m_has_custom_bg = old_has_bg;
    m_stroke_width = old_sw;
    m_stroke_color = old_sc;
}

} // namespace miqu
