#include "ui/statusbar.h"
#include "ui/icons.h"
#include "utils/file_type_icon_mapper.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <future>
#include <iomanip>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>
#include <unordered_map>

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

    // 当前样式（default / neovim / vscode / minimal / classic）
    std::string style = beautify_config_.style_name;
    if (style.empty()) {
        // 没有显式设置时，根据是否启用美化推断：
        // 启用 => 视为 neovim；未启用 => 视为 default（主题内置样式）
        style = beautify_config_.enabled ? "neovim" : "default";
    }
    const bool is_minimal = (style == "minimal");
    const bool is_vscode = (style == "vscode");
    const bool is_highlight = (style == "highlight");
    const bool is_unicode = (style == "unicode");
    // Unicode 风格使用双线竖框 ║ (U+2551) 作为分隔符
    const std::string sep = is_unicode ? " ║ " : " │ ";
    // 全部项高亮时给每个片段加背景
    auto hl = [&](Element e) -> Element {
        return is_highlight ? (e | bgcolor(colors.selection)) : e;
    };

    // Neovim 风格状态栏：左侧、中间、右侧三部分

    // ========== 左侧部分 ==========
    Elements left_elements;

    // 区域指示器（类似 neovim 的模式指示器）
    if (!region_name.empty()) {
        Color region_bg = colors.keyword;
        Color region_fg = colors.background;

        if (is_vscode) {
            // VSCode 风格：区域指示器弱化，不用高饱和色块，用细线+文字
            region_bg = colors.selection;    // 略深于状态栏背景
            region_fg = colors.statusbar_fg; // 与状态栏前景一致
        } else {
            // 根据区域类型设置不同颜色（neovim/default 等）
            if (region_name.find("Terminal") != std::string::npos) {
                region_bg = Color::Cyan;
            } else if (region_name.find("File Browser") != std::string::npos) {
                region_bg = Color::Blue;
            } else if (region_name.find("Tab Bar") != std::string::npos) {
                region_bg = Color::Yellow;
            } else if (region_name.find("Code Editor") != std::string::npos) {
                region_bg = Color::Green;
            }
        }

        // 区域名称（简短）
        std::string short_name = region_name;
        if (region_name == "Code Editor")
            short_name = "EDIT";
        else if (region_name == "File Browser")
            short_name = "FILES";
        else if (region_name == "Tab Bar")
            short_name = "TABS";
        else if (region_name == "Terminal")
            short_name = "TERM";

        if (is_vscode) {
            left_elements.push_back(
                hl(text(" " + short_name + " ") | bgcolor(region_bg) | color(region_fg)));
            left_elements.push_back(
                hl(text(" ") | bgcolor(colors.statusbar_bg) | color(region_bg)));
        } else {
            left_elements.push_back(
                hl(text(" " + short_name + " ") | bgcolor(region_bg) | color(region_fg) | bold));
            left_elements.push_back(
                hl(text(" ") | bgcolor(colors.statusbar_bg) | color(region_bg)));
        }
        // 平台图标
        {
            std::string platform_icon = getPlatformIcon();
            if (!platform_icon.empty()) {
                left_elements.push_back(
                    hl(text(" " + platform_icon + "  ") | color(colors.keyword) | bold));
            }
        }
    }

    // 文件 type icon and filename
    std::string file_display = filename.empty() ? "[Untitled]" : filename;
    std::string file_icon;
    if (filename == "Welcome") {
        file_icon = icons::ROCKET; // 欢迎界面使用火箭图标，不用文件类型图标
    } else {
        std::string ext;
        size_t pos = filename.find_last_of('.');
        if (pos != std::string::npos && pos + 1 < filename.size())
            ext = filename.substr(pos + 1);
        file_icon = utils::getIconForFile(filename, ext, &icon_mapper_);
    }
    if (!file_icon.empty()) {
        left_elements.push_back(hl(text(file_icon + " ") | color(colors.keyword)));
    }
    left_elements.push_back(hl(text(file_display) | bold));

    // 修改标记（红色圆点，Neovim 风格）
    if (is_modified) {
        left_elements.push_back(hl(text(" ●") | color(colors.error) | bold));
    }

    // 只读标记（紧凑）
    if (is_readonly) {
        left_elements.push_back(hl(text(" [RO]") | color(colors.comment) | dim));
    }

    // 选择状态（紧凑显示）
    if (has_selection) {
        std::ostringstream oss;
        // 在文件浏览器区域显示文件选中数量，在代码区域显示文本选择
        if (region_name.find("File Browser") != std::string::npos ||
            region_name.find("FILES") != std::string::npos) {
            oss << " [" << selection_length << " file" << (selection_length > 1 ? "s" : "") << "]";
        } else {
            oss << " [" << selection_length << "]";
        }
        left_elements.push_back(hl(text(oss.str()) | color(colors.warning) | dim));
    }

    // Git信息（分支和未提交文件数）
    if (!git_branch.empty()) {
        left_elements.push_back(hl(text(sep) | color(colors.comment) | dim));
        left_elements.push_back(hl(text(icons::GIT_BRANCH) | color(colors.keyword)));
        left_elements.push_back(hl(text(" " + git_branch) | color(colors.string) | bold));

        if (git_uncommitted_count > 0) {
            std::ostringstream git_oss;
            git_oss << " " << git_uncommitted_count;
            left_elements.push_back(hl(text(git_oss.str()) | color(colors.warning) | bold));
        }
    }

    // SSH连接状态
    if (!ssh_host.empty() && !ssh_user.empty()) {
        left_elements.push_back(hl(text(sep) | color(colors.comment) | dim));
        // 使用终端图标表示SSH连接
        left_elements.push_back(hl(text(icons::TERMINAL) | color(colors.success)));
        left_elements.push_back(
            hl(text(" " + ssh_user + "@" + ssh_host) | color(colors.function) | bold));
    }

    // ========== 中间部分 ==========
    Elements center_elements;

    // 状态消息（如果有，居中显示）
    if (!message.empty()) {
        // 检查是否包含 todo 提醒：TODO_REMINDER = 到期后 1 分钟内闪烁，TODO_REMINDER_STATIC =
        // 过期超过 1 分钟仅红色不闪烁
        const std::string todo_blink_start = "[[TODO_REMINDER]]";
        const std::string todo_blink_end = "[[/TODO_REMINDER]]";
        const std::string todo_static_start = "[[TODO_REMINDER_STATIC]]";
        const std::string todo_static_end = "[[/TODO_REMINDER_STATIC]]";

        size_t todo_start = message.find(todo_blink_start);
        std::string todo_marker_end = todo_blink_end;
        bool use_blink = true;
        if (todo_start == std::string::npos) {
            todo_start = message.find(todo_static_start);
            todo_marker_end = todo_static_end;
            use_blink = false;
        }

        const std::string todo_marker_start = use_blink ? todo_blink_start : todo_static_start;

        if (todo_start != std::string::npos) {
            size_t todo_end = message.find(todo_marker_end, todo_start);
            if (todo_end != std::string::npos) {
                std::string todo_text =
                    message.substr(todo_start + todo_marker_start.length(),
                                   todo_end - todo_start - todo_marker_start.length());

                std::string before_todo = message.substr(0, todo_start);
                std::string after_todo = message.substr(todo_end + todo_marker_end.length());
                std::string normal_message = before_todo + after_todo;

                Color todo_color = colors.error;
                if (use_blink) {
                    int priority = 5;
                    size_t priority_pos = todo_text.find("P");
                    if (priority_pos != std::string::npos &&
                        priority_pos + 1 < todo_text.length()) {
                        char priority_char = todo_text[priority_pos + 1];
                        if (priority_char >= '1' && priority_char <= '5') {
                            priority = priority_char - '0';
                        }
                    }
                    auto now = std::chrono::steady_clock::now();
                    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                  now.time_since_epoch())
                                  .count();
                    const int blink_period_ms = 300;
                    double phase = (static_cast<double>(ms % blink_period_ms) /
                                    static_cast<double>(blink_period_ms)) *
                                   2.0 * M_PI;
                    double intensity = (std::sin(phase) + 1.0) / 2.0;
                    double priority_threshold = 0.3 + (priority - 1) * 0.1;
                    todo_color = intensity < priority_threshold ? colors.error : colors.warning;
                }

                if (!normal_message.empty()) {
                    std::string clean_normal = normal_message;
                    if (clean_normal.find(" | ") == 0) {
                        clean_normal = clean_normal.substr(3);
                    }
                    if (!clean_normal.empty()) {
                        center_elements.push_back(
                            hl(text(" " + clean_normal) | color(colors.foreground) | dim));
                        center_elements.push_back(hl(text(" | ") | color(colors.comment) | dim));
                    }
                }

                center_elements.push_back(hl(text(todo_text) | color(todo_color) | bold));
            } else {
                std::string clean_message = message;
                center_elements.push_back(
                    hl(text(" " + clean_message) | color(colors.foreground) | dim));
            }
        } else {
            std::string clean_message = message;
            center_elements.push_back(
                hl(text(" " + clean_message) | color(colors.foreground) | dim));
        }
    }

    // ========== 右侧部分 ==========
    Elements right_elements;

    // 语法高亮状态（小图标，如果有图标）
    std::string highlight_icon = icons::HIGHLIGHT;
    if (!is_minimal && !highlight_icon.empty()) {
        if (syntax_highlighting) {
            right_elements.push_back(hl(text(highlight_icon) | color(colors.success)));
        } else {
            right_elements.push_back(hl(text(highlight_icon) | color(colors.comment) | dim));
        }
        right_elements.push_back(hl(text(" ") | color(colors.comment)));
    }

    // 编码（紧凑显示）
    if (!is_minimal) {
        right_elements.push_back(hl(text(encoding) | color(colors.comment) | dim));

        // 分隔符（Unicode 风格用 ║，否则 │）
        right_elements.push_back(hl(text(sep) | color(colors.comment) | dim));

        // 行尾类型（紧凑显示）
        right_elements.push_back(hl(text(line_ending) | color(colors.comment) | dim));

        // 分隔符
        right_elements.push_back(hl(text(sep) | color(colors.comment) | dim));
    }

    // 文件类型与版本信息在 minimal 风格下也可以省略，只保留位置和进度
    if (!is_minimal) {
        // 文件类型（如果有且不是text）
        if (!file_type.empty() && file_type != "text") {
            right_elements.push_back(hl(text(file_type) | color(colors.comment) | dim));
            right_elements.push_back(hl(text(sep) | color(colors.comment) | dim));
        }

        // 编译器/解释器版本（如果有）
        std::string version = version_detector_.getVersionForFileType(file_type);
        if (!version.empty()) {
            right_elements.push_back(hl(text(version) | color(colors.function) | dim));
            right_elements.push_back(hl(text(sep) | color(colors.comment) | dim));
        }
    }

    // 位置信息（Ln,Col 格式）
    std::ostringstream pos_oss;
    pos_oss << (current_line + 1) << "," << (current_col + 1);
    right_elements.push_back(hl(text(pos_oss.str()) | color(colors.foreground) | bold));

    // 分隔符
    right_elements.push_back(hl(text(sep) | color(colors.comment) | dim));

    // 进度百分比（突出显示）
    std::string progress = formatProgress(current_line, total_lines);
    Color progress_color = colors.comment;
    if (current_line >= total_lines - 1) {
        progress_color = colors.success;
    } else if (current_line == 0) {
        progress_color = colors.keyword;
    }
    right_elements.push_back(hl(text(progress) | color(progress_color) | bold));

    // 分隔符
    right_elements.push_back(hl(text(sep) | color(colors.comment) | dim));

    // 总行数（Ln 格式）
    std::ostringstream total_oss;
    total_oss << total_lines << "L";
    right_elements.push_back(hl(text(total_oss.str()) | color(colors.comment) | dim));

    // 应用美化配置 - 不同整体风格
    bool use_custom_colors = beautify_config_.enabled && !beautify_config_.follow_theme &&
                             beautify_config_.bg_color.size() >= 3 &&
                             beautify_config_.fg_color.size() >= 3;

    // 先构建基础内容（三段布局）
    Element core =
        hbox({hbox(left_elements) | flex_grow, hbox(center_elements) | flex, hbox(right_elements)});

    Element styled = core;
    if (style == "vscode") {
        // VSCode 风格：扁平条状，仅顶部一条细线分隔，无方括号无全边框
        Color top_line = use_custom_colors ? ftxui::Color::RGB(beautify_config_.fg_color[0],
                                                               beautify_config_.fg_color[1],
                                                               beautify_config_.fg_color[2])
                                           : colors.statusbar_fg;
        styled = vbox({
            separator() | color(top_line) | dim,
            core,
        });
    } else if (style == "minimal") {
        // Minimal 风格：整体弱化、去装饰
        styled = core | dim;
    } else if (style == "classic") {
        // Classic：在底部加下划线，类似传统编辑器
        styled = core | underlined;
    } else if (style == "default") {
        // Default：使用主题内置样式，不添加额外装饰
        styled = core;
    } else if (style == "highlight") {
        // Highlight：每项已用 hl() 加背景，无需额外包装
        styled = core;
    } else if (style == "rounded" || beautify_config_.rounded_corners) {
        // 圆角主题：使用 borderRounded 包裹整条状态栏
        styled = core | borderRounded | color(colors.line_number);
    } else if (style == "unicode") {
        // Unicode 框线：左右用 ╭─ 与 ─╮ 包裹（U+256D U+2500 U+256E）
        Color deco = use_custom_colors ? ftxui::Color::RGB(beautify_config_.fg_color[0],
                                                           beautify_config_.fg_color[1],
                                                           beautify_config_.fg_color[2])
                                       : colors.line_number;
        styled = hbox({
            text("╭─ ") | color(deco),
            core,
            text(" ─╮") | color(deco),
        });
    } else {
        // neovim 或未知：保持默认三段布局
        styled = core;
    }

    // 最后应用颜色：优先使用自定义颜色，否则使用主题状态栏颜色
    if (use_custom_colors) {
        return styled |
               bgcolor(ftxui::Color::RGB(beautify_config_.bg_color[0], beautify_config_.bg_color[1],
                                         beautify_config_.bg_color[2])) |
               color(ftxui::Color::RGB(beautify_config_.fg_color[0], beautify_config_.fg_color[1],
                                       beautify_config_.fg_color[2]));
    }

    return styled | bgcolor(colors.statusbar_bg) | color(colors.statusbar_fg);
}

