#pragma once

#include "miqutoolkit/view/image_view.hpp"

namespace miqu {

/**
 * CircleImageView is a specialized ImageView that clips its content into a circle.
 * Note: ImageView now supports .circle(true), .border(), and .rotation() natively.
 * CircleImageView is retained as a subclass for seamless backward compatibility.
 */
class CircleImageView : public ImageView {
public:
    CircleImageView() {
        set_circle(true);
    }
    explicit CircleImageView(std::string source) : ImageView(std::move(source)) {
        set_circle(true);
    }
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
