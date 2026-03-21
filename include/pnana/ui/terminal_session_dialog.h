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

enum class SessionType { Local, SSH };

struct TerminalSessionChoice {
    SessionType type;
    std::string label;
    std::string description;
    std::string shell_path;
    std::string host;
    std::string user;
    int port;
    std::string key_path;
    std::string password;

    TerminalSessionChoice(SessionType t = SessionType::Local, const std::string& l = "",
                          const std::string& d = "", const std::string& sp = "",
                          const std::string& h = "", const std::string& u = "", int p = 0,
                          const std::string& kp = "", const std::string& pw = "")
        : type(t), label(l), description(d), shell_path(sp), host(h), user(u), port(p),
          key_path(kp), password(pw) {}
};

class TerminalSessionDialog {
  public:
    explicit TerminalSessionDialog(Theme& theme);

    void show(std::function<void(const TerminalSessionChoice&)> on_confirm,
              std::function<void()> on_cancel, const std::string& ssh_host = "",
              const std::string& ssh_user = "", int ssh_port = 0,
              const std::string& ssh_key_path = "", const std::string& ssh_password = "");
    bool handleInput(ftxui::Event event);
    ftxui::Element render();

    bool isVisible() const {
        return visible_;
    }

  private:
    void resetChoices();
    void detectLocalShells();
    void navigateTabs(bool next);

    Theme& theme_;
    bool visible_;
    size_t selected_tab_;
    size_t selected_index_;
    std::vector<TerminalSessionChoice> local_choices_;
    std::vector<TerminalSessionChoice> ssh_choices_;
    std::function<void(const TerminalSessionChoice&)> on_confirm_;
    std::function<void()> on_cancel_;
};

} // namespace ui
} // namespace pnana

#endif