#include "biwaytoolkit/core/app_engine.hpp"
#include "biwaytoolkit/core/window.hpp"
#include <iostream>
#include <cstring>
#include <algorithm>
#include "wlr-layer-shell-unstable-v1-client-protocol.h"

namespace biway {

AppEngine* AppEngine::s_instance = nullptr;

const struct wl_registry_listener AppEngine::s_registry_listener = {
    .global = registry_global,
    .global_remove = registry_global_remove,
};

std::shared_ptr<AppEngine> AppEngine::create() {
    if (s_instance) {
        return nullptr;
    }

    auto engine = std::shared_ptr<AppEngine>(new AppEngine());
    if (!engine->init()) {
        return nullptr;
    }

    s_instance = engine.get();
    return engine;
}

AppEngine::~AppEngine() {
    if (m_seat) wl_seat_destroy(m_seat);
    if (m_layer_shell) zwlr_layer_shell_v1_destroy(m_layer_shell);
    if (m_shm) wl_shm_destroy(m_shm);
    if (m_compositor) wl_compositor_destroy(m_compositor);
    if (m_registry) wl_registry_destroy(m_registry);
    if (m_display) wl_display_disconnect(m_display);

    if (s_instance == this) {
        s_instance = nullptr;
    }
}

bool AppEngine::init() {
    m_display = wl_display_connect(nullptr);
    if (!m_display) {
        std::cerr << "[biwaytoolkit] Failed to connect to Wayland display." << std::endl;
        return false;
    }

    m_registry = wl_display_get_registry(m_display);
    if (!m_registry) {
        std::cerr << "[biwaytoolkit] Failed to get Wayland registry." << std::endl;
        return false;
    }

    wl_registry_add_listener(m_registry, &s_registry_listener, this);
    wl_display_roundtrip(m_display);

    if (!m_compositor || !m_shm || !m_layer_shell) {
        std::cerr << "[biwaytoolkit] Missing required Wayland globals (compositor, shm, or layer_shell)." << std::endl;
        return false;
    }

    return true;
}

void AppEngine::registry_global(void* data, struct wl_registry* registry, uint32_t name, const char* interface, uint32_t version) {
    auto* self = static_cast<AppEngine*>(data);

    if (std::strcmp(interface, wl_compositor_interface.name) == 0) {
        self->m_compositor = static_cast<struct wl_compositor*>(
            wl_registry_bind(registry, name, &wl_compositor_interface, std::min(version, 4u)));
    } else if (std::strcmp(interface, wl_shm_interface.name) == 0) {
        self->m_shm = static_cast<struct wl_shm*>(
            wl_registry_bind(registry, name, &wl_shm_interface, 1));
    } else if (std::strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
        self->m_layer_shell = static_cast<struct zwlr_layer_shell_v1*>(
            wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, std::min(version, 4u)));
    } else if (std::strcmp(interface, wl_seat_interface.name) == 0) {
        self->m_seat = static_cast<struct wl_seat*>(
            wl_registry_bind(registry, name, &wl_seat_interface, std::min(version, 7u)));
    }
}

void AppEngine::registry_global_remove(void* data, struct wl_registry* registry, uint32_t name) {
}

void AppEngine::register_window(std::shared_ptr<Window> win) {
    m_windows.push_back(win);
}

void AppEngine::unregister_window(std::shared_ptr<Window> win) {
    auto it = std::find(m_windows.begin(), m_windows.end(), win);
    if (it != m_windows.end()) {
        m_windows.erase(it);
    }
    if (m_windows.empty()) {
        quit(0);
    }
}

int AppEngine::enter_loop() {
    m_running = true;
    while (m_running && !m_windows.empty()) {
        if (wl_display_dispatch(m_display) < 0) {
            break;
        }
    }
    return m_exit_code;
}

void AppEngine::quit(int exit_code) {
    m_exit_code = exit_code;
    m_running = false;
}

} // namespace biway
