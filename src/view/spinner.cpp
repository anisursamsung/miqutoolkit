#include "miqutoolkit/view/spinner.hpp"
#include "miqutoolkit/view/card_view.hpp"
#include "miqutoolkit/core/config.hpp"
#include "miqutoolkit/core/window.hpp"
#include <pango/pangocairo.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <algorithm>
#include <iostream>

namespace miqu {

class SpinnerPopupView : public View {
public:
    SpinnerPopupView(Spinner* spinner,
                     std::vector<std::string> items,
                     int selected_idx,
                     std::string font_family,
                     int font_size,
                     int item_height,
                     int max_visible_items)
        : m_spinner(spinner),
          m_items(std::move(items)),
          m_selected_index(selected_idx),
          m_hovered_index(selected_idx >= 0 ? selected_idx : 0),
          m_font_family(std::move(font_family)),
          m_font_size(font_size),
          m_item_height(item_height),
          m_max_visible_items(max_visible_items) {
        ensure_visible(m_hovered_index);
    }

    void draw(cairo_t* cr, const Rect& bounds) override {
        if (!cr || bounds.width <= 0 || bounds.height <= 0) return;

        auto config = Config::get();
        int radius = config->metrics.corner_radius > 0 ? config->metrics.corner_radius : 8;

        cairo_save(cr);

        // 1. Draw subtle shadow / elevated background
        CardView::draw_rounded_rect(cr, bounds.x, bounds.y, bounds.width, bounds.height, radius);
        cairo_set_source_rgba(cr, config->colors.surface.r,
                                  config->colors.surface.g,
                                  config->colors.surface.b,
                                  0.98f);
        cairo_fill(cr);

        // 2. Draw border
        CardView::draw_rounded_rect(cr, bounds.x + 0.5, bounds.y + 0.5, bounds.width - 1.0, bounds.height - 1.0, std::max(0, radius - 1));
        cairo_set_source_rgba(cr, config->colors.outline.r,
                                  config->colors.outline.g,
                                  config->colors.outline.b,
                                  config->colors.outline.a);
        cairo_set_line_width(cr, 1.0);
        cairo_stroke(cr);

        // 3. Clip content inside rounded rect
        CardView::draw_rounded_rect(cr, bounds.x + 1, bounds.y + 1, bounds.width - 2, bounds.height - 2, std::max(0, radius - 1));
        cairo_clip(cr);

        // 4. Draw items
        std::string font_family = !m_font_family.empty() ? m_font_family : config->metrics.font_family;
        if (font_family.empty()) font_family = "Sans";
        int font_size = m_font_size > 0 ? m_font_size : config->metrics.font_size;
        if (font_size <= 0) font_size = 11;

        int total_items = static_cast<int>(m_items.size());
        int start_y = bounds.y + 4;

        for (int i = 0; i < total_items; ++i) {
            int item_y = start_y + (i * m_item_height) - m_scroll_offset;
            if (item_y + m_item_height < bounds.y || item_y > bounds.y + bounds.height) {
                continue;
            }

            Rect item_rect(bounds.x + 4, item_y, bounds.width - 8, m_item_height);

            // Item highlight
            bool is_selected = (i == m_selected_index);
            bool is_hovered = (i == m_hovered_index);

            if (is_selected) {
                CardView::draw_rounded_rect(cr, item_rect.x, item_rect.y, item_rect.width, item_rect.height, 6);
                cairo_set_source_rgba(cr, config->colors.primary.r,
                                          config->colors.primary.g,
                                          config->colors.primary.b,
                                          0.20f);
                cairo_fill(cr);
            } else if (is_hovered) {
                CardView::draw_rounded_rect(cr, item_rect.x, item_rect.y, item_rect.width, item_rect.height, 6);
                cairo_set_source_rgba(cr, config->colors.surface_variant.r,
                                          config->colors.surface_variant.g,
                                          config->colors.surface_variant.b,
                                          0.80f);
                cairo_fill(cr);
            }

            // Item text
            PangoLayout* layout = pango_cairo_create_layout(cr);
            pango_layout_set_text(layout, m_items[i].c_str(), -1);

            std::string font_desc_str = font_family + " " + std::to_string(font_size);
            if (is_selected) font_desc_str += " Bold";

            PangoFontDescription* desc = pango_font_description_from_string(font_desc_str.c_str());
            pango_layout_set_font_description(layout, desc);
            pango_font_description_free(desc);

            pango_layout_set_width(layout, (item_rect.width - 32) * PANGO_SCALE);
            pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);

            int text_w = 0, text_h = 0;
            pango_layout_get_pixel_size(layout, &text_w, &text_h);

            int text_x = item_rect.x + 12;
            int text_y = item_rect.y + (m_item_height - text_h) / 2;

            Color text_col = is_selected ? config->colors.primary : config->colors.on_surface;
            cairo_move_to(cr, text_x, text_y);
            cairo_set_source_rgba(cr, text_col.r, text_col.g, text_col.b, text_col.a);
            pango_cairo_show_layout(cr, layout);
            g_object_unref(layout);

            // Checkmark for selected item
            if (is_selected) {
                PangoLayout* check_layout = pango_cairo_create_layout(cr);
                pango_layout_set_text(check_layout, "✓", -1);
                PangoFontDescription* cdesc = pango_font_description_from_string((font_family + " Bold " + std::to_string(font_size)).c_str());
                pango_layout_set_font_description(check_layout, cdesc);
                pango_font_description_free(cdesc);

                int cw = 0, ch = 0;
                pango_layout_get_pixel_size(check_layout, &cw, &ch);
                int check_x = item_rect.x + item_rect.width - cw - 12;
                int check_y = item_rect.y + (m_item_height - ch) / 2;

                cairo_move_to(cr, check_x, check_y);
                cairo_set_source_rgba(cr, config->colors.primary.r, config->colors.primary.g, config->colors.primary.b, config->colors.primary.a);
                pango_cairo_show_layout(cr, check_layout);
                g_object_unref(check_layout);
            }
        }

