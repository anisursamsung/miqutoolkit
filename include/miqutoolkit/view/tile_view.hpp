#pragma once

#include "miqutoolkit/view/view.hpp"
#include "miqutoolkit/view/image_view.hpp"
#include <string>
#include <memory>

namespace miqu {

class TileView : public View {
public:
    TileView() = default;
    TileView(std::string title, std::string icon_source, bool is_image = false);

    void set_title(std::string title) { m_title = std::move(title); }
    const std::string& get_title() const { return m_title; }

    void set_subtitle(std::string subtitle) { m_subtitle = std::move(subtitle); }
    const std::string& get_subtitle() const { return m_subtitle; }

    void set_icon_source(std::string source) { m_icon_source = std::move(source); }
    const std::string& get_icon_source() const { return m_icon_source; }

    void set_is_image(bool is_image) { m_is_image = is_image; }
    bool is_image() const { return m_is_image; }

    void set_quality_mode(ImageQuality quality) { m_quality = quality; }
    ImageQuality get_quality_mode() const { return m_quality; }

    void set_corner_radius(int radius) { m_corner_radius = radius; }
    int get_corner_radius() const { return m_corner_radius; }

    void set_highlight_subtitle(bool highlight) { m_highlight_subtitle = highlight; }
    bool is_subtitle_highlighted() const { return m_highlight_subtitle; }

    void draw(cairo_t* cr, const Rect& bounds) override;

private:
    std::string m_title;
    std::string m_subtitle;
    std::string m_icon_source;
    bool m_is_image = false;
    ImageQuality m_quality = ImageQuality::FullOriginal;
    int m_corner_radius = 6;
    bool m_highlight_subtitle = false;
};

class TileViewBuilder : public std::enable_shared_from_this<TileViewBuilder> {
public:
    static std::shared_ptr<TileViewBuilder> create() {
        return std::make_shared<TileViewBuilder>();
    }

    TileViewBuilder() : m_view(std::make_shared<TileView>()) {}

    std::shared_ptr<TileViewBuilder> title(std::string title) {
        m_view->set_title(std::move(title));
        return shared_from_this();
    }

    std::shared_ptr<TileViewBuilder> subtitle(std::string sub) {
        m_view->set_subtitle(std::move(sub));
        return shared_from_this();
    }

    std::shared_ptr<TileViewBuilder> iconSource(std::string src) {
        m_view->set_icon_source(std::move(src));
        return shared_from_this();
    }

    std::shared_ptr<TileViewBuilder> isImage(bool is_img) {
        m_view->set_is_image(is_img);
        return shared_from_this();
    }

    std::shared_ptr<TileViewBuilder> qualityMode(ImageQuality q) {
        m_view->set_quality_mode(q);
        return shared_from_this();
    }

    std::shared_ptr<TileViewBuilder> cornerRadius(int r) {
        m_view->set_corner_radius(r);
        return shared_from_this();
    }

    std::shared_ptr<TileViewBuilder> highlightSubtitle(bool hl) {
        m_view->set_highlight_subtitle(hl);
        return shared_from_this();
    }

    std::shared_ptr<TileView> build() {
        return m_view;
    }

private:
    std::shared_ptr<TileView> m_view;
};

} // namespace miqu
