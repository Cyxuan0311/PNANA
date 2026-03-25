#ifndef PNANA_UI_HISTORY_DIFF_POPUP_H
#define PNANA_UI_HISTORY_DIFF_POPUP_H

#include "features/diff/myers_diff.h"
#include "ui/theme.h"
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <string>
#include <vector>

namespace pnana {
namespace ui {

class HistoryDiffPopup {
  public:
    explicit HistoryDiffPopup(Theme& theme);

    void open(const std::string& file_path, int from_version, int to_version,
              const std::vector<pnana::features::diff::DiffRecord>& records);
    void close();

    bool isOpen() const {
        return is_open_;
    }

    ftxui::Element render() const;
    bool handleInput(ftxui::Event event);

  private:
    Theme& theme_;
    bool is_open_ = false;
    std::string file_path_;
    int from_version_ = 0;
    int to_version_ = 0;
    std::vector<pnana::features::diff::DiffRecord> records_;

    size_t scroll_offset_ = 0;
    size_t page_size_ = 16;
    size_t horizontal_offset_ = 0;
    size_t horizontal_step_ = 24;

    ftxui::Element renderTitle() const;
    ftxui::Element renderList() const;
    ftxui::Element renderHelp() const;
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_HISTORY_DIFF_POPUP_H
