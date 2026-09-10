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
    int radius = (m_corner_radius >= 0) ? m_corner_radius : config->metrics.corner_radius;
    Color bg_color = m_has_custom_bg ? m_background_color : config->colors.surface;

    m_corner_radius = radius;
    m_background_color = bg_color;
    m_has_custom_bg = true;

    FrameLayout::draw(cr, bounds);
}

} // namespace miqu
