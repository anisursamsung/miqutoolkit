#pragma once

#include "miqutoolkit/view/view.hpp"
#include <string>
#include <vector>
#include <functional>

namespace miqu {

struct AppInfo {
    std::string id;
    std::string title;
    std::string subtitle;
    std::string icon_name;
    std::string icon_path;
    std::string exec_cmd;
    bool terminal = false;
};

enum class StretchMode {
    None,
    SpacingWidth,
    ColumnWidth,
};

class GridView : public View {
public:
    GridView();

    void set_num_columns(int cols) { m_cols = std::max(1, cols); m_auto_fit = false; }
    void set_auto_fit(int min_column_width = 100) { m_min_col_w = std::max(20, min_column_width); m_auto_fit = true; }
    void set_cell_size(int w, int h) { m_cell_w = w; m_cell_h = h; }
    void set_cell_height(int h) { m_cell_h = h; }
    void set_horizontal_spacing(int sx) { m_space_x = sx; }
    void set_vertical_spacing(int sy) { m_space_y = sy; }
    void set_stretch_mode(StretchMode mode) { m_stretch_mode = mode; }

    void set_adapter(std::vector<AppInfo> items);
    void set_filter_query(const std::string& filter);

    void set_on_item_click_listener(std::function<void(const AppInfo&)> cb) { m_on_item_click = std::move(cb); }

    const AppInfo* get_selected_item() const;

    void draw(cairo_t* cr, const Rect& bounds) override;
    bool on_key(const KeyPressEvent& event) override;
    bool on_mouse_move(int lx, int ly, const Rect& bounds) override;
    bool on_mouse_button(int lx, int ly, MouseButton button, bool pressed, const Rect& bounds) override;
    bool on_scroll(double delta) override;

private:
    int compute_columns(int bounds_w, int& out_cell_w) const;
    void ensure_visible(int viewport_height, int cols, int row_stride);
    int item_at(int lx, int ly, const Rect& bounds) const;

    bool m_auto_fit = true;
    int m_min_col_w = 100;
    int m_cols = 5;
    int m_cell_w = 100;
    int m_cell_h = 100;
    int m_space_x = 10;
    int m_space_y = 10;
    StretchMode m_stretch_mode = StretchMode::ColumnWidth;

    int m_selected_index = -1;
    int m_hovered_index = -1;
    double m_scroll_y = 0.0;
    mutable int m_last_width = 0;
    mutable int m_last_height = 0;
    mutable int m_effective_cols = 5;
    mutable int m_effective_cell_w = 100;

    std::vector<AppInfo> m_all_items;
    std::vector<AppInfo> m_filtered_items;
    std::string m_filter_query;

    std::function<void(const AppInfo&)> m_on_item_click;
};

class GridViewBuilder : public std::enable_shared_from_this<GridViewBuilder> {
public:
    GridViewBuilder() : m_view(std::make_shared<GridView>()) {}

    static std::shared_ptr<GridViewBuilder> create() {
        return std::make_shared<GridViewBuilder>();
    }

    std::shared_ptr<GridViewBuilder> numColumns(int cols) {
        m_view->set_num_columns(cols);
        return shared_from_this();
    }

    std::shared_ptr<GridViewBuilder> autoFit(int min_column_width = 100) {
        m_view->set_auto_fit(min_column_width);
        return shared_from_this();
    }

    std::shared_ptr<GridViewBuilder> cellSize(int w, int h) {
        m_view->set_cell_size(w, h);
        return shared_from_this();
    }

    std::shared_ptr<GridViewBuilder> cellHeight(int h) {
        m_view->set_cell_height(h);
        return shared_from_this();
    }

    std::shared_ptr<GridViewBuilder> spacing(int sx, int sy) {
        m_view->set_horizontal_spacing(sx);
        m_view->set_vertical_spacing(sy);
        return shared_from_this();
    }

    std::shared_ptr<GridViewBuilder> stretchMode(StretchMode mode) {
        m_view->set_stretch_mode(mode);
        return shared_from_this();
    }

    std::shared_ptr<GridViewBuilder> onItemClick(std::function<void(const AppInfo&)> cb) {
        m_view->set_on_item_click_listener(std::move(cb));
        return shared_from_this();
    }

    std::shared_ptr<GridViewBuilder> padding(const Padding& p) {
        m_view->set_padding(p);
        return shared_from_this();
    }

    std::shared_ptr<GridViewBuilder> padding(int uniform) {
        m_view->set_padding(uniform);
        return shared_from_this();
    }

    std::shared_ptr<GridViewBuilder> padding(int h, int v) {
        m_view->set_padding(h, v);
        return shared_from_this();
    }

    std::shared_ptr<GridViewBuilder> padding(int l, int t, int r, int b) {
        m_view->set_padding(l, t, r, b);
        return shared_from_this();
    }

    std::shared_ptr<GridViewBuilder> margin(const Margin& m) {
        m_view->set_margin(m);
        return shared_from_this();
    }

    std::shared_ptr<GridViewBuilder> margin(int uniform) {
        m_view->set_margin(uniform);
        return shared_from_this();
    }

    std::shared_ptr<GridViewBuilder> margin(int h, int v) {
        m_view->set_margin(h, v);
        return shared_from_this();
    }

    std::shared_ptr<GridViewBuilder> margin(int l, int t, int r, int b) {
        m_view->set_margin(l, t, r, b);
        return shared_from_this();
    }

    std::shared_ptr<GridView> build() {
        return m_view;
    }

private:
    std::shared_ptr<GridView> m_view;
};

} // namespace miqu
