#ifndef PNANA_UI_HISTORY_TIMELINE_POPUP_H
#define PNANA_UI_HISTORY_TIMELINE_POPUP_H

#include "features/history/file_history_manager.h"
#include "ui/theme.h"
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <functional>
#include <string>
#include <vector>

namespace pnana {
namespace ui {

class HistoryTimelinePopup {
  public:
    explicit HistoryTimelinePopup(Theme& theme);

    void open(const std::string& file_path,
              const std::vector<features::history::VersionMeta>& versions);
    void close();
    bool isOpen() const {
        return is_open_;
    }

    ftxui::Element render();
    bool handleInput(ftxui::Event event);

    void setOnPreview(std::function<void(int)> callback);
    void setOnRollback(std::function<void(int)> callback);

    int selectedVersion() const;

  private:
    Theme& theme_;
    bool is_open_ = false;
    std::string file_path_;
    std::vector<features::history::VersionMeta> versions_;
    size_t selected_index_ = 0;
    size_t scroll_offset_ = 0;
    size_t list_display_count_ = 12;

    std::function<void(int)> on_preview_;
    std::function<void(int)> on_rollback_;

    ftxui::Element renderTitle() const;
    ftxui::Element renderVersionList() const;
    ftxui::Element renderHelp() const;
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_HISTORY_TIMELINE_POPUP_H
