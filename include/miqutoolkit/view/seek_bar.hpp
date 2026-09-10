#pragma once

#include "miqutoolkit/view/view.hpp"
#include <functional>
#include <algorithm>

namespace miqu {

class SeekBar : public View {
public:
    SeekBar() = default;

    void set_progress(float progress) {
        m_progress = std::clamp(progress, 0.0f, 1.0f);
    }
    float get_progress() const { return m_progress; }

    void set_track_color(const Color& col) { m_track_color = col; m_custom_track = true; }
    void set_progress_color(const Color& col) { m_progress_color = col; m_custom_progress = true; }
    void set_thumb_color(const Color& col) { m_thumb_color = col; m_custom_thumb = true; }

    void set_track_height(int height) { m_track_height = std::max(1, height); }
    int get_track_height() const { return m_track_height; }

    void set_thumb_radius(int radius) { m_thumb_radius = std::max(0, radius); }
    int get_thumb_radius() const { return m_thumb_radius; }

    void set_thumb_visible(bool visible) { m_thumb_visible = visible; }
    bool is_thumb_visible() const { return m_thumb_visible; }

    void set_on_seek_listener(std::function<void(float progress, bool from_user)> listener) {
        m_on_seek = std::move(listener);
    }

    void draw(cairo_t* cr, const Rect& bounds) override;
    Size measure_size() const override;

    bool on_mouse_move(int lx, int ly, const Rect& bounds) override;
    bool on_mouse_button(int lx, int ly, MouseButton button, bool pressed, const Rect& bounds) override;

private:
    float m_progress = 0.0f;
    int m_track_height = 4;
    int m_thumb_radius = 6;
    bool m_thumb_visible = true;

    bool m_custom_track = false;
    Color m_track_color;

    bool m_custom_progress = false;
    Color m_progress_color;

    bool m_custom_thumb = false;
    Color m_thumb_color;

    bool m_dragging = false;
    bool m_hovered = false;

    std::function<void(float, bool)> m_on_seek;
};

class SeekBarBuilder : public std::enable_shared_from_this<SeekBarBuilder> {
public:
    SeekBarBuilder() : m_view(std::make_shared<SeekBar>()) {}

    static std::shared_ptr<SeekBarBuilder> create() {
        return std::make_shared<SeekBarBuilder>();
    }

    std::shared_ptr<SeekBarBuilder> progress(float p) {
        m_view->set_progress(p);
        return shared_from_this();
    }

    std::shared_ptr<SeekBarBuilder> trackColor(const Color& col) {
        m_view->set_track_color(col);
        return shared_from_this();
    }

    std::shared_ptr<SeekBarBuilder> progressColor(const Color& col) {
        m_view->set_progress_color(col);
        return shared_from_this();
    }

    std::shared_ptr<SeekBarBuilder> thumbColor(const Color& col) {
        m_view->set_thumb_color(col);
        return shared_from_this();
    }

    std::shared_ptr<SeekBarBuilder> trackHeight(int h) {
        m_view->set_track_height(h);
        return shared_from_this();
    }

    std::shared_ptr<SeekBarBuilder> thumbRadius(int r) {
        m_view->set_thumb_radius(r);
        return shared_from_this();
    }

    std::shared_ptr<SeekBarBuilder> thumbVisible(bool visible) {
        m_view->set_thumb_visible(visible);
        return shared_from_this();
    }

    std::shared_ptr<SeekBarBuilder> onSeek(std::function<void(float, bool)> listener) {
        m_view->set_on_seek_listener(std::move(listener));
        return shared_from_this();
    }

    std::shared_ptr<SeekBar> build() {
        return m_view;
    }

private:
    std::shared_ptr<SeekBar> m_view;
};

} // namespace miqu
