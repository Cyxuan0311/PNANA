#ifndef PNANA_UI_IMAGE_PROTOCOL_DIALOG_H
#define PNANA_UI_IMAGE_PROTOCOL_DIALOG_H

#include "ui/theme.h"
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <string>
#include <vector>

namespace pnana {
namespace ui {

class ImageProtocolDialog {
  public:
    explicit ImageProtocolDialog(Theme& theme);
    void open();
    void close();
    bool isVisible() const {
        return visible_;
    }
    bool handleInput(ftxui::Event event);
    ftxui::Element render();

    bool isProtocolEnabled() const {
        return protocol_enabled_;
    }
    std::string getPreferredProtocol() const {
        return preferred_protocol_;
    }

    void setProtocolEnabled(bool on) {
        protocol_enabled_ = on;
    }
    void setPreferredProtocol(const std::string& pref) {
        preferred_protocol_ = pref;
    }

    void setOnApply(std::function<void()> callback) {
        on_apply_ = callback;
    }

  private:
    Theme& theme_;
    bool visible_;
    bool protocol_enabled_;
    std::string preferred_protocol_;
    size_t selected_option_;

    std::function<void()> on_apply_;

    struct ProtocolStatus {
        std::string name;
        std::string display;
        bool available;
        bool active;
    };
    std::vector<ProtocolStatus> protocols_;

    ftxui::Element renderHeader();
    ftxui::Element renderToggle();
    ftxui::Element renderProtocolList();
    ftxui::Element renderPreferredSelector();
    ftxui::Element renderHelpBar();

    void selectNext();
    void selectPrevious();
    void apply();
    void refreshProtocolStatus();
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_IMAGE_PROTOCOL_DIALOG_H
