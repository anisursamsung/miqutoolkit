#include "miqutoolkit/view/seek_bar.hpp"
#include "miqutoolkit/view/card_view.hpp"
#include "miqutoolkit/core/config.hpp"
#include "miqutoolkit/core/window.hpp"
#include <cmath>

namespace miqu {

Size SeekBar::measure_size() const {
    int w = 120 + m_margin.left + m_margin.right;
    int h = std::max(m_track_height, m_thumb_radius * 2) + 12 + m_margin.top + m_margin.bottom;
    return Size(w, h);
}

void SeekBar::draw(cairo_t* cr, const Rect& bounds) {
    if (!is_visible() || !cr || bounds.width <= 0 || bounds.height <= 0) return;

    auto config = Config::get();

    int draw_x = bounds.x + m_margin.left;
    int draw_y = bounds.y + m_margin.top;
    int draw_w = std::max(0, bounds.width - m_margin.left - m_margin.right);
    int draw_h = std::max(0, bounds.height - m_margin.top - m_margin.bottom);

    if (draw_w <= 0 || draw_h <= 0) return;

    Color track_bg = m_custom_track ? m_track_color : config->colors.surface_variant.with_alpha(0.4f);
    Color prog_color = m_custom_progress ? m_progress_color : config->colors.primary;
    Color thumb_col = m_custom_thumb ? m_thumb_color : config->colors.primary;

    int cy = draw_y + draw_h / 2;
    int th = m_track_height;
    int tr = th / 2;

    int thumb_r = (m_dragging || m_hovered) ? m_thumb_radius + 2 : m_thumb_radius;
    int usable_w = draw_w - thumb_r * 2;
    if (usable_w < 1) usable_w = 1;

    int track_start_x = draw_x + thumb_r;
    int track_width = usable_w;
    int fill_width = static_cast<int>(usable_w * m_progress);

    cairo_save(cr);

    // 1. Draw Full Track Background
    CardView::draw_rounded_rect(cr, track_start_x, cy - tr, track_width, th, tr);
    cairo_set_source_rgba(cr, track_bg.r, track_bg.g, track_bg.b, track_bg.a);
    cairo_fill(cr);

    // 2. Draw Active Progress Fill
    if (fill_width > 0) {
        CardView::draw_rounded_rect(cr, track_start_x, cy - tr, fill_width, th, tr);
        cairo_set_source_rgba(cr, prog_color.r, prog_color.g, prog_color.b, prog_color.a);
        cairo_fill(cr);
    }

    // 3. Draw Thumb (if visible)
    if (m_thumb_visible && thumb_r > 0) {
        int thumb_cx = track_start_x + fill_width;

        // Ambient halo on hover/drag
        if (m_hovered || m_dragging) {
            cairo_arc(cr, thumb_cx, cy, thumb_r + 4, 0, 2 * M_PI);
            cairo_set_source_rgba(cr, thumb_col.r, thumb_col.g, thumb_col.b, 0.25f);
            cairo_fill(cr);
        }

        // Solid thumb circle
        cairo_arc(cr, thumb_cx, cy, thumb_r, 0, 2 * M_PI);
        cairo_set_source_rgba(cr, thumb_col.r, thumb_col.g, thumb_col.b, thumb_col.a);
        cairo_fill(cr);
    }

    cairo_restore(cr);
}

bool SeekBar::on_mouse_button(int lx, int ly, MouseButton button, bool pressed, const Rect& bounds) {
    if (button != MouseButton::Left) return false;

    int draw_x = bounds.x + m_margin.left;
    int draw_w = std::max(0, bounds.width - m_margin.left - m_margin.right);
    int thumb_r = m_thumb_radius;
    int usable_w = draw_w - thumb_r * 2;
    if (usable_w < 1) usable_w = 1;

    int track_start_x = draw_x + thumb_r;

    if (pressed) {
        m_dragging = true;
        float p = std::clamp(static_cast<float>(lx - track_start_x) / usable_w, 0.0f, 1.0f);
        m_progress = p;
        if (m_on_seek) {
            m_on_seek(m_progress, true);
        }
        if (m_window) m_window->schedule_redraw();
        return true;
    } else {
        if (m_dragging) {
            m_dragging = false;
            float p = std::clamp(static_cast<float>(lx - track_start_x) / usable_w, 0.0f, 1.0f);
            m_progress = p;
            if (m_on_seek) {
                m_on_seek(m_progress, true);
            }
            if (m_window) m_window->schedule_redraw();
            return true;
        }
    }
    return false;
}

bool SeekBar::on_mouse_move(int lx, int ly, const Rect& bounds) {
    int draw_x = bounds.x + m_margin.left;
    int draw_w = std::max(0, bounds.width - m_margin.left - m_margin.right);
    int thumb_r = m_thumb_radius;
    int usable_w = draw_w - thumb_r * 2;
    if (usable_w < 1) usable_w = 1;

    int track_start_x = draw_x + thumb_r;

    bool prev_hovered = m_hovered;
    m_hovered = (lx >= bounds.x && lx <= bounds.x + bounds.width &&
                 ly >= bounds.y && ly <= bounds.y + bounds.height);

    if (m_dragging) {
        float p = std::clamp(static_cast<float>(lx - track_start_x) / usable_w, 0.0f, 1.0f);
        if (std::abs(p - m_progress) > 0.001f) {
            m_progress = p;
            if (m_on_seek) {
                m_on_seek(m_progress, true);
            }
            if (m_window) m_window->schedule_redraw();
        }
        return true;
    }

    if (m_hovered != prev_hovered) {
        if (m_window) m_window->schedule_redraw();
        return true;
    }

    return false;
}

} // namespace miqu
