#include "miqutoolkit/core/config.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <algorithm>

namespace miqu {

namespace fs = std::filesystem;

static std::shared_ptr<Config> s_instance = nullptr;

static std::string trim_str(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n\"'");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n\"'");
    return str.substr(first, (last - first + 1));
}

static std::string expand_home(const std::string& path) {
    if (path.empty()) return path;
    if (path[0] == '~') {
        const char* home = getenv("HOME");
        if (home) {
            return std::string(home) + path.substr(1);
        }
    }
    return path;
}

std::shared_ptr<Config> Config::get() {
    if (!s_instance) {
        s_instance = std::make_shared<Config>();
    }
    return s_instance;
}

static void load_config_file_internal(const std::string& path, Config::Colors& colors, Config::Metrics& metrics, int depth) {
    if (depth > 10) return;

    std::string expanded = expand_home(path);
    if (!fs::exists(expanded)) return;

    std::ifstream file(expanded);
    if (!file.is_open()) return;

    fs::path parent_dir = fs::path(expanded).parent_path();
    const char* home_env = getenv("HOME");
    std::string config_base = home_env ? (std::string(home_env) + "/.config/miquland") : "";

    std::string line;
    std::string current_section = "";

    while (std::getline(file, line)) {
        line = trim_str(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;

        // Section header [section]
        if (line.front() == '[' && line.back() == ']') {
            current_section = trim_str(line.substr(1, line.size() - 2));
            std::transform(current_section.begin(), current_section.end(), current_section.begin(), ::tolower);
            continue;
        }

        // Section header section { ... }
        if (line.back() == '{') {
            current_section = trim_str(line.substr(0, line.size() - 1));
            std::transform(current_section.begin(), current_section.end(), current_section.begin(), ::tolower);
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
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);

        // Strip inline comments from values
        size_t c_pos = std::string::npos;
        if (!val.empty() && val[0] == '#') {
            size_t space_pos = val.find_first_of(" \t");
            if (space_pos != std::string::npos) {
                c_pos = val.find('#', space_pos);
            }
        } else {
            c_pos = val.find('#');
        }

        if (c_pos != std::string::npos) {
            val = trim_str(val.substr(0, c_pos));
        }

        if (key == "source" || key == "include") {
            std::string inc_path = expand_home(val);
            if (!fs::exists(inc_path)) {
                // Try relative to current file directory
                fs::path rel1 = parent_dir / val;
                if (fs::exists(rel1)) {
                    inc_path = rel1.string();
                } else if (!config_base.empty()) {
                    // Try relative to ~/.config/miquland/
                    fs::path rel2 = fs::path(config_base) / val;
                    if (fs::exists(rel2)) {
                        inc_path = rel2.string();
                    }
                }
            }
            load_config_file_internal(inc_path, colors, metrics, depth + 1);
            continue;
        }

        // Color mappings
        if (key == "color_background" || key == "background" || key == "bg_color" || key == "col.background") {
            colors.background = Color::from_hex(val, colors.background);
        } else if (key == "color_on_background" || key == "on_background" || key == "col.on_background") {
            colors.on_background = Color::from_hex(val, colors.on_background);
        } else if (key == "color_surface" || key == "surface" || key == "menu_bg" || key == "col.surface") {
            colors.surface = Color::from_hex(val, colors.surface);
        } else if (key == "color_on_surface" || key == "on_surface" || key == "text_color" || key == "text" || key == "col.on_surface" || key == "col.text") {
            colors.on_surface = Color::from_hex(val, colors.on_surface);
        } else if (key == "color_surface_variant" || key == "surface_variant" || key == "col.surface_variant") {
            colors.surface_variant = Color::from_hex(val, colors.surface_variant);
        } else if (key == "color_on_surface_variant" || key == "on_surface_variant" || key == "text_muted" || key == "col.on_surface_variant") {
            colors.on_surface_variant = Color::from_hex(val, colors.on_surface_variant);
        } else if (key == "color_primary" || key == "primary" || key == "accent" || key == "window_border_color_active" || key == "border_color_active" || key == "col.primary" || key == "col.active_border") {
            colors.primary = Color::from_hex(val, colors.primary);
        } else if (key == "color_on_primary" || key == "on_primary" || key == "col.on_primary") {
            colors.on_primary = Color::from_hex(val, colors.on_primary);
        } else if (key == "color_primary_container" || key == "primary_container" || key == "col.primary_container") {
            colors.primary_container = Color::from_hex(val, colors.primary_container);
        } else if (key == "color_on_primary_container" || key == "on_primary_container" || key == "col.on_primary_container") {
            colors.on_primary_container = Color::from_hex(val, colors.on_primary_container);
        } else if (key == "color_outline" || key == "outline" || key == "border" || key == "window_border_color_inactive" || key == "border_color_inactive" || key == "col.outline" || key == "col.inactive_border") {
            colors.outline = Color::from_hex(val, colors.outline);
        } else if (key == "color_outline_variant" || key == "outline_variant" || key == "col.outline_variant") {
            colors.outline_variant = Color::from_hex(val, colors.outline_variant);
        } else if (key == "icon_theme") {
            metrics.icon_theme = val;
        } else if (key == "font_family" || key == "font") {
            metrics.font_family = val;
        } else if (key == "font_size") {
            try { metrics.font_size = std::stoi(val); } catch (...) {}
        } else if (key == "window_border_radius" || key == "border_radius" || key == "corner_radius" || key == "rounding") {
            try { metrics.corner_radius = std::stoi(val); } catch (...) {}
        } else if (key == "window_border_width" || key == "border_width" || key == "border_size") {
            try { metrics.border_width = std::stoi(val); } catch (...) {}
        }
    }
}

bool Config::load_from_file(const std::string& path) {
    std::string expanded = expand_home(path);
    if (!fs::exists(expanded)) return false;
    load_config_file_internal(expanded, colors, metrics, 0);
    return true;
}

} // namespace miqu
