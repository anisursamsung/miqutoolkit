#include "biwaytoolkit/view/view_group.hpp"

namespace biway {

void ViewGroup::add_view(std::shared_ptr<View> child) {
    if (!child) return;
    if (m_window) child->set_window(m_window);
    m_children.push_back(std::move(child));
}

void ViewGroup::add_view(std::shared_ptr<View> child, const LayoutParams& params) {
    if (!child) return;
    child->set_layout_params(params);
    add_view(std::move(child));
}

void ViewGroup::remove_view(std::shared_ptr<View> child) {
    auto it = std::remove(m_children.begin(), m_children.end(), child);
    if (it != m_children.end()) {
        m_children.erase(it, m_children.end());
    }
}

void ViewGroup::clear_views() {
    m_children.clear();
    m_child_entries.clear();
}

void ViewGroup::set_window(Window* win) {
    View::set_window(win);
    for (auto& child : m_children) {
        if (child) child->set_window(win);
    }
}

bool ViewGroup::on_mouse_move(int lx, int ly, const Rect& bounds) {
    if (!is_visible()) return false;
    bool handled = false;

    for (auto& entry : m_child_entries) {
        if (entry.view && entry.view->is_visible()) {
            if (entry.view->on_mouse_move(lx, ly, entry.allocated_bounds)) {
                handled = true;
            }
        }
    }
    return handled;
}

bool ViewGroup::on_mouse_button(int lx, int ly, MouseButton button, bool pressed, const Rect& bounds) {
    if (!is_visible()) return false;

    // Traverse top-to-bottom (reverse order)
    for (auto it = m_child_entries.rbegin(); it != m_child_entries.rend(); ++it) {
        if (it->view && it->view->is_visible()) {
            if (it->view->on_mouse_button(lx, ly, button, pressed, it->allocated_bounds)) {
                return true;
            }
        }
    }
    return false;
}

bool ViewGroup::on_key(const KeyPressEvent& event) {
    if (!is_visible()) return false;

    for (auto& child : m_children) {
        if (child && child->is_visible()) {
            if (child->on_key(event)) {
                return true;
            }
        }
    }
    return false;
}

bool ViewGroup::on_scroll(double delta) {
    if (!is_visible()) return false;

    for (auto& child : m_children) {
        if (child && child->is_visible()) {
            if (child->on_scroll(delta)) {
                return true;
            }
        }
    }
    return false;
}

} // namespace biway
