#include "miqutoolkit/core/window.hpp"
#include "miqutoolkit/core/app_engine.hpp"
#include "miqutoolkit/core/color_scheme.hpp"
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>
#include <algorithm>

namespace miqu {

const struct zwlr_layer_surface_v1_listener Window::s_layer_surface_listener = {
    .configure = [](void* data, struct zwlr_layer_surface_v1* surface, uint32_t serial, uint32_t w, uint32_t h) {
        auto* self = static_cast<Window*>(data);
        zwlr_layer_surface_v1_ack_configure(surface, serial);

        if (w > 0 && h > 0) {
            self->m_width = w;
            self->m_height = h;
        }

        if (!self->m_shm_pool) {
            self->m_shm_pool = std::make_unique<ShmPool>(AppEngine::instance()->get_shm(), self->m_width, self->m_height);
        } else {
            self->m_shm_pool->resize(self->m_width, self->m_height);
        }

        self->m_configured = true;
        self->schedule_redraw();
    },
    .closed = [](void* data, struct zwlr_layer_surface_v1* surface) {
        auto* self = static_cast<Window*>(data);
        if (self->m_on_close) {
            self->m_on_close();
        }
        self->close();
    }
};

void Window::update_seat_capabilities(uint32_t caps) {
    auto* engine = AppEngine::instance();
    if (!engine || !engine->get_seat()) return;
    struct wl_seat* seat = engine->get_seat();

    if ((caps & WL_SEAT_CAPABILITY_POINTER) && !m_pointer) {
        m_pointer = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(m_pointer, &s_pointer_listener, this);
    } else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && m_pointer) {
        wl_pointer_destroy(m_pointer);
        m_pointer = nullptr;
    }

    if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) && !m_keyboard) {
        m_keyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(m_keyboard, &s_keyboard_listener, this);
    } else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && m_keyboard) {
        wl_keyboard_destroy(m_keyboard);
        m_keyboard = nullptr;
    }
}

const struct wl_pointer_listener Window::s_pointer_listener = {
    .enter = [](void* data, struct wl_pointer*, uint32_t serial, struct wl_surface* surface, wl_fixed_t sx, wl_fixed_t sy) {
        auto* self = static_cast<Window*>(data);
        self->m_last_x = wl_fixed_to_double(sx);
        self->m_last_y = wl_fixed_to_double(sy);
        if (self->m_root_view) {
            self->m_root_view->on_mouse_move(self->m_last_x, self->m_last_y, self->m_allocated_content_bounds);
            self->schedule_redraw();
        }
    },
    .leave = [](void* data, struct wl_pointer*, uint32_t serial, struct wl_surface* surface) {},
    .motion = [](void* data, struct wl_pointer*, uint32_t time, wl_fixed_t sx, wl_fixed_t sy) {
        auto* self = static_cast<Window*>(data);
        self->m_last_x = wl_fixed_to_double(sx);
        self->m_last_y = wl_fixed_to_double(sy);
        if (self->m_root_view) {
            if (self->m_root_view->on_mouse_move(self->m_last_x, self->m_last_y, self->m_allocated_content_bounds)) {
                self->schedule_redraw();
            }
        }
    },
    .button = [](void* data, struct wl_pointer*, uint32_t serial, uint32_t time, uint32_t button, uint32_t state) {
        auto* self = static_cast<Window*>(data);
        MouseButton mb = MouseButton::Left;
        if (button == 0x111) mb = MouseButton::Right;
        else if (button == 0x112) mb = MouseButton::Middle;

        bool pressed = (state == WL_POINTER_BUTTON_STATE_PRESSED);

        if (pressed && self->m_close_on_click_outside && !self->m_allocated_content_bounds.contains(self->m_last_x, self->m_last_y)) {
            if (self->m_on_close) self->m_on_close();
            self->close();
            return;
        }

        if (self->m_root_view) {
            if (self->m_root_view->on_mouse_button(self->m_last_x, self->m_last_y, mb, pressed, self->m_allocated_content_bounds)) {
                self->schedule_redraw();
            }
        }
    },
    .axis = [](void* data, struct wl_pointer*, uint32_t time, uint32_t axis, wl_fixed_t value) {
        auto* self = static_cast<Window*>(data);
        if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL && self->m_root_view) {
            double delta = wl_fixed_to_double(value);
            if (self->m_root_view->on_scroll(delta)) {
                self->schedule_redraw();
            }
        }
    },
    .frame = [](void*, struct wl_pointer*) {},
    .axis_source = [](void*, struct wl_pointer*, uint32_t) {},
    .axis_stop = [](void*, struct wl_pointer*, uint32_t, uint32_t) {},
    .axis_discrete = [](void*, struct wl_pointer*, uint32_t, int32_t) {},
    .axis_value120 = [](void*, struct wl_pointer*, uint32_t, int32_t) {},
    .axis_relative_direction = [](void*, struct wl_pointer*, uint32_t, uint32_t) {},
