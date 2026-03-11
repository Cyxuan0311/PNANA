#include "features/tui_config_manager.h"
#include <algorithm>
#include <sstream>

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
    if (remote_path_checker_) {
        for (const auto& path : config.config_paths) {
            // 优先查缓存（由 prefetchAvailableRemoteConfigs 填充）
            if (remote_cache_populated_) {
                auto it = remote_availability_cache_.find(path);
                if (it != remote_availability_cache_.end()) {
                    if (it->second)
                        return true;
                    continue;
                }
            }
            // 缓存未命中时回退到单次 SSH 检查，并写入缓存
            bool exists = remote_path_checker_(path);
            remote_availability_cache_[path] = exists;
            if (exists)
                return true;
        }
        return false;
    }
    for (const auto& path : config.config_paths) {
        try {
            std::filesystem::path config_path = expandPath(path);
            if (std::filesystem::exists(config_path)) {
                return true;
            }
        } catch (...) {
            continue;
        }
    }
    return false;
}

std::string TUIConfigManager::getFirstAvailableConfigPath(const TUIConfig& config) const {
    if (remote_path_checker_ && remote_path_resolver_) {
        for (const auto& path : config.config_paths) {
            if (remote_path_checker_(path))
                return remote_path_resolver_(path);
        }
        return "";
    }
    for (const auto& path : config.config_paths) {
        try {
            std::filesystem::path config_path = expandPath(path);
            if (std::filesystem::exists(config_path)) {
                return config_path.string();
            }
        } catch (...) {
            continue;
        }
    }
    return "";
}

void TUIConfigManager::setRemotePathChecker(std::function<bool(const std::string&)> fn) {
    remote_path_checker_ = std::move(fn);
}

void TUIConfigManager::setRemotePathResolver(std::function<std::string(const std::string&)> fn) {
    remote_path_resolver_ = std::move(fn);
}

void TUIConfigManager::clearRemoteContext() {
    remote_path_checker_ = nullptr;
    remote_path_resolver_ = nullptr;
    remote_availability_cache_.clear();
    remote_cache_populated_ = false;
}

