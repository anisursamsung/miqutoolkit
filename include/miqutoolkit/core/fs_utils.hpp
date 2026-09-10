#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <cstdlib>

namespace miqu {
namespace fs = std::filesystem;

class FsUtils {
public:
    static std::string expand_user_path(const std::string& path) {
        if (!path.empty() && path[0] == '~') {
            const char* home = std::getenv("HOME");
            if (home) {
                return std::string(home) + path.substr(1);
            }
        }
        return path;
    }

    static bool has_image_extension(const std::string& path) {
        std::string lower = path;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        const std::vector<std::string> exts = {
            ".png", ".jpg", ".jpeg", ".webp", ".svg", ".bmp", ".gif", ".ico", ".avif"
        };
        for (const auto& ext : exts) {
            if (lower.size() >= ext.size() && lower.compare(lower.size() - ext.size(), ext.size(), ext) == 0) {
                return true;
            }
        }
        return false;
    }

    static bool is_image_file(const std::string& path) {
        std::string expanded = expand_user_path(path);
        if (!has_image_extension(expanded)) return false;
        std::error_code ec;
        return fs::is_regular_file(expanded, ec);
    }
};

} // namespace miqu