        cairo_restore(cr);
    }

    bool on_mouse_move(int lx, int ly, const Rect& bounds) override {
        int rel_y = ly - (bounds.y + 4) + m_scroll_offset;
        int idx = (rel_y >= 0 && rel_y < static_cast<int>(m_items.size()) * m_item_height)
                      ? (rel_y / m_item_height)
                      : -1;

        if (idx != m_hovered_index) {
            m_hovered_index = idx;
            return true;
        }
        return false;
    }

    bool on_mouse_button(int lx, int ly, MouseButton button, bool pressed, const Rect& bounds) override {
        if (button != MouseButton::Left) return false;

        if (pressed) {
            int rel_y = ly - (bounds.y + 4) + m_scroll_offset;
            if (rel_y >= 0) {
                int idx = rel_y / m_item_height;
                if (idx >= 0 && idx < static_cast<int>(m_items.size()) && m_spinner) {
                    m_spinner->handle_popup_selected(idx);
                    return true;
                }
            }
        }
        return false;
    }

    bool on_scroll(double delta) override {
        int max_scroll = std::max(0, static_cast<int>(m_items.size()) * m_item_height - (m_max_visible_items * m_item_height));
        if (max_scroll <= 0) return false;

        int scroll_step = m_item_height;
        int new_offset = m_scroll_offset + static_cast<int>(delta * scroll_step);
        new_offset = std::clamp(new_offset, 0, max_scroll);

        if (new_offset != m_scroll_offset) {
            m_scroll_offset = new_offset;
            return true;
        }
        return false;
    }

