#include "ui/file_browser_view.h"
#include "features/file_browser.h"
#include "ui/icons.h"
#include <algorithm>
#include <cctype>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

namespace pnana {
namespace ui {

FileBrowserView::FileBrowserView(Theme& theme)
    : theme_(theme), color_mapper_(theme), scroll_offset_(0) {}

// 滚动控制方法
void FileBrowserView::scrollTo(size_t index) {
    scroll_offset_ = index;
}

void FileBrowserView::scrollUp(size_t lines) {
    if (scroll_offset_ >= lines) {
        scroll_offset_ -= lines;
    } else {
        scroll_offset_ = 0;
    }
}

void FileBrowserView::scrollDown(size_t lines) {
    scroll_offset_ += lines;
}

void FileBrowserView::scrollToTop() {
    scroll_offset_ = 0;
}

void FileBrowserView::scrollToBottom() {
    // 这个方法需要在渲染时根据实际项目数量来设置
    // 这里先设置一个大的值，在渲染时会调整
    scroll_offset_ = SIZE_MAX / 2; // 很大的值
}

Element FileBrowserView::render(const features::FileBrowser& browser, int height) {
    auto& colors = theme_.getColors();

    Elements content;

    // 标题栏
    content.push_back(renderHeader(browser.getCurrentDirectory()));
    content.push_back(separator());

    // 文件列表
    const auto& flat_items = browser.getFlatItems();
    size_t total_items = flat_items.size();
    size_t selected_index = browser.getSelectedIndex();

    // 计算可用高度：总高度 - 标题(1) - 分隔符(1) - 底部分隔符(1) - 状态栏(1) = height - 4
    size_t available_height = static_cast<size_t>(height - 4);

    // 根据选中项目调整滚动位置，确保选中项目可见
    // 使用更智能的滚动策略：保持选中项目在中间偏上位置
    size_t target_scroll = scroll_offset_;

    if (selected_index < scroll_offset_) {
        // 选中项目在可见区域上方
        // 将选中项目放在可见区域的中间位置
        size_t ideal_position = available_height / 3; // 选中项目在1/3位置
        if (selected_index >= ideal_position) {
            target_scroll = selected_index - ideal_position;
        } else {
            target_scroll = 0;
        }
    } else if (selected_index >= scroll_offset_ + available_height) {
        // 选中项目在可见区域下方
        // 将选中项目放在可见区域的中间位置
        size_t ideal_position = available_height / 3; // 选中项目在1/3位置
        target_scroll = selected_index - ideal_position;
    } else {
        // 选中项目已在可见范围内，检查是否需要微调以获得更好的视觉体验
        size_t relative_pos = selected_index - scroll_offset_;
        if (relative_pos == 0 && scroll_offset_ > 0) {
            // 选中项目在顶部，稍微向上滚动以获得上下文
            target_scroll = scroll_offset_ - 1;
        } else if (relative_pos == available_height - 1 &&
                   scroll_offset_ + available_height < total_items) {
            // 选中项目在底部，稍微向下滚动以获得上下文
            target_scroll = scroll_offset_ + 1;
        }
    }

    // 确保滚动偏移量不超出有效范围
    if (target_scroll >= total_items) {
        target_scroll = (total_items > available_height) ? total_items - available_height : 0;
    }
    if (target_scroll > selected_index) {
        target_scroll = selected_index;
    }

    scroll_offset_ = target_scroll;

    // 计算实际可见范围
    size_t visible_start = scroll_offset_;
    size_t visible_end = std::min(scroll_offset_ + available_height, total_items);

    // 渲染文件列表 - 只渲染可见的项目
    Elements file_list_elements;

    for (size_t i = visible_start; i < visible_end; ++i) {
        const features::FileItem* item = flat_items[i];
        if (item) {
            file_list_elements.push_back(renderFileItem(item, i, selected_index, flat_items));
        }
    }

    // 如果文件项数少于可用高度，填充空行
    while (file_list_elements.size() < available_height) {
        file_list_elements.push_back(text(""));
    }

    // 将文件列表添加到内容中
    content.push_back(vbox(file_list_elements));

    // 底部状态栏
    content.push_back(separator());
    content.push_back(renderStatusBar(browser));

    return vbox(content) | bgcolor(colors.background);
}

Element FileBrowserView::renderHeader(const std::string& current_directory) const {
    auto& colors = theme_.getColors();

    return hbox({text(" "), text(icons::FOLDER_OPEN) | color(colors.function), text(" "),
                 text(current_directory) | bold | color(colors.foreground), filler(),
                 text(" ") | color(colors.comment)}) |
           bgcolor(colors.menubar_bg);
}

Element FileBrowserView::renderFileList(const features::FileBrowser& browser, size_t visible_start,
                                        size_t visible_count) const {
    Elements content;
    const auto& flat_items = browser.getFlatItems();
    size_t selected_index = browser.getSelectedIndex();

    // 渲染可见的文件项
    for (size_t i = visible_start; i < flat_items.size() && i < visible_start + visible_count;
         ++i) {
        const features::FileItem* item = flat_items[i];
        if (item) {
            content.push_back(renderFileItem(item, i, selected_index, flat_items));
        }
    }

    return vbox(content);
}

Element FileBrowserView::renderStatusBar(const features::FileBrowser& browser) const {
    auto& colors = theme_.getColors();

    std::string selected_path_display = "";
    if (browser.hasSelection()) {
        std::string full_path = browser.getSelectedPath();
        selected_path_display = truncateMiddle(full_path, 40);
    } else {
        selected_path_display = "No selection";
    }

    return hbox({text(" "), text(icons::LOCATION) | color(colors.keyword), text(" "),
                 text(selected_path_display) | color(colors.comment)}) |
           bgcolor(colors.menubar_bg);
}

Element FileBrowserView::renderFileItem(const features::FileItem* item, size_t index,
                                        size_t selected_index,
                                        const std::vector<features::FileItem*>& flat_items) const {
    auto& colors = theme_.getColors();

    std::string icon = getFileIcon(*item);
    Color item_color = color_mapper_.getFileColor(item->name, item->is_directory);

    // 构建树形结构连接线
    std::string tree_prefix = buildTreePrefix(item, index, flat_items);

    // 展开/折叠图标和连接线
    std::string expand_prefix = buildExpandPrefix(item, index, flat_items);
    std::string expand_icon = "";

    if (item->is_directory) {
        expand_icon = item->expanded ? "▼" : "▶";
    } else {
        expand_icon = " ";
    }

    std::string display_name = item->name;

    // 构建行元素
    Elements row_elements = {text(" "),
                             text(tree_prefix) | color(colors.comment),
                             text(expand_prefix) | color(colors.comment),
                             text(expand_icon) | color(item_color),
                             text(" "),
                             text(icon) | color(item_color),
                             text(" "),
                             text(display_name) | color(item_color)};

    auto item_text = hbox(row_elements);

    // 选中项高亮
    if (index == selected_index) {
        item_text = item_text | bgcolor(colors.selection) | bold;
    } else {
        item_text = item_text | bgcolor(colors.background);
    }

    return item_text;
}

std::string FileBrowserView::buildTreePrefix(
    const features::FileItem* item, size_t index,
    const std::vector<features::FileItem*>& flat_items) const {
    std::string prefix = "";

    for (int d = 0; d < item->depth; ++d) {
        // 检查这个深度层级是否有后续兄弟节点
        bool has_sibling = false;
        for (size_t j = index + 1; j < flat_items.size(); ++j) {
            if (flat_items[j]->depth == d) {
                has_sibling = true;
                break;
            }
            if (flat_items[j]->depth < d) {
                break;
            }
        }

        if (has_sibling) {
            prefix += "│ "; // 有后续兄弟，显示竖线
        } else {
            prefix += "  "; // 没有后续兄弟，显示空格
        }
    }

    return prefix;
}

std::string FileBrowserView::buildExpandPrefix(
    const features::FileItem* item, size_t index,
    const std::vector<features::FileItem*>& flat_items) const {
    // 检查是否有后续兄弟节点（同深度）
    bool has_sibling = false;
    for (size_t j = index + 1; j < flat_items.size(); ++j) {
        if (flat_items[j]->depth == item->depth) {
            has_sibling = true;
            break;
        }
        if (flat_items[j]->depth < item->depth) {
            break;
        }
    }

    return has_sibling ? "├─" : "└─";
}

std::string FileBrowserView::getFileIcon(const features::FileItem& item) const {
    if (item.is_directory) {
        if (item.name == "..") {
            return icons::FOLDER_UP; // 上级目录
        }
        return icons::FOLDER; // 文件夹图标
    }

    std::string ext = getFileExtension(item.name);
    std::string name_lower = item.name;
    std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);

