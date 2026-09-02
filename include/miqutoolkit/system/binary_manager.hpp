#pragma once

#include <vector>
#include <string>

namespace miqu {

class BinaryManager {
public:
    static const std::vector<std::string>& get_system_binaries();
    static bool is_terminal_command(const std::string& cmd);
    static void launch_command(const std::string& cmd, bool terminal = false);
};

} // namespace miqu
