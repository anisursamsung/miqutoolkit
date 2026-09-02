#pragma once

#include "miqutoolkit/core/grid_item.hpp"
#include <vector>
#include <string>

namespace miqu {

class PackageManager {
public:
    static std::vector<GridItem> get_installed_applications();
    static void launch(const GridItem& app);
    static std::string clean_exec(const std::string& raw);
};

} // namespace miqu
