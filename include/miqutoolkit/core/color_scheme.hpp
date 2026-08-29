#pragma once

#include "miqutoolkit/core/color.hpp"
#include <memory>
#include <string>

namespace miqu {

class ColorScheme {
public:
    struct Colors {
        Color background           = Color::rgba(0.12f, 0.13f, 0.18f, 0.96f);
        Color on_background        = Color::rgba(0.90f, 0.90f, 0.95f, 1.0f);
        Color surface              = Color::rgba(0.16f, 0.17f, 0.24f, 0.96f);
        Color on_surface           = Color::rgba(0.90f, 0.90f, 0.95f, 1.0f);
        Color surface_variant      = Color::rgba(0.20f, 0.22f, 0.32f, 0.96f);
        Color on_surface_variant  = Color::rgba(0.70f, 0.72f, 0.80f, 1.0f);
        Color primary              = Color::rgba(0.53f, 0.47f, 0.98f, 1.0f);
        Color on_primary           = Color::rgba(0.05f, 0.05f, 0.10f, 1.0f);
        Color primary_container    = Color::rgba(0.35f, 0.30f, 0.70f, 0.50f);
        Color on_primary_container = Color::rgba(0.95f, 0.95f, 1.00f, 1.0f);
        Color outline              = Color::rgba(0.30f, 0.32f, 0.45f, 0.80f);
        Color outline_variant      = Color::rgba(0.22f, 0.24f, 0.35f, 0.60f);
        Color backdrop             = Color::rgba(0.0f, 0.0f, 0.0f, 0.50f);
    } colors;

    struct Metrics {
        int corner_radius = 12;
        int border_width = 1;
        std::string icon_theme = "Tela-circle";
        std::string font_family = "Sans";
        int font_size = 11;
    } metrics;

    static std::shared_ptr<ColorScheme> get();
    void load_user_theme();
};

} // namespace miqu
