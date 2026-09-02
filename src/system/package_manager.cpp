#include "miqutoolkit/system/package_manager.hpp"
#include "miqutoolkit/system/binary_manager.hpp"
#include "miqutoolkit/view/image_view.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <set>
#include <algorithm>
#include <unistd.h>
#include <cstdlib>

namespace miqu {

namespace fs = std::filesystem;

std::string PackageManager::clean_exec(const std::string& raw) {
    std::string result;
    result.reserve(raw.size());

    size_t i = 0;
    while (i < raw.size()) {
        if (raw[i] == '%' && i + 1 < raw.size() && raw[i + 1] == '%') {
            result += '%';
            i += 2;
            continue;
        }

        if (raw[i] == '%' && i + 1 < raw.size()) {
            char code = raw[i + 1];
            if (code == 'f' || code == 'F' || code == 'u' || code == 'U' ||
                code == 'd' || code == 'D' || code == 'n' || code == 'N' ||
                code == 'i' || code == 'c' || code == 'k' || code == 'v' ||
                code == 'm') {
                
                if (!result.empty() && result.back() == ' ') {
                    if (i + 2 == raw.size() || raw[i + 2] == ' ' || raw[i + 2] == '"' || raw[i + 2] == '\'') {
                        result.pop_back();
                    }
                }
                
                if (result.size() >= 1 && (result.back() == '"' || result.back() == '\'')) {
                    char quote = result.back();
                    if (i + 2 < raw.size() && raw[i + 2] == quote) {
                        result.pop_back();
                        if (!result.empty() && result.back() == ' ') {
                            result.pop_back();
                        }
                        i += 3;
                        continue;
                    }
                }

                i += 2;
                continue;
            }
        }

        result += raw[i++];
    }

    while (!result.empty() && (result.back() == ' ' || result.back() == '\t')) {
        result.pop_back();
    }

    return result;
}

std::vector<GridItem> PackageManager::get_installed_applications() {
    std::vector<GridItem> items;

    const char* home = getenv("HOME");
    std::string home_str = home ? home : "";

    std::vector<std::string> raw_dirs;

    const char* xdg_data_home = getenv("XDG_DATA_HOME");
    if (xdg_data_home && *xdg_data_home) {
        raw_dirs.push_back(std::string(xdg_data_home) + "/applications");
    } else if (!home_str.empty()) {
        raw_dirs.push_back(home_str + "/.local/share/applications");
    }

    if (!home_str.empty()) {
        raw_dirs.push_back(home_str + "/.local/share/flatpak/exports/share/applications");
    }

    const char* xdg_data_dirs = getenv("XDG_DATA_DIRS");
    if (xdg_data_dirs && *xdg_data_dirs) {
        std::stringstream ss(xdg_data_dirs);
        std::string dir;
        while (std::getline(ss, dir, ':')) {
            if (!dir.empty()) {
                raw_dirs.push_back(dir + "/applications");
            }
        }
    }

    raw_dirs.push_back("/usr/share/applications");
    raw_dirs.push_back("/usr/local/share/applications");
    raw_dirs.push_back("/var/lib/flatpak/exports/share/applications");
    raw_dirs.push_back("/var/lib/snapd/desktop/applications");

    std::vector<std::string> dirs;
    std::set<std::string> seen_dirs;
    for (const auto& d : raw_dirs) {
        if (!d.empty() && seen_dirs.find(d) == seen_dirs.end()) {
            seen_dirs.insert(d);
            dirs.push_back(d);
        }
    }

    std::set<std::string> seen_ids;
    std::set<std::string> seen_names;

    for (const auto& dir : dirs) {
        if (!fs::exists(dir)) continue;

        std::error_code ec;
        for (const auto& entry : fs::directory_iterator(dir, ec)) {
            if (ec) break;
            if (!entry.is_regular_file() && !entry.is_symlink()) continue;
            if (entry.path().extension() != ".desktop") continue;

            std::string desktop_id = entry.path().filename().string();
            if (seen_ids.find(desktop_id) != seen_ids.end()) {
                continue;
            }

            std::ifstream file(entry.path());
            if (!file.is_open()) continue;

            std::string line;
            bool in_desktop_entry = false;
            std::string name, generic_name, comment, exec, icon;
            bool no_display = false;
            bool terminal = false;

            while (std::getline(file, line)) {
                if (line.empty() || line[0] == '#') continue;

                if (line[0] == '[') {
                    in_desktop_entry = (line == "[Desktop Entry]");
                    continue;
                }

                if (!in_desktop_entry) continue;

                auto eq = line.find('=');
                if (eq == std::string::npos) continue;

                std::string key = line.substr(0, eq);
                std::string val = line.substr(eq + 1);

                if (key == "Name" && name.empty()) name = val;
                else if (key == "GenericName" && generic_name.empty()) generic_name = val;
                else if (key == "Comment" && comment.empty()) comment = val;
                else if (key == "Exec" && exec.empty()) exec = val;
                else if (key == "Icon" && icon.empty()) icon = val;
                else if (key == "NoDisplay" && val == "true") no_display = true;
                else if (key == "Terminal" && val == "true") terminal = true;
            }

            seen_ids.insert(desktop_id);

            if (no_display || name.empty() || exec.empty()) {
                continue;
            }

            if (seen_names.find(name) != seen_names.end()) {
                continue;
            }
            seen_names.insert(name);

            std::string resolved_icon = ImageView::resolve_icon_path(icon);
            std::string clean_command = clean_exec(exec);
            std::string subtitle = !generic_name.empty() ? generic_name : comment;

            GridItem item;
            item.id = desktop_id;
            item.title = name;
            item.subtitle = subtitle;
            item.icon_name = icon;
            item.icon_path = resolved_icon;
            item.exec_cmd = clean_command;
            item.terminal = terminal;

            items.push_back(std::move(item));
        }
    }

    std::sort(items.begin(), items.end(), [](const GridItem& a, const GridItem& b) {
        return a.title < b.title;
    });

    return items;
}

void PackageManager::launch(const GridItem& item) {
    if (item.exec_cmd.empty()) return;
    BinaryManager::launch_command(item.exec_cmd, item.terminal);
}

} // namespace miqu
