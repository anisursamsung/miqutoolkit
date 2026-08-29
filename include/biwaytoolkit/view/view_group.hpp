#pragma once

#include "biwaytoolkit/view/view.hpp"
#include <vector>
#include <memory>
#include <algorithm>

namespace biway {

class ViewGroup : public View {
public:
    ViewGroup() = default;
    virtual ~ViewGroup() = default;

    virtual void add_view(std::shared_ptr<View> child);
    virtual void add_view(std::shared_ptr<View> child, const LayoutParams& params);
    virtual void remove_view(std::shared_ptr<View> child);
    virtual void clear_views();

    size_t get_child_count() const { return m_children.size(); }
    std::shared_ptr<View> get_child_at(size_t index) const {
        if (index < m_children.size()) return m_children[index];
        return nullptr;
    }

    void set_window(Window* win) override;

    bool on_mouse_move(int lx, int ly, const Rect& bounds) override;
    bool on_mouse_button(int lx, int ly, MouseButton button, bool pressed, const Rect& bounds) override;
    bool on_key(const KeyPressEvent& event) override;
    bool on_scroll(double delta) override;

protected:
    struct ChildEntry {
        std::shared_ptr<View> view;
        Rect allocated_bounds;
    };

    std::vector<std::shared_ptr<View>> m_children;
    std::vector<ChildEntry> m_child_entries;
};

} // namespace biway
