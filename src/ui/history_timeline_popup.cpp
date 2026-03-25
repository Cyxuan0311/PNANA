#include "ui/history_timeline_popup.h"
#include "ui/icons.h"
#include "utils/logger.h"
#include <algorithm>
#include <ctime>
#include <filesystem>
#include <ftxui/dom/elements.hpp>
#include <sstream>

using namespace ftxui;

namespace {
static inline Decorator borderWithColor(Color border_color) {
    return [=](Element child) -> Element {
        return child | border | ftxui::color(border_color);
    };
}

std::string formatTs(long long ms) {
    std::time_t t = static_cast<std::time_t>(ms / 1000);
    std::tm tmv{};
#ifdef _WIN32
    localtime_s(&tmv, &t);
#else
    localtime_r(&t, &tmv);
#endif
    std::ostringstream oss;
    oss << (tmv.tm_year + 1900) << "-" << (tmv.tm_mon + 1) << "-" << tmv.tm_mday << " "
        << tmv.tm_hour << ":" << tmv.tm_min << ":" << tmv.tm_sec;
    return oss.str();
}
} // namespace

namespace pnana {
namespace ui {

HistoryTimelinePopup::HistoryTimelinePopup(Theme& theme) : theme_(theme) {}

void HistoryTimelinePopup::open(const std::string& file_path,
                                const std::vector<features::history::VersionMeta>& versions) {
    file_path_ = file_path;
    versions_ = versions;
    selected_index_ = 0;
    is_open_ = true;
}

void HistoryTimelinePopup::close() {
    is_open_ = false;
    versions_.clear();
    selected_index_ = 0;
}

void HistoryTimelinePopup::setOnPreview(std::function<void(int)> callback) {
    on_preview_ = std::move(callback);
}

void HistoryTimelinePopup::setOnRollback(std::function<void(int)> callback) {
    on_rollback_ = std::move(callback);
}

int HistoryTimelinePopup::selectedVersion() const {
    if (selected_index_ >= versions_.size()) {
        return 0;
    }
    return versions_[selected_index_].version;
}

Element HistoryTimelinePopup::renderTitle() const {
    const auto& colors = theme_.getColors();
    std::string filename = file_path_;
    try {
        filename = std::filesystem::path(file_path_).filename().string();
    } catch (...) {
    }

    return hbox({text(" "), text(pnana::ui::icons::GIT_HISTORY) | color(colors.success),
                 text(" History Timeline - " + filename + " "), text(" ")}) |
           bold | bgcolor(colors.dialog_title_bg) | color(colors.dialog_title_fg) | center;
}

Element HistoryTimelinePopup::renderVersionList() const {
    const auto& colors = theme_.getColors();
    Elements rows;

    if (versions_.empty()) {
        rows.push_back(text("  No history versions") | color(colors.comment) | dim);
        return vbox(std::move(rows));
    }

    size_t max_items = std::min<size_t>(versions_.size(), 12);
    for (size_t i = 0; i < max_items; ++i) {
        const auto& v = versions_[i];
        bool selected = (i == selected_index_);

        Element row = hbox({
            text(selected ? "► " : "  ") | color(colors.success) | bold,
            text("v" + std::to_string(v.version)) |
                (selected ? color(colors.dialog_fg) | bold : color(colors.keyword)),
            filler(),
            text(formatTs(v.timestamp)) |
                (selected ? color(colors.dialog_fg) : color(colors.comment)),
            text("  "),
        });

        if (selected) {
            row = row | bgcolor(colors.selection);
        }
        rows.push_back(row);
    }

    return vbox(std::move(rows));
}

Element HistoryTimelinePopup::renderHelp() const {
    const auto& colors = theme_.getColors();
    return hbox({text("  "), text("↑↓") | color(colors.helpbar_key) | bold, text(": Select  "),
                 text("P") | color(colors.helpbar_key) | bold, text(": Preview  "),
                 text("Enter") | color(colors.helpbar_key) | bold, text(": Rollback  "),
                 text("Esc") | color(colors.helpbar_key) | bold, text(": Close")}) |
           bgcolor(colors.helpbar_bg) | color(colors.helpbar_fg) | dim;
}

Element HistoryTimelinePopup::render() {
    if (!is_open_) {
        return text("");
    }

    const auto& colors = theme_.getColors();
    Elements content;
    content.push_back(renderTitle());
    content.push_back(separator());
    content.push_back(renderVersionList());
    content.push_back(separator());
    content.push_back(renderHelp());

    int h = std::min<int>(22, 10 + static_cast<int>(versions_.size()));

    return window(text(""), vbox(std::move(content))) | size(WIDTH, EQUAL, 72) |
           size(HEIGHT, EQUAL, h) | bgcolor(colors.dialog_bg) |
           borderWithColor(colors.dialog_border);
}

bool HistoryTimelinePopup::handleInput(ftxui::Event event) {
    if (!is_open_) {
        return false;
    }

    if (event == Event::Escape) {
        close();
        return true;
    }

    if (versions_.empty()) {
        return true;
    }

    if (event == Event::ArrowDown) {
        selected_index_ = (selected_index_ + 1) % versions_.size();
        return true;
    }

    if (event == Event::ArrowUp) {
        if (selected_index_ == 0) {
            selected_index_ = versions_.size() - 1;
        } else {
            selected_index_--;
        }
        return true;
    }

    if (event == Event::Return) {
        if (on_rollback_) {
            on_rollback_(selectedVersion());
        }
        return true;
    }

    if (event.is_character()) {
        std::string ch = event.character();
        if (ch == "p" || ch == "P") {
            if (on_preview_) {
                on_preview_(selectedVersion());
            }
            return true;
        }
    }

    return false;
}

} // namespace ui
} // namespace pnana