std::string Statusbar::getFileTypeIcon(const std::string& file_type) {
    return icon_mapper_.getIcon(file_type);
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

std::string Statusbar::getPlatformIcon() {
    namespace icons = pnana::ui::icons;

    // 如果配置中已有平台图标，直接返回
    if (!beautify_config_.platform_icon.empty()) {
        return beautify_config_.platform_icon;
    }

    // 首次调用时检测操作系统并设置图标
    std::string os_name = this->getOperatingSystem();
    std::string found_icon = icons::LINUX;

    if (os_name.find("Ubuntu") != std::string::npos) {
        found_icon = icons::UBUNTU;
    } else if (os_name.find("Fedora") != std::string::npos) {
        found_icon = icons::FEDORA;
    } else if (os_name.find("CentOS") != std::string::npos) {
        found_icon = icons::CENTOS;
    } else if (os_name.find("Red Hat") != std::string::npos ||
               os_name.find("RHEL") != std::string::npos) {
        found_icon = icons::REDHAT;
    } else if (os_name.find("Debian") != std::string::npos) {
        found_icon = icons::DEBIAN;
    } else if (os_name.find("Arch") != std::string::npos) {
        found_icon = icons::ARCHLINUX;
    } else if (os_name.find("Manjaro") != std::string::npos) {
        found_icon = icons::MANJARO;
    } else if (os_name.find("SUSE") != std::string::npos ||
               os_name.find("openSUSE") != std::string::npos) {
        found_icon = icons::SUSE;
    } else if (os_name.find("Gentoo") != std::string::npos) {
        found_icon = icons::GENTOO;
    } else if (os_name.find("Linux Mint") != std::string::npos) {
        found_icon = icons::MINT;
    } else if (os_name.find("Pop!_OS") != std::string::npos) {
        found_icon = icons::POP_OS;
    } else if (os_name.find("Elementary") != std::string::npos) {
        found_icon = icons::ELEMENTARY;
    } else if (os_name.find("Windows") != std::string::npos) {
        found_icon = icons::WINDOWS;
    } else if (os_name.find("macOS") != std::string::npos ||
               os_name.find("Darwin") != std::string::npos) {
        found_icon = icons::MACOS;
    } else if (os_name.find("FreeBSD") != std::string::npos) {
        found_icon = icons::FREEBSD;
    } else if (os_name.find("OpenBSD") != std::string::npos) {
        found_icon = icons::OPENBSD;
    } else if (os_name.find("NetBSD") != std::string::npos) {
        found_icon = icons::NETBSD;
    } else if (os_name.find("Solaris") != std::string::npos) {
        found_icon = icons::SOLARIS;
    } else if (os_name.find("Linux") != std::string::npos) {
        found_icon = icons::LINUX;
    } else {
        found_icon = icons::LINUX;
    }

    // 持久化到配置中，避免后续重复检测
    beautify_config_.platform_icon = found_icon;

    return found_icon;
}

std::string Statusbar::getOperatingSystem() {
    std::array<char, 256> buffer;
    std::string result;

    // 首先尝试读取 /etc/os-release (Linux)
    std::unique_ptr<FILE, FileDeleter> pipe(popen(
        "cat /etc/os-release 2>/dev/null | grep PRETTY_NAME | cut -d'=' -f2 | tr -d '\"'", "r"));
    if (pipe) {
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        if (!result.empty()) {
            // 移除末尾换行符
            if (result.back() == '\n')
                result.pop_back();
            return result;
        }
    }

    // 如果是Windows，检查环境变量
    const char* windows_env = std::getenv("OS");
    if (windows_env && std::string(windows_env).find("Windows") != std::string::npos) {
        return "Windows";
    }

    // macOS检测
    pipe.reset(popen("sw_vers -productName 2>/dev/null", "r"));
    if (pipe) {
        result.clear();
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        if (!result.empty()) {
            if (result.back() == '\n')
                result.pop_back();
            return result;
        }
    }

    // FreeBSD/OpenBSD/NetBSD
    pipe.reset(popen("uname -s 2>/dev/null", "r"));
    if (pipe) {
        result.clear();
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        if (!result.empty()) {
            if (result.back() == '\n')
                result.pop_back();
            return result;
        }
    }

    // 如果都检测不到，返回Unknown
    return "Unknown";
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