    // 根据扩展名返回图标（使用 JetBrains Nerd Font 图标）

    // C/C++ 文件
    if (ext == "cpp" || ext == "cc" || ext == "cxx" || ext == "c++")
        return icons::CPP;
    if (ext == "h" || ext == "hpp" || ext == "hxx" || ext == "hh")
        return icons::CPP;
    if (ext == "c")
        return icons::C;

    // 脚本语言
    if (ext == "py" || ext == "pyw" || ext == "pyi")
        return icons::PYTHON;
    if (ext == "js" || ext == "jsx" || ext == "mjs")
        return icons::JAVASCRIPT;
    if (ext == "ts" || ext == "tsx")
        return icons::TYPESCRIPT;
    if (ext == "rb" || ext == "rbw")
        return icons::RUBY;
    if (ext == "php" || ext == "php3" || ext == "php4" || ext == "php5" || ext == "phtml")
        return icons::PHP;
    if (ext == "lua")
        return icons::LUA;
    if (ext == "sh" || ext == "bash" || ext == "zsh" || ext == "fish")
        return icons::SHELL;
    if (name_lower == "makefile" || name_lower == "makefile.am" || name_lower == "makefile.in")
        return icons::MAKEFILE;

    // 编译语言
    if (ext == "java" || ext == "class" || ext == "jar")
        return icons::JAVA;
    if (ext == "go")
        return icons::GO;
    if (ext == "rs")
        return icons::RUST;

