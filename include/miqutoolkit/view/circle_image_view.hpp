#pragma once

#include "miqutoolkit/view/image_view.hpp"

namespace miqu {

class CircleImageView : public ImageView {
public:
    CircleImageView() = default;
    explicit CircleImageView(std::string source) : ImageView(std::move(source)) {}

    void set_border(int width, const Color& color) {
        m_border_width = std::max(0, width);
        m_border_color = color;
    }
    int get_border_width() const { return m_border_width; }
    const Color& get_border_color() const { return m_border_color; }

    void set_rotation_angle(double degrees) {
        m_rotation_degrees = degrees;
    }
    double get_rotation_angle() const { return m_rotation_degrees; }

    void draw(cairo_t* cr, const Rect& bounds) override;

private:
    int m_border_width = 0;
    Color m_border_color = Color::transparent();
    double m_rotation_degrees = 0.0;
};

class CircleImageViewBuilder : public std::enable_shared_from_this<CircleImageViewBuilder> {
public:
    CircleImageViewBuilder() : m_view(std::make_shared<CircleImageView>()) {}

    static std::shared_ptr<CircleImageViewBuilder> create() {
        return std::make_shared<CircleImageViewBuilder>();
    }

    std::shared_ptr<CircleImageViewBuilder> imageResource(std::string source) {
        m_view->set_image_resource(std::move(source));
        return shared_from_this();
    }

    std::shared_ptr<CircleImageViewBuilder> targetSize(int size) {
        m_view->set_target_size(size);
        return shared_from_this();
    }

    std::shared_ptr<CircleImageViewBuilder> border(int width, const Color& color) {
        m_view->set_border(width, color);
        return shared_from_this();
    }

    std::shared_ptr<CircleImageViewBuilder> rotation(double degrees) {
        m_view->set_rotation_angle(degrees);
        return shared_from_this();
    }

    std::shared_ptr<CircleImageViewBuilder> fitMode(FitMode mode) {
        m_view->set_fit_mode(mode);
        return shared_from_this();
    }

    std::shared_ptr<CircleImageView> build() {
        return m_view;
    }

private:
    std::shared_ptr<CircleImageView> m_view;
};

} // namespace miqu
