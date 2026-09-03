#pragma once

#include "miqutoolkit/view/view.hpp"
#include <string>

namespace miqu {

enum class FitMode {
    Contain, // Scales to fit inside bounds, preserving aspect ratio (default)
    Cover,   // Scales to fill entire bounds, preserving aspect ratio, cropped
    Fill,    // Stretches to fill bounds width & height (ignores aspect ratio)
    Center,  // Keeps target size, centered within bounds
};

class ImageView : public View {
public:
    ImageView() = default;
    explicit ImageView(std::string source) : m_source(std::move(source)) {}

    void set_image_resource(std::string source) { m_source = std::move(source); }
    const std::string& get_image_resource() const { return m_source; }

    void set_target_size(int size) { m_target_size = size; }
    int get_target_size() const { return m_target_size; }

    void set_fit_mode(FitMode mode) { m_fit_mode = mode; }
    FitMode get_fit_mode() const { return m_fit_mode; }

    void set_corner_radius(int radius) { m_corner_radius = radius; }
    int get_corner_radius() const { return m_corner_radius; }

    void set_alpha(float opacity) { m_opacity = opacity; }
    float get_alpha() const { return m_opacity; }

    void draw(cairo_t* cr, const Rect& bounds) override;
    Size measure_size() const override { return Size(m_target_size, m_target_size); }

    static std::string resolve_icon_path(const std::string& icon_name);
    static void clear_cache();

private:
    std::string m_source;
    int m_target_size = 48;
    FitMode m_fit_mode = FitMode::Contain;
    int m_corner_radius = 0;
    float m_opacity = 1.0f;
};

class ImageViewBuilder : public std::enable_shared_from_this<ImageViewBuilder> {
public:
    ImageViewBuilder() : m_view(std::make_shared<ImageView>()) {}

    static std::shared_ptr<ImageViewBuilder> create() {
        return std::make_shared<ImageViewBuilder>();
    }

    std::shared_ptr<ImageViewBuilder> source(std::string src) {
        m_view->set_image_resource(std::move(src));
        return shared_from_this();
    }

    std::shared_ptr<ImageViewBuilder> imageResource(std::string src) {
        m_view->set_image_resource(std::move(src));
        return shared_from_this();
    }

    std::shared_ptr<ImageViewBuilder> targetSize(int sz) {
        m_view->set_target_size(sz);
        return shared_from_this();
    }

    std::shared_ptr<ImageViewBuilder> fitMode(FitMode mode) {
        m_view->set_fit_mode(mode);
        return shared_from_this();
    }

    std::shared_ptr<ImageViewBuilder> cornerRadius(int radius) {
        m_view->set_corner_radius(radius);
        return shared_from_this();
    }

    std::shared_ptr<ImageViewBuilder> alpha(float op) {
        m_view->set_alpha(op);
        return shared_from_this();
    }

    std::shared_ptr<ImageViewBuilder> padding(const Padding& p) {
        m_view->set_padding(p);
        return shared_from_this();
    }

    std::shared_ptr<ImageViewBuilder> padding(int uniform) {
        m_view->set_padding(uniform);
        return shared_from_this();
    }

    std::shared_ptr<ImageViewBuilder> padding(int h, int v) {
        m_view->set_padding(h, v);
        return shared_from_this();
    }

    std::shared_ptr<ImageViewBuilder> padding(int l, int t, int r, int b) {
        m_view->set_padding(l, t, r, b);
        return shared_from_this();
    }

    std::shared_ptr<ImageViewBuilder> margin(const Margin& m) {
        m_view->set_margin(m);
        return shared_from_this();
    }

    std::shared_ptr<ImageViewBuilder> margin(int uniform) {
        m_view->set_margin(uniform);
        return shared_from_this();
    }

    std::shared_ptr<ImageViewBuilder> margin(int h, int v) {
        m_view->set_margin(h, v);
        return shared_from_this();
    }

    std::shared_ptr<ImageViewBuilder> margin(int l, int t, int r, int b) {
        m_view->set_margin(l, t, r, b);
        return shared_from_this();
    }

    std::shared_ptr<ImageView> build() {
        return m_view;
    }

private:
    std::shared_ptr<ImageView> m_view;
};

} // namespace miqu