    // Web 技术
    if (ext == "html" || ext == "htm" || ext == "xhtml")
        return icons::HTML;
    if (ext == "css" || ext == "scss" || ext == "sass" || ext == "less")
        return icons::CSS;

    // 数据格式
    if (ext == "json" || ext == "jsonc") {
        // 特殊 JSON 文件
        if (name_lower == "package.json")
            return icons::PACKAGE_JSON;
        if (name_lower == "package-lock.json")
            return icons::PACKAGE_LOCK;
        if (name_lower == "composer.json")
            return icons::COMPOSER;
        if (name_lower == "tsconfig.json" || name_lower == "tsconfig.base.json")
            return icons::TSCONFIG;
        if (name_lower == ".prettierrc" || name_lower == ".prettierrc.json")
            return icons::PRETTIER;
        if (name_lower == ".eslintrc" || name_lower == ".eslintrc.json" ||
            name_lower == "eslint.config.json")
            return icons::ESLINT;
        if (name_lower == ".babelrc" || name_lower == ".babelrc.json")
            return icons::BABEL;
        return icons::JSON;
    }
    if (ext == "xml" || ext == "xsd" || ext == "xsl") {
        if (name_lower == "pom.xml")
            return icons::MAVEN;
        return icons::XML;
    }
    if (ext == "yml" || ext == "yaml") {
        if (name_lower == ".travis.yml")
            return icons::TRAVIS;
        if (name_lower == "docker-compose.yml" || name_lower == "docker-compose.yaml")
            return icons::DOCKER_COMPOSE;
        if (name_lower == ".github/workflows" || name_lower.find(".github") != std::string::npos)
            return icons::GITHUB_ACTIONS;
        return icons::YAML;
    }
    if (ext == "toml") {
        if (name_lower == "cargo.toml")
            return icons::CARGO;
        if (name_lower == "pyproject.toml")
            return icons::POETRY;
        return icons::CONFIG;
    }
    // Cargo 和 Poetry lock 文件（无扩展名或不同扩展名）
    if (name_lower == "cargo.lock")
        return icons::CARGO;
    if (name_lower == "poetry.lock")
        return icons::POETRY;

