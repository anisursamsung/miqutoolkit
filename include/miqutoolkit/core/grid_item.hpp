#pragma once

#include <string>

namespace miqu {

struct GridItem {
    std::string id;
    std::string title;
    std::string subtitle;
    std::string icon_name;
    std::string icon_path;
    std::string badge;
    std::string exec_cmd;
    bool terminal = false;
};

} // namespace miqu
