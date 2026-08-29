#pragma once

#include "miqutoolkit/core/geometry.hpp"
#include "miqutoolkit/core/color.hpp"
#include "miqutoolkit/core/types.hpp"
#include "miqutoolkit/view/layout_params.hpp"
#include <cairo.h>
#include <memory>
#include <vector>
#include <functional>

namespace miqu {

class Window;

enum class Visibility {
    Visible,
    Invisible,
    Gone,
};

class View : public std::enable_shared_from_this<View> {
public:
    virtual ~View() = default;

    virtual void draw(cairo_t* cr, const Rect& bounds) = 0;

    virtual Size measure_size() const {
        return Size(m_bounds.width, m_bounds.height);
    }

    virtual bool on_mouse_move(int lx, int ly, const Rect& bounds) { return false; }
    virtual bool on_mouse_button(int lx, int ly, MouseButton button, bool pressed, const Rect& bounds) { return false; }
    virtual bool on_key(const KeyPressEvent& event) { return false; }
    virtual bool on_scroll(double delta) { return false; }

    // Bounds & Geometry
    void set_bounds(const Rect& r) { m_bounds = r; }
    const Rect& get_bounds() const { return m_bounds; }

    // Padding (Inner spacing)
    void set_padding(const Padding& p) { m_padding = p; }
    void set_padding(int uniform) { m_padding = Padding(uniform); }
    void set_padding(int horizontal, int vertical) { m_padding = Padding(horizontal, vertical); }
    void set_padding(int l, int t, int r, int b) { m_padding = Padding(l, t, r, b); }
    const Padding& get_padding() const { return m_padding; }

    // Margin (Outer spacing)
    void set_margin(const Margin& m) { m_margin = m; }
    void set_margin(int uniform) { m_margin = Margin(uniform); }
    void set_margin(int horizontal, int vertical) { m_margin = Margin(horizontal, vertical); }
    void set_margin(int l, int t, int r, int b) { m_margin = Margin(l, t, r, b); }
    const Margin& get_margin() const { return m_margin; }

    // Layout Params
    void set_layout_params(const LayoutParams& params) { m_layout_params = params; }
    const LayoutParams& get_layout_params() const { return m_layout_params; }
    LayoutParams& get_layout_params() { return m_layout_params; }

    // Content Rectangle after applying padding
    Rect get_content_rect(const Rect& bounds) const {
        int cx = bounds.x + m_padding.left;
        int cy = bounds.y + m_padding.top;
        int cw = std::max(0, bounds.width - m_padding.left - m_padding.right);
        int ch = std::max(0, bounds.height - m_padding.top - m_padding.bottom);
        return Rect(cx, cy, cw, ch);
    }

    // Visibility
    void set_visibility(Visibility v) { m_visibility = v; }
    Visibility get_visibility() const { return m_visibility; }
    bool is_visible() const { return m_visibility == Visibility::Visible; }

    // Click Listener
    void set_on_click_listener(std::function<void()> listener) { m_on_click = std::move(listener); }

    // Window Reference
    virtual void set_window(Window* win) { m_window = win; }
    Window* get_window() const { return m_window; }

protected:
    Rect m_bounds;
    Padding m_padding;
    Margin m_margin;
    LayoutParams m_layout_params;
    Visibility m_visibility = Visibility::Visible;
    Window* m_window = nullptr;
    std::function<void()> m_on_click;
};

} // namespace miqu