#ifdef WL_POINTER_WARP_SINCE_VERSION
    .warp = [](void*, struct wl_pointer*, wl_fixed_t, wl_fixed_t) {},
#endif
};

const struct wl_keyboard_listener Window::s_keyboard_listener = {
    .keymap = [](void* data, struct wl_keyboard*, uint32_t format, int32_t fd, uint32_t size) {
        auto* self = static_cast<Window*>(data);
        if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
            ::close(fd);
            return;
        }

        char* map_shm = static_cast<char*>(mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0));
        if (map_shm == MAP_FAILED) {
            ::close(fd);
            return;
        }

        if (!self->m_xkb_ctx) {
            self->m_xkb_ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
        }

        if (self->m_xkb_keymap) xkb_keymap_unref(self->m_xkb_keymap);
        self->m_xkb_keymap = xkb_keymap_new_from_string(
            self->m_xkb_ctx,
            map_shm,
            XKB_KEYMAP_FORMAT_TEXT_V1,
            XKB_KEYMAP_COMPILE_NO_FLAGS
        );

        munmap(map_shm, size);
        ::close(fd);

        if (self->m_xkb_state) xkb_state_unref(self->m_xkb_state);
        self->m_xkb_state = xkb_state_new(self->m_xkb_keymap);
    },
    .enter = [](void*, struct wl_keyboard*, uint32_t, struct wl_surface*, struct wl_array*) {},
    .leave = [](void*, struct wl_keyboard*, uint32_t, struct wl_surface*) {},
    .key = [](void* data, struct wl_keyboard*, uint32_t serial, uint32_t time, uint32_t key, uint32_t state) {
        auto* self = static_cast<Window*>(data);
        if (!self->m_xkb_state) return;

        uint32_t keycode = key + 8;
        xkb_keysym_t sym = xkb_state_key_get_one_sym(self->m_xkb_state, keycode);
        bool pressed = (state == WL_KEYBOARD_KEY_STATE_PRESSED);

        if (pressed && self->m_close_on_escape && sym == XKB_KEY_Escape) {
            if (self->m_on_close) self->m_on_close();
            self->close();
            return;
        }

        char utf8_buf[64] = {0};
        xkb_state_key_get_utf8(self->m_xkb_state, keycode, utf8_buf, sizeof(utf8_buf));

        KeyPressEvent event;
        event.keysym = sym;
        event.raw_code = keycode;
        event.utf8_text = utf8_buf;
        event.pressed = pressed;
        event.modifiers = self->m_modifiers;

        if (self->m_on_key) {
            self->m_on_key(event);
        }

        if (self->m_root_view) {
            if (self->m_root_view->on_key(event)) {
                self->schedule_redraw();
            }
        }
    },
    .modifiers = [](void* data, struct wl_keyboard*, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group) {
        auto* self = static_cast<Window*>(data);
        if (self->m_xkb_state) {
            xkb_state_update_mask(self->m_xkb_state, mods_depressed, mods_latched, mods_locked, 0, 0, group);
            self->m_modifiers = 0;
            if (xkb_state_mod_name_is_active(self->m_xkb_state, XKB_MOD_NAME_SHIFT, XKB_STATE_MODS_EFFECTIVE) > 0) self->m_modifiers |= static_cast<uint32_t>(KeyboardModifier::Shift);
            if (xkb_state_mod_name_is_active(self->m_xkb_state, XKB_MOD_NAME_CTRL, XKB_STATE_MODS_EFFECTIVE) > 0) self->m_modifiers |= static_cast<uint32_t>(KeyboardModifier::Control);
            if (xkb_state_mod_name_is_active(self->m_xkb_state, XKB_MOD_NAME_ALT, XKB_STATE_MODS_EFFECTIVE) > 0) self->m_modifiers |= static_cast<uint32_t>(KeyboardModifier::Alt);
            if (xkb_state_mod_name_is_active(self->m_xkb_state, XKB_MOD_NAME_LOGO, XKB_STATE_MODS_EFFECTIVE) > 0) self->m_modifiers |= static_cast<uint32_t>(KeyboardModifier::Super);
        }
    },
    .repeat_info = [](void*, struct wl_keyboard*, int32_t, int32_t) {}
};

const struct wl_callback_listener Window::s_frame_listener = {
    .done = [](void* data, struct wl_callback* callback, uint32_t time) {
        auto* self = static_cast<Window*>(data);
        if (self->m_frame_callback) {
            wl_callback_destroy(self->m_frame_callback);
            self->m_frame_callback = nullptr;
        }
        if (self->m_needs_redraw) {
            self->render_frame();
        }
    }
};

