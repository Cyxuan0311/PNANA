#include "ui/ssh_dialog.h"
#include "features/cursor/cursor_renderer.h"
#include "ui/icons.h"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <ftxui/dom/elements.hpp>
#include <nlohmann/json.hpp>
#include <sstream>

using namespace ftxui;
using json = nlohmann::json;

static inline Decorator borderWithColor(Color border_color) {
    return [=](Element child) -> Element {
        return child | border | ftxui::color(border_color);
    };
}

namespace pnana {
namespace ui {

// ─────────────────────────────────────────────────────────────────────────────
// 构造 / show / reset
// ─────────────────────────────────────────────────────────────────────────────

SSHDialog::SSHDialog(Theme& theme)
    : theme_(theme), visible_(false), mode_(SSHDialogMode::HISTORY), current_field_(0),
      cursor_position_(0) {
    loadHistory();
}

void SSHDialog::show(std::function<void(const SSHConfig&)> on_confirm,
                     std::function<void()> on_cancel, const SSHConfig* current_connection,
                     std::function<void()> on_disconnect) {
    visible_ = true;
    on_confirm_ = std::move(on_confirm);
    on_cancel_ = std::move(on_cancel);
    on_disconnect_ = std::move(on_disconnect);
    current_connection_ = current_connection;

    // 已连接状态
    if (current_connection_ && !current_connection_->host.empty()) {
        mode_ = SSHDialogMode::CONNECTED;
        return;
    }

    // 有历史时先展示历史列表
    if (!history_.empty()) {
        mode_ = SSHDialogMode::HISTORY;
        history_index_ = 0;
    } else {
        mode_ = SSHDialogMode::NEW_FORM;
    }

    // 重置表单
    host_input_.clear();
    user_input_.clear();
    password_input_.clear();
    key_path_input_.clear();
    port_input_ = "22";
    remote_path_input_.clear();
    current_field_ = 0;
    cursor_position_ = 0;
    history_password_input_.clear();
    history_password_cursor_ = 0;
}

void SSHDialog::reset() {
    visible_ = false;
    mode_ = SSHDialogMode::HISTORY;
    current_field_ = 0;
    cursor_position_ = 0;
    host_input_.clear();
    user_input_.clear();
    password_input_.clear();
    key_path_input_.clear();
    port_input_ = "22";
    remote_path_input_.clear();
    history_password_input_.clear();
    history_password_cursor_ = 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// 历史持久化
// ─────────────────────────────────────────────────────────────────────────────

std::string SSHDialog::getHistoryFilePath() const {
    const char* home = std::getenv("HOME");
    std::string base = home ? std::string(home) + "/.config/pnana" : "/tmp";
    return base + "/ssh_history.json";
}

void SSHDialog::loadHistory() {
    history_.clear();
    std::string path = getHistoryFilePath();
    std::ifstream ifs(path);
    if (!ifs.is_open())
        return;
    try {
        json j;
        ifs >> j;
        if (!j.is_array())
            return;
        for (const auto& item : j) {
            SSHHistoryEntry e;
            e.host = item.value("host", "");
            e.user = item.value("user", "");
            e.port = item.value("port", 22);
            e.key_path = item.value("key_path", "");
            e.remote_path = item.value("remote_path", "");
            if (!e.host.empty())
                history_.push_back(std::move(e));
        }
    } catch (...) {
        history_.clear();
    }
}

void SSHDialog::saveHistory() const {
    std::string path = getHistoryFilePath();
    try {
        std::filesystem::create_directories(std::filesystem::path(path).parent_path());
        json j = json::array();
        for (const auto& e : history_) {
            j.push_back({{"host", e.host},
                         {"user", e.user},
                         {"port", e.port},
                         {"key_path", e.key_path},
                         {"remote_path", e.remote_path}});
        }
        std::ofstream ofs(path);
        ofs << j.dump(2);
    } catch (...) {
    }
}

void SSHDialog::pushHistory(const SSHConfig& config) {
    if (config.host.empty())
        return;
    // 若已存在相同 host+user+port 则先移除旧条目
    history_.erase(std::remove_if(history_.begin(), history_.end(),
                                  [&](const SSHHistoryEntry& e) {
                                      return e.host == config.host && e.user == config.user &&
                                             e.port == config.port;
                                  }),
                   history_.end());
    // 插到最前
    SSHHistoryEntry entry;
    entry.host = config.host;
    entry.user = config.user;
    entry.port = config.port;
    entry.key_path = config.key_path;
    entry.remote_path = config.remote_path;
    history_.insert(history_.begin(), std::move(entry));
    // 限制最多 kMaxHistory 条
    if ((int)history_.size() > kMaxHistory)
        history_.resize(kMaxHistory);
    saveHistory();
}

// ─────────────────────────────────────────────────────────────────────────────
// handleInput 分发
// ─────────────────────────────────────────────────────────────────────────────

bool SSHDialog::handleInput(Event event) {
    if (!visible_)
        return false;
    switch (mode_) {
        case SSHDialogMode::HISTORY:
            return handleHistoryInput(event);
        case SSHDialogMode::NEW_FORM:
            return handleNewFormInput(event);
        case SSHDialogMode::PASSWORD:
            return handlePasswordInput(event);
        case SSHDialogMode::CONNECTED:
            return handleConnectedInput(event);
    }
    return false;
}

// ── HISTORY ──────────────────────────────────────────────────────────────────

bool SSHDialog::handleHistoryInput(Event event) {
    if (event == Event::Escape) {
        visible_ = false;
        if (on_cancel_)
            on_cancel_();
        return true;
    }
    if (event == Event::ArrowUp) {
        if (history_index_ > 0)
            history_index_--;
        return true;
    }
    if (event == Event::ArrowDown) {
        if (history_index_ < (int)history_.size() - 1)
            history_index_++;
        return true;
    }
    // Enter：从历史项连接，先要求输入密码
    if (event == Event::Return) {
        if (!history_.empty()) {
            history_password_input_.clear();
            history_password_cursor_ = 0;
            mode_ = SSHDialogMode::PASSWORD;
        }
        return true;
    }
    // n 或 Tab：切换到新建连接表单
    if (event == Event::Character('n') || event == Event::Tab) {
        mode_ = SSHDialogMode::NEW_FORM;
        host_input_.clear();
        user_input_.clear();
        password_input_.clear();
        key_path_input_.clear();
        port_input_ = "22";
        remote_path_input_.clear();
        current_field_ = 0;
        cursor_position_ = 0;
        return true;
    }
    // Delete：删除选中的历史条目
    if (event == Event::Delete) {
        if (!history_.empty()) {
            history_.erase(history_.begin() + history_index_);
            if (history_index_ >= (int)history_.size() && history_index_ > 0)
                history_index_--;
            saveHistory();
        }
        return true;
    }
    return true;
}

// ── NEW_FORM ──────────────────────────────────────────────────────────────────

bool SSHDialog::handleNewFormInput(Event event) {
    if (event == Event::Escape) {
        // 有历史时退回到历史列表，否则直接关闭
        if (!history_.empty()) {
            mode_ = SSHDialogMode::HISTORY;
        } else {
            visible_ = false;
            if (on_cancel_)
                on_cancel_();
        }
        return true;
    }
    if (event == Event::Return) {
        if (current_field_ < 5) {
            moveToNextField();
        } else {
            SSHConfig cfg = buildConfig();
            visible_ = false;
            pushHistory(cfg);
            if (on_confirm_)
                on_confirm_(cfg);
        }
        return true;
    }
    if (event == Event::Tab) {
        moveToNextField();
        return true;
    }
    if (event == Event::TabReverse) {
        moveToPreviousField();
        return true;
    }
    if (event == Event::ArrowUp) {
        moveToPreviousField();
        return true;
    }
    if (event == Event::ArrowDown) {
        moveToNextField();
        return true;
    }
    if (event == Event::ArrowLeft) {
        moveCursorLeft();
        return true;
    }
    if (event == Event::ArrowRight) {
        moveCursorRight();
        return true;
    }
    if (event == Event::Backspace) {
        backspace();
        return true;
    }
    if (event == Event::Delete) {
        deleteChar();
        return true;
    }
    if (event == Event::Home) {
        cursor_position_ = 0;
        return true;
    }
    if (event == Event::End) {
        std::string* field = getCurrentField();
        if (field)
            cursor_position_ = field->length();
        return true;
    }
    if (event.is_character()) {
        std::string ch = event.character();
        if (ch.length() == 1) {
            char c = ch[0];
            if (current_field_ == 4) { // 端口只接受数字
                if (c >= '0' && c <= '9')
                    insertChar(c);
            } else if (c >= 32 && c < 127) {
                insertChar(c);
            }
        }
        return true;
    }
    return false;
}

// ── PASSWORD ──────────────────────────────────────────────────────────────────

bool SSHDialog::handlePasswordInput(Event event) {
    if (event == Event::Escape) {
        // 退回历史列表
        mode_ = SSHDialogMode::HISTORY;
        history_password_input_.clear();
        return true;
    }
    if (event == Event::Return) {
        if (history_index_ < (int)history_.size()) {
            const SSHHistoryEntry& e = history_[history_index_];
            SSHConfig cfg;
            cfg.host = e.host;
            cfg.user = e.user;
            cfg.port = e.port;
            cfg.key_path = e.key_path;
            cfg.remote_path = e.remote_path;
            cfg.password = history_password_input_;
            visible_ = false;
            // 更新历史顺序（最近使用排最前）
            pushHistory(cfg);
            if (on_confirm_)
                on_confirm_(cfg);
        }
        return true;
    }
    if (event == Event::Backspace) {
        if (history_password_cursor_ > 0) {
            history_password_input_.erase(history_password_cursor_ - 1, 1);
            history_password_cursor_--;
        }
        return true;
    }
    if (event == Event::Delete) {
        if (history_password_cursor_ < history_password_input_.size()) {
            history_password_input_.erase(history_password_cursor_, 1);
        }
        return true;
    }
    if (event == Event::ArrowLeft) {
        if (history_password_cursor_ > 0)
            history_password_cursor_--;
        return true;
    }
    if (event == Event::ArrowRight) {
        if (history_password_cursor_ < history_password_input_.size())
            history_password_cursor_++;
        return true;
    }
    if (event == Event::Home) {
        history_password_cursor_ = 0;
        return true;
    }
    if (event == Event::End) {
        history_password_cursor_ = history_password_input_.size();
        return true;
    }
    if (event.is_character()) {
        std::string ch = event.character();
        if (ch.length() == 1 && (unsigned char)ch[0] >= 32 && ch[0] != 127) {
            history_password_input_.insert(history_password_cursor_, 1, ch[0]);
            history_password_cursor_++;
        }
        return true;
    }
    return true;
}

// ── CONNECTED ────────────────────────────────────────────────────────────────

bool SSHDialog::handleConnectedInput(Event event) {
    if (event == Event::Escape) {
        visible_ = false;
        if (on_cancel_)
            on_cancel_();
        return true;
    }
    if (event == Event::Delete) {
        visible_ = false;
        if (on_disconnect_)
            on_disconnect_();
        return true;
    }
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// render 分发
// ─────────────────────────────────────────────────────────────────────────────

Element SSHDialog::render() {
    if (!visible_)
        return text("");
    switch (mode_) {
        case SSHDialogMode::HISTORY:
            return renderHistoryView();
        case SSHDialogMode::NEW_FORM:
            return renderNewFormView();
        case SSHDialogMode::PASSWORD:
            return renderPasswordView();
        case SSHDialogMode::CONNECTED:
            return renderConnectedView();
    }
    return text("");
}

// ── HISTORY 视图 ──────────────────────────────────────────────────────────────

Element SSHDialog::renderHistoryView() {
    auto& colors = theme_.getColors();
    Elements rows;

    // 标题
    rows.push_back(hbox({text(icons::TERMINAL) | color(Color::Cyan),
                         text(" SSH — Recent Connections ") | color(colors.foreground) | bold}) |
                   center);
    rows.push_back(separator());
    rows.push_back(text(""));

    if (history_.empty()) {
        rows.push_back(text("  No saved connections.") | color(colors.comment) | dim);
        rows.push_back(text("  Press 'n' to create a new connection.") | color(colors.comment));
    } else {
        for (int i = 0; i < (int)history_.size(); ++i) {
            bool selected = (i == history_index_);
            std::string label = "  " + std::to_string(i + 1) + ".  " + history_[i].displayLabel();
            if (!history_[i].remote_path.empty())
                label += "  (" + history_[i].remote_path + ")";

            Element row = text(label);
            if (selected) {
                row = text(label) | color(colors.background) | bgcolor(colors.function) | bold;
            } else {
                row = text(label) | color(colors.foreground);
            }
            rows.push_back(row);
        }
    }

    rows.push_back(text(""));
    rows.push_back(separator());
    rows.push_back(hbox({text("↑↓") | color(colors.keyword) | bold, text(": Select  "),
                         text("Enter") | color(colors.keyword) | bold, text(": Connect  "),
                         text("n") | color(colors.keyword) | bold, text(": New  "),
                         text("Del") | color(colors.keyword) | bold, text(": Remove  "),
                         text("Esc") | color(colors.keyword) | bold, text(": Cancel")}) |
                   color(colors.comment) | center);

    return window(text("SSH Connection"), vbox(rows)) | size(WIDTH, GREATER_THAN, 60) |
           size(HEIGHT, GREATER_THAN, 14) | bgcolor(colors.background) |
           borderWithColor(colors.dialog_border);
}

// ── NEW_FORM 视图 ─────────────────────────────────────────────────────────────

Element SSHDialog::renderNewFormView() {
    auto& colors = theme_.getColors();
    Elements fields;

    fields.push_back(hbox({text(icons::TERMINAL) | color(Color::Cyan),
                           text(" SSH — New Connection ") | color(colors.foreground) | bold}) |
                     center);
    fields.push_back(separator());
    fields.push_back(text(""));

    CursorRenderer cursor_renderer;
    cursor_renderer.setConfig(cursor_config_);

    auto renderField = [&](const std::string& label, std::string& value, size_t field_idx,
                           bool is_password = false) -> Element {
        bool is_focused = (current_field_ == field_idx);
        std::string display_value = is_password ? std::string(value.length(), '*') : value;
        Elements fe;
        fe.push_back(text(label + ": ") | color(colors.comment) | size(WIDTH, EQUAL, 15));
        size_t cursor_pos = is_focused ? cursor_position_ : display_value.length();
        if (is_focused) {
            if (cursor_pos > display_value.length())
                cursor_pos = display_value.length();
            std::string before = display_value.substr(0, cursor_pos);
            std::string cur_ch =
                cursor_pos < display_value.length() ? display_value.substr(cursor_pos, 1) : " ";
            std::string after =
                cursor_pos < display_value.length() ? display_value.substr(cursor_pos + 1) : "";
            fe.push_back(hbox({text(before) | color(colors.foreground),
                               cursor_renderer.renderCursorElement(
                                   cur_ch, cursor_pos, display_value.length(), colors.foreground,
                                   colors.background),
                               text(after) | color(colors.foreground)}) |
                         bgcolor(colors.current_line));
        } else {
            fe.push_back(text(display_value.empty() ? "(empty)" : display_value) |
                         color(display_value.empty() ? colors.comment : colors.foreground) | dim);
        }
        return hbox(fe);
    };

    fields.push_back(renderField("Host", host_input_, 0));
    fields.push_back(renderField("User", user_input_, 1));
    fields.push_back(renderField("Port", port_input_, 4));
    fields.push_back(renderField("Password", password_input_, 2, true));
    fields.push_back(renderField("Key Path", key_path_input_, 3));
    fields.push_back(renderField("Remote Path", remote_path_input_, 5));

    fields.push_back(text(""));
    fields.push_back(separator());
    fields.push_back(hbox({text("↑↓/Tab") | color(colors.keyword) | bold, text(": Navigate  "),
                           text("Enter") | color(colors.keyword) | bold, text(": Connect  "),
                           text("Esc") | color(colors.keyword) | bold,
                           text(!history_.empty() ? ": Back to history" : ": Cancel")}) |
                     color(colors.comment) | center);

    return window(text("SSH Connection"), vbox(fields)) | size(WIDTH, GREATER_THAN, 70) |
           size(HEIGHT, GREATER_THAN, 20) | bgcolor(colors.background) |
           borderWithColor(colors.dialog_border);
}

// ── PASSWORD 视图 ─────────────────────────────────────────────────────────────

Element SSHDialog::renderPasswordView() {
    auto& colors = theme_.getColors();
    Elements rows;

    rows.push_back(hbox({text(icons::TERMINAL) | color(Color::Cyan),
                         text(" SSH — Enter Password ") | color(colors.foreground) | bold}) |
                   center);
    rows.push_back(separator());
    rows.push_back(text(""));

    // 显示选中的连接信息
    if (history_index_ < (int)history_.size()) {
        const auto& e = history_[history_index_];
        rows.push_back(hbox({text("  Connecting to: ") | color(colors.comment),
                             text(e.displayLabel()) | color(colors.function) | bold}));
        if (!e.remote_path.empty()) {
            rows.push_back(hbox({text("  Remote path:   ") | color(colors.comment),
                                 text(e.remote_path) | color(colors.foreground)}));
        }
        if (!e.key_path.empty()) {
            rows.push_back(hbox({text("  Key path:      ") | color(colors.comment),
                                 text(e.key_path) | color(colors.foreground)}));
        }
    }

    rows.push_back(text(""));

    // 密码输入框
    {
        CursorRenderer pwd_cursor;
        pwd_cursor.setConfig(cursor_config_);

        std::string masked = std::string(history_password_input_.size(), '*');
        size_t cp = history_password_cursor_;
        if (cp > masked.size())
            cp = masked.size();
        std::string before = masked.substr(0, cp);
        std::string cur_ch = cp < masked.size() ? masked.substr(cp, 1) : " ";
        std::string after = cp < masked.size() ? masked.substr(cp + 1) : "";

        rows.push_back(
            hbox({text("  Password:      ") | color(colors.comment),
                  hbox({text(before) | color(colors.foreground),
                        pwd_cursor.renderCursorElement(cur_ch, cp, masked.size(), colors.foreground,
                                                       colors.background),
                        text(after) | color(colors.foreground)}) |
                      bgcolor(colors.current_line)}));
    }

    rows.push_back(text(""));
    rows.push_back(separator());
    rows.push_back(hbox({text("Enter") | color(colors.keyword) | bold, text(": Connect  "),
                         text("Esc") | color(colors.keyword) | bold, text(": Back to list")}) |
                   color(colors.comment) | center);

    return window(text("SSH Connection"), vbox(rows)) | size(WIDTH, GREATER_THAN, 60) |
           size(HEIGHT, GREATER_THAN, 14) | bgcolor(colors.background) |
           borderWithColor(colors.dialog_border);
}

// ── CONNECTED 视图 ────────────────────────────────────────────────────────────

Element SSHDialog::renderConnectedView() {
    auto& colors = theme_.getColors();
    std::string conn_text = current_connection_->user + "@" + current_connection_->host;
    if (current_connection_->port > 0 && current_connection_->port != 22)
        conn_text += ":" + std::to_string(current_connection_->port);

    Elements rows;
    rows.push_back(hbox({text(icons::TERMINAL) | color(Color::Cyan),
                         text(" SSH Connection Status ") | color(colors.foreground) | bold}) |
                   center);
    rows.push_back(separator());
    rows.push_back(text(""));
    rows.push_back(hbox({text("Connected: ") | color(colors.comment),
                         text(conn_text) | color(colors.function) | bold}) |
                   center);
    rows.push_back(text(""));
    rows.push_back(hbox({text("Delete: Disconnect  ") | color(colors.keyword),
                         text("Esc: Close") | color(colors.comment)}) |
                   center);

    return window(text("SSH Connection"), vbox(rows)) | size(WIDTH, GREATER_THAN, 50) |
           size(HEIGHT, GREATER_THAN, 12) | bgcolor(colors.background) |
           borderWithColor(colors.dialog_border);
}

// ─────────────────────────────────────────────────────────────────────────────
// 表单辅助方法
// ─────────────────────────────────────────────────────────────────────────────

std::string* SSHDialog::getCurrentField() {
    switch (current_field_) {
        case 0:
            return &host_input_;
        case 1:
            return &user_input_;
        case 2:
            return &password_input_;
        case 3:
            return &key_path_input_;
        case 4:
            return &port_input_;
        case 5:
            return &remote_path_input_;
        default:
            return nullptr;
    }
}

void SSHDialog::insertChar(char ch) {
    std::string* field = getCurrentField();
    if (field && cursor_position_ <= field->length()) {
        field->insert(cursor_position_, 1, ch);
        cursor_position_++;
    }
}

void SSHDialog::deleteChar() {
    std::string* field = getCurrentField();
    if (field && cursor_position_ < field->length())
        field->erase(cursor_position_, 1);
}

void SSHDialog::backspace() {
    std::string* field = getCurrentField();
    if (field && cursor_position_ > 0) {
        field->erase(cursor_position_ - 1, 1);
        cursor_position_--;
    }
}

void SSHDialog::moveCursorLeft() {
    if (cursor_position_ > 0)
        cursor_position_--;
}

void SSHDialog::moveCursorRight() {
    std::string* field = getCurrentField();
    if (field && cursor_position_ < field->length())
        cursor_position_++;
}

void SSHDialog::moveToNextField() {
    if (current_field_ < 5) {
        current_field_++;
        std::string* field = getCurrentField();
        if (field)
            cursor_position_ = field->length();
    }
}

void SSHDialog::moveToPreviousField() {
    if (current_field_ > 0) {
        current_field_--;
        std::string* field = getCurrentField();
        if (field)
            cursor_position_ = field->length();
    }
}

SSHConfig SSHDialog::buildConfig() const {
    SSHConfig config;
    config.host = host_input_;
    config.user = user_input_;
    config.password = password_input_;
    config.key_path = key_path_input_;
    config.remote_path = remote_path_input_;
    try {
        config.port = std::stoi(port_input_);
    } catch (...) {
        config.port = 22;
    }
    return config;
}

} // namespace ui
} // namespace pnana