    bool on_key(const KeyPressEvent& event) override {
        if (!event.pressed || m_items.empty()) return false;

        if (event.keysym == XKB_KEY_Up) {
            if (m_hovered_index > 0) {
                m_hovered_index--;
            } else {
                m_hovered_index = static_cast<int>(m_items.size()) - 1;
            }
            ensure_visible(m_hovered_index);
            return true;
        } else if (event.keysym == XKB_KEY_Down) {
            if (m_hovered_index < static_cast<int>(m_items.size()) - 1) {
                m_hovered_index++;
            } else {
                m_hovered_index = 0;
            }
            ensure_visible(m_hovered_index);
            return true;
        } else if (event.keysym == XKB_KEY_Return || event.keysym == XKB_KEY_KP_Enter || event.keysym == XKB_KEY_space) {
            if (m_hovered_index >= 0 && m_hovered_index < static_cast<int>(m_items.size()) && m_spinner) {
                m_spinner->handle_popup_selected(m_hovered_index);
                return true;
            }
        } else if (event.keysym == XKB_KEY_Escape) {
            if (m_spinner) {
                m_spinner->close_dropdown();
                return true;
            }
        }

        return false;
    }

private:
    void ensure_visible(int index) {
        if (index < 0 || index >= static_cast<int>(m_items.size())) return;
        int item_top = index * m_item_height;
        int item_bottom = item_top + m_item_height;
        int visible_h = m_max_visible_items * m_item_height;

        if (item_top < m_scroll_offset) {
            m_scroll_offset = item_top;
        } else if (item_bottom > m_scroll_offset + visible_h) {
            m_scroll_offset = item_bottom - visible_h;
        }
    }

    Spinner* m_spinner = nullptr;
    std::vector<std::string> m_items;
    int m_selected_index = -1;
    int m_hovered_index = -1;
    std::string m_font_family;
    int m_font_size = -1;
    int m_item_height = 36;
    int m_max_visible_items = 6;
    int m_scroll_offset = 0;
};

Spinner::Spinner() = default;

Spinner::~Spinner() {
    close_dropdown();
}

void Spinner::set_items(std::vector<std::string> items) {
    m_items = std::move(items);
    if (m_selected_index < 0 && !m_items.empty()) {
        m_selected_index = 0;
    } else if (m_selected_index >= static_cast<int>(m_items.size())) {
        m_selected_index = m_items.empty() ? -1 : static_cast<int>(m_items.size()) - 1;
    }
    request_redraw();
}

void Spinner::add_item(std::string item) {
    m_items.push_back(std::move(item));
    if (m_selected_index < 0) {
        m_selected_index = 0;
    }
    request_redraw();
}

void Spinner::clear_items() {
    m_items.clear();
    m_selected_index = -1;
    close_dropdown();
    request_redraw();
}

void Spinner::set_selected_index(int index) {
    if (index >= -1 && index < static_cast<int>(m_items.size())) {
        if (m_selected_index != index) {
            m_selected_index = index;
            request_redraw();
            if (m_on_item_selected && m_selected_index >= 0) {
                m_on_item_selected(m_selected_index, m_items[m_selected_index]);
            }
        }
    }
}

std::string Spinner::get_selected_item() const {
    if (m_selected_index >= 0 && m_selected_index < static_cast<int>(m_items.size())) {
        return m_items[m_selected_index];
    }
    return "";
}

Size Spinner::measure_size() const {
    int w = (m_layout_params.width >= 0) ? m_layout_params.width : m_bounds.width;
    int h = (m_layout_params.height >= 0) ? m_layout_params.height : m_bounds.height;

    if (w > 0 && h > 0) {
        return Size(w, h);
    }

    auto config = Config::get();
    int font_size = m_font_size > 0 ? m_font_size : config->metrics.font_size;
    if (font_size <= 0) font_size = 11;

    int pad_h = (m_padding.left + m_padding.right > 0) ? (m_padding.left + m_padding.right) : 28;
    int pad_v = (m_padding.top + m_padding.bottom > 0) ? (m_padding.top + m_padding.bottom) : 14;

    int max_text_w = 80;
    for (const auto& it : m_items) {
        max_text_w = std::max(max_text_w, static_cast<int>(it.size()) * 8);
    }
    if (!m_prompt.empty()) {
        max_text_w = std::max(max_text_w, static_cast<int>(m_prompt.size()) * 8);
    }

    int measured_w = (w > 0) ? w : (max_text_w + 32 + pad_h);
    int measured_h = (h > 0) ? h : (std::max(24, font_size + 14) + pad_v);

    return Size(measured_w, measured_h);
}

