#include "miqutoolkit/system/binary_manager.hpp"
#include <filesystem>
#include <sstream>
#include <set>
#include <algorithm>
#include <unistd.h>
#include <cstdlib>

namespace miqu {

namespace fs = std::filesystem;

const std::vector<std::string>& BinaryManager::get_system_binaries() {
    static std::vector<std::string> s_binaries;
    static bool s_loaded = false;

    if (s_loaded) {
        return s_binaries;
    }

    const char* path_env = getenv("PATH");
    std::string path_str = path_env ? path_env : "/usr/local/bin:/usr/bin:/bin";

    std::set<std::string> unique_bins;
    std::stringstream ss(path_str);
    std::string dir;

    while (std::getline(ss, dir, ':')) {
        if (dir.empty()) continue;
        std::error_code ec;
        if (!fs::is_directory(dir, ec)) continue;

        for (const auto& entry : fs::directory_iterator(dir, fs::directory_options::skip_permission_denied, ec)) {
            if (ec) break;
            if (entry.is_regular_file(ec) || entry.is_symlink(ec)) {
                std::string name = entry.path().filename().string();
                if (!name.empty() && name[0] != '.' && unique_bins.find(name) == unique_bins.end()) {
                    if (::access(entry.path().c_str(), X_OK) == 0) {
                        unique_bins.insert(std::move(name));
                    }
                }
            }
        }
    }

    s_binaries.assign(unique_bins.begin(), unique_bins.end());
    std::sort(s_binaries.begin(), s_binaries.end());
    s_loaded = true;
    return s_binaries;
}

bool BinaryManager::is_terminal_command(const std::string& cmd) {
    if (cmd.empty()) return false;

    std::string binary = cmd;
    size_t space = binary.find_first_of(" \t");
    if (space != std::string::npos) {
        binary = binary.substr(0, space);
    }
    size_t slash = binary.find_last_of('/');
    if (slash != std::string::npos) {
        binary = binary.substr(slash + 1);
    }

    static const std::set<std::string> tui_tools = {
        "top", "htop", "btop", "atop", "iotop", "iftop", "nethogs",
        "vim", "nvim", "vi", "nano", "emacs", "micro", "helix", "neovim",
        "less", "more", "man", "tail", "journalctl", "dmesg",
        "bash", "sh", "zsh", "fish", "tmux", "screen", "ssh", "scp", "sftp",
        "ping", "traceroute", "mtr", "curl", "wget", "nc", "netcat", "nmap",
        "yazi", "ranger", "lf", "nnn", "fzf", "lazygit", "gitui", "tig",
        "nmtui", "alsamixer", "pulsemixer", "cmatrix", "neofetch", "fastfetch",
        "cal", "bc", "gdb", "lldb", "strace", "lsof", "watch"
    };

    return tui_tools.find(binary) != tui_tools.end();
}

void BinaryManager::launch_command(const std::string& cmd, bool terminal) {
    if (cmd.empty()) return;

    std::string full_cmd = cmd;
    bool needs_terminal = terminal || is_terminal_command(cmd);

    if (needs_terminal) {
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
