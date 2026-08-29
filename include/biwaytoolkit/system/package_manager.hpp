#pragma once

#include "biwaytoolkit/view/grid_view.hpp"
#include <vector>
#include <string>

namespace biway {

class PackageManager {
public:
    static std::vector<AppInfo> get_installed_applications();
    static void launch(const AppInfo& app);
    static std::string clean_exec(const std::string& raw);
};

} // namespace biway
