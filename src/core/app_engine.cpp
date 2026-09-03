#include "miqutoolkit/core/app_engine.hpp"
#include "miqutoolkit/core/window.hpp"
#include "miqutoolkit/system/window_manager.hpp"
#include "miqutoolkit/system/workspace_manager.hpp"
#include <iostream>
#include <cstring>
#include <algorithm>
#include "xdg-shell-client-protocol.h"
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include "wlr-foreign-toplevel-management-unstable-v1-client-protocol.h"
#include "ext-workspace-v1-client-protocol.h"

namespace miqu {

AppEngine* AppEngine::s_instance = nullptr;

const struct xdg_wm_base_listener AppEngine::s_wm_base_listener = {
    .ping = [](void*, struct xdg_wm_base* wm_base, uint32_t serial) {
        xdg_wm_base_pong(wm_base, serial);
    }
};

const struct wl_registry_listener AppEngine::s_registry_listener = {
    .global = registry_global,
    .global_remove = registry_global_remove,
};

const struct wl_seat_listener AppEngine::s_seat_listener = {
    .capabilities = [](void* data, struct wl_seat*, uint32_t caps) {
        auto* self = static_cast<AppEngine*>(data);
        self->m_seat_capabilities = caps;
        for (auto& win : self->m_windows) {
            if (win) win->update_seat_capabilities(caps);
        }
    },
    .name = [](void*, struct wl_seat*, const char*) {}
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
    m_windows.clear();

    if (m_ext_workspace_manager) ext_workspace_manager_v1_destroy(m_ext_workspace_manager);
    if (m_foreign_toplevel_manager) zwlr_foreign_toplevel_manager_v1_destroy(m_foreign_toplevel_manager);
    if (m_xdg_wm_base) xdg_wm_base_destroy(m_xdg_wm_base);
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
        std::cerr << "[miqutoolkit] Failed to connect to Wayland display." << std::endl;
        return false;
    }

    m_registry = wl_display_get_registry(m_display);
    if (!m_registry) {
        std::cerr << "[miqutoolkit] Failed to get Wayland registry." << std::endl;
        return false;
    }

    wl_registry_add_listener(m_registry, &s_registry_listener, this);
    wl_display_roundtrip(m_display);

    if (!m_compositor || !m_shm || (!m_layer_shell && !m_xdg_wm_base)) {
        std::cerr << "[miqutoolkit] Missing required Wayland globals (compositor, shm, and layer_shell or xdg_wm_base)." << std::endl;
        return false;
    }

    wl_display_roundtrip(m_display);

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
    } else if (std::strcmp(interface, xdg_wm_base_interface.name) == 0) {
        self->m_xdg_wm_base = static_cast<struct xdg_wm_base*>(
            wl_registry_bind(registry, name, &xdg_wm_base_interface, 1));
        xdg_wm_base_add_listener(self->m_xdg_wm_base, &s_wm_base_listener, self);
    } else if (std::strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
        self->m_layer_shell = static_cast<struct zwlr_layer_shell_v1*>(
            wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, std::min(version, 4u)));
    } else if (std::strcmp(interface, zwlr_foreign_toplevel_manager_v1_interface.name) == 0) {
        self->m_foreign_toplevel_manager = static_cast<struct zwlr_foreign_toplevel_manager_v1*>(
            wl_registry_bind(registry, name, &zwlr_foreign_toplevel_manager_v1_interface, std::min(version, 3u)));
        WindowManager::get()->init_protocol(self->m_foreign_toplevel_manager);
    } else if (std::strcmp(interface, ext_workspace_manager_v1_interface.name) == 0) {
        self->m_ext_workspace_manager = static_cast<struct ext_workspace_manager_v1*>(
            wl_registry_bind(registry, name, &ext_workspace_manager_v1_interface, 1));
        WorkspaceManager::get()->init_protocol(self->m_ext_workspace_manager);
    } else if (std::strcmp(interface, wl_seat_interface.name) == 0) {
        self->m_seat = static_cast<struct wl_seat*>(
            wl_registry_bind(registry, name, &wl_seat_interface, std::min(version, 7u)));
        wl_seat_add_listener(self->m_seat, &s_seat_listener, self);
    }
}

void AppEngine::registry_global_remove(void* data, struct wl_registry* registry, uint32_t name) {
}

void AppEngine::register_window(std::shared_ptr<Window> win) {
    m_windows.push_back(win);
    if (win && m_seat_capabilities != 0) {
        win->update_seat_capabilities(m_seat_capabilities);
    }
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

} // namespace miqu
