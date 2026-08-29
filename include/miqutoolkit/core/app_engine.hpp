#pragma once

#include <wayland-client.h>
#include <memory>
#include <vector>
#include <functional>
#include <string>

struct zwlr_layer_shell_v1;

namespace miqu {

class Window;

class AppEngine {
public:
    static std::shared_ptr<AppEngine> create();
    ~AppEngine();

    int enter_loop();
    void quit(int exit_code = 0);

    struct wl_display* get_display() const { return m_display; }
    struct wl_compositor* get_compositor() const { return m_compositor; }
    struct wl_shm* get_shm() const { return m_shm; }
    struct zwlr_layer_shell_v1* get_layer_shell() const { return m_layer_shell; }
    struct wl_seat* get_seat() const { return m_seat; }

    void register_window(std::shared_ptr<Window> win);
    void unregister_window(std::shared_ptr<Window> win);

    static AppEngine* instance() { return s_instance; }

private:
    AppEngine() = default;
    bool init();

    static void registry_global(void* data, struct wl_registry* registry, uint32_t name, const char* interface, uint32_t version);
    static void registry_global_remove(void* data, struct wl_registry* registry, uint32_t name);
    static const struct wl_registry_listener s_registry_listener;

    static AppEngine* s_instance;

    struct wl_display* m_display = nullptr;
    struct wl_registry* m_registry = nullptr;
    struct wl_compositor* m_compositor = nullptr;
    struct wl_shm* m_shm = nullptr;
    struct zwlr_layer_shell_v1* m_layer_shell = nullptr;
    struct wl_seat* m_seat = nullptr;

    bool m_running = false;
    int m_exit_code = 0;
    std::vector<std::shared_ptr<Window>> m_windows;
};

} // namespace miqu