Size Spinner::measure_size(int avail_width) const {
    if (m_layout_params.width == static_cast<int>(LayoutDimension::MatchParent) && avail_width > 0) {
        Size s = measure_size();
        return Size(avail_width, s.height);
    }
    return measure_size();
}

void Spinner::draw(cairo_t* cr, const Rect& bounds) {
    if (!is_visible() || !cr || bounds.width <= 0 || bounds.height <= 0) return;

    m_last_drawn_bounds = bounds;
    auto config = Config::get();

    int radius = (m_corner_radius >= 0) ? m_corner_radius : config->metrics.corner_radius;
    if (radius <= 0) radius = 8;

    cairo_save(cr);

    // 1. Background
    CardView::draw_rounded_rect(cr, bounds.x, bounds.y, bounds.width, bounds.height, radius);
    if (m_has_custom_bg) {
        cairo_set_source_rgba(cr, m_bg_color.r, m_bg_color.g, m_bg_color.b, m_bg_color.a);
    } else if (m_is_open) {
        cairo_set_source_rgba(cr, config->colors.surface.r,
                                  config->colors.surface.g,
                                  config->colors.surface.b,
                                  config->colors.surface.a);
    } else if (m_hovered) {
        cairo_set_source_rgba(cr, config->colors.surface_variant.r,
                                  config->colors.surface_variant.g,
                                  config->colors.surface_variant.b,
                                  0.90f);
    } else {
        cairo_set_source_rgba(cr, config->colors.surface_variant.r,
                                  config->colors.surface_variant.g,
                                  config->colors.surface_variant.b,
                                  0.65f);
    }
    cairo_fill(cr);

    // 2. Border
    CardView::draw_rounded_rect(cr, bounds.x + 0.5, bounds.y + 0.5, bounds.width - 1.0, bounds.height - 1.0, std::max(0, radius - 1));
    if (m_is_open) {
        cairo_set_source_rgba(cr, config->colors.primary.r,
                                  config->colors.primary.g,
                                  config->colors.primary.b,
                                  0.90f);
        cairo_set_line_width(cr, 1.5);
    } else {
        cairo_set_source_rgba(cr, config->colors.outline_variant.r,
                                  config->colors.outline_variant.g,
                                  config->colors.outline_variant.b,
                                  config->colors.outline_variant.a);
        cairo_set_line_width(cr, 1.0);
    }
    cairo_stroke(cr);

    // 3. Label Text
    std::string text_to_display = get_selected_item();
    bool is_placeholder = false;
    if (text_to_display.empty()) {
        text_to_display = m_prompt.empty() ? "Select item..." : m_prompt;
        is_placeholder = true;
    }

    std::string font_family = !m_font_family.empty() ? m_font_family : config->metrics.font_family;
    if (font_family.empty()) font_family = "Sans";
    int font_size = m_font_size > 0 ? m_font_size : config->metrics.font_size;
    if (font_size <= 0) font_size = 11;

    int pad_l = (m_padding.left > 0) ? m_padding.left : 14;
    int pad_r = (m_padding.right > 0) ? m_padding.right : 14;
    int arrow_space = 24;
    int text_max_w = std::max(0, bounds.width - pad_l - pad_r - arrow_space);

    PangoLayout* layout = pango_cairo_create_layout(cr);
    pango_layout_set_text(layout, text_to_display.c_str(), -1);

    std::string font_desc_str = font_family + " " + std::to_string(font_size);
    PangoFontDescription* desc = pango_font_description_from_string(font_desc_str.c_str());
    pango_layout_set_font_description(layout, desc);
    pango_font_description_free(desc);

    pango_layout_set_width(layout, text_max_w * PANGO_SCALE);
    pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);

    int text_w = 0, text_h = 0;
    pango_layout_get_pixel_size(layout, &text_w, &text_h);

    int text_x = bounds.x + pad_l;
    int text_y = bounds.y + (bounds.height - text_h) / 2;

    Color text_col;
    if (m_has_custom_color) {
        text_col = m_text_color;
    } else {
        text_col = is_placeholder ? config->colors.on_surface_variant : config->colors.on_surface;
    }

    cairo_move_to(cr, text_x, text_y);
    cairo_set_source_rgba(cr, text_col.r, text_col.g, text_col.b, text_col.a);
    pango_cairo_show_layout(cr, layout);
    g_object_unref(layout);

    // 4. Dropdown Chevron Arrow (▾)
    PangoLayout* arrow_layout = pango_cairo_create_layout(cr);
    pango_layout_set_text(arrow_layout, m_is_open ? "▴" : "▾", -1);
    PangoFontDescription* adesc = pango_font_description_from_string((font_family + " " + std::to_string(font_size + 1)).c_str());
    pango_layout_set_font_description(arrow_layout, adesc);
    pango_font_description_free(adesc);

    int aw = 0, ah = 0;
    pango_layout_get_pixel_size(arrow_layout, &aw, &ah);
    int arrow_x = bounds.x + bounds.width - pad_r - aw;
    int arrow_y = bounds.y + (bounds.height - ah) / 2;

    Color arrow_col = m_is_open ? config->colors.primary : config->colors.on_surface_variant;
    cairo_move_to(cr, arrow_x, arrow_y);
    cairo_set_source_rgba(cr, arrow_col.r, arrow_col.g, arrow_col.b, arrow_col.a);
    pango_cairo_show_layout(cr, arrow_layout);
    g_object_unref(arrow_layout);

    cairo_restore(cr);
}

