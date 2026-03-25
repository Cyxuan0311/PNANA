#ifdef BUILD_LUA_SUPPORT

#include "core/editor.h"

namespace pnana {
namespace core {

void Editor::showConfirmDialogForLua(const std::string& title, const std::string& message,
                                     std::function<void(bool)> on_result) {
    if (popup_manager_) {
        popup_manager_->openConfirm(title, message, [this, on_result](bool ok) {
            if (on_result) {
                on_result(ok);
            }
            setStatusMessage(ok ? "Dialog confirmed" : "Dialog cancelled");
        });
        return;
    }

    dialog_.showConfirm(
        title, message,
        [this, on_result]() {
            if (on_result) {
                on_result(true);
            }
            setStatusMessage("Dialog confirmed");
        },
        [this, on_result]() {
            if (on_result) {
                on_result(false);
            }
            setStatusMessage("Dialog cancelled");
        });
}

void Editor::showInputDialogForLua(const std::string& title, const std::string& prompt,
                                   const std::string& default_value,
                                   std::function<void(bool, const std::string&)> on_result) {
    if (popup_manager_) {
        popup_manager_->openInput(title, prompt, default_value,
                                  [this, on_result](bool ok, const std::string& value) {
                                      if (on_result) {
                                          on_result(ok, value);
                                      }
                                      setStatusMessage(ok ? "Input confirmed" : "Input cancelled");
                                  });
        return;
    }

    dialog_.showInput(
        title, prompt, default_value,
        [this, on_result](const std::string& value) {
            if (on_result) {
                on_result(true, value);
            }
            setStatusMessage("Input confirmed");
        },
        [this, on_result]() {
            if (on_result) {
                on_result(false, "");
            }
            setStatusMessage("Input cancelled");
        });
}

void Editor::showSelectDialogForLua(const std::string& title, const std::string& prompt,
                                    const std::vector<std::string>& items,
                                    std::function<void(bool, size_t)> on_result) {
    if (items.empty()) {
        if (on_result) {
            on_result(false, 0);
        }
        setStatusMessage("Select cancelled: no items");
        return;
    }

    if (popup_manager_) {
        popup_manager_->openSelect(title, prompt, items, [this, on_result](bool ok, size_t idx) {
            if (on_result) {
                on_result(ok, idx);
            }
            setStatusMessage(ok ? "Select confirmed" : "Select cancelled");
        });
        return;
    }

    std::string message = prompt + "\n\n";
    for (size_t i = 0; i < items.size(); ++i) {
        message += std::to_string(i + 1) + ". " + items[i] + "\n";
    }
    message += "\nConfirm = first item, Cancel = no selection";

    dialog_.showConfirm(
        title, message,
        [this, on_result]() {
            if (on_result) {
                on_result(true, 1);
            }
            setStatusMessage("Select confirmed");
        },
        [this, on_result]() {
            if (on_result) {
                on_result(false, 0);
            }
            setStatusMessage("Select cancelled");
        });
}

} // namespace core
} // namespace pnana

#endif // BUILD_LUA_SUPPORT