    // 文档
    if (ext == "md" || ext == "markdown") {
        if (name_lower == "readme.md" || name_lower == "readme")
            return icons::README;
        if (name_lower == "changelog.md" || name_lower == "changelog")
            return icons::CHANGELOG;
        if (name_lower == "contributing.md" || name_lower == "contributing")
            return icons::CONTRIBUTING;
        return icons::MARKDOWN;
    }
    if (ext == "txt") {
        if (name_lower == "license" || name_lower == "license.txt")
            return icons::LICENSE;
        if (name_lower == "authors" || name_lower == "authors.txt")
            return icons::AUTHORS;
        if (name_lower == "todo" || name_lower == "todo.txt")
            return icons::TODO;
        return icons::FILE_TEXT;
    }
    if (ext == "log")
        return icons::LOG;
    if (ext == "pdf")
        return icons::PDF;

    // 数据库
    if (ext == "sql")
        return icons::SQL;
    if (ext == "db" || ext == "sqlite" || ext == "sqlite3" || ext == "db3")
        return icons::DATABASE;

    // 图片
    if (ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "gif" || ext == "bmp" ||
        ext == "svg" || ext == "ico" || ext == "webp") {
        return icons::IMAGE;
    }

    // 视频
    if (ext == "mp4" || ext == "avi" || ext == "mov" || ext == "wmv" || ext == "flv" ||
        ext == "mkv" || ext == "webm") {
        return icons::VIDEO;
    }

    // 音频
    if (ext == "mp3" || ext == "wav" || ext == "flac" || ext == "aac" || ext == "ogg" ||
        ext == "m4a") {
        return icons::AUDIO;
    }

    // 压缩包
    if (ext == "zip" || ext == "tar" || ext == "gz" || ext == "bz2" || ext == "xz" || ext == "7z" ||
        ext == "rar" || ext == "z") {
        return icons::ARCHIVE;
    }

    // 环境配置文件
    if (name_lower == ".env" || name_lower == ".env.local" || name_lower == ".env.development" ||
        name_lower == ".env.production" || name_lower == ".env.test" ||
        (name_lower.length() > 5 && name_lower.substr(0, 5) == ".env.")) {
        return icons::ENV;
    }

    // 配置文件
    if (ext == "conf" || ext == "config" || ext == "ini" || ext == "cfg" || ext == "properties" ||
        name_lower == ".editorconfig") {
        if (name_lower == ".editorconfig")
            return icons::EDITORCONFIG;
        return icons::CONFIG;
    }

    // Git 相关
    if (name_lower == ".gitignore" || name_lower == ".gitattributes" ||
        name_lower == ".gitmodules" || name_lower == ".gitconfig" || name_lower == ".gitkeep") {
        return icons::GITIGNORE;
    }

    // CMake
    if (ext == "cmake" || name_lower == "cmakelists.txt")
        return icons::CMAKE;

    // Docker
    if (name_lower == "dockerfile" ||
        (name_lower.length() > 11 && name_lower.substr(0, 11) == "dockerfile.") ||
        ext == "dockerignore" || name_lower == ".dockerignore") {
        return icons::DOCKER;
    }

    // 构建和依赖管理文件
    if (name_lower == "requirements.txt" || name_lower == "requirements-dev.txt" ||
        name_lower == "requirements-prod.txt" || name_lower == "setup.py" ||
        name_lower == "setup.cfg" || name_lower == "pipfile" || name_lower == "pipfile.lock") {
        return icons::PIP;
    }
    if (name_lower == "gemfile" || name_lower == "gemfile.lock") {
        if (name_lower == "gemfile.lock")
            return icons::GEMFILE_LOCK;
        return icons::GEMFILE;
    }
    if (name_lower == "go.mod")
        return icons::GO_MOD;
    if (name_lower == "go.sum")
        return icons::GO_SUM;
    if (name_lower == "build.gradle" || name_lower == "build.gradle.kts" ||
        name_lower == "settings.gradle" || name_lower == "gradlew" ||
        name_lower == "gradle.properties") {
        return icons::GRADLE;
    }
    if (name_lower == "yarn.lock")
        return icons::YARN_LOCK;
    if (name_lower == "pnpm-lock.yaml")
        return icons::PNPM_LOCK;

    // 测试文件
    if (name_lower.find("test") != std::string::npos ||
        name_lower.find("spec") != std::string::npos || ext == "test" || ext == "spec") {
        if (ext == "spec" || name_lower.find(".spec.") != std::string::npos)
            return icons::SPEC;
        return icons::TEST;
    }