Window::Window() {
    m_anchors = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
                ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
}

Window::~Window() {
    if (m_frame_callback) wl_callback_destroy(m_frame_callback);
    if (m_xkb_state) xkb_state_unref(m_xkb_state);
    if (m_xkb_keymap) xkb_keymap_unref(m_xkb_keymap);
    if (m_xkb_ctx) xkb_context_unref(m_xkb_ctx);
    if (m_pointer) wl_pointer_destroy(m_pointer);
    if (m_keyboard) wl_keyboard_destroy(m_keyboard);
    if (m_layer_surface) zwlr_layer_surface_v1_destroy(m_layer_surface);
    if (m_surface) wl_surface_destroy(m_surface);
}

bool Window::init() {
    auto* engine = AppEngine::instance();
    if (!engine) return false;

    m_surface = wl_compositor_create_surface(engine->get_compositor());
    if (!m_surface) return false;

    uint32_t layer = ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY;
    if (m_role == WindowRole::LayerTop) layer = ZWLR_LAYER_SHELL_V1_LAYER_TOP;
    else if (m_role == WindowRole::LayerBottom) layer = ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM;
    else if (m_role == WindowRole::LayerBackground) layer = ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND;

    m_layer_surface = zwlr_layer_shell_v1_get_layer_surface(
        engine->get_layer_shell(),
        m_surface,
        nullptr,
        layer,
        "miqutoolkit-overlay"
    );

    if (!m_layer_surface) return false;

    zwlr_layer_surface_v1_add_listener(m_layer_surface, &s_layer_surface_listener, this);

    if (engine->get_seat() && engine->get_seat_capabilities() != 0) {
        update_seat_capabilities(engine->get_seat_capabilities());
    }

    zwlr_layer_surface_v1_set_anchor(m_layer_surface, m_anchors);
    zwlr_layer_surface_v1_set_exclusive_zone(m_layer_surface, m_exclusive_zone);

    if (m_kb_interactive) {
        zwlr_layer_surface_v1_set_keyboard_interactivity(m_layer_surface, ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_EXCLUSIVE);
    }

    wl_surface_commit(m_surface);
    wl_display_roundtrip(engine->get_display());

    engine->register_window(shared_from_this());
    return true;
}

void Window::set_content_view(std::shared_ptr<View> view) {
    m_root_view = view;
    if (m_root_view) {
        m_root_view->set_window(this);
    }
    schedule_redraw();
}

void Window::schedule_redraw() {
    if (!m_configured) return;
    if (m_frame_callback) {
        m_needs_redraw = true;
        return;
    }
    render_frame();
}

void Window::render_frame() {
    if (!m_configured || !m_shm_pool || !m_surface) return;
    m_needs_redraw = false;

    auto* buf = m_shm_pool->get_next_buffer();
    if (!buf || !buf->cr) return;

    // Clear buffer
    cairo_save(buf->cr);
    cairo_set_operator(buf->cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(buf->cr);
    cairo_restore(buf->cr);

    // 1. Draw Dim Backdrop if enabled
    if (m_dim_backdrop) {
        auto theme = ColorScheme::get();
        cairo_save(buf->cr);
        cairo_set_source_rgba(buf->cr, theme->colors.backdrop.r,
                                      theme->colors.backdrop.g,
                                      theme->colors.backdrop.b,
                                      theme->colors.backdrop.a);
        cairo_rectangle(buf->cr, 0, 0, m_width, m_height);
        cairo_fill(buf->cr);
        cairo_restore(buf->cr);
    }

    // 2. Draw Content View
    if (m_root_view && m_root_view->is_visible()) {
        Rect content_bounds(0, 0, m_width, m_height);
        if (m_content_w > 0 && m_content_h > 0) {
            int cw = std::min(m_content_w, m_width);
            int ch = std::min(m_content_h, m_height);
            int cx = (m_width - cw) / 2;
            int cy = (m_height - ch) / 2;
            content_bounds = Rect(cx, cy, cw, ch);
        }
        m_allocated_content_bounds = content_bounds;
        m_root_view->draw(buf->cr, content_bounds);
    }

    cairo_surface_flush(buf->cairo_surf);
    buf->busy = true;

    wl_surface_attach(m_surface, buf->wl_buf, 0, 0);
    wl_surface_damage_buffer(m_surface, 0, 0, m_width, m_height);

    m_frame_callback = wl_surface_frame(m_surface);
    wl_callback_add_listener(m_frame_callback, &s_frame_listener, this);

    wl_surface_commit(m_surface);
    wl_display_flush(AppEngine::instance()->get_display());
}

void Window::close() {
    auto* engine = AppEngine::instance();
    if (engine) {
        engine->unregister_window(shared_from_this());
    }
}

} // namespace miqu
