#ifndef PNANA_UTILS_MATCH_HIGHLIGHT_H
#define PNANA_UTILS_MATCH_HIGHLIGHT_H

#include <ftxui/dom/elements.hpp>
#include <string>

namespace pnana {
namespace utils {

/**
 * 在列表项文本中高亮与查询匹配的部分，用于文件选择器、命令面板、符号导航等输入框过滤场景。
 * 匹配为不区分大小写的子串匹配，匹配到的片段使用 highlight_color，其余使用 default_color。
 * @param text 原始显示文本
 * @param query 用户输入的过滤串（为空则不高亮）
 * @param default_color 默认文字颜色
 * @param highlight_color 匹配片段的主题高亮颜色（如 keyword / success）
 * @return FTXUI Element（hbox 或单个 text），可直接放入行布局
 */
ftxui::Element highlightMatch(const std::string& text, const std::string& query,
                              ftxui::Color default_color, ftxui::Color highlight_color);

} // namespace utils
} // namespace pnana

#endif // PNANA_UTILS_MATCH_HIGHLIGHT_H
