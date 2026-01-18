#ifndef PNANA_UI_STATUSBAR_H
#define PNANA_UI_STATUSBAR_H

#include "ui/theme.h"
#include <ftxui/dom/elements.hpp>
#include <string>
#include <tuple>

namespace pnana {
namespace ui {

// 状态栏组件
class Statusbar {
  public:
    explicit Statusbar(Theme& theme);

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

    // 获取文件类型图标
    std::string getFileTypeIcon(const std::string& file_type);

    // 格式化位置信息
    std::string formatPosition(size_t line, size_t col);

    // 格式化进度
    std::string formatProgress(size_t current, size_t total);

    // 获取区域图标
    std::string getRegionIcon(const std::string& region_name);

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
