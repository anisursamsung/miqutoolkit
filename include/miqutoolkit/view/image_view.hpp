#pragma once

#include "miqutoolkit/view/view.hpp"
#include <string>

namespace miqu {

class ImageView : public View {
public:
    ImageView() = default;
    explicit ImageView(std::string source) : m_source(std::move(source)) {}

    void set_image_resource(std::string source) { m_source = std::move(source); }
    void set_target_size(int size) { m_target_size = size; }
    void set_alpha(float opacity) { m_opacity = opacity; }

    void draw(cairo_t* cr, const Rect& bounds) override;
    Size measure_size() const override { return Size(m_target_size, m_target_size); }

    static std::string resolve_icon_path(const std::string& icon_name);
    static void clear_cache();

private:
    std::string m_source;
    int m_target_size = 48;
    float m_opacity = 1.0f;
};

class ImageViewBuilder : public std::enable_shared_from_this<ImageViewBuilder> {
public:
    ImageViewBuilder() : m_view(std::make_shared<ImageView>()) {}

    static std::shared_ptr<ImageViewBuilder> create() {
        return std::make_shared<ImageViewBuilder>();
    }

    std::shared_ptr<ImageViewBuilder> imageResource(std::string src) {
        m_view->set_image_resource(std::move(src));
        return shared_from_this();
    }

    std::shared_ptr<ImageViewBuilder> targetSize(int sz) {
        m_view->set_target_size(sz);
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
