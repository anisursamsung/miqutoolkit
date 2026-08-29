#pragma once

#include "miqutoolkit/view/grid_view.hpp"
#include <vector>
#include <string>

namespace miqu {

class PackageManager {
public:
    static std::vector<AppInfo> get_installed_applications();
    static void launch(const AppInfo& app);
    static std::string clean_exec(const std::string& raw);
};

} // namespace miqu
