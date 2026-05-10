#include "utils/clipboard_history.h"
#include <algorithm>

namespace pnana {
namespace utils {

void ClipboardHistory::addEntry(const std::string& content) {
    if (content.empty())
        return;

    for (auto it = entries_.begin(); it != entries_.end(); ++it) {
        if (it->content == content) {
            entries_.erase(it);
            break;
        }
    }

    entries_.insert(entries_.begin(), ClipboardEntry(content));

    while (entries_.size() > MAX_ENTRIES) {
        entries_.pop_back();
    }
}

void ClipboardHistory::removeEntry(size_t index) {
    if (index < entries_.size()) {
        entries_.erase(entries_.begin() + static_cast<long>(index));
    }
}

void ClipboardHistory::removeSelectedEntries() {
    entries_.erase(std::remove_if(entries_.begin(), entries_.end(),
                                  [](const ClipboardEntry& e) {
                                      return e.selected;
                                  }),
                   entries_.end());
}

void ClipboardHistory::clear() {
    entries_.clear();
}

} // namespace utils
} // namespace pnana
