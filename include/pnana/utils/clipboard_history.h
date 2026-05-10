#ifndef PNANA_UTILS_CLIPBOARD_HISTORY_H
#define PNANA_UTILS_CLIPBOARD_HISTORY_H

#include <chrono>
#include <cstddef>
#include <string>
#include <vector>

namespace pnana {
namespace utils {

struct ClipboardEntry {
    std::string content;
    std::string timestamp;
    std::string preview;
    bool selected = false;

    ClipboardEntry(const std::string& c) : content(c) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto tm = *std::localtime(&time_t);
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
        timestamp = buf;

        size_t len = std::min(c.size(), size_t(60));
        preview = c.substr(0, len);
        size_t pos = 0;
        while ((pos = preview.find('\n', pos)) != std::string::npos) {
            preview.replace(pos, 1, "\\n");
            pos += 2;
        }
        if (c.size() > 60) {
            preview += "...";
        }
    }
};

class ClipboardHistory {
  public:
    ClipboardHistory() = default;

    void addEntry(const std::string& content);
    const std::vector<ClipboardEntry>& getEntries() const {
        return entries_;
    }
    std::vector<ClipboardEntry>& getEntries() {
        return entries_;
    }
    void removeEntry(size_t index);
    void removeSelectedEntries();
    void clear();
    size_t size() const {
        return entries_.size();
    }
    bool empty() const {
        return entries_.empty();
    }

    static constexpr size_t MAX_ENTRIES = 100;

  private:
    std::vector<ClipboardEntry> entries_;
};

} // namespace utils
} // namespace pnana

#endif // PNANA_UTILS_CLIPBOARD_HISTORY_H
