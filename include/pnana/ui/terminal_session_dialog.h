#ifndef PNANA_UI_TERMINAL_SESSION_DIALOG_H
#define PNANA_UI_TERMINAL_SESSION_DIALOG_H

#include "ui/theme.h"
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <functional>
#include <string>
#include <vector>

namespace pnana {
namespace ui {

struct TerminalSessionChoice {
    std::string label;
    std::string description;
    std::string shell_path; // empty means use env $SHELL
};

class TerminalSessionDialog {
  public:
    explicit TerminalSessionDialog(Theme& theme);

    void show(std::function<void(const TerminalSessionChoice&)> on_confirm,
              std::function<void()> on_cancel);
    bool handleInput(ftxui::Event event);
    ftxui::Element render();

    bool isVisible() const {
        return visible_;
    }

  private:
    void resetChoices();

    Theme& theme_;
    bool visible_;
    size_t selected_index_;
    std::vector<TerminalSessionChoice> choices_;
    std::function<void(const TerminalSessionChoice&)> on_confirm_;
    std::function<void()> on_cancel_;
};

} // namespace ui
} // namespace pnana

#endif