void TUIConfigManager::prefetchAvailableRemoteConfigs(RemoteExecutor executor) {
    if (!executor)
        return;

    // 收集所有需要检测的路径（去重），转换 ~/ 前缀为相对路径
    // 远程 SSH 命令以 home 目录为 cwd，所以去掉 ~/ 即为相对路径
    std::vector<std::string> rel_paths;  // 相对于 home 的路径（用于 SSH 命令）
    std::vector<std::string> orig_paths; // 对应的原始路径（~/... 形式）

    for (const auto& config : tui_configs_) {
        for (const auto& path : config.config_paths) {
            std::string rel = path;
            if (rel.size() >= 2 && rel.compare(0, 2, "~/") == 0) {
                rel = rel.substr(2);
            } else if (rel == "~") {
                rel = ".";
            }
            rel_paths.push_back(rel);
            orig_paths.push_back(path);
        }
    }

    // 构建批量检测命令：一次 SSH 调用检测所有路径
    // 命令格式：for p in p1 p2 ...; do [ -e "$p" ] && echo "Y:$p"; done
    std::string cmd = "cd ~ 2>/dev/null; for p in";
    for (const auto& p : rel_paths) {
        // 简单处理：配置路径不含空格，无需额外转义
        cmd += " ";
        cmd += p;
    }
    cmd += "; do [ -e \"$p\" ] && echo \"Y:$p\"; done 2>/dev/null";

    auto [ok, out] = executor(cmd);

    // 先把所有路径标记为不存在
    remote_availability_cache_.clear();
    for (const auto& orig : orig_paths) {
        remote_availability_cache_[orig] = false;
    }

    // 解析输出，标记存在的路径
    std::istringstream iss(out);
    std::string line;
    while (std::getline(iss, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
            line.pop_back();
        if (line.size() > 2 && line.compare(0, 2, "Y:") == 0) {
            std::string found_rel = line.substr(2);
            // 将相对路径映射回原始路径
            for (size_t i = 0; i < rel_paths.size(); ++i) {
                if (rel_paths[i] == found_rel) {
                    remote_availability_cache_[orig_paths[i]] = true;
                }
            }
        }
    }

    remote_cache_populated_ = true;
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
    // 按分类初始化配置
    initializeTerminalEmulators();
    initializeEditors();
    initializeFileManagers();
    initializeMultiplexers();
    initializeShells();
    initializeVersionControl();
    initializeSystemTools();
    initializeUtilities();
    initializeDevTools();
    initializeDatabaseClients();
    initializePackageManagers();
    initializeNetworkTools();
    initializeTextProcessing();
    initializeContainerTools();
    initializeSystemMonitoring();
    initializeTerminalEnhancements();
    initializeMailReaders();
    initializeWebBrowsers();
    initializePIMTools();
    initializeBuildSystems();
    initializeLanguageTools();
}

void TUIConfigManager::initializeTerminalEmulators() {
    // 主流终端模拟器
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

    // KDE/GNOME/XFCE 桌面终端
    addTUIConfig("konsole", "Konsole", "KDE terminal emulator",
                 {"~/.config/konsolerc", "~/.local/share/konsole"}, "terminal");

    addTUIConfig("gnome-terminal", "GNOME Terminal", "GNOME desktop terminal emulator",
                 {"~/.config/gnome-terminal/profiles.d", "~/.gconf/apps/gnome-terminal"},
                 "terminal");

    addTUIConfig("xfce4-terminal", "XFCE4 Terminal", "XFCE desktop terminal emulator",
                 {"~/.config/xfce4/terminal/terminalrc"}, "terminal");

    // 轻量级/传统终端
    addTUIConfig("st", "Simple Terminal", "Simple terminal implementation for X",
                 {"~/.config/st/config.h", "~/.xresources"}, "terminal");

    addTUIConfig("rxvt", "Rxvt", "VT102 terminal emulator for X", {"~/.Xdefaults", "~/.Xresources"},
                 "terminal");

    // 其他终端模拟器
    addTUIConfig("terminator", "Terminator", "Multiple terminals in one window",
                 {"~/.config/terminator/config"}, "terminal");

    addTUIConfig("tilix", "Tilix", "Tiling terminal emulator", {"~/.config/tilix"}, "terminal");

    addTUIConfig("cool-retro-term", "Cool Retro Term",
                 "Terminal emulator mimicking old cathode displays", {"~/.config/cool-retro-term"},
                 "terminal");

    addTUIConfig("hyper", "Hyper", "Terminal built on web technologies",
                 {"~/.hyper.js", "~/.config/Hyper"}, "terminal");

    addTUIConfig("tabby", "Tabby", "A terminal for a modern age", {"~/.config/tabby/config.yaml"},
                 "terminal");
}

void TUIConfigManager::initializeEditors() {
    // 现代终端编辑器
    addTUIConfig("pnana", "pnana", "Modern terminal text editor with native shortcuts",
                 {"~/.config/pnana/config.json"}, "editor");

    addTUIConfig(
        "neovim", "Neovim", "Hyperextensible Vim-based text editor",
        {"~/.config/nvim/init.lua", "~/.config/nvim/init.vim", "~/.vimrc", "~/.config/nvim/lua"},
        "editor");

    addTUIConfig("vim", "Vim", "Vi IMproved, a programmer's text editor",
                 {"~/.vimrc", "~/.vim/vimrc", "~/.config/vim/vimrc"}, "editor");

    addTUIConfig(
        "helix", "Helix", "A post-modern modal text editor",
        {"~/.config/helix/config.toml", "~/.config/helix/themes", "~/.config/helix/languages.toml"},
        "editor");

    addTUIConfig("kakoune", "Kakoune", "A vim-inspired, selection-first editor",
                 {"~/.config/kak/kakrc", "~/.config/kak/autoload"}, "editor");

    addTUIConfig("micro", "Micro", "A modern and intuitive terminal-based text editor",
                 {"~/.config/micro/settings.json", "~/.config/micro/bindings.json"}, "editor");

    // 传统终端编辑器
    addTUIConfig("nano", "Nano", "GNU nano text editor", {"~/.config/nano/nanorc", "~/.nanorc"},
                 "editor");

    addTUIConfig("emacs", "Emacs", "Extensible, customizable, free/libre text editor",
                 {"~/.emacs", "~/.emacs.d/init.el", "~/.config/emacs/init.el"}, "editor");

    addTUIConfig("joe", "Joe", "Joe's Own Editor", {"~/.joerc", "~/.config/joe/joerc"}, "editor");

    addTUIConfig("mg", "Mg", "Micro GNU/Emacs", {"~/.mg", "~/.config/mg/init.el"}, "editor");

    // 其他终端编辑器
    addTUIConfig("vis", "Vis", "Modern, legacy-free Vim-like editor",
                 {"~/.config/vis", "~/.config/vis/visrc.lua"}, "editor");

    addTUIConfig("amp", "Amp", "A complete text editor for your terminal", {"~/.config/amp"},
                 "editor");

    addTUIConfig("ne", "NE", "Nice Editor, a free text editor", {"~/.ne"}, "editor");

    addTUIConfig("jed", "Jed", "Programmer's editor", {"~/.jedrc"}, "editor");

    addTUIConfig("le", "LE", "Text editor with block and binary operations", {"~/.lerec"},
                 "editor");

    // GUI 编辑器（带 CLI 配置）
    addTUIConfig("vscode", "VS Code", "Visual Studio Code (with CLI config)",
                 {"~/.config/Code/User/settings.json", "~/.vscode/settings.json"}, "editor");

    addTUIConfig("subl", "Sublime Text", "Sophisticated text editor for code, markup and prose",
                 {"~/.config/sublime-text/Packages/User", "~/.config/sublime-text-3/Packages/User"},
                 "editor");

    addTUIConfig("atom", "Atom", "Hackable text editor for the 21st Century",
                 {"~/.atom/config.cson", "~/.atom/init.coffee", "~/.atom/packages"}, "editor");

    addTUIConfig("code-oss", "Code OSS", "Open source build of VS Code",
                 {"~/.config/Code - OSS/User/settings.json"}, "editor");

    addTUIConfig("vscodium", "VSCodium", "Free/Libre Open Source Software Binaries of VSCode",
                 {"~/.config/VSCodium/User/settings.json"}, "editor");

    addTUIConfig("lite-xl", "Lite XL", "A lightweight text editor written in Lua",
                 {"~/.config/lite-xl"}, "editor");

    addTUIConfig("lapce", "Lapce", "Lightning-fast and Powerful Code Editor",
                 {"~/.config/lapce/settings.toml", "~/.config/lapce/keymaps.toml"}, "editor");

    addTUIConfig("zed", "Zed", "Code at the speed of thought",
                 {"~/.config/zed/settings.json", "~/.config/zed/keymap.json"}, "editor");
}

void TUIConfigManager::initializeFileManagers() {
    // Rust/现代文件管理器
    addTUIConfig(
        "yazi", "Yazi", "Blazing fast terminal file manager written in Rust",
        {"~/.config/yazi/yazi.toml", "~/.config/yazi/keymap.toml", "~/.config/yazi/theme.toml"},
        "file_manager");

    addTUIConfig("lf", "LF", "Terminal file manager",
                 {"~/.config/lf/lfrc", "~/.lfrc", "~/.config/lf/colors"}, "file_manager");

    // VI 风格文件管理器
    addTUIConfig("ranger", "Ranger", "Console file manager with VI key bindings",
                 {"~/.config/ranger/rc.conf", "~/.ranger/rc.conf", "~/.config/ranger/commands.py"},
                 "file_manager");

    addTUIConfig("vifm", "Vifm", "VI File Manager", {"~/.config/vifm/vifmrc", "~/.vifm/vifmrc"},
                 "file_manager");

    // 轻量级文件管理器
    addTUIConfig("nnn", "NNN", "The fastest terminal file manager ever written",
                 {"~/.config/nnn/plugins", "~/.nnncp", "~/.config/nnn/hotkeys"}, "file_manager");

    addTUIConfig("mc", "Midnight Commander", "Visual file manager",
                 {"~/.config/mc/ini", "~/.mc/ini"}, "file_manager");

    addTUIConfig("fff", "FFF", "Fast file manager written in bash",
                 {"~/.config/fff/fff.sh", "~/.config/fff/config"}, "file_manager");

    addTUIConfig("clifm", "CliFM", "Command Line Interface File Manager",
                 {"~/.config/clifm/profiles", "~/.config/clifm/clifmrc"}, "file_manager");

    // 桌面文件管理器
    addTUIConfig("dolphin", "Dolphin", "KDE file manager",
                 {"~/.config/dolphinrc", "~/.local/share/dolphin"}, "file_manager");

    addTUIConfig("thunar", "Thunar", "Modern file manager for the Xfce Desktop Environment",
                 {"~/.config/Thunar", "~/.config/xfce4/xfconf/xfce-perchannel-xml/thunar.xml"},
                 "file_manager");

    addTUIConfig("pcmanfm", "PCManFM", "Lightweight file manager",
                 {"~/.config/pcmanfm/default/pcmanfm.conf"}, "file_manager");

    addTUIConfig("nemo", "Nemo", "Cinnamon desktop file manager", {"~/.config/nemo"},
                 "file_manager");

    addTUIConfig("caja", "Caja", "MATE desktop file manager", {"~/.config/caja"}, "file_manager");

    addTUIConfig("nautilus", "Nautilus", "GNOME file manager",
                 {"~/.config/nautilus", "~/.local/share/nautilus"}, "file_manager");

    addTUIConfig("doublecmd", "Double Commander", "Cross platform open source file manager",
                 {"~/.config/doublecmd"}, "file_manager");

    // 其他文件管理器
    addTUIConfig("cfm", "CFM", "Simple file manager with no dependencies", {"~/.cfm"},
                 "file_manager");

    addTUIConfig("noice", "Noice", "Small file browser", {"~/.config/noice"}, "file_manager");
}

void TUIConfigManager::initializeMultiplexers() {
    // 主流终端复用器
    addTUIConfig("tmux", "Tmux", "Terminal multiplexer",
                 {"~/.tmux.conf", "~/.config/tmux/tmux.conf", "~/.tmux/tmux.conf"}, "multiplexer");

    addTUIConfig("screen", "GNU Screen", "Full-screen window manager",
                 {"~/.screenrc", "~/.config/screen/screenrc"}, "multiplexer");

    addTUIConfig("zellij", "Zellij", "A terminal workspace with batteries included",
                 {"~/.config/zellij/config.kdl", "~/.config/zellij/layouts"}, "multiplexer");

    // 其他复用器
    addTUIConfig("dvtm", "Dvtm", "Dynamic virtual terminal manager",
                 {"~/.config/dvtm/config.h", "~/.dvtmrc"}, "multiplexer");

    addTUIConfig("tmate", "Tmate", "Instant terminal sharing", {"~/.tmate.conf"}, "multiplexer");

    addTUIConfig("byobu", "Byobu", "Text-based window manager and terminal multiplexer",
                 {"~/.byobu"}, "multiplexer");

    addTUIConfig("abduco", "Abduco", "Session management in a clean and simple way",
                 {"~/.config/abduco"}, "multiplexer");
}

void TUIConfigManager::initializeShells() {
    // 主流 Shell
    addTUIConfig("zsh", "Zsh", "Powerful shell with lots of features",
                 {"~/.zshrc", "~/.config/zsh/.zshrc", "~/.zsh/zshrc"}, "shell");

    addTUIConfig("bash", "Bash", "Bourne Again SHell",
                 {"~/.bashrc", "~/.bash_profile", "~/.bash_login", "~/.profile"}, "shell");

    addTUIConfig("fish", "Fish", "The friendly interactive shell",
                 {"~/.config/fish/config.fish", "~/.config/fish/functions"}, "shell");

    // 现代 Shell
    addTUIConfig("nushell", "Nushell", "A new type of shell",
                 {"~/.config/nushell/config.nu", "~/.config/nushell/env.nu"}, "shell");

    addTUIConfig("xonsh", "Xonsh", "Python-powered shell", {"~/.config/xonsh/rc.xsh", "~/.xonshrc"},
                 "shell");

    addTUIConfig("elvish", "Elvish", "Expressive programming language and a shell",
                 {"~/.config/elvish/rc.elv", "~/.elvish/rc.elv"}, "shell");

    addTUIConfig("ion", "Ion", "Modern shell with a syntax similar to Rust", {"~/.config/ion"},
                 "shell");

    addTUIConfig("murex", "Murex", "Shell designed for DevOps", {"~/.murex_profile"}, "shell");

    addTUIConfig("oil", "Oil", "Shell with structured data", {"~/.config/oil"}, "shell");

    // 传统 Shell
    addTUIConfig("dash", "Dash", "POSIX-compliant shell", {"~/.dashrc"}, "shell");

    addTUIConfig("ash", "Ash", "Almquist shell", {"~/.ashrc"}, "shell");

    addTUIConfig("ksh", "Ksh", "Korn shell", {"~/.kshrc"}, "shell");

    addTUIConfig("tcsh", "Tcsh", "TENEX C shell", {"~/.tcshrc", "~/.cshrc"}, "shell");
}

void TUIConfigManager::initializeVersionControl() {
    // 主流版本控制
    addTUIConfig("git", "Git", "Distributed version control system",
                 {"~/.gitconfig", "~/.config/git/config", "~/.config/git/ignore"},
                 "version_control");

    addTUIConfig("mercurial", "Mercurial", "Distributed version control system",
                 {"~/.hgrc", "~/.config/hg/hgrc"}, "version_control");

    addTUIConfig("svn", "Subversion", "Centralized version control system",
                 {"~/.subversion/config", "~/.config/subversion/config"}, "version_control");

    // 其他版本控制
    addTUIConfig("fossil", "Fossil", "Distributed version control system",
                 {"~/.fossil", "~/.config/fossil"}, "version_control");

    addTUIConfig("pijul", "Pijul", "A patch-based distributed version control system",
                 {"~/.config/pijul", "~/.local/share/pijul"}, "version_control");

    addTUIConfig("darcs", "Darcs", "Distributed, interactive, smart revision control system",
                 {"~/.darcs", "~/.config/darcs"}, "version_control");
}

void TUIConfigManager::initializeSystemTools() {
    // 系统监控工具
    addTUIConfig("htop", "Htop", "Interactive process viewer",
                 {"~/.config/htop/htoprc", "~/.htoprc"}, "system");

    addTUIConfig("top", "Top", "Display Linux processes", {"~/.config/top/toprc"}, "system");

    addTUIConfig("btop", "Btop++", "Resource monitor that shows usage and stats",
                 {"~/.config/btop"}, "system");

    addTUIConfig("bashtop", "Bashtop", "Resource monitor", {"~/.config/bashtop"}, "system");

    addTUIConfig("gotop", "Gotop", "Terminal based graphical activity monitor", {"~/.config/gotop"},
                 "system");

    addTUIConfig("glances", "Glances", "Curses-based monitoring tool", {"~/.config/glances"},
                 "system");

    // 系统信息工具
    addTUIConfig(
        "neofetch", "Neofetch", "Fast, highly customizable system info script",
        {"~/.config/neofetch/config.conf", "~/.config/neofetch/config", "~/.config/neofetch"},
        "system");

    addTUIConfig("conky", "Conky", "Light-weight system monitor", {"~/.conkyrc", "~/.config/conky"},
                 "system");

    // I/O 监控工具
    addTUIConfig("iotop", "Iotop", "Simple top-like I/O monitor", {"~/.config/iotop/iotoprc"},
                 "system");

    addTUIConfig("iftop", "Iftop", "Display bandwidth usage on an interface",
                 {"~/.iftoprc", "~/.config/iftop/config"}, "system");

    addTUIConfig("nload", "Nload", "Realtime console network usage monitor",
                 {"~/.nload", "~/.config/nload"}, "system");

    addTUIConfig("dstat", "Dstat", "Versatile resource statistics tool", {"~/.dstatrc"}, "system");

    // GPU/电源监控
    addTUIConfig("powertop", "Powertop", "Tool to diagnose issues with power consumption",
                 {"~/.config/powertop"}, "system");

    addTUIConfig("nvtop", "Nvtop", "GPUs process monitoring for AMD, Intel and NVIDIA",
                 {"~/.config/nvtop"}, "system");

    addTUIConfig("s-tui", "S-TUI", "Terminal-based CPU stress and monitoring utility",
                 {"~/.config/s-tui"}, "system");

    addTUIConfig("radeontop", "Radeontop", "Radeon graphics card utilization",
                 {"~/.config/radeontop"}, "system");

    // 高级系统监控
    addTUIConfig("atop", "Atop", "Advanced System & Process Monitor",
                 {"~/.atoprc", "~/.config/atop"}, "system");

    addTUIConfig("slurm", "Slurm", "Simple Linux Utility for Resource Management",
                 {"~/.config/slurm"}, "system");

    addTUIConfig("collectl", "Collectl", "Collects data that describes the current system status",
                 {"~/.collectl"}, "system");
}

void TUIConfigManager::initializeUtilities() {
    // 搜索/导航工具
    addTUIConfig("fzf", "FZF", "General-purpose command-line fuzzy finder",
                 {"~/.fzf.bash", "~/.fzf.zsh", "~/.config/fzf/fzf.bash"}, "utility");

    addTUIConfig("ripgrep", "Ripgrep", "Recursively searches directories for a regex pattern",
                 {"~/.config/ripgrep/config", "~/.ripgreprc"}, "utility");

    addTUIConfig("fd", "FD", "Simple, fast and user-friendly alternative to find",
                 {"~/.config/fd/ignore", "~/.config/fd/fd.toml"}, "utility");

    addTUIConfig("zoxide", "Zoxide", "Smarter cd command", {"~/.config/zoxide/config.toml"},
                 "utility");

    // 输出增强工具
    addTUIConfig("bat", "Bat", "A cat clone with syntax highlighting",
                 {"~/.config/bat/config", "~/.config/bat/themes"}, "utility");

    addTUIConfig("exa", "Exa", "Modern replacement for ls", {"~/.config/exa/exa.conf"}, "utility");

    addTUIConfig("lsd", "LSD", "Modern ls replacement", {"~/.config/lsd/config.yaml"}, "utility");

    addTUIConfig("delta", "Delta", "A syntax-highlighting pager for git",
                 {"~/.config/delta/config", "~/.config/delta/themes"}, "utility");

    // 文档/帮助工具
    addTUIConfig("tldr", "TLDR", "Simplified and community-driven man pages",
                 {"~/.config/tldr/config.toml"}, "utility");

    addTUIConfig("tealdeer", "Tealdeer", "Rust implementation of tldr",
                 {"~/.config/tealdeer/config.toml"}, "utility");

    addTUIConfig("glow", "Glow", "Render markdown on the CLI", {"~/.config/glow/glow.yml"},
                 "utility");

    addTUIConfig("navi", "Navi", "Interactive cheatsheet tool",
                 {"~/.config/navi/config.yaml", "~/.config/navi/cheats"}, "utility");

    // Git TUI 客户端
    addTUIConfig("lazygit", "Lazygit", "Simple terminal UI for git commands",
                 {"~/.config/lazygit/config.yml"}, "utility");

    addTUIConfig("tig", "Tig", "Text-mode interface for Git", {"~/.tigrc", "~/.config/tig"},
                 "utility");

    addTUIConfig("gitui", "GitUI", "Blazing fast terminal UI for Git", {"~/.config/gitui"},
                 "utility");

    addTUIConfig("gh", "GitHub CLI", "GitHub command-line tool", {"~/.config/gh"}, "utility");

    addTUIConfig("glab", "GitLab CLI", "GitLab command-line tool", {"~/.config/glab"}, "utility");

    // Docker TUI 客户端
    addTUIConfig("lazydocker", "Lazydocker", "Simple terminal UI for docker",
                 {"~/.config/lazydocker/config.yml"}, "utility");

    // Shell 提示/会话管理
    addTUIConfig("starship", "Starship", "Minimal, fast shell prompt",
                 {"~/.config/starship.toml", "~/.config/starship/config.toml"}, "utility");

    addTUIConfig("tmuxp", "Tmuxp", "Tmux session manager", {"~/.tmuxp", "~/.config/tmuxp"},
                 "utility");

    addTUIConfig("dtach", "Dtach", "Program that emulates the detach feature of screen",
                 {"~/.config/dtach"}, "utility");

    // 任务/时间管理
    addTUIConfig("task", "Taskwarrior", "Command-line task management",
                 {"~/.taskrc", "~/.config/task"}, "utility");

    addTUIConfig("timewarrior", "Timewarrior", "Command-line time tracking", {"~/.timewarrior"},
                 "utility");

    // 其他实用工具
    addTUIConfig("direnv", "Direnv", "Environment variable manager",
                 {"~/.direnvrc", "~/.config/direnv/config.toml"}, "utility");

    addTUIConfig("autojump", "Autojump", "Faster directory navigation",
                 {"~/.autojumprc", "~/.config/autojump/config"}, "utility");

    addTUIConfig("fasd", "Fasd", "Quick access to files/directories",
                 {"~/.fasdrc", "~/.config/fasd/config"}, "utility");

    addTUIConfig("thefuck", "TheFuck", "Correct command typos", {"~/.config/thefuck/settings.py"},
                 "utility");

    addTUIConfig("cheat", "Cheat", "Command-line cheatsheets",
                 {"~/.cheatrc", "~/.config/cheat/config.yml"}, "utility");

    addTUIConfig("broot", "Broot", "Interactive directory tree viewer",
                 {"~/.config/broot/conf.toml"}, "utility");

    addTUIConfig("xsv", "XSV", "Fast CSV toolkit", {"~/.config/xsv/config"}, "utility");

    addTUIConfig("choose", "Choose", "Human-friendly alternative to cut/awk",
                 {"~/.config/choose/config"}, "utility");
}

void TUIConfigManager::initializeDevTools() {
    // 容器/编排工具
    addTUIConfig("docker", "Docker", "Container platform",
                 {"~/.docker/config.json", "~/.config/docker/config.json"}, "utility");

    addTUIConfig("podman", "Podman", "Container engine", {"~/.config/containers/podman.conf"},
                 "utility");

    addTUIConfig("podman-compose", "Podman Compose", "Docker Compose for Podman",
                 {"~/.config/podman-compose/config.yaml"}, "utility");

    addTUIConfig("lxc", "LXC", "Linux containers", {"~/.config/lxc/config", "~/.lxc/config"},
                 "utility");

    addTUIConfig("lxd", "LXD", "Container hypervisor", {"~/.config/lxd/config"}, "utility");

    // Kubernetes 工具
    addTUIConfig("kubectl", "Kubectl", "Kubernetes command-line tool",
                 {"~/.kube/config", "~/.config/kube"}, "utility");

    addTUIConfig("helm", "Helm", "Kubernetes package manager", {"~/.config/helm", "~/.helm"},
                 "utility");

    addTUIConfig("k9s", "K9s", "Kubernetes CLI", {"~/.config/k9s/config.yaml"}, "utility");

    addTUIConfig("kubectx", "Kubectx", "Kubernetes context switcher",
                 {"~/.kubectxrc", "~/.config/kubectx/config"}, "utility");

    addTUIConfig("kubens", "Kubens", "Kubernetes namespace switcher",
                 {"~/.kubensrc", "~/.config/kubens/config"}, "utility");

    addTUIConfig("kind", "Kind", "Kubernetes in Docker", {"~/.config/kind/config.yaml"}, "utility");

    addTUIConfig("minikube", "Minikube", "Local Kubernetes",
                 {"~/.minikube/config", "~/.config/minikube/config"}, "utility");

    addTUIConfig("k3d", "K3D", "K3s in Docker", {"~/.config/k3d/config.yaml"}, "utility");

    addTUIConfig("helmfile", "Helmfile", "Helm release management",
                 {"~/.config/helmfile/config.yaml"}, "utility");

    addTUIConfig("kustomize", "Kustomize", "Kubernetes configuration customization",
                 {"~/.config/kustomize/config"}, "utility");

    addTUIConfig("skaffold", "Skaffold", "Kubernetes development tool",
                 {"~/.skaffoldrc", "~/.config/skaffold/config"}, "utility");

    // IaC 工具
    addTUIConfig("terraform", "Terraform", "Infrastructure as code software tool",
                 {"~/.terraformrc", "~/.config/terraform"}, "utility");

    addTUIConfig("terraform-docs", "Terraform Docs", "Generate Terraform docs",
                 {"~/.config/terraform-docs/config.yaml"}, "utility");

    addTUIConfig("tflint", "TFLint", "Terraform linter",
                 {"~/.tflint.hcl", "~/.config/tflint/config.hcl"}, "utility");

    addTUIConfig("ansible", "Ansible", "Configuration management tool",
                 {"~/.ansible.cfg", "~/.config/ansible"}, "utility");
}

void TUIConfigManager::initializeDatabaseClients() {
    // SQL 数据库客户端
    addTUIConfig("sqlite3", "SQLite3", "Command-line interface for SQLite",
                 {"~/.sqliterc", "~/.config/sqlite3"}, "utility");

    addTUIConfig("mysql", "MySQL Client", "MySQL command-line client",
                 {"~/.my.cnf", "~/.config/mysql"}, "utility");

    addTUIConfig("psql", "PostgreSQL Client", "PostgreSQL command-line client",
                 {"~/.psqlrc", "~/.config/psql"}, "utility");

    // NoSQL 数据库客户端
    addTUIConfig("mongosh", "MongoDB Shell", "MongoDB command-line shell",
                 {"~/.mongoshrc.js", "~/.config/mongosh"}, "utility");

    addTUIConfig("redis-cli", "Redis CLI", "Redis command-line interface", {"~/.rediscli"},
                 "utility");
}

void TUIConfigManager::initializePackageManagers() {
    // Python 包管理
    addTUIConfig("pip", "Pip", "Python package installer",
                 {"~/.config/pip/pip.conf", "~/.pip/pip.conf"}, "utility");

    addTUIConfig("poetry", "Poetry", "Python dependency management",
                 {"~/.config/pypoetry/config.toml", "~/.poetry/config.toml"}, "utility");

    addTUIConfig("pipenv", "Pipenv", "Python dev workflow tool",
                 {"~/.pipenv/config", "~/.config/pipenv"}, "utility");

    addTUIConfig("conda", "Conda", "Package manager for any language",
                 {"~/.condarc", "~/.config/conda/.condarc"}, "utility");

    // Node.js 包管理
    addTUIConfig("npm", "NPM", "Node Package Manager", {"~/.npmrc", "~/.config/npm"}, "utility");

    addTUIConfig("yarn", "Yarn", "Fast, reliable, and secure dependency management",
                 {"~/.yarnrc", "~/.config/yarn"}, "utility");

    addTUIConfig("pnpm", "PNPM", "Fast, disk space efficient package manager",
                 {"~/.config/pnpm", "~/.pnpmrc"}, "utility");

    // 版本管理器
    addTUIConfig("nvm", "NVM", "Node Version Manager", {"~/.nvmrc", "~/.config/nvm/config"},
                 "utility");

    addTUIConfig("fnm", "FNM", "Fast Node Manager", {"~/.fnm/config.toml", "~/.config/fnm"},
                 "utility");

    addTUIConfig("asdf", "ASDF", "Extensible version manager",
                 {"~/.asdfrc", "~/.config/asdf/config"}, "utility");

    addTUIConfig("rbenv", "rbenv", "Ruby version manager", {"~/.rbenvrc", "~/.config/rbenv/config"},
                 "utility");

    addTUIConfig("pyenv", "Pyenv", "Python version manager",
                 {"~/.pyenvrc", "~/.config/pyenv/config"}, "utility");

    // Rust 工具链
    addTUIConfig("cargo", "Cargo", "Rust package manager",
                 {"~/.cargo/config", "~/.cargo/config.toml"}, "utility");

    addTUIConfig("rustup", "Rustup", "Rust toolchain installer", {"~/.rustup/settings.toml"},
                 "utility");

    // Go 工具链
    addTUIConfig("go", "Go", "Go programming language tools", {"~/.config/go/env", "~/.go"},
                 "utility");
}

void TUIConfigManager::initializeNetworkTools() {
    // 数据传输工具
    addTUIConfig("curl", "cURL", "Command line tool for transferring data",
                 {"~/.curlrc", "~/.config/curl"}, "utility");

    addTUIConfig("wget", "Wget", "Free utility for non-interactive download",
                 {"~/.wgetrc", "~/.config/wget"}, "utility");

    addTUIConfig("httpie", "HTTPie", "User-friendly command-line HTTP client",
                 {"~/.config/httpie", "~/.httpie"}, "utility");

    addTUIConfig("aria2", "Aria2",
                 "Lightweight multi-protocol & multi-source command-line download utility",
                 {"~/.aria2/aria2.conf", "~/.config/aria2"}, "utility");

    addTUIConfig("rsync", "Rsync", "File synchronization tool",
                 {"~/.rsyncrc", "~/.config/rsync/config"}, "utility");

    // SSH 相关
    addTUIConfig("ssh", "SSH", "Secure shell", {"~/.ssh/config", "~/.config/ssh/config"},
                 "utility");

    addTUIConfig("scp", "SCP", "Secure copy", {"~/.scprc", "~/.config/scp/config"}, "utility");

    addTUIConfig("sshuttle", "SSHuttle", "VPN over SSH",
                 {"~/.sshuttle.cfg", "~/.config/sshuttle/config"}, "utility");

    // 网络诊断
    addTUIConfig("ping", "Ping", "Network reachability test",
                 {"~/.pingrc", "~/.config/ping/config"}, "utility");

    addTUIConfig("traceroute", "Traceroute", "Trace network path",
                 {"~/.tracerouterc", "~/.config/traceroute/config"}, "utility");

    addTUIConfig("mtr", "MTR", "Combined ping/traceroute", {"~/.mtrrc", "~/.config/mtr/config"},
                 "utility");

    addTUIConfig("dig", "Dig", "DNS lookup tool", {"~/.digrc", "~/.config/dig/config"}, "utility");

    addTUIConfig("nslookup", "NSLookup", "DNS query tool",
                 {"~/.nslookuprc", "~/.config/nslookup/config"}, "utility");

    // 网络统计
    addTUIConfig("ss", "SS", "Socket statistics", {"~/.ssrc", "~/.config/ss/config"}, "utility");

    addTUIConfig("netstat", "Netstat", "Network statistics",
                 {"~/.netstatrc", "~/.config/netstat/config"}, "utility");

    // 包分析
    addTUIConfig("tcpdump", "TCPDump", "Network packet analyzer",
                 {"~/.tcpdumprc", "~/.config/tcpdump/config"}, "utility");

    addTUIConfig("wireshark-cli", "Wireshark CLI", "Packet capture/analysis (tshark)",
                 {"~/.wireshark/cli.conf", "~/.config/wireshark/tshark.conf"}, "utility");
}

void TUIConfigManager::initializeTextProcessing() {
    // JSON/YAML 处理
    addTUIConfig("jq", "JQ", "Command-line JSON processor", {"~/.jq", "~/.config/jq"}, "utility");

    addTUIConfig("yq", "YQ", "Command-line YAML processor", {"~/.config/yq"}, "utility");

    // 文档转换
    addTUIConfig("pandoc", "Pandoc", "Universal markup converter",
                 {"~/.pandoc", "~/.config/pandoc"}, "utility");

    addTUIConfig("pandoc-citeproc", "Pandoc Citeproc", "Pandoc citation processing library",
                 {"~/.pandoc"}, "utility");
}

void TUIConfigManager::initializeContainerTools() {
    // 已在 initializeDevTools 中覆盖了大部分容器工具
    // 这里保留一些额外的容器相关工具
}

void TUIConfigManager::initializeSystemMonitoring() {
    // 内存/CPU 统计
    addTUIConfig("vmstat", "VMStat", "Virtual memory statistics",
                 {"~/.vmstatrc", "~/.config/vmstat/config"}, "system");

    addTUIConfig("iostat", "IOStat", "I/O statistics", {"~/.iostatrc", "~/.config/iostat/config"},
                 "system");

    addTUIConfig("mpstat", "MPStat", "CPU statistics", {"~/.mpstatrc", "~/.config/mpstat/config"},
                 "system");

    addTUIConfig("sar", "SAR", "System activity reporter", {"~/.sarrc", "~/.config/sar/config"},
                 "system");

    // 系统管理
    addTUIConfig("sshd", "SSHD", "SSH daemon", {"~/.ssh/sshd_config", "/etc/ssh/sshd_config"},
                 "system");

    addTUIConfig("cron", "Cron", "Task scheduler", {"~/.crontab", "~/.config/cron/tasks"},
                 "system");

    addTUIConfig("logrotate", "Logrotate", "Log file manager",
                 {"~/.logrotaterc", "~/.config/logrotate/config"}, "system");

    addTUIConfig("systemd", "SystemD", "System manager", {"~/.config/systemd/user.conf"}, "system");

    addTUIConfig("journalctl", "JournalCTL", "System log viewer", {"~/.config/journalctl/config"},
                 "system");

    // 安全/防火墙
    addTUIConfig("ufw", "UFW", "Uncomplicated Firewall", {"~/.ufwrc", "~/.config/ufw/config"},
                 "system");

    addTUIConfig("iptables", "IPTables", "Firewall", {"~/.iptablesrc", "~/.config/iptables/rules"},
                 "system");

    addTUIConfig("fail2ban", "Fail2Ban", "Intrusion prevention", {"~/.config/fail2ban/jail.local"},
                 "system");
}

void TUIConfigManager::initializeTerminalEnhancements() {
    // 分页器
    addTUIConfig("man", "Man", "Manual pages", {"~/.manrc", "~/.config/man/config"}, "utility");

    addTUIConfig("less", "Less", "Pager", {"~/.lessrc", "~/.config/less/config"}, "utility");

    addTUIConfig("more", "More", "Pager", {"~/.morerc", "~/.config/more/config"}, "utility");

    addTUIConfig("most", "Most", "Advanced pager", {"~/.mostrc", "~/.config/most/config"},
                 "utility");

    // 目录浏览
    addTUIConfig("tree", "Tree", "Directory listing as tree",
                 {"~/.treerc", "~/.config/tree/config"}, "utility");
}

void TUIConfigManager::initializeMailReaders() {
    // 终端邮件客户端
    addTUIConfig("neomutt", "NeoMutt", "Command-line mail reader",
                 {"~/.neomuttrc", "~/.config/neomutt"}, "utility");

    addTUIConfig("mutt", "Mutt", "Command-line mail reader", {"~/.muttrc", "~/.config/mutt"},
                 "utility");

    addTUIConfig("alpine", "Alpine", "Program for Internet News and Email",
                 {"~/.pinerc", "~/.alpine"}, "utility");
}

void TUIConfigManager::initializeWebBrowsers() {
    // 终端浏览器
    addTUIConfig("lynx", "Lynx", "Text-based web browser", {"~/.lynxrc", "~/.config/lynx"},
                 "utility");

    addTUIConfig("w3m", "W3M", "Text-based web browser", {"~/.w3m/config", "~/.config/w3m"},
                 "utility");

    addTUIConfig("links", "Links", "Text-based web browser",
                 {"~/.links/links.cfg", "~/.config/links"}, "utility");

    addTUIConfig("elinks", "ELinks", "Enhanced Links text-based browser",
                 {"~/.elinks", "~/.config/elinks"}, "utility");
}

void TUIConfigManager::initializePIMTools() {
    // 日历/联系人管理
    addTUIConfig("khal", "Khal", "CalDAV command-line client",
                 {"~/.config/khal", "~/.config/khard"}, "utility");

    addTUIConfig("vdirsyncer", "Vdirsyncer", "Synchronize calendars and contacts",
                 {"~/.config/vdirsyncer"}, "utility");

    addTUIConfig("newsboat", "Newsboat", "RSS/Atom feed reader for the text console",
                 {"~/.newsboat"}, "utility");
}

void TUIConfigManager::initializeBuildSystems() {
    // 构建系统
    addTUIConfig("make", "Make", "Build automation tool", {"~/.makerc", "~/.config/make/config"},
                 "utility");

    addTUIConfig("cmake", "CMake", "Cross-platform build system",
                 {"~/.cmake/config", "~/.config/cmake/config.txt"}, "utility");

    addTUIConfig("ninja", "Ninja", "Small build system with a focus on speed",
                 {"~/.ninja_config", "~/.config/ninja/config"}, "utility");

    addTUIConfig("meson", "Meson", "Fast and user-friendly build system",
                 {"~/.config/meson/config.ini"}, "utility");

    addTUIConfig("bazel", "Bazel", "Build and test tool",
                 {"~/.bazelrc", "~/.config/bazel/.bazelrc"}, "utility");

    addTUIConfig("gradle", "Gradle", "Build automation tool for JVM languages",
                 {"~/.gradle/gradle.properties", "~/.config/gradle/gradle.properties"}, "utility");

    addTUIConfig("maven", "Maven", "Build automation tool for Java",
                 {"~/.m2/settings.xml", "~/.config/maven/settings.xml"}, "utility");
}

void TUIConfigManager::initializeLanguageTools() {
    // 代码质量工具
    addTUIConfig("golangci-lint", "GolangCI-Lint", "Go linter aggregator",
                 {"~/.golangci.yml", "~/.config/golangci/config.yml"}, "utility");

    addTUIConfig("rustfmt", "Rustfmt", "Rust code formatter", {"~/.config/rustfmt/rustfmt.toml"},
                 "utility");

    addTUIConfig("prettier", "Prettier", "Code formatter",
                 {"~/.prettierrc", "~/.config/prettier/config.json"}, "utility");

    addTUIConfig("eslint", "ESLint", "JavaScript linter",
                 {"~/.eslintrc", "~/.config/eslint/config.js"}, "utility");
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
