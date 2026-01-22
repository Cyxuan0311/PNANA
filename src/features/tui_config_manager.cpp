#include "features/tui_config_manager.h"
#include <algorithm>

namespace pnana {
namespace features {

TUIConfigManager::TUIConfigManager() {
    initializeTUIConfigs();
}

std::vector<TUIConfig> TUIConfigManager::getAvailableTUIConfigs() const {
    std::vector<TUIConfig> available_configs;

    for (const auto& config : tui_configs_) {
        if (configExists(config)) {
            available_configs.push_back(config);
        }
    }

    return available_configs;
}

bool TUIConfigManager::configExists(const TUIConfig& config) const {
    for (const auto& path : config.config_paths) {
        try {
            std::filesystem::path config_path = expandPath(path);
            if (std::filesystem::exists(config_path)) {
                return true;
            }
        } catch (...) {
            // 忽略路径解析错误
            continue;
        }
    }
    return false;
}

std::string TUIConfigManager::getFirstAvailableConfigPath(const TUIConfig& config) const {
    for (const auto& path : config.config_paths) {
        try {
            std::filesystem::path config_path = expandPath(path);
            if (std::filesystem::exists(config_path)) {
                return config_path.string();
            }
        } catch (...) {
            // 忽略路径解析错误
            continue;
        }
    }
    return "";
}

void TUIConfigManager::setConfigOpenCallback(std::function<void(const std::string&)> callback) {
    config_open_callback_ = callback;
}

void TUIConfigManager::openConfig(const TUIConfig& config) {
    std::string config_path = getFirstAvailableConfigPath(config);
    if (!config_path.empty() && config_open_callback_) {
        config_open_callback_(config_path);
    }
}

void TUIConfigManager::initializeTUIConfigs() {
    // 终端模拟器配置
    addTUIConfig("kitty", "Kitty", "Fast, feature-rich, GPU based terminal emulator",
                 {"~/.config/kitty/kitty.conf", "~/.kitty.conf"}, "terminal");

    addTUIConfig("ghostty", "Ghostty", "Fast, native, feature-rich terminal emulator",
                 {"~/.config/ghostty/config"}, "terminal");

    addTUIConfig("alacritty", "Alacritty", "Cross-platform, OpenGL terminal emulator",
                 {"~/.config/alacritty/alacritty.yml", "~/.alacritty.yml"}, "terminal");

    addTUIConfig("wezterm", "WezTerm", "GPU-accelerated cross-platform terminal emulator",
                 {"~/.config/wezterm/wezterm.lua", "~/.wezterm.lua"}, "terminal");

    // 编辑器配置
    addTUIConfig("neovim", "Neovim", "Hyperextensible Vim-based text editor",
                 {"~/.config/nvim/init.lua", "~/.config/nvim/init.vim", "~/.vimrc"}, "editor");

    addTUIConfig("vim", "Vim", "Vi IMproved, a programmer's text editor",
                 {"~/.vimrc", "~/.config/vim/vimrc"}, "editor");

    addTUIConfig("helix", "Helix", "A post-modern modal text editor",
                 {"~/.config/helix/config.toml"}, "editor");

    addTUIConfig("kakoune", "Kakoune", "A vim-inspired, selection-first editor",
                 {"~/.config/kak/kakrc"}, "editor");

    addTUIConfig("micro", "Micro", "A modern and intuitive terminal-based text editor",
                 {"~/.config/micro/settings.json"}, "editor");

    // 文件管理器配置
    addTUIConfig("yazi", "Yazi", "Blazing fast terminal file manager written in Rust",
                 {"~/.config/yazi/yazi.toml", "~/.config/yazi/keymap.toml"}, "file_manager");

    addTUIConfig("lf", "LF", "Terminal file manager", {"~/.config/lf/lfrc", "~/.lfrc"},
                 "file_manager");

    addTUIConfig("ranger", "Ranger", "Console file manager with VI key bindings",
                 {"~/.config/ranger/rc.conf", "~/.ranger/rc.conf"}, "file_manager");

    addTUIConfig("nnn", "NNN", "The fastest terminal file manager ever written",
                 {"~/.config/nnn/plugins", "~/.nnncp"}, "file_manager");

    // 多路复用器配置
    addTUIConfig("tmux", "Tmux", "Terminal multiplexer",
                 {"~/.tmux.conf", "~/.config/tmux/tmux.conf"}, "multiplexer");

    addTUIConfig("screen", "GNU Screen", "Full-screen window manager", {"~/.screenrc"},
                 "multiplexer");

    addTUIConfig("zellij", "Zellij", "A terminal workspace with batteries included",
                 {"~/.config/zellij/config.kdl"}, "multiplexer");

    // Shell配置
    addTUIConfig("zsh", "Zsh", "Powerful shell with lots of features",
                 {"~/.zshrc", "~/.config/zsh/.zshrc"}, "shell");

    addTUIConfig("bash", "Bash", "Bourne Again SHell", {"~/.bashrc", "~/.bash_profile"}, "shell");

    addTUIConfig("fish", "Fish", "The friendly interactive shell", {"~/.config/fish/config.fish"},
                 "shell");

    // 版本控制配置
    addTUIConfig("git", "Git", "Distributed version control system",
                 {"~/.gitconfig", "~/.config/git/config"}, "version_control");

    // 其他工具配置
    addTUIConfig("htop", "Htop", "Interactive process viewer", {"~/.config/htop/htoprc"}, "system");

    addTUIConfig("btop", "Btop++", "Resource monitor that shows usage and stats",
                 {"~/.config/btop/btop.conf"}, "system");

    addTUIConfig("fzf", "FZF", "General-purpose command-line fuzzy finder",
                 {"~/.fzf.bash", "~/.fzf.zsh"}, "utility");

    addTUIConfig("ripgrep", "Ripgrep", "Recursively searches directories for a regex pattern",
                 {"~/.config/ripgrep/config"}, "utility");

    addTUIConfig("fd", "FD", "Simple, fast and user-friendly alternative to find",
                 {"~/.config/fd/ignore"}, "utility");
}

void TUIConfigManager::addTUIConfig(const std::string& name, const std::string& display_name,
                                    const std::string& description,
                                    const std::vector<std::string>& paths,
                                    const std::string& category) {
    TUIConfig config;
    config.name = name;
    config.display_name = display_name;
    config.description = description;
    config.config_paths = paths;
    config.category = category;

    tui_configs_.push_back(config);
}

std::filesystem::path TUIConfigManager::expandPath(const std::string& path) const {
    std::string expanded = path;

    // 展开家目录
    if (expanded.find("~") == 0) {
        const char* home = std::getenv("HOME");
        if (home) {
            expanded = std::string(home) + expanded.substr(1);
        }
    }

    // 展开环境变量
    size_t start = 0;
    while ((start = expanded.find("$", start)) != std::string::npos) {
        size_t end = start + 1;
        while (end < expanded.size() && (std::isalnum(expanded[end]) || expanded[end] == '_')) {
            ++end;
        }

        if (end > start + 1) {
            std::string var_name = expanded.substr(start + 1, end - start - 1);
            const char* var_value = std::getenv(var_name.c_str());
            if (var_value) {
                expanded.replace(start, end - start, var_value);
                start += std::string(var_value).size();
            } else {
                start = end;
            }
        } else {
            ++start;
        }
    }

    return std::filesystem::path(expanded);
}

} // namespace features
} // namespace pnana
