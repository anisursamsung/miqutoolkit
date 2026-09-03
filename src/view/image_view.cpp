#include "miqutoolkit/view/image_view.hpp"
#include "miqutoolkit/view/card_view.hpp"
#include "miqutoolkit/core/color_scheme.hpp"
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <set>
#include <algorithm>
#include <iostream>
#include <cmath>

namespace miqu {

namespace fs = std::filesystem;

static std::map<std::string, cairo_surface_t*> s_surface_cache;
static std::map<std::string, std::string> s_path_cache;

static std::string trim_str(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n\"'");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n\"'");
    return str.substr(first, (last - first + 1));
}

static std::string str_to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

void ImageView::clear_cache() {
    for (auto& pair : s_surface_cache) {
        if (pair.second) {
            cairo_surface_destroy(pair.second);
        }
    }
    s_surface_cache.clear();
    s_path_cache.clear();
}

static std::vector<std::string> get_icon_base_roots() {
    std::vector<std::string> roots;
    const char* home = getenv("HOME");
    std::string home_str = home ? home : "";

    const char* xdg_data_home = getenv("XDG_DATA_HOME");
    if (xdg_data_home && *xdg_data_home) {
        roots.push_back(std::string(xdg_data_home) + "/icons");
    } else if (!home_str.empty()) {
        roots.push_back(home_str + "/.local/share/icons");
    }

    if (!home_str.empty()) {
        roots.push_back(home_str + "/.icons");
    }

    const char* xdg_data_dirs = getenv("XDG_DATA_DIRS");
    if (xdg_data_dirs && *xdg_data_dirs) {
        std::stringstream ss(xdg_data_dirs);
        std::string dir;
        while (std::getline(ss, dir, ':')) {
            dir = trim_str(dir);
            if (!dir.empty()) {
                roots.push_back(dir + "/icons");
            }
        }
    }

    roots.push_back("/usr/share/icons");
    roots.push_back("/usr/local/share/icons");
    roots.push_back("/var/lib/flatpak/exports/share/icons");

    std::vector<std::string> unique_roots;
    std::set<std::string> seen;
    for (const auto& r : roots) {
        if (seen.find(r) == seen.end() && fs::exists(r)) {
            seen.insert(r);
            unique_roots.push_back(r);
        }
    }
    return unique_roots;
}

static std::vector<std::string> get_all_theme_dirs(const std::string& theme_name, const std::vector<std::string>& base_roots) {
    std::vector<std::string> result;
    if (theme_name.empty()) return result;

    std::string lower_theme = str_to_lower(theme_name);

    for (const auto& root : base_roots) {
        if (!fs::exists(root)) continue;

        std::string exact = root + "/" + theme_name;
        if (fs::exists(exact) && fs::is_directory(exact)) {
            result.push_back(exact);
            continue;
        }

        std::error_code ec;
        for (const auto& entry : fs::directory_iterator(root, ec)) {
            if (entry.is_directory()) {
                if (str_to_lower(entry.path().filename().string()) == lower_theme) {
                    result.push_back(entry.path().string());
                }
            }
        }
    }
    return result;
}

static void parse_index_theme(const std::string& theme_dir,
                              std::vector<std::string>& out_inherits,
                              std::vector<std::string>& out_subdirs) {
    std::string path = theme_dir + "/index.theme";
    if (!fs::exists(path)) return;

    std::ifstream file(path);
    if (!file.is_open()) return;

    std::string line;
    bool in_section = false;

    while (std::getline(file, line)) {
        line = trim_str(line);
        if (line.empty() || line[0] == '#') continue;

        if (line[0] == '[') {
            in_section = (line == "[Icon Theme]");
            continue;
        }

        if (!in_section) continue;

        auto eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key = trim_str(line.substr(0, eq));
        std::string val = trim_str(line.substr(eq + 1));

        if (key == "Inherits") {
            std::stringstream ss(val);
            std::string item;
            while (std::getline(ss, item, ',')) {
                item = trim_str(item);
                if (!item.empty()) out_inherits.push_back(item);
            }
        } else if (key == "Directories" || key == "ScaledDirectories") {
            std::stringstream ss(val);
            std::string item;
            while (std::getline(ss, item, ',')) {
                item = trim_str(item);
                if (!item.empty()) out_subdirs.push_back(item);
            }
        }
    }
}