void Spinner::open_dropdown() {
    if (m_is_open || !m_window || m_items.empty()) return;

    m_is_open = true;

    int visible_count = std::min(static_cast<int>(m_items.size()), m_max_visible_items);
    int popup_h = (visible_count * m_item_height) + 8;
    int popup_w = std::max(m_last_drawn_bounds.width, 140);

    int win_w = m_window->get_width();
    int win_h = m_window->get_height();

    int popup_x = std::clamp(m_last_drawn_bounds.x, 6, std::max(6, win_w - popup_w - 6));
    int popup_y = m_last_drawn_bounds.y + m_last_drawn_bounds.height + 4;

    // Flip upwards if exceeding window bottom bounds
    if (popup_y + popup_h > win_h - 10 && m_last_drawn_bounds.y - popup_h - 4 >= 10) {
        popup_y = m_last_drawn_bounds.y - popup_h - 4;
    }

    m_popup_view = std::make_shared<SpinnerPopupView>(
        this,
        m_items,
        m_selected_index,
        m_font_family,
        m_font_size,
        m_item_height,
        m_max_visible_items
    );

    m_window->show_popup(m_popup_view, Rect(popup_x, popup_y, popup_w, popup_h));
    request_redraw();
}

void Spinner::close_dropdown() {
    if (!m_is_open) return;
    m_is_open = false;
    if (m_window) {
        m_window->dismiss_popup();
    }
    m_popup_view = nullptr;
    request_redraw();
}

void Spinner::handle_popup_selected(int index) {
    close_dropdown();
    set_selected_index(index);
}

void Spinner::handle_popup_dismissed() {
    m_is_open = false;
    m_popup_view = nullptr;
    request_redraw();
}

bool Spinner::on_mouse_move(int lx, int ly, const Rect& bounds) {
    bool hovered = bounds.contains(Point(lx, ly));
    if (hovered != m_hovered) {
        m_hovered = hovered;
        request_redraw();
        return true;
    }
    return false;
}

bool Spinner::on_mouse_button(int lx, int ly, MouseButton button, bool pressed, const Rect& bounds) {
    if (button != MouseButton::Left) return false;

    if (pressed && bounds.contains(Point(lx, ly))) {
        if (m_is_open) {
            close_dropdown();
        } else {
            open_dropdown();
        }
        return true;
    }
    return false;
}

bool Spinner::on_key(const KeyPressEvent& event) {
    if (!event.pressed) return false;

    if (event.keysym == XKB_KEY_Return || event.keysym == XKB_KEY_KP_Enter || event.keysym == XKB_KEY_space || event.keysym == XKB_KEY_Down) {
        if (!m_is_open) {
            open_dropdown();
            return true;
        }
    }
    return false;
}

} // namespace miqu
