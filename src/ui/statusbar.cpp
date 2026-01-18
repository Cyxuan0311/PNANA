#include "ui/statusbar.h"
#include "ui/icons.h"
#include <array>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <memory>
#include <sstream>

using namespace ftxui;
// 不使用 using namespace icons，避免 FILE 名称冲突

// 自定义删除器用于 FILE*，避免函数指针属性警告
struct FileDeleter {
    void operator()(FILE* file) const {
        if (file) {
            pclose(file);
        }
    }
};

namespace pnana {
namespace ui {

Statusbar::Statusbar(Theme& theme) : theme_(theme) {}

Element Statusbar::render(const std::string& filename, bool is_modified, bool is_readonly,
                          size_t current_line, size_t current_col, size_t total_lines,
                          const std::string& encoding, const std::string& line_ending,
                          const std::string& file_type, const std::string& message,
                          const std::string& region_name, bool syntax_highlighting,
                          bool has_selection, size_t selection_length,
                          const std::string& git_branch, int git_uncommitted_count,
                          const std::string& ssh_host, const std::string& ssh_user) {
    auto& colors = theme_.getColors();

    // Neovim 风格状态栏：左侧、中间、右侧三部分

    // ========== 左侧部分 ==========
    Elements left_elements;

    // 区域指示器（类似 neovim 的模式指示器）
    if (!region_name.empty()) {
        Color region_bg = colors.keyword;
        Color region_fg = colors.background;

        // 根据区域类型设置不同颜色
        if (region_name.find("Terminal") != std::string::npos) {
            region_bg = Color::Cyan;
        } else if (region_name.find("File Browser") != std::string::npos) {
            region_bg = Color::Blue;
        } else if (region_name.find("Tab Bar") != std::string::npos) {
            region_bg = Color::Yellow;
        } else if (region_name.find("Code Editor") != std::string::npos) {
            region_bg = Color::Green;
        }

        // Neovim 风格：区域名称（简短）
        std::string short_name = region_name;
        if (region_name == "Code Editor")
            short_name = "EDIT";
        else if (region_name == "File Browser")
            short_name = "FILES";
        else if (region_name == "Tab Bar")
            short_name = "TABS";
        else if (region_name == "Terminal")
            short_name = "TERM";

        left_elements.push_back(text(" " + short_name + " ") | bgcolor(region_bg) |
                                color(region_fg) | bold);
        // 分隔符（Neovim 风格）
        left_elements.push_back(text(" ") | bgcolor(colors.statusbar_bg) | color(region_bg));
    }

    // 文件 type icon and filename
    std::string file_display = filename.empty() ? "[Untitled]" : filename;
    std::string file_icon = getFileTypeIcon(file_type);
    if (!file_icon.empty()) {
        left_elements.push_back(text(file_icon + " ") | color(colors.keyword));
    }
    left_elements.push_back(text(file_display) | bold);

    // 修改标记（红色圆点，Neovim 风格）
    if (is_modified) {
        left_elements.push_back(text(" ●") | color(colors.error) | bold);
    }

    // 只读标记（紧凑）
    if (is_readonly) {
        left_elements.push_back(text(" [RO]") | color(colors.comment) | dim);
    }

    // 选择状态（紧凑显示）
    if (has_selection) {
        std::ostringstream oss;
        oss << " [" << selection_length << "]";
        left_elements.push_back(text(oss.str()) | color(colors.warning) | dim);
    }

    // Git信息（分支和未提交文件数）
    if (!git_branch.empty()) {
        left_elements.push_back(text(" │ ") | color(colors.comment) | dim);
        left_elements.push_back(text(icons::GIT_BRANCH) | color(colors.keyword));
        left_elements.push_back(text(" " + git_branch) | color(colors.string) | bold);

        if (git_uncommitted_count > 0) {
            std::ostringstream git_oss;
            git_oss << " " << git_uncommitted_count;
            left_elements.push_back(text(git_oss.str()) | color(colors.warning) | bold);
        }
    }

    // SSH连接状态
    if (!ssh_host.empty() && !ssh_user.empty()) {
        left_elements.push_back(text(" │ ") | color(colors.comment) | dim);
        // 使用终端图标表示SSH连接
        left_elements.push_back(text(icons::TERMINAL) | color(colors.success));
        left_elements.push_back(text(" " + ssh_user + "@" + ssh_host) | color(colors.function) |
                                bold);
    }

    // ========== 中间部分 ==========
    Elements center_elements;

    // 状态消息（如果有，居中显示）
    if (!message.empty()) {
        // 移除图标前缀（如果有）
        std::string clean_message = message;
        // 可以在这里清理消息格式
        center_elements.push_back(text(" " + clean_message) | color(colors.foreground) | dim);
    }

    // ========== 右侧部分 ==========
    Elements right_elements;

    // 语法高亮状态（小图标，如果有图标）
    std::string highlight_icon = icons::HIGHLIGHT;
    if (!highlight_icon.empty()) {
        if (syntax_highlighting) {
            right_elements.push_back(text(highlight_icon) | color(colors.success));
        } else {
            right_elements.push_back(text(highlight_icon) | color(colors.comment) | dim);
        }
        right_elements.push_back(text(" ") | color(colors.comment));
    }

    // 编码（紧凑显示）
    right_elements.push_back(text(encoding) | color(colors.comment) | dim);

    // 分隔符（Neovim 风格：使用竖线）
    right_elements.push_back(text(" │ ") | color(colors.comment) | dim);

    // 行尾类型（紧凑显示）
    right_elements.push_back(text(line_ending) | color(colors.comment) | dim);

    // 分隔符
    right_elements.push_back(text(" │ ") | color(colors.comment) | dim);

    // 文件类型（如果有且不是text）
    if (!file_type.empty() && file_type != "text") {
        right_elements.push_back(text(file_type) | color(colors.comment) | dim);
        right_elements.push_back(text(" │ ") | color(colors.comment) | dim);
    }

    // 位置信息（Ln,Col 格式，类似 neovim）
    std::ostringstream pos_oss;
    pos_oss << (current_line + 1) << "," << (current_col + 1);
    right_elements.push_back(text(pos_oss.str()) | color(colors.foreground) | bold);

    // 分隔符
    right_elements.push_back(text(" │ ") | color(colors.comment) | dim);

    // 进度百分比（突出显示）
    std::string progress = formatProgress(current_line, total_lines);
    Color progress_color = colors.comment;
    if (current_line >= total_lines - 1) {
        progress_color = colors.success;
    } else if (current_line == 0) {
        progress_color = colors.keyword;
    }
    right_elements.push_back(text(progress) | color(progress_color) | bold);

    // 分隔符
    right_elements.push_back(text(" │ ") | color(colors.comment) | dim);

    // 总行数（Ln 格式，紧凑，类似 neovim）
    std::ostringstream total_oss;
    total_oss << total_lines << "L";
    right_elements.push_back(text(total_oss.str()) | color(colors.comment) | dim);

    // 组合所有元素（Neovim 风格布局）
    return hbox({hbox(left_elements) | flex_grow, hbox(center_elements) | flex,
                 hbox(right_elements)}) |
           bgcolor(colors.statusbar_bg) | color(colors.statusbar_fg);
}

std::string Statusbar::getFileTypeIcon(const std::string& file_type) {
    namespace icons = pnana::ui::icons;
    if (file_type == "cpp" || file_type == "c")
        return icons::CPP;
    if (file_type == "python")
        return icons::PYTHON;
    if (file_type == "javascript" || file_type == "typescript")
        return icons::JAVASCRIPT;
    if (file_type == "java")
        return icons::JAVA;
    if (file_type == "go")
        return icons::GO;
    if (file_type == "rust")
        return icons::RUST;
    if (file_type == "markdown")
        return icons::MARKDOWN;
    if (file_type == "json")
        return icons::JSON;
    if (file_type == "html")
        return icons::HTML;
    if (file_type == "css")
        return icons::CSS;
    if (file_type == "shell")
        return icons::SHELL;
    if (file_type == "lua")
        return icons::LUA;
    if (file_type == "ruby")
        return icons::RUBY;
    if (file_type == "php")
        return icons::PHP;
    if (file_type == "xml")
        return icons::XML;
    if (file_type == "yaml" || file_type == "yml")
        return icons::YAML;
    if (file_type == "sql")
        return icons::SQL;
    if (file_type == "swift")
        return icons::SWIFT;
    if (file_type == "kotlin")
        return icons::KOTLIN;
    if (file_type == "scala")
        return icons::SCALA;
    if (file_type == "r")
        return icons::R;
    if (file_type == "perl")
        return icons::PERL;
    if (file_type == "haskell")
        return icons::HASKELL;
    if (file_type == "tcl")
        return icons::TCL;
    if (file_type == "fortran")
        return icons::FORTRAN;
    if (file_type == "vim")
        return icons::VIM;
    if (file_type == "powershell")
        return icons::POWERSHELL;
    if (file_type == "dockerfile")
        return icons::DOCKER;
    if (file_type == "makefile")
        return icons::MAKEFILE;
    if (file_type == "cmake")
        return icons::CMAKE;

    // 新增语言图标映射
    if (file_type == "elixir")
        return icons::ELIXIR;
    if (file_type == "clojure")
        return icons::CLOJURE;
    if (file_type == "erlang")
        return icons::ERLANG;
    if (file_type == "julia")
        return icons::JULIA;
    if (file_type == "dart")
        return icons::DART;
    if (file_type == "nim")
        return icons::NIM;
    if (file_type == "crystal")
        return icons::CRYSTAL;
    if (file_type == "zig")
        return icons::ZIG;
    if (file_type == "ocaml")
        return icons::OCAML;
    if (file_type == "coq")
        return icons::COQ;
    if (file_type == "agda")
        return icons::AGDA;
    if (file_type == "idris")
        return icons::IDRIS;
    if (file_type == "purescript")
        return icons::PURESCRIPT;
    if (file_type == "reason")
        return icons::REASON;
    if (file_type == "sml")
        return icons::SML;
    if (file_type == "groovy")
        return icons::GROOVY;
    if (file_type == "coffeescript")
        return icons::COFFEESCRIPT;
    if (file_type == "pug")
        return icons::PUG;
    if (file_type == "stylus")
        return icons::STYLUS;
    if (file_type == "sass")
        return icons::SASS;
    if (file_type == "less")
        return icons::LESS;
    if (file_type == "postcss")
        return icons::POSTCSS;
    if (file_type == "graphql")
        return icons::GRAPHQL;
    if (file_type == "vue")
        return icons::VUE;
    if (file_type == "svelte")
        return icons::SVELTE;
    if (file_type == "fsharp")
        return icons::F_SHARP;
    if (file_type == "csharp")
        return icons::CSHARP;
    if (file_type == "vb")
        return icons::VB;
    if (file_type == "assembly")
        return icons::ASSEMBLY;
    if (file_type == "webassembly")
        return icons::WEBASSEMBLY;
    if (file_type == "verilog")
        return icons::VERILOG;
    if (file_type == "vhdl")
        return icons::VHDL;
    if (file_type == "matlab")
        return icons::MATLAB;
    if (file_type == "octave")
        return icons::OCTAVE;
    if (file_type == "racket")
        return icons::RACKET;
    if (file_type == "scheme")
        return icons::SCHEME;
    if (file_type == "commonlisp")
        return icons::COMMON_LISP;
    if (file_type == "emacslisp")
        return icons::EMACS_LISP;
    if (file_type == "prolog")
        return icons::PROLOG;
    if (file_type == "mercury")
        return icons::MERCURY;
    if (file_type == "alloy")
        return icons::ALLOY;
    if (file_type == "dafny")
        return icons::DAFNY;
    if (file_type == "lean")
        return icons::LEAN;
    if (file_type == "ballerina")
        return icons::BALLERINA;
    if (file_type == "cadence")
        return icons::CADENCE;
    if (file_type == "clarity")
        return icons::CLARITY;
    if (file_type == "solidity")
        return icons::SOLIDITY;
    if (file_type == "vyper")
        return icons::VYPER;
    if (file_type == "carbon")
        return icons::CARBON;
    if (file_type == "vala")
        return icons::VALA;
    if (file_type == "genie")
        return icons::GENIE;
    if (file_type == "dlang")
        return icons::D;
    if (file_type == "pony")
        return icons::PONY;
    if (file_type == "vlang")
        return icons::V_LANG;
    if (file_type == "odin")
        return icons::ODIN;
    if (file_type == "jai")
        return icons::JAI;
    if (file_type == "nelua")
        return icons::NELUA;
    if (file_type == "wren")
        return icons::WREN;
    if (file_type == "moonscript")
        return icons::MOONSCRIPT;
    if (file_type == "fantom")
        return icons::FANTOM;
    if (file_type == "smalltalk")
        return icons::SMALLTALK;
    if (file_type == "apl")
        return icons::APL;
    if (file_type == "jlang")
        return icons::J;
    if (file_type == "klang")
        return icons::K;
    if (file_type == "qlang")
        return icons::Q;

    // 更多脚本语言和特殊语言映射
    if (file_type == "bash")
        return icons::BASH;
    if (file_type == "zsh")
        return icons::ZSH;
    if (file_type == "fish")
        return icons::FISH;
    if (file_type == "applescript")
        return icons::APPLESCRIPT;
    if (file_type == "visualbasic")
        return icons::VISUAL_BASIC;
    if (file_type == "delphi")
        return icons::DELPHI;
    if (file_type == "pascal")
        return icons::PASCAL;
    if (file_type == "ada")
        return icons::ADA;
    if (file_type == "cobol")
        return icons::COBOL;
    if (file_type == "forth")
        return icons::FORTH;
    if (file_type == "lisp")
        return icons::LISP;
    if (file_type == "batch")
        return icons::BATCH;
    if (file_type == "cmd")
        return icons::WINDOWS_CMD;
    if (file_type == "clojure")
        return icons::CLOJURE;
    if (file_type == "erlang")
        return icons::ERLANG;
    if (file_type == "julia")
        return icons::JULIA;
    if (file_type == "dart")
        return icons::DART;
    if (file_type == "nim")
        return icons::NIM;
    if (file_type == "crystal")
        return icons::CRYSTAL;
    if (file_type == "zig")
        return icons::ZIG;
    if (file_type == "ocaml")
        return icons::OCAML;
    if (file_type == "coq")
        return icons::COQ;
    if (file_type == "agda")
        return icons::AGDA;
    if (file_type == "idris")
        return icons::IDRIS;
    if (file_type == "purescript")
        return icons::PURESCRIPT;
    if (file_type == "reason")
        return icons::REASON;
    if (file_type == "sml")
        return icons::SML;
    if (file_type == "groovy")
        return icons::GROOVY;
    if (file_type == "coffeescript")
        return icons::COFFEESCRIPT;
    if (file_type == "pug")
        return icons::PUG;
    if (file_type == "stylus")
        return icons::STYLUS;
    if (file_type == "sass")
        return icons::SASS;
    if (file_type == "less")
        return icons::LESS;
    if (file_type == "postcss")
        return icons::POSTCSS;
    if (file_type == "graphql")
        return icons::GRAPHQL;
    if (file_type == "vue")
        return icons::VUE;
    if (file_type == "svelte")
        return icons::SVELTE;
    if (file_type == "fsharp")
        return icons::F_SHARP;
    if (file_type == "csharp")
        return icons::CSHARP;
    if (file_type == "vb")
        return icons::VB;
    if (file_type == "assembly")
        return icons::ASSEMBLY;
    if (file_type == "webassembly")
        return icons::WEBASSEMBLY;
    if (file_type == "verilog")
        return icons::VERILOG;
    if (file_type == "vhdl")
        return icons::VHDL;
    if (file_type == "matlab")
        return icons::MATLAB;
    if (file_type == "octave")
        return icons::OCTAVE;
    if (file_type == "racket")
        return icons::RACKET;
    if (file_type == "scheme")
        return icons::SCHEME;
    if (file_type == "commonlisp")
        return icons::COMMON_LISP;
    if (file_type == "emacslisp")
        return icons::EMACS_LISP;
    if (file_type == "prolog")
        return icons::PROLOG;
    if (file_type == "mercury")
        return icons::MERCURY;
    if (file_type == "alloy")
        return icons::ALLOY;
    if (file_type == "dafny")
        return icons::DAFNY;
    if (file_type == "lean")
        return icons::LEAN;
    if (file_type == "ballerina")
        return icons::BALLERINA;
    if (file_type == "cadence")
        return icons::CADENCE;
    if (file_type == "clarity")
        return icons::CLARITY;
    if (file_type == "solidity")
        return icons::SOLIDITY;
    if (file_type == "vyper")
        return icons::VYPER;
    if (file_type == "carbon")
        return icons::CARBON;
    if (file_type == "vala")
        return icons::VALA;
    if (file_type == "genie")
        return icons::GENIE;
    if (file_type == "dlang")
        return icons::D;
    if (file_type == "pony")
        return icons::PONY;
    if (file_type == "vlang")
        return icons::V_LANG;
    if (file_type == "odin")
        return icons::ODIN;
    if (file_type == "jai")
        return icons::JAI;
    if (file_type == "nelua")
        return icons::NELUA;
    if (file_type == "wren")
        return icons::WREN;
    if (file_type == "moonscript")
        return icons::MOONSCRIPT;
    if (file_type == "fantom")
        return icons::FANTOM;
    if (file_type == "smalltalk")
        return icons::SMALLTALK;
    if (file_type == "apl")
        return icons::APL;
    if (file_type == "jlang")
        return icons::J;
    if (file_type == "klang")
        return icons::K;
    if (file_type == "qlang")
        return icons::Q;

    // 更多脚本语言和特殊语言映射
    if (file_type == "bash")
        return icons::BASH;
    if (file_type == "zsh")
        return icons::ZSH;
    if (file_type == "fish")
        return icons::FISH;
    if (file_type == "applescript")
        return icons::APPLESCRIPT;
    if (file_type == "visualbasic")
        return icons::VISUAL_BASIC;
    if (file_type == "delphi")
        return icons::DELPHI;
    if (file_type == "pascal")
        return icons::PASCAL;
    if (file_type == "ada")
        return icons::ADA;
    if (file_type == "cobol")
        return icons::COBOL;
    if (file_type == "forth")
        return icons::FORTH;
    if (file_type == "lisp")
        return icons::LISP;
    if (file_type == "batch")
        return icons::BATCH;
    if (file_type == "cmd")
        return icons::WINDOWS_CMD;

    return icons::FILE; // 使用命名空间别名避免冲突
}

std::string Statusbar::formatPosition(size_t line, size_t col) {
    std::ostringstream oss;
    oss << "Ln " << (line + 1) << ", Col " << (col + 1);
    return oss.str();
}

std::string Statusbar::formatProgress(size_t current, size_t total) {
    if (total == 0)
        return "0%";

    int percent;
    if (current >= total - 1) {
        percent = 100;
    } else {
        percent = static_cast<int>((current * 100) / total);
    }

    std::ostringstream oss;
    oss << percent << "%";
    return oss.str();
}

std::string Statusbar::getRegionIcon(const std::string& region_name) {
    namespace icons = pnana::ui::icons;
    if (region_name.find("Code") != std::string::npos ||
        region_name.find("代码") != std::string::npos)
        return icons::CODE;
    if (region_name.find("Tab") != std::string::npos ||
        region_name.find("标签") != std::string::npos)
        return icons::TAB;
    if (region_name.find("File Browser") != std::string::npos ||
        region_name.find("浏览器") != std::string::npos)
        return icons::FOLDER;
    if (region_name.find("Terminal") != std::string::npos ||
        region_name.find("终端") != std::string::npos)
        return "";
    if (region_name.find("Help") != std::string::npos ||
        region_name.find("帮助") != std::string::npos)
        return icons::HELP;
    return icons::INFO;
}

Element Statusbar::createIndicator(const std::string& icon, const std::string& label,
                                   Color fg_color, Color bg_color) {
    Elements elements;
    if (!icon.empty()) {
        elements.push_back(text(" " + icon) | color(fg_color) | bgcolor(bg_color));
    }
    if (!label.empty()) {
        elements.push_back(text(" " + label + " ") | color(fg_color) | bgcolor(bg_color));
    }
    return hbox(elements);
}

// Git相关方法实现
std::tuple<std::string, int> Statusbar::getGitInfo() {
    std::string branch = getGitBranch();
    int uncommitted_count = getGitUncommittedCount();
    return std::make_tuple(branch, uncommitted_count);
}

std::string Statusbar::getGitBranch() {
    // 执行 git branch --show-current 命令获取当前分支
    std::array<char, 128> buffer;
    std::string result;

    // 使用 popen 执行 git 命令
    std::unique_ptr<FILE, FileDeleter> pipe(popen("git branch --show-current 2>/dev/null", "r"));
    if (!pipe) {
        return "";
    }

    // 读取命令输出
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }

    // 移除末尾的换行符
    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }

    return result;
}

int Statusbar::getGitUncommittedCount() {
    // 执行 git status --porcelain 命令获取未提交的文件数
    std::array<char, 1024> buffer;
    std::string result;
    int count = 0;

    // 使用 popen 执行 git 命令
    std::unique_ptr<FILE, FileDeleter> pipe(popen("git status --porcelain 2>/dev/null", "r"));
    if (!pipe) {
        return 0;
    }

    // 读取命令输出并计数
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        // 每行代表一个未提交的文件
        if (strlen(buffer.data()) > 0) {
            count++;
        }
    }

    return count;
}

} // namespace ui
} // namespace pnana