    // 数据文件
    if (ext == "csv")
        return icons::CSV;
    if (ext == "tsv")
        return icons::TSV;
    if (ext == "xls" || ext == "xlsx" || ext == "xlsm")
        return icons::EXCEL;

    // CI/CD 配置文件
    if (name_lower == "jenkinsfile" || name_lower == "jenkinsfile.groovy")
        return icons::JENKINS;
    if (name_lower.find(".github/workflows") != std::string::npos ||
        name_lower.find("workflows") != std::string::npos) {
        return icons::GITHUB_ACTIONS;
    }
    if (name_lower == ".circleci" || name_lower.find("circle") != std::string::npos)
        return icons::CI;

    // 证书和密钥文件
    if (ext == "pem" || ext == "key" || ext == "crt" || ext == "cer" || ext == "cert") {
        if (ext == "key")
            return icons::KEY;
        return icons::CERTIFICATE;
    }

    // 字体文件
    if (ext == "ttf" || ext == "otf" || ext == "woff" || ext == "woff2" || ext == "eot") {
        return icons::FONT;
    }

    // 样式文件扩展
    if (ext == "styl" || ext == "stylus")
        return icons::STYLUS;
    if (ext == "sass" ||
        (name_lower.length() > 5 && name_lower.substr(name_lower.length() - 5) == ".sass")) {
        return icons::SASS;
    }

    // 临时和缓存文件
    if (ext == "tmp" || ext == "temp" || (name_lower.length() > 0 && name_lower[0] == '~') ||
        (name_lower.length() > 4 && name_lower.substr(0, 4) == ".swp")) {
        return icons::TEMP;
    }
    if (name_lower.find("cache") != std::string::npos || ext == "cache") {
        return icons::CACHE;
    }

    // 特殊目录（通过名称判断，但这里只处理文件）
    // 如果是目录，会在上面的 is_directory 判断中处理

    // 可执行文件（Unix）
    if (ext == "exe" || ext == "bin" || ext == "out" || ext == "app") {
        return icons::EXECUTABLE;
    }

    // 更多编程语言支持
    if (ext == "swift")
        return icons::SWIFT;
    if (ext == "kt" || ext == "kts")
        return icons::KOTLIN;
    if (ext == "scala" || ext == "sc")
        return icons::SCALA;
    if (ext == "r" || ext == "R")
        return icons::R;
    if (ext == "pl" || ext == "pm" || ext == "t")
        return icons::PERL;
    if (ext == "hs" || ext == "lhs")
        return icons::HASKELL;
    if (ext == "tcl")
        return icons::TCL;
    if (ext == "f" || ext == "f90" || ext == "f95" || ext == "for")
        return icons::FORTRAN;
    if (ext == "vim")
        return icons::VIM;
    if (ext == "ps1" || ext == "psm1" || ext == "psd1")
        return icons::POWERSHELL;

    // 函数式编程语言
    if (ext == "ex" || ext == "exs")
        return icons::ELIXIR;
    if (ext == "clj" || ext == "cljs" || ext == "cljc" || ext == "edn")
        return icons::CLOJURE;
    if (ext == "erl" || ext == "hrl")
        return icons::ERLANG;
    if (ext == "ml" || ext == "mli")
        return icons::OCAML;
    if (ext == "fs" || ext == "fsi" || ext == "fsx")
        return icons::F_SHARP;
    if (ext == "dart")
        return icons::DART;
    if (ext == "nim" || ext == "nims")
        return icons::NIM;
    if (ext == "cr")
        return icons::CRYSTAL;
    if (ext == "zig")
        return icons::ZIG;
    if (ext == "jl")
        return icons::JULIA;
    if (ext == "purs")
        return icons::PURESCRIPT;
    if (ext == "elm")
        return icons::ELIXIR; // 暂时使用 Elixir 图标

    // Web 框架和工具
    if (ext == "vue")
        return icons::VUE;
    if (ext == "svelte")
        return icons::SVELTE;
    if (ext == "pug" || ext == "jade")
        return icons::PUG;
    if (ext == "styl")
        return icons::STYLUS;
    if (ext == "sass")
        return icons::SASS;
    if (ext == "less")
        return icons::LESS;
    if (ext == "postcss")
        return icons::POSTCSS;
    if (ext == "graphql" || ext == "gql")
        return icons::GRAPHQL;
    if (ext == "coffee")
        return icons::COFFEESCRIPT;

