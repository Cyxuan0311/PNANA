#include "ui/file_browser_view.h"
#include "ui/icons.h"
#include "features/file_browser.h"
#include <algorithm>
#include <cctype>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

namespace pnana {
namespace ui {

FileBrowserView::FileBrowserView(Theme& theme)
    : theme_(theme) {
}

Element FileBrowserView::render(const features::FileBrowser& browser, int height) {
    auto& colors = theme_.getColors();
    
    Elements content;
    
    // 标题栏
    content.push_back(renderHeader(browser.getCurrentDirectory()));
    content.push_back(separator());
    
    // 文件列表
    size_t total_items = browser.getFlatItems().size();
    
    // 计算可用高度：总高度 - 标题(1) - 分隔符(1) - 底部分隔符(1) - 状态栏(1) = height - 4
    size_t available_height = static_cast<size_t>(height - 4);
    size_t visible_start = 0;
    size_t visible_count = available_height;  // 使用所有可用高度
    
    // 调整滚动位置
    size_t selected_index = browser.getSelectedIndex();
    
    if (selected_index >= visible_start + visible_count && total_items > visible_count) {
        visible_start = selected_index - visible_count + 1;
    }
    if (selected_index < visible_start) {
        visible_start = selected_index;
    }
    
    // 渲染文件列表 - 直接获取文件项元素，而不是嵌套的 vbox
    Elements file_list_elements;
    const auto& flat_items = browser.getFlatItems();
    
    size_t rendered_items = 0;
    for (size_t i = visible_start; 
         i < flat_items.size() && i < visible_start + visible_count; 
         ++i) {
        const features::FileItem* item = flat_items[i];
        if (item) {
            file_list_elements.push_back(renderFileItem(item, i, selected_index, flat_items));
            rendered_items++;
        }
    }
    
    // 如果文件项数少于可用高度，填充空行
    if (rendered_items < available_height) {
        for (size_t i = rendered_items; i < available_height; ++i) {
            file_list_elements.push_back(text(""));
        }
    }
    
    // 将文件列表添加到内容中
    if (!file_list_elements.empty()) {
        content.push_back(vbox(file_list_elements));
    }
    
    // 底部状态栏
    content.push_back(separator());
    content.push_back(renderStatusBar(browser));
    
    return vbox(content) | bgcolor(colors.background);
}

Element FileBrowserView::renderHeader(const std::string& current_directory) const {
    auto& colors = theme_.getColors();
    
    return hbox({
        text(" "),
        text(icons::FOLDER_OPEN) | color(colors.function),
        text(" "),
        text(current_directory) | bold | color(colors.foreground),
        filler(),
        text(" ") | color(colors.comment)
    }) | bgcolor(colors.menubar_bg);
}

Element FileBrowserView::renderFileList(const features::FileBrowser& browser,
                                        size_t visible_start,
                                        size_t visible_count) const {
    Elements content;
    const auto& flat_items = browser.getFlatItems();
    size_t selected_index = browser.getSelectedIndex();
    
    // 渲染可见的文件项
    for (size_t i = visible_start; 
         i < flat_items.size() && i < visible_start + visible_count; 
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
    
    return hbox({
        text(" "),
        text(icons::LOCATION) | color(colors.keyword),
        text(" "),
        text(selected_path_display) | color(colors.comment)
    }) | bgcolor(colors.menubar_bg);
}

Element FileBrowserView::renderFileItem(const features::FileItem* item,
                                        size_t index,
                                        size_t selected_index,
                                        const std::vector<features::FileItem*>& flat_items) const {
    auto& colors = theme_.getColors();
    
    std::string icon = getFileIcon(*item);
    Color item_color = getFileColor(*item);
    
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
    Elements row_elements = {
        text(" "),
        text(tree_prefix) | color(colors.comment),
        text(expand_prefix) | color(colors.comment),
        text(expand_icon) | color(item_color),
        text(" "),
        text(icon) | color(item_color),
        text(" "),
        text(display_name) | color(item_color)
    };
    
    auto item_text = hbox(row_elements);
    
    // 选中项高亮
    if (index == selected_index) {
        item_text = item_text | bgcolor(colors.selection) | bold;
    } else {
        item_text = item_text | bgcolor(colors.background);
    }
    
    return item_text;
}

std::string FileBrowserView::buildTreePrefix(const features::FileItem* item,
                                             size_t index,
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
            prefix += "│ ";  // 有后续兄弟，显示竖线
        } else {
            prefix += "  ";  // 没有后续兄弟，显示空格
        }
    }
    
