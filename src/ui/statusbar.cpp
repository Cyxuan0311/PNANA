#include "ui/statusbar.h"
#include "ui/icons.h"
#include "utils/logger.h"
#include <array>
#include <chrono>
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
        // 平台图标（显示当前操作系统），移动到区域指示器之后并使用更明显的主题颜色
        {
            std::string platform_icon = getPlatformIcon();
            if (!platform_icon.empty()) {
                // 使用主题中较明显的颜色（keyword）并加粗，靠近区域指示器显示
                // 把图标放在单个 text() 中，确保不会被截断或出现显示不全的问题
                left_elements.push_back(text(" " + platform_icon + "  ") | color(colors.keyword) |
                                        bold);
            }
        }
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
        // 在文件浏览器区域显示文件选中数量，在代码区域显示文本选择
        if (region_name.find("File Browser") != std::string::npos ||
            region_name.find("FILES") != std::string::npos) {
            oss << " [" << selection_length << " file" << (selection_length > 1 ? "s" : "") << "]";
        } else {
            oss << " [" << selection_length << "]";
        }
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
        // 检查是否包含 todo 提醒（使用特殊标记）
        std::string todo_marker_start = "[[TODO_REMINDER]]";
        std::string todo_marker_end = "[[/TODO_REMINDER]]";
        size_t todo_start = message.find(todo_marker_start);

        if (todo_start != std::string::npos) {
            // 包含 todo 提醒，需要分别渲染
            size_t todo_end = message.find(todo_marker_end, todo_start);
            if (todo_end != std::string::npos) {
                // 提取 todo 提醒文本
                std::string todo_text =
                    message.substr(todo_start + todo_marker_start.length(),
                                   todo_end - todo_start - todo_marker_start.length());

                // 提取普通消息（todo 之前和之后的部分）
                std::string before_todo = message.substr(0, todo_start);
                std::string after_todo = message.substr(todo_end + todo_marker_end.length());
                std::string normal_message = before_todo + after_todo;

                // 使用时间戳实现颜色闪烁效果（每500ms切换一次）
                auto now = std::chrono::steady_clock::now();
                auto ms =
                    std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch())
                        .count();
                int blink_phase = (ms / 500) % 4; // 4个阶段：红->黄->红->黄

                Color todo_color;
                if (blink_phase == 0 || blink_phase == 2) {
                    // 红色闪烁
                    todo_color = colors.error;
                } else {
                    // 黄色闪烁
                    todo_color = colors.warning;
                }

                // 渲染普通消息（如果有）
                if (!normal_message.empty()) {
                    std::string clean_normal = normal_message;
                    // 移除多余的分隔符
                    if (clean_normal.find(" | ") == 0) {
                        clean_normal = clean_normal.substr(3);
                    }
                    if (!clean_normal.empty()) {
                        center_elements.push_back(text(" " + clean_normal) |
                                                  color(colors.foreground) | dim);
                        center_elements.push_back(text(" | ") | color(colors.comment) | dim);
                    }
                }

                // 渲染 todo 提醒（带闪烁颜色）
                center_elements.push_back(text(todo_text) | color(todo_color) | bold);
            } else {
                // 标记不完整，使用普通渲染
                std::string clean_message = message;
                center_elements.push_back(text(" " + clean_message) | color(colors.foreground) |
                                          dim);
            }
        } else {
            // 普通消息，正常渲染
            std::string clean_message = message;
            center_elements.push_back(text(" " + clean_message) | color(colors.foreground) | dim);
        }
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

    // 编译器/解释器版本（如果有）
    std::string version = version_detector_.getVersionForFileType(file_type);
    if (!version.empty()) {
        right_elements.push_back(text(version) | color(colors.function) | dim);
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

    // 应用美化配置 - 增强视觉效果
    if (beautify_config_.enabled && beautify_config_.bg_color.size() >= 3 &&
        beautify_config_.fg_color.size() >= 3) {
        // 应用美化背景和颜色效果
        auto content = hbox(
            {hbox(left_elements) | flex_grow, hbox(center_elements) | flex, hbox(right_elements)});

        // 应用美化背景和边框效果
        return content |
               bgcolor(ftxui::Color::RGB(beautify_config_.bg_color[0], beautify_config_.bg_color[1],
                                         beautify_config_.bg_color[2])) |
               color(ftxui::Color::RGB(beautify_config_.fg_color[0], beautify_config_.fg_color[1],
                                       beautify_config_.fg_color[2]));
    } else {
        // 默认样式
        return hbox({hbox(left_elements) | flex_grow, hbox(center_elements) | flex,
                     hbox(right_elements)}) |
               bgcolor(colors.statusbar_bg) | color(colors.statusbar_fg);
    }
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
