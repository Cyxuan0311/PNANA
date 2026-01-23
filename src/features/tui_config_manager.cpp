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
                 {"~/.config/kitty/kitty.conf", "~/.kitty.conf", "~/.config/kitty/kitty.conf.d"},
                 "terminal");

    addTUIConfig("ghostty", "Ghostty", "Fast, native, feature-rich terminal emulator",
                 {"~/.config/ghostty/config", "~/.config/ghostty/config.d"}, "terminal");

    addTUIConfig("alacritty", "Alacritty", "Cross-platform, OpenGL terminal emulator",
                 {"~/.config/alacritty/alacritty.yml", "~/.alacritty.yml",
                  "~/.config/alacritty/alacritty.toml"},
                 "terminal");

    addTUIConfig(
        "wezterm", "WezTerm", "GPU-accelerated cross-platform terminal emulator",
        {"~/.config/wezterm/wezterm.lua", "~/.wezterm.lua", "~/.config/wezterm/wezterm.toml"},
        "terminal");

    addTUIConfig("foot", "Foot", "Fast, lightweight and minimalistic Wayland terminal emulator",
                 {"~/.config/foot/foot.ini", "~/.config/foot/foot.d"}, "terminal");

    addTUIConfig("konsole", "Konsole", "KDE terminal emulator",
                 {"~/.config/konsolerc", "~/.local/share/konsole"}, "terminal");

    addTUIConfig("gnome-terminal", "GNOME Terminal", "GNOME desktop terminal emulator",
                 {"~/.config/gnome-terminal/profiles.d", "~/.gconf/apps/gnome-terminal"},
                 "terminal");

    addTUIConfig("xfce4-terminal", "XFCE4 Terminal", "XFCE desktop terminal emulator",
                 {"~/.config/xfce4/terminal/terminalrc"}, "terminal");

    addTUIConfig("st", "Simple Terminal", "Simple terminal implementation for X",
                 {"~/.config/st/config.h", "~/.xresources"}, "terminal");

    addTUIConfig("rxvt", "Rxvt", "VT102 terminal emulator for X", {"~/.Xdefaults", "~/.Xresources"},
                 "terminal");

    // 编辑器配置
    addTUIConfig(
        "neovim", "Neovim", "Hyperextensible Vim-based text editor",
        {"~/.config/nvim/init.lua", "~/.config/nvim/init.vim", "~/.vimrc", "~/.config/nvim/lua"},
        "editor");

    addTUIConfig("vim", "Vim", "Vi IMproved, a programmer's text editor",
                 {"~/.vimrc", "~/.config/vim/vimrc", "~/.vim/vimrc"}, "editor");

    addTUIConfig(
        "helix", "Helix", "A post-modern modal text editor",
        {"~/.config/helix/config.toml", "~/.config/helix/themes", "~/.config/helix/languages.toml"},
        "editor");

    addTUIConfig("kakoune", "Kakoune", "A vim-inspired, selection-first editor",
                 {"~/.config/kak/kakrc", "~/.config/kak/autoload"}, "editor");

    addTUIConfig("micro", "Micro", "A modern and intuitive terminal-based text editor",
                 {"~/.config/micro/settings.json", "~/.config/micro/bindings.json"}, "editor");

    addTUIConfig("nano", "Nano", "GNU nano text editor", {"~/.config/nano/nanorc", "~/.nanorc"},
                 "editor");

    addTUIConfig("emacs", "Emacs", "Extensible, customizable, free/libre text editor",
                 {"~/.emacs", "~/.emacs.d/init.el", "~/.config/emacs/init.el"}, "editor");

    addTUIConfig("joe", "Joe", "Joe's Own Editor", {"~/.joerc", "~/.config/joe/joerc"}, "editor");

    addTUIConfig("mg", "Mg", "Micro GNU/Emacs", {"~/.mg", "~/.config/mg/init.el"}, "editor");

    // 文件管理器配置
    addTUIConfig(
        "yazi", "Yazi", "Blazing fast terminal file manager written in Rust",
        {"~/.config/yazi/yazi.toml", "~/.config/yazi/keymap.toml", "~/.config/yazi/theme.toml"},
        "file_manager");

    addTUIConfig("lf", "LF", "Terminal file manager",
                 {"~/.config/lf/lfrc", "~/.lfrc", "~/.config/lf/colors"}, "file_manager");

    addTUIConfig("ranger", "Ranger", "Console file manager with VI key bindings",
                 {"~/.config/ranger/rc.conf", "~/.ranger/rc.conf", "~/.config/ranger/commands.py"},
                 "file_manager");

    addTUIConfig("nnn", "NNN", "The fastest terminal file manager ever written",
                 {"~/.config/nnn/plugins", "~/.nnncp", "~/.config/nnn/hotkeys"}, "file_manager");

    addTUIConfig("vifm", "Vifm", "VI File Manager", {"~/.config/vifm/vifmrc", "~/.vifm/vifmrc"},
                 "file_manager");

    addTUIConfig("mc", "Midnight Commander", "Visual file manager",
                 {"~/.config/mc/ini", "~/.mc/ini"}, "file_manager");

    addTUIConfig("fff", "FFF", "Fast file manager written in bash",
                 {"~/.config/fff/fff.sh", "~/.config/fff/config"}, "file_manager");

    addTUIConfig("clifm", "CliFM", "Command Line Interface File Manager",
                 {"~/.config/clifm/profiles", "~/.config/clifm/clifmrc"}, "file_manager");

    // 多路复用器配置
    addTUIConfig("tmux", "Tmux", "Terminal multiplexer",
                 {"~/.tmux.conf", "~/.config/tmux/tmux.conf", "~/.tmux/tmux.conf"}, "multiplexer");

    addTUIConfig("screen", "GNU Screen", "Full-screen window manager",
                 {"~/.screenrc", "~/.config/screen/screenrc"}, "multiplexer");

    addTUIConfig("zellij", "Zellij", "A terminal workspace with batteries included",
                 {"~/.config/zellij/config.kdl", "~/.config/zellij/layouts"}, "multiplexer");

    addTUIConfig("dvtm", "Dvtm", "Dynamic virtual terminal manager",
                 {"~/.config/dvtm/config.h", "~/.dvtmrc"}, "multiplexer");

    // Shell配置
    addTUIConfig("zsh", "Zsh", "Powerful shell with lots of features",
                 {"~/.zshrc", "~/.config/zsh/.zshrc", "~/.zsh/zshrc"}, "shell");

    addTUIConfig("bash", "Bash", "Bourne Again SHell",
                 {"~/.bashrc", "~/.bash_profile", "~/.bash_login", "~/.profile"}, "shell");

    addTUIConfig("fish", "Fish", "The friendly interactive shell",
                 {"~/.config/fish/config.fish", "~/.config/fish/functions"}, "shell");

    addTUIConfig("nushell", "Nushell", "A new type of shell",
                 {"~/.config/nushell/config.nu", "~/.config/nushell/env.nu"}, "shell");

    addTUIConfig("xonsh", "Xonsh", "Python-powered shell", {"~/.config/xonsh/rc.xsh", "~/.xonshrc"},
                 "shell");

    addTUIConfig("elvish", "Elvish", "Expressive programming language and a shell",
                 {"~/.config/elvish/rc.elv", "~/.elvish/rc.elv"}, "shell");

    // 版本控制配置
    addTUIConfig("git", "Git", "Distributed version control system",
                 {"~/.gitconfig", "~/.config/git/config", "~/.config/git/ignore"},
                 "version_control");

    addTUIConfig("mercurial", "Mercurial", "Distributed version control system",
                 {"~/.hgrc", "~/.config/hg/hgrc"}, "version_control");

    addTUIConfig("svn", "Subversion", "Centralized version control system",
                 {"~/.subversion/config", "~/.config/subversion/config"}, "version_control");

    // 系统工具配置
    addTUIConfig("htop", "Htop", "Interactive process viewer",
                 {"~/.config/htop/htoprc", "~/.htoprc"}, "system");

    addTUIConfig("btop", "Btop++", "Resource monitor that shows usage and stats",
                 {"~/.config/btop/btop.conf", "~/.config/btop/themes"}, "system");

    addTUIConfig("top", "Top", "Display Linux processes", {"~/.config/top/toprc"}, "system");

    addTUIConfig("iotop", "Iotop", "Simple top-like I/O monitor", {"~/.config/iotop/iotoprc"},
                 "system");

    addTUIConfig("glances", "Glances", "Curses-based monitoring tool",
                 {"~/.config/glances/glances.conf"}, "system");

    addTUIConfig("gotop", "Gotop", "Terminal based graphical activity monitor",
                 {"~/.config/gotop/config.toml"}, "system");

    addTUIConfig("bashtop", "Bashtop", "Resource monitor", {"~/.config/bashtop/bashtop.conf"},
                 "system");

    // 工具配置
    addTUIConfig("fzf", "FZF", "General-purpose command-line fuzzy finder",
                 {"~/.fzf.bash", "~/.fzf.zsh", "~/.config/fzf/fzf.bash"}, "utility");

    addTUIConfig("ripgrep", "Ripgrep", "Recursively searches directories for a regex pattern",
                 {"~/.config/ripgrep/config", "~/.ripgreprc"}, "utility");

    addTUIConfig("fd", "FD", "Simple, fast and user-friendly alternative to find",
                 {"~/.config/fd/ignore", "~/.config/fd/fd.toml"}, "utility");

    addTUIConfig("bat", "Bat", "A cat clone with syntax highlighting",
                 {"~/.config/bat/config", "~/.config/bat/themes"}, "utility");

    addTUIConfig("exa", "Exa", "Modern replacement for ls", {"~/.config/exa/exa.conf"}, "utility");

    addTUIConfig("delta", "Delta", "A syntax-highlighting pager for git",
                 {"~/.config/delta/config", "~/.config/delta/themes"}, "utility");

    addTUIConfig("tldr", "TLDR", "Simplified and community-driven man pages",
                 {"~/.config/tldr/config.toml"}, "utility");

    addTUIConfig("glow", "Glow", "Render markdown on the CLI", {"~/.config/glow/glow.yml"},
                 "utility");

    addTUIConfig("lazygit", "Lazygit", "Simple terminal UI for git commands",
                 {"~/.config/lazygit/config.yml"}, "utility");

    addTUIConfig("lazydocker", "Lazydocker", "Simple terminal UI for docker",
                 {"~/.config/lazydocker/config.yml"}, "utility");

    addTUIConfig("navi", "Navi", "Interactive cheatsheet tool",
                 {"~/.config/navi/config.yaml", "~/.config/navi/cheats"}, "utility");

    addTUIConfig("tealdeer", "Tealdeer", "Rust implementation of tldr",
                 {"~/.config/tealdeer/config.toml"}, "utility");

    addTUIConfig("zoxide", "Zoxide", "Smarter cd command", {"~/.config/zoxide/config.toml"},
                 "utility");

    addTUIConfig("starship", "Starship", "Minimal, fast shell prompt",
                 {"~/.config/starship.toml", "~/.config/starship/config.toml"}, "utility");
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