    return prefix;
}

std::string FileBrowserView::buildExpandPrefix(const features::FileItem* item,
                                               size_t index,
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
    if (ext == "cpp" || ext == "cc" || ext == "cxx" || ext == "c++") return icons::CPP;
    if (ext == "h" || ext == "hpp" || ext == "hxx" || ext == "hh") return icons::CPP;
    if (ext == "c") return icons::C;
    
    // 脚本语言
    if (ext == "py" || ext == "pyw" || ext == "pyi") return icons::PYTHON;
    if (ext == "js" || ext == "jsx" || ext == "mjs") return icons::JAVASCRIPT;
    if (ext == "ts" || ext == "tsx") return icons::TYPESCRIPT;
    if (ext == "rb" || ext == "rbw") return icons::RUBY;
    if (ext == "php" || ext == "php3" || ext == "php4" || ext == "php5" || ext == "phtml") return icons::PHP;
    if (ext == "lua") return icons::LUA;
    if (ext == "sh" || ext == "bash" || ext == "zsh" || ext == "fish") return icons::SHELL;
    if (name_lower == "makefile" || name_lower == "makefile.am" || name_lower == "makefile.in") return icons::MAKEFILE;
    
    // 编译语言
    if (ext == "java" || ext == "class" || ext == "jar") return icons::JAVA;
    if (ext == "go") return icons::GO;
    if (ext == "rs") return icons::RUST;
    
    // Web 技术
    if (ext == "html" || ext == "htm" || ext == "xhtml") return icons::HTML;
    if (ext == "css" || ext == "scss" || ext == "sass" || ext == "less") return icons::CSS;
    
    // 数据格式
    if (ext == "json" || ext == "jsonc") {
        // 特殊 JSON 文件
        if (name_lower == "package.json") return icons::PACKAGE_JSON;
        if (name_lower == "package-lock.json") return icons::PACKAGE_LOCK;
        if (name_lower == "composer.json") return icons::COMPOSER;
        if (name_lower == "tsconfig.json" || name_lower == "tsconfig.base.json") return icons::TSCONFIG;
        if (name_lower == ".prettierrc" || name_lower == ".prettierrc.json") return icons::PRETTIER;
        if (name_lower == ".eslintrc" || name_lower == ".eslintrc.json" || name_lower == "eslint.config.json") return icons::ESLINT;
        if (name_lower == ".babelrc" || name_lower == ".babelrc.json") return icons::BABEL;
        return icons::JSON;
    }
    if (ext == "xml" || ext == "xsd" || ext == "xsl") {
        if (name_lower == "pom.xml") return icons::MAVEN;
        return icons::XML;
    }
    if (ext == "yml" || ext == "yaml") {
        if (name_lower == ".travis.yml") return icons::TRAVIS;
        if (name_lower == "docker-compose.yml" || name_lower == "docker-compose.yaml") return icons::DOCKER_COMPOSE;
        if (name_lower == ".github/workflows" || name_lower.find(".github") != std::string::npos) return icons::GITHUB_ACTIONS;
        return icons::YAML;
    }
    if (ext == "toml") {
        if (name_lower == "cargo.toml") return icons::CARGO;
        if (name_lower == "pyproject.toml") return icons::POETRY;
        return icons::CONFIG;
    }
    // Cargo 和 Poetry lock 文件（无扩展名或不同扩展名）
    if (name_lower == "cargo.lock") return icons::CARGO;
    if (name_lower == "poetry.lock") return icons::POETRY;
    
    // 文档
    if (ext == "md" || ext == "markdown") {
        if (name_lower == "readme.md" || name_lower == "readme") return icons::README;
        if (name_lower == "changelog.md" || name_lower == "changelog") return icons::CHANGELOG;
        if (name_lower == "contributing.md" || name_lower == "contributing") return icons::CONTRIBUTING;
        return icons::MARKDOWN;
    }
    if (ext == "txt") {
        if (name_lower == "license" || name_lower == "license.txt") return icons::LICENSE;
        if (name_lower == "authors" || name_lower == "authors.txt") return icons::AUTHORS;
        if (name_lower == "todo" || name_lower == "todo.txt") return icons::TODO;
        return icons::FILE_TEXT;
    }
    if (ext == "log") return icons::LOG;
    if (ext == "pdf") return icons::PDF;
    
    // 数据库
    if (ext == "sql") return icons::SQL;
    if (ext == "db" || ext == "sqlite" || ext == "sqlite3" || ext == "db3") return icons::DATABASE;
    
    // 图片
    if (ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "gif" || 
        ext == "bmp" || ext == "svg" || ext == "ico" || ext == "webp") {
        return icons::IMAGE;
    }
    
    // 视频
    if (ext == "mp4" || ext == "avi" || ext == "mov" || ext == "wmv" || 
        ext == "flv" || ext == "mkv" || ext == "webm") {
        return icons::VIDEO;
    }
    
    // 音频
    if (ext == "mp3" || ext == "wav" || ext == "flac" || ext == "aac" || 
        ext == "ogg" || ext == "m4a") {
        return icons::AUDIO;
    }
    
    // 压缩包
    if (ext == "zip" || ext == "tar" || ext == "gz" || ext == "bz2" || 
        ext == "xz" || ext == "7z" || ext == "rar" || ext == "z") {
        return icons::ARCHIVE;
    }
    
    // 环境配置文件
    if (name_lower == ".env" || name_lower == ".env.local" || name_lower == ".env.development" ||
        name_lower == ".env.production" || name_lower == ".env.test" || 
        (name_lower.length() > 5 && name_lower.substr(0, 5) == ".env.")) {
        return icons::ENV;
    }
    
    // 配置文件
    if (ext == "conf" || ext == "config" || ext == "ini" || ext == "cfg" || 
        ext == "properties" || name_lower == ".editorconfig") {
        if (name_lower == ".editorconfig") return icons::EDITORCONFIG;
        return icons::CONFIG;
    }
    
    // Git 相关
    if (name_lower == ".gitignore" || name_lower == ".gitattributes" || 
        name_lower == ".gitmodules" || name_lower == ".gitconfig" || name_lower == ".gitkeep") {
        return icons::GITIGNORE;
    }
    
    // CMake
    if (ext == "cmake" || name_lower == "cmakelists.txt") return icons::CMAKE;
    
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
        if (name_lower == "gemfile.lock") return icons::GEMFILE_LOCK;
        return icons::GEMFILE;
    }
    if (name_lower == "go.mod") return icons::GO_MOD;
    if (name_lower == "go.sum") return icons::GO_SUM;
    if (name_lower == "build.gradle" || name_lower == "build.gradle.kts" ||
        name_lower == "settings.gradle" || name_lower == "gradlew" || name_lower == "gradle.properties") {
        return icons::GRADLE;
    }
    if (name_lower == "yarn.lock") return icons::YARN_LOCK;
    if (name_lower == "pnpm-lock.yaml") return icons::PNPM_LOCK;
    
