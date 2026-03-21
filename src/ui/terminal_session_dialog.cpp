#include "ui/terminal_session_dialog.h"
#include "ui/icons.h"
#include <ftxui/dom/elements.hpp>
#include <unistd.h>
#include <vector>

using namespace ftxui;

static inline Decorator borderWithColor(Color border_color) {
    return [=](Element child) -> Element {
        return child | border | ftxui::color(border_color);
    };
}

static inline Element padded(Element e) {
    return hbox({text(" "), std::move(e), text(" ")});
}

namespace {

struct ShellInfo {
    std::string path;
    std::string name;
    std::string description;
};

bool isExecutable(const std::string& path) {
    return access(path.c_str(), X_OK) == 0;
}

std::string getEnvShell() {
    const char* shell = getenv("SHELL");
    if (shell && shell[0] != '\0') {
        return shell;
    }
    return "";
}

std::vector<ShellInfo> detectAvailableShells() {
    std::vector<ShellInfo> available;

    std::vector<std::pair<std::string, std::string>> shell_candidates = {
        {"/bin/bash", "GNU Bash"},
        {"/usr/bin/bash", "GNU Bash (usr)"},
        {"/bin/zsh", "Z Shell"},
        {"/usr/bin/zsh", "Z Shell (usr)"},
        {"/usr/bin/fish", "Friendly Interactive Shell"},
        {"/bin/fish", "Friendly Interactive Shell"},
        {"/bin/dash", "Debian Almquist Shell"},
        {"/usr/bin/dash", "Debian Almquist Shell"},
        {"/bin/sh", "POSIX Shell"},
        {"/usr/bin/sh", "POSIX Shell"},
        {"/bin/ash", "Almquist Shell"},
        {"/usr/bin/ash", "Almquist Shell"},
        {"/bin/tsh", "Tenex C Shell"},
        {"/usr/bin/tsh", "Tenex C Shell"},
        {"/bin/csh", "C Shell"},
        {"/usr/bin/csh", "C Shell"},
    };

    for (const auto& candidate : shell_candidates) {
        if (isExecutable(candidate.first)) {
            ShellInfo info;
            info.path = candidate.first;
            info.name = candidate.second;
            info.description = candidate.first;
            available.push_back(info);
        }
    }

    return available;
}

} // namespace

