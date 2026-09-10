#include "miqutoolkit/system/app_manager.hpp"
#include "miqutoolkit/view/image_view.hpp"
#include "miqutoolkit/core/thread_pool.hpp"
#include "miqutoolkit/core/app_engine.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <set>
#include <algorithm>
#include <unistd.h>
#include <cstdlib>

namespace miqu {

namespace fs = std::filesystem;

AppManager* AppManager::get() {
    static AppManager s_instance;
    return &s_instance;
}

std::string AppManager::clean_exec(const std::string& raw) {
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

void AppManager::rescan_internal() {
    m_apps.clear();

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
            std::string name, generic_name, comment, exec, icon, categories, keywords;
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
                else if (key == "Categories" && categories.empty()) categories = val;
                else if (key == "Keywords" && keywords.empty()) keywords = val;
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

            // If icon is an explicit path, keep it; otherwise resolve on demand during view draw
            std::string resolved_icon;
            if (!icon.empty() && (icon[0] == '/' || icon[0] == '.' || icon[0] == '~')) {
                resolved_icon = icon;
            }
            std::string clean_command = clean_exec(exec);

            DesktopApp app;
            app.id = desktop_id;
            app.name = name;
            app.generic_name = generic_name;
            app.comment = comment;
            app.icon_name = icon;
            app.icon_path = resolved_icon;
            app.exec_cmd = clean_command;
            app.categories = categories;
            app.keywords = keywords;
            app.terminal = terminal;
            app.no_display = no_display;

            m_apps.push_back(std::move(app));
        }
    }

    std::sort(m_apps.begin(), m_apps.end(), [](const DesktopApp& a, const DesktopApp& b) {
        return a.name < b.name;
    });

    m_scanned = true;
}

const std::vector<DesktopApp>& AppManager::get_installed_apps(bool force_rescan) {
    if (!m_scanned || force_rescan) {
        rescan_internal();
    }
    return m_apps;
}

void AppManager::scan_async(std::function<void(const std::vector<DesktopApp>&)> callback) {
    ThreadPool::get().enqueue([this, callback]() {
        rescan_internal();
        if (callback) {
            if (AppEngine::instance()) {
                AppEngine::instance()->post([this, callback]() {
                    callback(this->m_apps);
                });
            } else {
                callback(this->m_apps);
            }
        }
    });
}

void AppManager::launch(const DesktopApp& app) {
    if (app.exec_cmd.empty()) return;
    launch_command(app.exec_cmd, app.terminal);
}

void AppManager::launch_command(const std::string& cmd, bool terminal) {
    if (cmd.empty()) return;

    std::string full_cmd = cmd;
    if (terminal) {
        const char* env_term = getenv("TERMINAL");
        std::string term = (env_term && *env_term) ? env_term : "kitty || foot || alacritty || wezterm || weston-terminal || xterm";
        full_cmd = term + " -e " + cmd;
    }

    pid_t pid = fork();
    if (pid == 0) {
        setsid();
        execl("/bin/sh", "sh", "-c", full_cmd.c_str(), nullptr);
        _exit(1);
    }
}

} // namespace miqu
