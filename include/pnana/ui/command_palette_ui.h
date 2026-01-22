#ifndef PNANA_UI_COMMAND_PALETTE_UI_H
#define PNANA_UI_COMMAND_PALETTE_UI_H

#include "ui/theme.h"
#include <ftxui/dom/elements.hpp>
#include <functional>
#include <string>
#include <vector>

namespace pnana {
namespace features {

// 前向声明Command结构体
struct Command;

} // namespace features

namespace ui {

// 命令面板UI类 - 负责渲染命令面板的UI界面
class CommandPaletteUI {
  public:
    explicit CommandPaletteUI(Theme& theme);

    // 设置需要渲染的数据
    void setData(bool is_open, const std::string& input,
                 const std::vector<features::Command>& filtered_commands, size_t selected_index,
                 size_t scroll_offset);

    // 渲染命令面板
    ftxui::Element render();

  private:
    Theme& theme_;
    bool is_open_;
    std::string input_;
    std::vector<features::Command> filtered_commands_;
    size_t selected_index_;
    size_t scroll_offset_;

    // 渲染各个组件
    ftxui::Element renderTitle() const;
    ftxui::Element renderInputBox() const;
    ftxui::Element renderCommandList() const;
    ftxui::Element renderHelpBar() const;
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_COMMAND_PALETTE_UI_H
