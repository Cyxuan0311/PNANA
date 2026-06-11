#ifndef PNANA_UI_DEPENDENCY_STATUS_POPUP_H
#define PNANA_UI_DEPENDENCY_STATUS_POPUP_H

#include "ui/theme.h"
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <string>
#include <vector>

namespace pnana {
namespace ui {

struct DependencyEntry {
    std::string name;
    std::string version;
    std::string type;     // Required / Optional / Bundled / Build
    bool enabled = false; // enabled at compile time
    std::string description;
};

class DependencyStatusPopup {
  public:
    explicit DependencyStatusPopup(Theme& theme);

    void open();
    void close();
    bool isOpen() const {
        return is_open_;
    }

    void setData(const std::vector<DependencyEntry>& entries);

    bool handleInput(ftxui::Event event);
    ftxui::Element render();

  private:
    Theme& theme_;
    bool is_open_ = false;
    std::vector<DependencyEntry> entries_;
    size_t selected_index_ = 0;
    size_t scroll_offset_ = 0;
    static constexpr size_t display_count_ = 12;

    ftxui::Element renderTitle() const;
    ftxui::Element renderTable() const;
    ftxui::Element renderHelpBar() const;
};

} // namespace ui
} // namespace pnana

#endif
