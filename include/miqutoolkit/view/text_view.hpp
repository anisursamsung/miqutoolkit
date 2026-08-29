#pragma once

#include "miqutoolkit/view/view.hpp"
#include <string>

namespace miqu {

enum class TextAlignment {
    Left,
    Center,
    Right
};

class TextView : public View {
public:
    TextView() = default;
    explicit TextView(std::string text) : m_text(std::move(text)) {}

    void set_text(std::string text) { m_text = std::move(text); }
    const std::string& get_text() const { return m_text; }

    void set_text_color(const Color& color) { m_color = color; }
    void set_text_size(int size) { m_font_size = size; }
    void set_bold(bool bold) { m_bold = bold; }
    void set_italic(bool italic) { m_italic = italic; }
    void set_text_alignment(TextAlignment align) { m_align = align; }
    void set_ellipsize(bool ellipsize) { m_ellipsize = ellipsize; }

    void draw(cairo_t* cr, const Rect& bounds) override;
    Size measure_size() const override;

private:
    std::string m_text;
    Color m_color = Color::rgba(0.90f, 0.90f, 0.95f, 1.0f);
    int m_font_size = 11;
    bool m_bold = false;
    bool m_italic = false;
    TextAlignment m_align = TextAlignment::Left;
    bool m_ellipsize = true;
};

class TextViewBuilder : public std::enable_shared_from_this<TextViewBuilder> {
public:
    TextViewBuilder() : m_view(std::make_shared<TextView>()) {}

    static std::shared_ptr<TextViewBuilder> create() {
        return std::make_shared<TextViewBuilder>();
    }

    std::shared_ptr<TextViewBuilder> text(std::string text) {
        m_view->set_text(std::move(text));
        return shared_from_this();
    }

    std::shared_ptr<TextViewBuilder> textColor(const Color& col) {
        m_view->set_text_color(col);
        return shared_from_this();
    }

    std::shared_ptr<TextViewBuilder> textSize(int size) {
        m_view->set_text_size(size);
        return shared_from_this();
    }

    std::shared_ptr<TextViewBuilder> bold(bool b = true) {
        m_view->set_bold(b);
        return shared_from_this();
    }

    std::shared_ptr<TextViewBuilder> textAlignment(TextAlignment a) {
        m_view->set_text_alignment(a);
        return shared_from_this();
    }

    std::shared_ptr<TextViewBuilder> ellipsize(bool e = true) {
        m_view->set_ellipsize(e);
        return shared_from_this();
    }

    std::shared_ptr<TextViewBuilder> padding(const Padding& p) {
        m_view->set_padding(p);
        return shared_from_this();
    }

    std::shared_ptr<TextViewBuilder> padding(int uniform) {
        m_view->set_padding(uniform);
        return shared_from_this();
    }

    std::shared_ptr<TextViewBuilder> padding(int h, int v) {
        m_view->set_padding(h, v);
        return shared_from_this();
    }

    std::shared_ptr<TextViewBuilder> padding(int l, int t, int r, int b) {
        m_view->set_padding(l, t, r, b);
        return shared_from_this();
    }

    std::shared_ptr<TextViewBuilder> margin(const Margin& m) {
        m_view->set_margin(m);
        return shared_from_this();
    }

    std::shared_ptr<TextViewBuilder> margin(int uniform) {
        m_view->set_margin(uniform);
        return shared_from_this();
    }

    std::shared_ptr<TextViewBuilder> margin(int h, int v) {
        m_view->set_margin(h, v);
        return shared_from_this();
    }

    std::shared_ptr<TextViewBuilder> margin(int l, int t, int r, int b) {
        m_view->set_margin(l, t, r, b);
        return shared_from_this();
    }

    std::shared_ptr<TextView> build() {
        return m_view;
    }

private:
    std::shared_ptr<TextView> m_view;
};

} // namespace miqu