    // .NET 相关
    if (ext == "cs" || ext == "csx")
        return icons::CSHARP;
    if (ext == "vb")
        return icons::VB;
    if (ext == "fsproj" || ext == "csproj" || ext == "vbproj")
        return icons::DOTNET;

    // 系统编程
    if (ext == "rs")
        return icons::RUST;
    if (ext == "go")
        return icons::GO;
    if (ext == "d")
        return icons::D;
    if (ext == "vala" || ext == "vapi")
        return icons::VALA;
    if (ext == "pony")
        return icons::PONY;
    if (ext == "v")
        return icons::V_LANG;
    if (ext == "odin")
        return icons::ODIN;
    if (ext == "jai")
        return icons::JAI;
    if (ext == "nelua")
        return icons::NELUA;
    if (ext == "wren")
        return icons::WREN;
    if (ext == "moon")
        return icons::MOONSCRIPT;

    // 学术和研究语言
    if (ext == "m") {
        if (name_lower.find("octave") != std::string::npos)
            return icons::OCTAVE;
        return icons::MATLAB;
    }
    if (ext == "nb" || ext == "cdf")
        return icons::MATLAB; // Mathematica
    if (ext == "ipynb")
        return icons::PYTHON; // Jupyter notebook

    // Lisp 家族
    if (ext == "scm" || ext == "ss")
        return icons::SCHEME;
    if (ext == "lsp" || ext == "cl")
        return icons::COMMON_LISP;
    if (ext == "el" || ext == "elc")
        return icons::EMACS_LISP;

    // 逻辑编程
    if (ext == "pro" || ext == "pl")
        return icons::PROLOG;
    if (ext == "asp")
        return icons::PROLOG; // Answer Set Programming

    // 证明助手
    if (ext == "v")
        return icons::COQ;
    if (ext == "thy")
        return icons::COQ; // Isabelle
    if (ext == "lean")
        return icons::LEAN;
    if (ext == "agda")
        return icons::AGDA;
    if (ext == "idr")
        return icons::IDRIS;

    // 建模和规范语言
    if (ext == "als")
        return icons::ALLOY;
    if (ext == "dfy")
        return icons::DAFNY;
    if (ext == "tla")
        return icons::TLA;

    // 区块链和智能合约
    if (ext == "sol")
        return icons::SOLIDITY;
    if (ext == "vy")
        return icons::VYPER;
    if (ext == "clar")
        return icons::CLARITY;
    if (ext == "move")
        return icons::MOVE;

    // 移动开发
    if (ext == "bal")
        return icons::BALLERINA;
    if (ext == "kt" || ext == "kts")
        return icons::KOTLIN;
    if (ext == "swift")
        return icons::SWIFT;

    // 硬件描述语言
    if (ext == "v" || ext == "vh")
        return icons::VERILOG;
    if (ext == "vhd" || ext == "vhdl")
        return icons::VHDL;

    // 汇编和底层
    if (ext == "asm" || ext == "s" || ext == "S")
        return icons::ASSEMBLY;
    if (ext == "wat")
        return icons::WEBASSEMBLY;

    // 其他脚本语言
    if (ext == "awk")
        return icons::SHELL;
    if (ext == "sed")
        return icons::SHELL;
    if (ext == "groovy")
        return icons::GROOVY;
    if (ext == "gradle")
        return icons::GRADLE;

    return icons::FILE; // 默认文件图标
}

std::string FileBrowserView::getFileExtension(const std::string& filename) const {
    size_t pos = filename.find_last_of('.');
    if (pos != std::string::npos && pos > 0) {
        return filename.substr(pos + 1);
    }
    return "";
}

std::string FileBrowserView::truncateMiddle(const std::string& str, size_t max_length) const {
    if (str.length() <= max_length) {
        return str;
    }

    if (max_length < 5) {
        return str.substr(0, max_length);
    }

    // 计算两端保留的长度
    size_t left_len = (max_length - 3) / 2;
    size_t right_len = max_length - 3 - left_len;

    return str.substr(0, left_len) + "..." + str.substr(str.length() - right_len);
}

} // namespace ui
} // namespace pnana