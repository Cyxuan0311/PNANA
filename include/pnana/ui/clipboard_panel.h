#ifndef PNANA_UI_CLIPBOARD_PANEL_H
#define PNANA_UI_CLIPBOARD_PANEL_H

#include "ui/theme.h"
#include "utils/clipboard_history.h"
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/elements.hpp>
#include <functional>
#include <string>

namespace pnana {
namespace ui {

class ClipboardPanel {
  public:
    ClipboardPanel(Theme& theme);
    ~ClipboardPanel() = default;

    ftxui::Element render();
    ftxui::Component getComponent();

    void show();
    void hide();
    bool isVisible() const {
        return visible_;
    }

    void addEntry(const std::string& content);
    utils::ClipboardHistory& getHistory() {
        return history_;
    }

    bool handleInput(ftxui::Event event);

    void setOnInsertText(std::function<void(const std::string&)> callback) {
        on_insert_text_ = callback;
    }

  private:
    ftxui::Element renderEntries();
    ftxui::Element renderEntry(const utils::ClipboardEntry& entry, bool is_selected, size_t index,
                               size_t total) const;
    ftxui::Element renderStats() const;
    ftxui::Element renderFooter();

    void moveUp();
    void moveDown();
    void insertSelected();
    void toggleSelect();
    void deleteSelected();
    void deleteAll();
    void resizePanel(int delta);

    Theme& theme_;
    bool visible_;
    utils::ClipboardHistory history_;
    int selected_index_;
    int scroll_offset_;
    int panel_width_;

    static constexpr int ENTRY_PREVIEW_LINES = 3;
    static constexpr int MAX_VISIBLE_ENTRIES = 15;

    std::function<void(const std::string&)> on_insert_text_;
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_CLIPBOARD_PANEL_H