namespace pnana {
namespace ui {

TerminalSessionDialog::TerminalSessionDialog(Theme& theme)
    : theme_(theme), visible_(false), selected_tab_(0), selected_index_(0) {
    detectLocalShells();
}

void TerminalSessionDialog::detectLocalShells() {
    local_choices_.clear();

    std::string env_shell = getEnvShell();
    if (!env_shell.empty()) {
        local_choices_.push_back(
            TerminalSessionChoice(SessionType::Local, std::string("Default (") + env_shell + ")",
                                  std::string("Use your login shell ($SHELL)"), std::string(),
                                  std::string(), std::string(), 0, std::string(), std::string()));
    } else {
        local_choices_.push_back(TerminalSessionChoice(SessionType::Local, std::string("Default"),
                                                       std::string("Use your login shell ($SHELL)"),
                                                       std::string(), std::string(), std::string(),
                                                       0, std::string(), std::string()));
    }

    std::vector<ShellInfo> available = detectAvailableShells();
    for (const auto& shell : available) {
        local_choices_.push_back(
            TerminalSessionChoice(SessionType::Local, shell.description, shell.name, shell.path,
                                  std::string(), std::string(), 0, std::string(), std::string()));
    }
}

void TerminalSessionDialog::resetChoices() {
    ssh_choices_.clear();
}

void TerminalSessionDialog::navigateTabs(bool next) {
    size_t tab_count = ssh_choices_.empty() ? 1 : 2;
    if (tab_count <= 1)
        return;

    if (next) {
        selected_tab_ = (selected_tab_ + 1) % tab_count;
    } else {
        selected_tab_ = (selected_tab_ + tab_count - 1) % tab_count;
    }
    selected_index_ = 0;
}

void TerminalSessionDialog::show(std::function<void(const TerminalSessionChoice&)> on_confirm,
                                 std::function<void()> on_cancel, const std::string& ssh_host,
                                 const std::string& ssh_user, int ssh_port,
                                 const std::string& ssh_key_path, const std::string& ssh_password) {
    visible_ = true;
    selected_tab_ = 0;
    selected_index_ = 0;

    ssh_choices_.clear();
    if (!ssh_host.empty()) {
        std::string env_shell = getEnvShell();
        if (!env_shell.empty()) {
            ssh_choices_.push_back(
                TerminalSessionChoice(SessionType::SSH, std::string("Default (") + env_shell + ")",
                                      std::string("Use login shell on ") + ssh_host, std::string(),
                                      ssh_host, ssh_user, ssh_port, ssh_key_path, ssh_password));
        } else {
            ssh_choices_.push_back(
                TerminalSessionChoice(SessionType::SSH, std::string("Default"),
                                      std::string("Use login shell on ") + ssh_host, std::string(),
                                      ssh_host, ssh_user, ssh_port, ssh_key_path, ssh_password));
        }

        std::vector<ShellInfo> available = detectAvailableShells();
        for (const auto& shell : available) {
            ssh_choices_.push_back(TerminalSessionChoice(
                SessionType::SSH, shell.description + " @ " + ssh_host, shell.name, shell.path,
                ssh_host, ssh_user, ssh_port, ssh_key_path, ssh_password));
        }
    }

    on_confirm_ = std::move(on_confirm);
    on_cancel_ = std::move(on_cancel);
}

bool TerminalSessionDialog::handleInput(ftxui::Event event) {
    if (!visible_) {
        return false;
    }

    if (event == Event::Escape) {
        visible_ = false;
        if (on_cancel_) {
            on_cancel_();
        }
        return true;
    }

    if (event == Event::Tab || event == Event::TabReverse) {
        bool next = (event == Event::Tab);
        navigateTabs(next);
        return true;
    }

    std::vector<TerminalSessionChoice>& current_choices =
        (selected_tab_ == 0 || ssh_choices_.empty()) ? local_choices_ : ssh_choices_;

    if (event == Event::ArrowUp) {
        if (selected_index_ > 0) {
            selected_index_--;
        }
        return true;
    }

    if (event == Event::ArrowDown) {
        if (selected_index_ + 1 < current_choices.size()) {
            selected_index_++;
        }
        return true;
    }

    if (event == Event::Return) {
        if (selected_index_ < current_choices.size() && on_confirm_) {
            on_confirm_(current_choices[selected_index_]);
        }
        visible_ = false;
        return true;
    }

    return false;
}

ftxui::Element TerminalSessionDialog::render() {
    if (!visible_) {
        return text("");
    }

    auto& colors = theme_.getColors();

    size_t tab_count = ssh_choices_.empty() ? 1 : 2;

    Elements header_parts = {
        text(" "),
        text(icons::TERMINAL) | color(colors.success),
        text(" New Terminal Session ") | bold | color(colors.dialog_title_fg),
    };

    if (tab_count == 2) {
        std::string local_label = "Local";
        std::string ssh_label = "SSH";

        Elements tabs;
        if (selected_tab_ == 0) {
            tabs.push_back(text("[") | color(colors.helpbar_key));
            tabs.push_back(text(local_label) | color(colors.helpbar_key) | bold | inverted);
            tabs.push_back(text("]") | color(colors.helpbar_key));
            tabs.push_back(text(" | ") | color(colors.comment));
            tabs.push_back(text("[") | color(colors.dialog_border) | dim);
            tabs.push_back(text(ssh_label) | color(colors.comment) | dim);
            tabs.push_back(text("]") | color(colors.dialog_border) | dim);
        } else {
            tabs.push_back(text("[") | color(colors.dialog_border) | dim);
            tabs.push_back(text(local_label) | color(colors.comment) | dim);
            tabs.push_back(text("]") | color(colors.dialog_border) | dim);
            tabs.push_back(text(" | ") | color(colors.comment));
            tabs.push_back(text("[") | color(colors.helpbar_key));
            tabs.push_back(text(ssh_label) | color(colors.helpbar_key) | bold | inverted);
            tabs.push_back(text("]") | color(colors.helpbar_key));
        }
        header_parts.push_back(filler());
        header_parts.insert(header_parts.end(), tabs.begin(), tabs.end());
    }

    header_parts.push_back(filler());
    header_parts.push_back(text("Enter") | color(colors.helpbar_key) | bold);
    header_parts.push_back(text(" Open  ") | color(colors.dialog_title_fg) | dim);
    header_parts.push_back(text("Esc") | color(colors.helpbar_key) | bold);
    header_parts.push_back(text(" Cancel ") | color(colors.dialog_title_fg) | dim);

    Element header = hbox(header_parts) | bgcolor(colors.dialog_title_bg);

    std::vector<TerminalSessionChoice>& current_choices =
        (selected_tab_ == 0 || ssh_choices_.empty()) ? local_choices_ : ssh_choices_;

    Elements choice_rows;
    choice_rows.reserve(current_choices.size());
    for (size_t i = 0; i < current_choices.size(); ++i) {
        const auto& choice = current_choices[i];
        bool selected = (i == selected_index_);

        Element row = hbox({
            text(selected ? "► " : "  ") | color(selected ? colors.success : colors.comment),
            text(choice.label) |
                (selected ? color(colors.foreground) | bold : color(colors.foreground)),
            filler(),
            text(choice.description) | color(colors.comment) | dim,
        });

        if (selected) {
            row = row | bgcolor(colors.selection);
        }
        choice_rows.push_back(padded(std::move(row)));
    }

    Element body = vbox(std::move(choice_rows)) | flex;

    Elements footer_parts = {
        text(" "),
        text("↑↓") | color(colors.helpbar_key) | bold,
        text(": Navigate  ") | color(colors.helpbar_fg) | dim,
    };

    if (tab_count == 2) {
        footer_parts.push_back(text("Tab") | color(colors.helpbar_key) | bold);
        footer_parts.push_back(text(": Switch Tab  ") | color(colors.helpbar_fg) | dim);
    }

    footer_parts.push_back(text("Enter") | color(colors.helpbar_key) | bold);
    footer_parts.push_back(text(": Open  ") | color(colors.helpbar_fg) | dim);
    footer_parts.push_back(text("Esc") | color(colors.helpbar_key) | bold);
    footer_parts.push_back(text(": Cancel") | color(colors.helpbar_fg) | dim);
    footer_parts.push_back(filler());

    Element footer = hbox(footer_parts) | bgcolor(colors.helpbar_bg);

    return window(text(" Terminal ") | color(colors.success) | bold, vbox({
                                                                         header,
                                                                         separator(),
                                                                         body,
                                                                         separator(),
                                                                         footer,
                                                                     })) |
           size(WIDTH, GREATER_THAN, 64) | size(HEIGHT, GREATER_THAN, 11) |
           bgcolor(colors.background) | borderWithColor(colors.dialog_border) | center;
}

} // namespace ui
} // namespace pnana