std::string ImageView::resolve_icon_path(const std::string& icon_name) {
    if (icon_name.empty()) return "";

    std::string path_str = icon_name;
    if (path_str[0] == '~') {
        const char* home = getenv("HOME");
        if (home) {
            path_str = std::string(home) + path_str.substr(1);
        }
    }

    if ((path_str[0] == '/' || path_str[0] == '.') && fs::exists(path_str)) {
        return path_str;
    }

    std::string user_theme = ColorScheme::get()->metrics.icon_theme;
    if (user_theme.empty()) user_theme = "Papirus";

    std::string cache_key = user_theme + ":" + icon_name;
    auto it = s_path_cache.find(cache_key);
    if (it != s_path_cache.end()) {
        return it->second;
    }

    auto base_roots = get_icon_base_roots();

    std::string clean_name = icon_name;
    for (const char* ext : {".svg", ".png", ".xpm", ".jpg", ".jpeg", ".ico"}) {
        std::string ext_str(ext);
        if (clean_name.size() > ext_str.size() &&
            clean_name.substr(clean_name.size() - ext_str.size()) == ext_str) {
            clean_name = clean_name.substr(0, clean_name.size() - ext_str.size());
            break;
        }
    }

    std::vector<std::string> name_candidates;
    name_candidates.push_back(clean_name);
    if (str_to_lower(clean_name) != clean_name) {
        name_candidates.push_back(str_to_lower(clean_name));
    }
    if (clean_name.size() > 9 && clean_name.substr(clean_name.size() - 9) == "-symbolic") {
        name_candidates.push_back(clean_name.substr(0, clean_name.size() - 9));
    } else {
        name_candidates.push_back(clean_name + "-symbolic");
    }

    std::vector<std::string> theme_search_list;
    std::set<std::string> visited_themes;

    auto add_theme = [&](const std::string& t) {
        if (t.empty()) return;
        std::string lt = str_to_lower(t);
        if (visited_themes.find(lt) == visited_themes.end()) {
            visited_themes.insert(lt);
            theme_search_list.push_back(t);
        }
    };

    add_theme(user_theme);

    for (size_t i = 0; i < theme_search_list.size(); ++i) {
        std::string t = theme_search_list[i];
        auto dirs = get_all_theme_dirs(t, base_roots);
        for (const auto& t_dir : dirs) {
            std::vector<std::string> inherits, subdirs;
            parse_index_theme(t_dir, inherits, subdirs);
            for (const auto& inh : inherits) {
                add_theme(inh);
            }
        }
    }

    add_theme("Papirus");
    add_theme("Papirus-Dark");
    add_theme("hicolor");
    add_theme("Adwaita");
    add_theme("breeze");

    const std::vector<std::string> exts = { ".svg", ".png", ".xpm", ".jpg", "" };

    const std::vector<std::string> fallback_subdirs = {
        "scalable/apps", "48x48/apps", "64x64/apps", "32x32/apps", "128x128/apps", "256x256/apps", "512x512/apps",
        "24x24/apps", "22x22/apps", "16x16/apps", "16/apps", "22/apps", "24/apps", "32/apps", "48/apps", "64/apps", "128/apps", "apps",
        "scalable/categories", "48x48/categories", "scalable/devices", "48x48/devices",
        "scalable/places", "48x48/places", "scalable/mimetypes", "48x48/mimetypes",
        "scalable/actions", "48x48/actions", "scalable/status", "48x48/status",
        "symbolic/apps", "symbolic",
        "48x48", "64x64", "32x32", "scalable", ""
    };

    for (const auto& th : theme_search_list) {
        auto theme_dirs = get_all_theme_dirs(th, base_roots);
        if (theme_dirs.empty()) continue;

        for (const auto& theme_dir : theme_dirs) {
            std::vector<std::string> inherits, theme_subdirs;
            parse_index_theme(theme_dir, inherits, theme_subdirs);

            std::vector<std::string> subdirs_to_check = fallback_subdirs;
            for (const auto& d : theme_subdirs) {
                if (std::find(subdirs_to_check.begin(), subdirs_to_check.end(), d) == subdirs_to_check.end()) {
                    subdirs_to_check.push_back(d);
                }
            }

            for (const auto& name_cand : name_candidates) {
                for (const auto& sub : subdirs_to_check) {
                    std::string search_dir = sub.empty() ? theme_dir : (theme_dir + "/" + sub);
                    if (!fs::exists(search_dir)) continue;

                    for (const auto& ext : exts) {
                        std::string candidate = search_dir + "/" + name_cand + ext;
                        if (fs::exists(candidate) && !fs::is_directory(candidate)) {
                            s_path_cache[cache_key] = candidate;
                            return candidate;
                        }
                    }
                }
            }
        }
    }

    // Pixmap directory fallback
    const char* home = getenv("HOME");
    std::string home_str = home ? home : "";
    std::vector<std::string> pixmap_dirs = {
        "/usr/share/pixmaps",
        "/usr/local/share/pixmaps",
        home_str + "/.local/share/pixmaps"
    };

    for (const auto& pdir : pixmap_dirs) {
        if (!fs::exists(pdir)) continue;
        for (const auto& name_cand : name_candidates) {
            for (const auto& ext : exts) {
                std::string candidate = pdir + "/" + name_cand + ext;
                if (fs::exists(candidate) && !fs::is_directory(candidate)) {
                    s_path_cache[cache_key] = candidate;
                    return candidate;
                }
            }
        }
    }

    s_path_cache[cache_key] = "";
    return "";
}

