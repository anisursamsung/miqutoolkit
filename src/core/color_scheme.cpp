#include "miqutoolkit/core/color_scheme.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace miqu {

namespace fs = std::filesystem;

static std::shared_ptr<ColorScheme> s_instance = nullptr;

static std::string trim_str(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n\"'");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n\"'");
    return str.substr(first, (last - first + 1));
}

std::shared_ptr<ColorScheme> ColorScheme::get() {
    if (!s_instance) {
        s_instance = std::make_shared<ColorScheme>();
        s_instance->load_user_theme();
    }
    return s_instance;
}

void ColorScheme::load_user_theme() {
    const char* home = getenv("HOME");
    if (!home) return;

    std::string config_path = std::string(home) + "/.config/biway/biway.conf";
    if (!fs::exists(config_path)) return;

    std::ifstream file(config_path);
    if (!file.is_open()) return;

    std::string line;
    std::string current_section = "";

    while (std::getline(file, line)) {
        line = trim_str(line);
        if (line.empty() || line[0] == '#') continue;

        if (line.back() == '{') {
            current_section = trim_str(line.substr(0, line.size() - 1));
            continue;
        }
        if (line == "}") {
            current_section = "";
            continue;
        }

        auto eq_pos = line.find('=');
        if (eq_pos == std::string::npos) continue;

        std::string key = trim_str(line.substr(0, eq_pos));
        std::string val = trim_str(line.substr(eq_pos + 1));

        if (current_section == "theme" || current_section == "appearance" || current_section == "general" || current_section == "decoration") {
            if (key == "icon_theme") metrics.icon_theme = val;
            else if (key == "font_family") metrics.font_family = val;
            else if (key == "font_size") {
                try { metrics.font_size = std::stoi(val); } catch (...) {}
            }
            else if (key == "corner_radius" || key == "rounding") {
                try { metrics.corner_radius = std::stoi(val); } catch (...) {}
            }
            else if (key == "border_width" || key == "border_size") {
                try { metrics.border_width = std::stoi(val); } catch (...) {}
            }
            else if (key == "col.background" || key == "background") colors.background = Color::from_hex(val);
            else if (key == "col.surface" || key == "surface") colors.surface = Color::from_hex(val);
            else if (key == "col.surface_variant" || key == "surface_variant") colors.surface_variant = Color::from_hex(val);
            else if (key == "col.primary" || key == "primary" || key == "col.active_border") colors.primary = Color::from_hex(val);
            else if (key == "col.outline" || key == "outline" || key == "col.inactive_border") colors.outline = Color::from_hex(val);
        } else if (current_section.empty()) {
            if (key == "icon_theme") metrics.icon_theme = val;
            else if (key == "background") colors.background = Color::from_hex(val);
            else if (key == "surface") colors.surface = Color::from_hex(val);
            else if (key == "primary") colors.primary = Color::from_hex(val);
        }
    }
}

} // namespace miqu
