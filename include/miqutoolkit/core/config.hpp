#pragma once

#include "miqutoolkit/core/color.hpp"
#include <memory>
#include <string>
#include <vector>

namespace miqu {

class Config {
public:
    struct Colors {
        Color background           = Color::rgba(0.957f, 0.973f, 0.988f, 0.96f);
        Color on_background        = Color::rgba(0.059f, 0.090f, 0.165f, 1.0f);
        Color surface              = Color::rgba(1.0f, 1.0f, 1.0f, 0.96f);
        Color on_surface           = Color::rgba(0.059f, 0.090f, 0.165f, 1.0f);
        Color surface_variant      = Color::rgba(0.902f, 0.937f, 0.973f, 0.96f);
        Color on_surface_variant  = Color::rgba(0.278f, 0.333f, 0.412f, 1.0f);
        Color primary              = Color::rgba(0.0f, 0.40f, 1.0f, 1.0f);
        Color on_primary           = Color::rgba(1.0f, 1.0f, 1.0f, 1.0f);
        Color primary_container    = Color::rgba(0.80f, 0.898f, 1.0f, 0.50f);
        Color on_primary_container = Color::rgba(0.0f, 0.169f, 0.40f, 1.0f);
        Color outline              = Color::rgba(0.60f, 0.761f, 1.0f, 0.80f);
        Color outline_variant      = Color::rgba(0.859f, 0.918f, 0.996f, 0.60f);
        Color backdrop             = Color::rgba(0.0f, 0.0f, 0.0f, 0.30f);
    } colors;

    struct Metrics {
        int corner_radius = 12;
        int border_width = 1;
        std::string icon_theme = "Tela-circle";
        std::string font_family = "Sans";
        int font_size = 11;
    } metrics;

    static std::shared_ptr<Config> get();
    bool load_from_file(const std::string& path);
    void set_colors(const Colors& new_colors) { colors = new_colors; }
    void set_metrics(const Metrics& new_metrics) { metrics = new_metrics; }

    static std::string ensure_user_config(
        const std::string& app_name,
        const std::string& main_file = "",
        const std::vector<std::string>& additional_files = {}
    );
};

} // namespace miqu