static cairo_surface_t* load_surface(const std::string& path_or_name, int box_w, int box_h, FitMode fit_mode, int target_size) {
    if (path_or_name.empty() || box_w <= 0 || box_h <= 0) return nullptr;

    std::string resolved = ImageView::resolve_icon_path(path_or_name);
    if (resolved.empty() || !fs::exists(resolved)) {
        return nullptr;
    }

    int req_w = box_w;
    int req_h = box_h;
    gboolean preserve_aspect = TRUE;

    if (fit_mode == FitMode::Center) {
        req_w = (target_size > 0) ? target_size : box_w;
        req_h = (target_size > 0) ? target_size : box_h;
        preserve_aspect = TRUE;
    } else if (fit_mode == FitMode::Fill) {
        req_w = box_w;
        req_h = box_h;
        preserve_aspect = FALSE;
    } else if (fit_mode == FitMode::Cover) {
        int orig_w = 0, orig_h = 0;
        if (gdk_pixbuf_get_file_info(resolved.c_str(), &orig_w, &orig_h) && orig_w > 0 && orig_h > 0) {
            double scale = std::max(static_cast<double>(box_w) / orig_w, static_cast<double>(box_h) / orig_h);
            req_w = std::max(1, static_cast<int>(std::round(orig_w * scale)));
            req_h = std::max(1, static_cast<int>(std::round(orig_h * scale)));
        } else {
            req_w = std::max(box_w, box_h);
            req_h = req_w;
        }
        preserve_aspect = TRUE;
    } else { // FitMode::Contain
        req_w = box_w;
        req_h = box_h;
        preserve_aspect = TRUE;
    }

    std::string cache_key = resolved + "@" + std::to_string(req_w) + "x" + std::to_string(req_h) +
                            (preserve_aspect ? "p" : "s");
    auto it = s_surface_cache.find(cache_key);
    if (it != s_surface_cache.end() && it->second != nullptr) {
        return it->second;
    }

    GError* error = nullptr;
    GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file_at_scale(resolved.c_str(), req_w, req_h, preserve_aspect, &error);
    if (!pixbuf) {
        if (error) g_error_free(error);
        return nullptr;
    }

    int p_width = gdk_pixbuf_get_width(pixbuf);
    int p_height = gdk_pixbuf_get_height(pixbuf);
    int p_stride = gdk_pixbuf_get_rowstride(pixbuf);
    int p_channels = gdk_pixbuf_get_n_channels(pixbuf);
    const guchar* p_pixels = gdk_pixbuf_get_pixels(pixbuf);

    cairo_surface_t* surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, p_width, p_height);
    unsigned char* c_data = cairo_image_surface_get_data(surf);
    int c_stride = cairo_image_surface_get_stride(surf);

    cairo_surface_flush(surf);
    for (int y = 0; y < p_height; ++y) {
        const guchar* src = p_pixels + y * p_stride;
        auto* dst = reinterpret_cast<uint32_t*>(c_data + y * c_stride);
        for (int x = 0; x < p_width; ++x) {
            uint8_t r = src[0];
            uint8_t g = src[1];
            uint8_t b = src[2];
            uint8_t a = (p_channels == 4) ? src[3] : 255;
            uint8_t pr = (r * a) / 255;
            uint8_t pg = (g * a) / 255;
            uint8_t pb = (b * a) / 255;
            dst[x] = (static_cast<uint32_t>(a) << 24) | (static_cast<uint32_t>(pr) << 16) |
                     (static_cast<uint32_t>(pg) << 8) | static_cast<uint32_t>(pb);
            src += p_channels;
        }
    }
    cairo_surface_mark_dirty(surf);
    g_object_unref(pixbuf);

    s_surface_cache[cache_key] = surf;
    return surf;
}

void ImageView::draw(cairo_t* cr, const Rect& bounds) {
    if (!is_visible() || !cr || bounds.width <= 0 || bounds.height <= 0) return;

    cairo_surface_t* surf = load_surface(m_source, bounds.width, bounds.height, m_fit_mode, m_target_size);

    cairo_save(cr);

    // Apply corner radius or rectangular clipping
    if (m_corner_radius > 0) {
        CardView::draw_rounded_rect(cr, bounds.x, bounds.y, bounds.width, bounds.height, m_corner_radius);
        cairo_clip(cr);
    } else {
        cairo_rectangle(cr, bounds.x, bounds.y, bounds.width, bounds.height);
        cairo_clip(cr);
    }

    if (!surf) {
        cairo_set_source_rgba(cr, 0.20, 0.22, 0.32, 0.85);
        cairo_fill_preserve(cr);

        cairo_set_source_rgba(cr, 0.40, 0.45, 0.65, 0.9);
        cairo_set_line_width(cr, 1.0);
        cairo_stroke(cr);
        cairo_restore(cr);
        return;
    }

    int surf_w = cairo_image_surface_get_width(surf);
    int surf_h = cairo_image_surface_get_height(surf);

    double draw_x = bounds.x + (bounds.width - surf_w) / 2.0;
    double draw_y = bounds.y + (bounds.height - surf_h) / 2.0;

    cairo_set_source_surface(cr, surf, draw_x, draw_y);
    if (m_opacity < 1.0f) {
        cairo_paint_with_alpha(cr, m_opacity);
    } else {
        cairo_paint(cr);
    }
    cairo_restore(cr);
}

} // namespace miqu
