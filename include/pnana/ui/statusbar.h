#ifndef PNANA_UI_STATUSBAR_H
#define PNANA_UI_STATUSBAR_H

#include "ui/theme.h"
#include "utils/file_type_icon_mapper.h"
#include "utils/logger.h"
#include "utils/version_detector.h"
#include <algorithm>
#include <chrono>
#include <ftxui/dom/elements.hpp>
#include <future>
#include <mutex>
#include <string>
#include <tuple>
#include <unordered_map>

namespace pnana {
namespace ui {

// 状态栏美化配置
struct StatusbarBeautifyConfig {
    // 是否启用美化
    bool enabled = false;

    // 是否跟随主题颜色（如果为 true，即使启用了美化，背景和前景色也使用主题颜色）
    bool follow_theme = true;

    // 背景颜色 RGB（仅在 follow_theme = false 时使用）
    std::vector<int> bg_color = {45, 45, 45}; // 默认状态栏背景色

    // 前景颜色 RGB（仅在 follow_theme = false 时使用）
    std::vector<int> fg_color = {248, 248, 242}; // 默认状态栏前景色

    // 特殊效果
    bool show_gradient = false;
    bool show_shadows = false;
    bool rounded_corners = false;

    // 图标样式
    std::string icon_style = "default"; // default, filled, outlined

    // 图标增强配置（保留兼容性，实际由 Lua 控制）
    std::map<std::string, std::string> file_icons;
    std::map<std::string, std::string> region_icons;
    std::map<std::string, std::string> status_icons;
    std::map<std::string, std::vector<int>> element_colors;

    // 平台图标（持久化缓存）
    std::string platform_icon;
};

// 状态栏组件
class Statusbar {
  public:
    explicit Statusbar(Theme& theme);

    // 设置美化配置
    void setBeautifyConfig(const StatusbarBeautifyConfig& config) {
        beautify_config_ = config;
        // 调试信息
        if (config.enabled) {
            pnana::utils::Logger::getInstance().log("Statusbar beautify config set: ENABLED");
        } else {
            pnana::utils::Logger::getInstance().log("Statusbar beautify config set: DISABLED");
        }

        // 更新图标映射器
        icon_mapper_.clearCustomIcons();
        for (const auto& [file_type, icon] : config.file_icons) {
            icon_mapper_.setCustomIcon(file_type, icon);
        }
    }

    // 获取美化配置
    const StatusbarBeautifyConfig& getBeautifyConfig() const {
        return beautify_config_;
    }

    // 渲染状态栏
    ftxui::Element render(const std::string& filename, bool is_modified, bool is_readonly,
                          size_t current_line, size_t current_col, size_t total_lines,
                          const std::string& encoding, const std::string& line_ending,
                          const std::string& file_type, const std::string& message = "",
                          const std::string& region_name = "", bool syntax_highlighting = true,
                          bool has_selection = false, size_t selection_length = 0,
                          const std::string& git_branch = "", int git_uncommitted_count = 0,
                          const std::string& ssh_host = "", const std::string& ssh_user = "");

    // Git相关方法（公共接口）
    static std::tuple<std::string, int> getGitInfo();

  private:
    Theme& theme_;
    StatusbarBeautifyConfig beautify_config_;
    utils::FileTypeIconMapper icon_mapper_;
    utils::VersionDetector version_detector_;

    // 获取文件类型图标
    std::string getFileTypeIcon(const std::string& file_type);

    // 格式化位置信息
    std::string formatPosition(size_t line, size_t col);

    // 格式化进度
    std::string formatProgress(size_t current, size_t total);

    // 获取区域图标
    std::string getRegionIcon(const std::string& region_name);

    // 获取平台图标
    std::string getPlatformIcon();

    // 获取操作系统信息
    std::string getOperatingSystem();

    // 创建状态指示器
    ftxui::Element createIndicator(const std::string& icon, const std::string& label,
                                   ftxui::Color fg_color, ftxui::Color bg_color);

    // Git相关方法
    static std::string getGitBranch();
    static int getGitUncommittedCount();
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_STATUSBAR_H
