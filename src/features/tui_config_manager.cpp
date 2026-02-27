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
    addTUIConfig("pnana", "pnana", "Modern terminal text editor with native shortcuts",
                 {"~/.config/pnana/config.json"}, "editor");

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

    // 更多编辑器配置
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

    // 更多文件管理器配置
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

    // 更多版本控制配置
    addTUIConfig("fossil", "Fossil", "Distributed version control system",
                 {"~/.fossil", "~/.config/fossil"}, "version_control");

    addTUIConfig("pijul", "Pijul", "A patch-based distributed version control system",
                 {"~/.config/pijul", "~/.local/share/pijul"}, "version_control");

    addTUIConfig("darcs", "Darcs", "Distributed, interactive, smart revision control system",
                 {"~/.darcs", "~/.config/darcs"}, "version_control");

    // 更多系统工具配置
    addTUIConfig("iftop", "Iftop", "Display bandwidth usage on an interface",
                 {"~/.iftoprc", "~/.config/iftop/config"}, "system");

    addTUIConfig("nload", "Nload", "Realtime console network usage monitor",
                 {"~/.nload", "~/.config/nload"}, "system");

    addTUIConfig("powertop", "Powertop", "Tool to diagnose issues with power consumption",
                 {"~/.config/powertop"}, "system");

    addTUIConfig("nvtop", "Nvtop", "GPUs process monitoring for AMD, Intel and NVIDIA",
                 {"~/.config/nvtop"}, "system");

    addTUIConfig("s-tui", "S-TUI", "Terminal-based CPU stress and monitoring utility",
                 {"~/.config/s-tui"}, "system");

    addTUIConfig("radeontop", "Radeontop", "Radeon graphics card utilization",
                 {"~/.config/radeontop"}, "system");

    // 开发工具配置
    addTUIConfig("docker", "Docker", "Container platform",
                 {"~/.docker/config.json", "~/.config/docker/config.json"}, "utility");

    addTUIConfig("kubectl", "Kubectl", "Kubernetes command-line tool",
                 {"~/.kube/config", "~/.config/kube"}, "utility");

    addTUIConfig("helm", "Helm", "Kubernetes package manager", {"~/.config/helm", "~/.helm"},
                 "utility");

    addTUIConfig("terraform", "Terraform", "Infrastructure as code software tool",
                 {"~/.terraformrc", "~/.config/terraform"}, "utility");

    addTUIConfig("ansible", "Ansible", "Configuration management tool",
                 {"~/.ansible.cfg", "~/.config/ansible"}, "utility");

    // 文本处理工具配置
    addTUIConfig("jq", "JQ", "Command-line JSON processor", {"~/.jq", "~/.config/jq"}, "utility");

    addTUIConfig("yq", "YQ", "Command-line YAML processor", {"~/.config/yq"}, "utility");

    addTUIConfig("pandoc", "Pandoc", "Universal markup converter",
                 {"~/.pandoc", "~/.config/pandoc"}, "utility");

    addTUIConfig("pandoc-citeproc", "Pandoc Citeproc", "Pandoc citation processing library",
                 {"~/.pandoc"}, "utility");

    // 网络工具配置
    addTUIConfig("curl", "cURL", "Command line tool for transferring data",
                 {"~/.curlrc", "~/.config/curl"}, "utility");

    addTUIConfig("wget", "Wget", "Free utility for non-interactive download",
                 {"~/.wgetrc", "~/.config/wget"}, "utility");

    addTUIConfig("httpie", "HTTPie", "User-friendly command-line HTTP client",
                 {"~/.config/httpie", "~/.httpie"}, "utility");

    addTUIConfig("aria2", "Aria2",
                 "Lightweight multi-protocol & multi-source command-line download utility",
                 {"~/.aria2/aria2.conf", "~/.config/aria2"}, "utility");

    // 数据库客户端配置
    addTUIConfig("sqlite3", "SQLite3", "Command-line interface for SQLite",
                 {"~/.sqliterc", "~/.config/sqlite3"}, "utility");

    addTUIConfig("mysql", "MySQL Client", "MySQL command-line client",
                 {"~/.my.cnf", "~/.config/mysql"}, "utility");

    addTUIConfig("psql", "PostgreSQL Client", "PostgreSQL command-line client",
                 {"~/.psqlrc", "~/.config/psql"}, "utility");

    addTUIConfig("mongosh", "MongoDB Shell", "MongoDB command-line shell",
                 {"~/.mongoshrc.js", "~/.config/mongosh"}, "utility");

    addTUIConfig("redis-cli", "Redis CLI", "Redis command-line interface", {"~/.rediscli"},
                 "utility");

    // 编程语言包管理器配置
    addTUIConfig("pip", "Pip", "Python package installer",
                 {"~/.config/pip/pip.conf", "~/.pip/pip.conf"}, "utility");

    addTUIConfig("npm", "NPM", "Node Package Manager", {"~/.npmrc", "~/.config/npm"}, "utility");

    addTUIConfig("yarn", "Yarn", "Fast, reliable, and secure dependency management",
                 {"~/.yarnrc", "~/.config/yarn"}, "utility");

    addTUIConfig("pnpm", "PNPM", "Fast, disk space efficient package manager",
                 {"~/.config/pnpm", "~/.pnpmrc"}, "utility");

    addTUIConfig("cargo", "Cargo", "Rust package manager",
                 {"~/.cargo/config", "~/.cargo/config.toml"}, "utility");

    addTUIConfig("go", "Go", "Go programming language tools", {"~/.config/go/env", "~/.go"},
                 "utility");

    addTUIConfig("rustup", "Rustup", "Rust toolchain installer", {"~/.rustup/settings.toml"},
                 "utility");

    // 更多实用工具配置
    addTUIConfig("tmuxp", "Tmuxp", "Tmux session manager", {"~/.tmuxp", "~/.config/tmuxp"},
                 "utility");

    addTUIConfig("byobu", "Byobu", "Text-based window manager and terminal multiplexer",
                 {"~/.byobu"}, "multiplexer");

    addTUIConfig("abduco", "Abduco", "Session management in a clean and simple way",
                 {"~/.config/abduco"}, "multiplexer");

    addTUIConfig("dtach", "Dtach", "Program that emulates the detach feature of screen",
                 {"~/.config/dtach"}, "utility");

    addTUIConfig("tig", "Tig", "Text-mode interface for Git", {"~/.tigrc", "~/.config/tig"},
                 "utility");

    addTUIConfig("gitui", "GitUI", "Blazing fast terminal UI for Git", {"~/.config/gitui"},
                 "utility");

    addTUIConfig("lazygit", "Lazygit", "Simple terminal UI for git commands", {"~/.config/lazygit"},
                 "utility");

    addTUIConfig("gh", "GitHub CLI", "GitHub command-line tool", {"~/.config/gh"}, "utility");

    addTUIConfig("glab", "GitLab CLI", "GitLab command-line tool", {"~/.config/glab"}, "utility");

    addTUIConfig("task", "Taskwarrior", "Command-line task management",
                 {"~/.taskrc", "~/.config/task"}, "utility");

    addTUIConfig("timewarrior", "Timewarrior", "Command-line time tracking", {"~/.timewarrior"},
                 "utility");

    addTUIConfig("khal", "Khal", "CalDAV command-line client",
                 {"~/.config/khal", "~/.config/khard"}, "utility");

    addTUIConfig("vdirsyncer", "Vdirsyncer", "Synchronize calendars and contacts",
                 {"~/.config/vdirsyncer"}, "utility");

    addTUIConfig("newsboat", "Newsboat", "RSS/Atom feed reader for the text console",
                 {"~/.newsboat"}, "utility");

    addTUIConfig("neomutt", "NeoMutt", "Command-line mail reader",
                 {"~/.neomuttrc", "~/.config/neomutt"}, "utility");

    addTUIConfig("mutt", "Mutt", "Command-line mail reader", {"~/.muttrc", "~/.config/mutt"},
                 "utility");

    addTUIConfig("alpine", "Alpine", "Program for Internet News and Email",
                 {"~/.pinerc", "~/.alpine"}, "utility");

    addTUIConfig("lynx", "Lynx", "Text-based web browser", {"~/.lynxrc", "~/.config/lynx"},
                 "utility");

    addTUIConfig("w3m", "W3M", "Text-based web browser", {"~/.w3m/config", "~/.config/w3m"},
                 "utility");

    addTUIConfig("links", "Links", "Text-based web browser",
                 {"~/.links/links.cfg", "~/.config/links"}, "utility");

    addTUIConfig("elinks", "ELinks", "Advanced text-based web browser",
                 {"~/.elinks", "~/.config/elinks"}, "utility");

    // 更多终端模拟器配置
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

    // 更多编辑器配置
    addTUIConfig("vis", "Vis", "Modern, legacy-free Vim-like editor",
                 {"~/.config/vis", "~/.config/vis/visrc.lua"}, "editor");

    addTUIConfig("amp", "Amp", "A complete text editor for your terminal", {"~/.config/amp"},
                 "editor");

    addTUIConfig("ne", "NE", "Nice Editor, a free text editor", {"~/.ne"}, "editor");

    addTUIConfig("jed", "Jed", "Programmer's editor", {"~/.jedrc"}, "editor");

    addTUIConfig("jed", "JED", "Programmer's editor", {"~/.jedrc"}, "editor");

    addTUIConfig("le", "LE", "Text editor with block and binary operations", {"~/.lerec"},
                 "editor");

    // 更多文件管理器配置
    addTUIConfig("cfm", "CFM", "Simple file manager with no dependencies", {"~/.cfm"},
                 "file_manager");

    addTUIConfig("noice", "Noice", "Small file browser", {"~/.config/noice"}, "file_manager");

    addTUIConfig("fff", "FFF", "Fast file manager written in bash", {"~/.config/fff"},
                 "file_manager");

    addTUIConfig("clifm", "CliFM", "Command Line Interface File Manager", {"~/.config/clifm"},
                 "file_manager");

    // 更多多路复用器配置
    addTUIConfig("tmate", "Tmate", "Instant terminal sharing", {"~/.tmate.conf"}, "multiplexer");

    // 更多Shell配置
    addTUIConfig("dash", "Dash", "POSIX-compliant shell", {"~/.dashrc"}, "shell");

    addTUIConfig("ash", "Ash", "Almquist shell", {"~/.ashrc"}, "shell");

    addTUIConfig("ksh", "Ksh", "Korn shell", {"~/.kshrc"}, "shell");

    addTUIConfig("tcsh", "Tcsh", "TENEX C shell", {"~/.tcshrc", "~/.cshrc"}, "shell");

    addTUIConfig("ion", "Ion", "Modern shell with a syntax similar to Rust", {"~/.config/ion"},
                 "shell");

    addTUIConfig("murex", "Murex", "Shell designed for DevOps", {"~/.murex_profile"}, "shell");

    addTUIConfig("oil", "Oil", "Shell with structured data", {"~/.config/oil"}, "shell");

    // 更多系统工具配置
    addTUIConfig("atop", "Atop", "Advanced System & Process Monitor",
                 {"~/.atoprc", "~/.config/atop"}, "system");

    addTUIConfig("slurm", "Slurm", "Simple Linux Utility for Resource Management",
                 {"~/.config/slurm"}, "system");

    addTUIConfig("conky", "Conky", "Light-weight system monitor", {"~/.conkyrc", "~/.config/conky"},
                 "system");

    addTUIConfig("dstat", "Dstat", "Versatile resource statistics tool", {"~/.dstatrc"}, "system");

    addTUIConfig("collectl", "Collectl", "Collects data that describes the current system status",
                 {"~/.collectl"}, "system");

    // 更多工具配置
    addTUIConfig("ranger", "Ranger", "Console file manager with VI key bindings",
                 {"~/.config/ranger"}, "file_manager");

    // 'nnn' already registered earlier with more comprehensive paths; skip duplicate here.
    addTUIConfig("vifm", "Vifm", "VI File Manager", {"~/.config/vifm"}, "file_manager");

    addTUIConfig("htop", "Htop", "Interactive process viewer", {"~/.config/htop"}, "system");

    addTUIConfig("btop", "Btop++", "Resource monitor that shows usage and stats",
                 {"~/.config/btop"}, "system");

    addTUIConfig("glances", "Glances", "Curses-based monitoring tool", {"~/.config/glances"},
                 "system");

    addTUIConfig("gotop", "Gotop", "Terminal based graphical activity monitor", {"~/.config/gotop"},
                 "system");

    addTUIConfig("bashtop", "Bashtop", "Resource monitor", {"~/.config/bashtop"}, "system");

    addTUIConfig("fzf", "FZF", "General-purpose command-line fuzzy finder", {"~/.config/fzf"},
                 "utility");

    addTUIConfig("ripgrep", "Ripgrep", "Recursively searches directories for a regex pattern",
                 {"~/.config/ripgrep"}, "utility");

    addTUIConfig("fd", "FD", "Simple, fast and user-friendly alternative to find", {"~/.config/fd"},
                 "utility");

    addTUIConfig("bat", "Bat", "A cat clone with syntax highlighting", {"~/.config/bat"},
                 "utility");

    addTUIConfig("exa", "Exa", "Modern replacement for ls", {"~/.config/exa"}, "utility");

    addTUIConfig("delta", "Delta", "A syntax-highlighting pager for git", {"~/.config/delta"},
                 "utility");

    addTUIConfig("tldr", "TLDR", "Simplified and community-driven man pages", {"~/.config/tldr"},
                 "utility");

    addTUIConfig("glow", "Glow", "Render markdown on the CLI", {"~/.config/glow"}, "utility");

    addTUIConfig("lazygit", "Lazygit", "Simple terminal UI for git commands", {"~/.config/lazygit"},
                 "utility");

    addTUIConfig("lazydocker", "Lazydocker", "Simple terminal UI for docker",
                 {"~/.config/lazydocker"}, "utility");

    addTUIConfig("navi", "Navi", "Interactive cheatsheet tool", {"~/.config/navi"}, "utility");

    addTUIConfig("tealdeer", "Tealdeer", "Rust implementation of tldr", {"~/.config/tealdeer"},
                 "utility");

    addTUIConfig("zoxide", "Zoxide", "Smarter cd command", {"~/.config/zoxide"}, "utility");
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