    // 测试文件
    if (name_lower.find("test") != std::string::npos || name_lower.find("spec") != std::string::npos ||
        ext == "test" || ext == "spec") {
        if (ext == "spec" || name_lower.find(".spec.") != std::string::npos) return icons::SPEC;
        return icons::TEST;
    }
    
    // 数据文件
    if (ext == "csv") return icons::CSV;
    if (ext == "tsv") return icons::TSV;
    if (ext == "xls" || ext == "xlsx" || ext == "xlsm") return icons::EXCEL;
    
    // CI/CD 配置文件
    if (name_lower == "jenkinsfile" || name_lower == "jenkinsfile.groovy") return icons::JENKINS;
    if (name_lower.find(".github/workflows") != std::string::npos || 
        name_lower.find("workflows") != std::string::npos) {
        return icons::GITHUB_ACTIONS;
    }
    if (name_lower == ".circleci" || name_lower.find("circle") != std::string::npos) return icons::CI;
    
    // 证书和密钥文件
    if (ext == "pem" || ext == "key" || ext == "crt" || ext == "cer" || ext == "cert") {
        if (ext == "key") return icons::KEY;
        return icons::CERTIFICATE;
    }
    
    // 字体文件
    if (ext == "ttf" || ext == "otf" || ext == "woff" || ext == "woff2" || ext == "eot") {
        return icons::FONT;
    }
    
    // 样式文件扩展
    if (ext == "styl" || ext == "stylus") return icons::STYLUS;
    if (ext == "sass" || (name_lower.length() > 5 && name_lower.substr(name_lower.length() - 5) == ".sass")) {
        return icons::SASS;
    }
    
    // 临时和缓存文件
    if (ext == "tmp" || ext == "temp" || 
        (name_lower.length() > 0 && name_lower[0] == '~') || 
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
    
    return icons::FILE; // 默认文件图标
}

std::string FileBrowserView::getFileExtension(const std::string& filename) const {
    size_t pos = filename.find_last_of('.');
    if (pos != std::string::npos && pos > 0) {
        return filename.substr(pos + 1);
    }
    return "";
}

Color FileBrowserView::getFileColor(const features::FileItem& item) const {
    auto& colors = theme_.getColors();
    
    if (item.is_directory) {
        return colors.function;  // 蓝色
    }
    
    std::string ext = getFileExtension(item.name);
    
    // 根据文件类型返回颜色
    if (ext == "cpp" || ext == "c" || ext == "h" || ext == "hpp") {
        return colors.keyword;  // 紫色
    }
    if (ext == "py" || ext == "js" || ext == "ts" || ext == "java") {
        return colors.string;  // 绿色
    }
    if (ext == "md" || ext == "txt") {
        return colors.foreground;  // 白色
    }
    if (ext == "json" || ext == "xml" || ext == "yml") {
        return colors.number;  // 橙色
    }
    
    return colors.comment;  // 灰色
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

