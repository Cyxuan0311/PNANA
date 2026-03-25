#include "features/terminal/terminal_session.h"
#include "features/terminal/terminal_pty.h"
#include "features/terminal/terminal_pty_backend.h"
#include "utils/logger.h"
#include <algorithm>
#include <csignal>
#include <sstream>

namespace pnana {
namespace features {
namespace terminal {

#ifdef BUILD_LIBVTERM_SUPPORT

// 过滤对老版本 libvterm 危险的序列：
//   1. Unicode 私用区字符 U+E000–U+F8FF（Nerd Font 等图标）→ 替换为空格
//   2. OSC/DCS/APC/PM 序列（跨 read 需要保持状态）
class VTermStreamFilter {
  public:
    explicit VTermStreamFilter(TerminalSession& owner) : owner_(owner) {}

    std::string filter(const char* data, size_t len) {
        std::string out;
        out.reserve(len);
        size_t i = 0;
        while (i < len) {
            unsigned char b = static_cast<unsigned char>(data[i]);

            if (owner_.esc_pending_) {
                owner_.esc_pending_ = false;
                if (b == ']' || b == 'P' || b == '_' || b == '^') {
                    owner_.filter_mode_ = toMode(b);
                    i++;
                    continue;
                }
                out += '\x1b';
                out += static_cast<char>(b);
                i++;
                continue;
            }

            if (owner_.filter_mode_ != TerminalSession::FilterMode::None) {
                if (owner_.seq_esc_pending_) {
                    owner_.seq_esc_pending_ = false;
                    if (b == '\\') {
                        owner_.filter_mode_ = TerminalSession::FilterMode::None;
                        i++;
                        continue;
                    }
                }
                if (b == 0x07) {
                    owner_.filter_mode_ = TerminalSession::FilterMode::None;
                    i++;
                    continue;
                }
                if (b == 0x1B) {
                    owner_.seq_esc_pending_ = true;
                    i++;
                    continue;
                }
                i++;
                continue;
            }

            if (b == 0x1B) {
                owner_.esc_pending_ = true;
                i++;
                continue;
            }

            // UTF-8 解码（跨包时保留未完整序列）
            if ((b & 0x80) == 0x00) {
                out += static_cast<char>(b);
                i++;
                continue;
            }

            size_t seq_len = 1;
            if ((b & 0xE0) == 0xC0)
                seq_len = 2;
            else if ((b & 0xF0) == 0xE0)
                seq_len = 3;
            else if ((b & 0xF8) == 0xF0)
                seq_len = 4;
            else {
                out += static_cast<char>(b);
                i++;
                continue;
            }

            if (i + seq_len > len) {
                owner_.utf8_pending_.assign(data + i, data + len);
                break;
            }

            uint32_t cp = 0;
            if (seq_len == 2) {
                cp = (b & 0x1F) << 6;
                cp |= (static_cast<unsigned char>(data[i + 1]) & 0x3F);
            } else if (seq_len == 3) {
                cp = (b & 0x0F) << 12;
                cp |= (static_cast<unsigned char>(data[i + 1]) & 0x3F) << 6;
                cp |= (static_cast<unsigned char>(data[i + 2]) & 0x3F);
            } else {
                cp = (b & 0x07) << 18;
                cp |= (static_cast<unsigned char>(data[i + 1]) & 0x3F) << 12;
                cp |= (static_cast<unsigned char>(data[i + 2]) & 0x3F) << 6;
                cp |= (static_cast<unsigned char>(data[i + 3]) & 0x3F);
            }

            if (cp >= 0xE000 && cp <= 0xF8FF) {
                out += ' ';
            } else {
                for (size_t j = 0; j < seq_len; j++)
                    out += static_cast<char>(data[i + j]);
            }
            i += seq_len;
        }
        return out;
    }

  private:
    TerminalSession& owner_;

    TerminalSession::FilterMode toMode(unsigned char b) {
        switch (b) {
            case ']':
                return TerminalSession::FilterMode::OSC;
            case 'P':
                return TerminalSession::FilterMode::DCS;
            case '_':
                return TerminalSession::FilterMode::APC;
            case '^':
                return TerminalSession::FilterMode::PM;
            default:
                return TerminalSession::FilterMode::None;
        }
    }
};

#endif // BUILD_LIBVTERM_SUPPORT

std::string escapeSingleQuotes(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '\'')
            out += "'\\''";
        else
            out += c;
    }
    return out;
}

std::string escapeSingleQuotesForShell(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '\'')
            out += "'\\''";
        else
            out += c;
    }
    return out;
}

TerminalSession::TerminalSession() = default;

TerminalSession::~TerminalSession() {
    on_output_ = nullptr;
    on_exit_ = nullptr;
}

bool TerminalSession::startLocalShell(const std::string& cwd) {
    LOG("[TerminalSession] startLocalShell enter cwd=" + cwd);
    PTYResult result = PTYExecutor::createInteractiveShell(cwd);
    if (!result.success) {
        LOG("[TerminalSession] createInteractiveShell failed");
        return false;
    }

    LOG("[TerminalSession] PTY created, creating backend");
    backend_ = PTYBackend::create(result);
    if (!backend_)
        return false;

    title_ = "bash";
#ifdef BUILD_LIBVTERM_SUPPORT
    vterm_ = std::make_unique<VTermScreenModel>(24, 80);
    backend_->resize(80, 24);
    setReadCoalesceWindow(std::chrono::milliseconds(4));
    // libvterm 处理 DA/状态查询时会生成响应字节，必须写回 PTY，否则输出缓冲区满后挂死
    vterm_->setOutputCallback([this](const char* data, size_t len) {
        if (backend_)
            backend_->write(std::string_view(data, len));
    });
#endif

    backend_->setOnRead([this](const char* data, size_t len) {
        onPtyRead(data, len);
    });
    backend_->setOnExit([this](int code) {
        onPtyExit(code);
    });
    LOG("[TerminalSession] startLocalShell done");
    return true;
}

bool TerminalSession::startLocalShellWithPath(const std::string& cwd,
                                              const std::string& shell_path) {
    LOG("[TerminalSession] startLocalShellWithPath enter cwd=" + cwd);
    PTYResult result = PTYExecutor::createInteractiveShellWithPath(cwd, shell_path);
    if (!result.success) {
        LOG("[TerminalSession] createInteractiveShellWithPath failed");
        return false;
    }

    LOG("[TerminalSession] PTY created, creating backend");
    backend_ = PTYBackend::create(result);
    if (!backend_)
        return false;

    title_ = shell_path.empty() ? "shell" : shell_path;
#ifdef BUILD_LIBVTERM_SUPPORT
    vterm_ = std::make_unique<VTermScreenModel>(24, 80);
    backend_->resize(80, 24);
    setReadCoalesceWindow(std::chrono::milliseconds(4));
    vterm_->setOutputCallback([this](const char* data, size_t len) {
        if (backend_)
            backend_->write(std::string_view(data, len));
    });
#endif

    backend_->setOnRead([this](const char* data, size_t len) {
        onPtyRead(data, len);
    });
    backend_->setOnExit([this](int code) {
        onPtyExit(code);
    });
    LOG("[TerminalSession] startLocalShellWithPath done");
    return true;
}

bool TerminalSession::startSSH(const std::string& host, const std::string& user, int port,
                               const std::string& key_path, const std::string& password) {
    int p = (port > 0) ? port : 22;
    std::string port_opt = (p != 22) ? (" -p " + std::to_string(p)) : "";
    std::string key_opt = key_path.empty() ? "" : (" -i " + key_path);
    std::string opts =
        " -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o LogLevel=ERROR";
    std::string target = user + "@" + host;
    std::string cmd;
    if (!password.empty()) {
        cmd = "sshpass -p '" + escapeSingleQuotesForShell(password) + "' ssh -t" + opts + port_opt +
              " " + target;
    } else {
        cmd = "ssh -t" + opts + port_opt + key_opt + " " + target;
    }

    PTYResult result = PTYExecutor::createPTY(cmd, ".", {});
    if (!result.success)
        return false;

    backend_ = PTYBackend::create(result);
    if (!backend_)
        return false;

    title_ = "ssh " + host;
#ifdef BUILD_LIBVTERM_SUPPORT
    vterm_ = std::make_unique<VTermScreenModel>(24, 80);
    backend_->resize(80, 24);
    setReadCoalesceWindow(std::chrono::milliseconds(4));
    vterm_->setOutputCallback([this](const char* data, size_t len) {
        if (backend_)
            backend_->write(std::string_view(data, len));
    });
#endif

    backend_->setOnRead([this](const char* data, size_t len) {
        onPtyRead(data, len);
    });
    backend_->setOnExit([this](int code) {
        onPtyExit(code);
    });
    return true;
}

bool TerminalSession::startContainer(const std::string& container_id, const std::string& shell) {
    std::string cmd = "docker exec -it " + container_id + " " + shell;
    PTYResult result = PTYExecutor::createPTY(cmd, ".", {});
    if (!result.success) {
        cmd = "podman exec -it " + container_id + " " + shell;
        result = PTYExecutor::createPTY(cmd, ".", {});
    }
    if (!result.success)
        return false;

    backend_ = PTYBackend::create(result);
    if (!backend_)
        return false;

    title_ = "container:" + container_id.substr(0, 12);
#ifdef BUILD_LIBVTERM_SUPPORT
    vterm_ = std::make_unique<VTermScreenModel>(24, 80);
    backend_->resize(80, 24);
    setReadCoalesceWindow(std::chrono::milliseconds(4));
    vterm_->setOutputCallback([this](const char* data, size_t len) {
        if (backend_)
            backend_->write(std::string_view(data, len));
    });
#endif

    backend_->setOnRead([this](const char* data, size_t len) {
        onPtyRead(data, len);
    });
    backend_->setOnExit([this](int code) {
        onPtyExit(code);
    });
    return true;
}

void TerminalSession::terminate() {
    if (backend_)
        backend_->terminate(SIGTERM);
}

void TerminalSession::sendBytes(std::string_view data) {
    if (backend_)
        backend_->write(data);
}

#ifdef BUILD_LIBVTERM_SUPPORT
void TerminalSession::setReadCoalesceWindow(std::chrono::milliseconds window) {
    read_coalesce_window_ = window;
}

bool TerminalSession::pendingFeedOverflowed() const {
    return pending_overflowed_;
}
#endif

void TerminalSession::sendKey(const KeyEvent& ev) {
    sendBytes(keyEventToBytes(ev));
}

std::string TerminalSession::keyEventToBytes(const KeyEvent& ev) const {
    // 辅助函数：检查是否处于应用光标模式
    auto isAppCursorMode = [this]() -> bool {
#ifdef BUILD_LIBVTERM_SUPPORT
        return isApplicationCursorMode();
#else
        return false;
#endif
    };

    switch (ev.type) {
        case KeyEvent::Type::Char:
            return std::string(1, ev.ch);
        case KeyEvent::Type::Enter:
            // 对交互式程序（python/gdb/nano 等），Enter 更应该是 CR(\r)。
            // 发送 CRLF(\r\n) 可能导致重复提示符或多换行。
            return "\r";
        case KeyEvent::Type::Tab:
            return "\t";
        case KeyEvent::Type::Backspace:
            return "\x08";
        case KeyEvent::Type::KeyUp:
            return isAppCursorMode() ? "\x1bOA" : "\x1b[A";
        case KeyEvent::Type::KeyDown:
            return isAppCursorMode() ? "\x1bOB" : "\x1b[B";
        case KeyEvent::Type::KeyLeft:
            return isAppCursorMode() ? "\x1bOD" : "\x1b[D";
        case KeyEvent::Type::KeyRight:
            return isAppCursorMode() ? "\x1bOC" : "\x1b[C";
        case KeyEvent::Type::Home:
            return isAppCursorMode() ? "\x1bOH" : "\x1b[H";
        case KeyEvent::Type::End:
            return isAppCursorMode() ? "\x1bOF" : "\x1b[F";
        case KeyEvent::Type::PageUp:
            return "\x1b[5~";
        case KeyEvent::Type::PageDown:
            return "\x1b[6~";
        case KeyEvent::Type::Delete:
            return "\x1b[3~";
        case KeyEvent::Type::Escape:
            return "\x1b";
        case KeyEvent::Type::CtrlC:
            return "\x03";
        case KeyEvent::Type::CtrlD:
            return "\x04";
        case KeyEvent::Type::CtrlZ:
            return "\x1a";
        case KeyEvent::Type::CtrlL:
            return "\x0c";
        case KeyEvent::Type::CtrlU:
            return "\x15";
        case KeyEvent::Type::CtrlK:
            return "\x0b";
        case KeyEvent::Type::CtrlA:
            return "\x01";
        case KeyEvent::Type::CtrlE:
            return "\x05";
        case KeyEvent::Type::CtrlW:
            return "\x17";
        case KeyEvent::Type::CtrlH:
            return "\x08";
        case KeyEvent::Type::CtrlX:
            return "\x18";
    }
    return "";
}

#ifdef BUILD_LIBVTERM_SUPPORT
void TerminalSession::feedPending() {
    if (!vterm_ || !vterm_->isReady()) {
        std::lock_guard<std::mutex> lock(pending_feed_mutex_);
        pending_chunks_.clear();
        pending_bytes_ = 0;
        pending_overflowed_ = false;
        LOG("[TerminalSession] feedPending skipped: vterm_ready=" +
            std::to_string(vterm_ && vterm_->isReady()));
        return;
    }
    fixVTermCallbackUser();
    LOG("[TerminalSession] feedPending start chunks=" + std::to_string(pending_chunks_.size()) +
        " bytes=" + std::to_string(pending_bytes_) +
        " overflowed=" + std::to_string(pending_overflowed_));
    std::deque<std::string> batch;
    {
        std::lock_guard<std::mutex> lock(pending_feed_mutex_);
        batch.swap(pending_chunks_);
        pending_bytes_ = 0;
    }
    VTermStreamFilter filter(*this);
    while (!batch.empty()) {
        const std::string& s = batch.front();
        if (!s.empty()) {
            std::string combined;
            if (!utf8_pending_.empty()) {
                combined = std::move(utf8_pending_);
                utf8_pending_.clear();
                combined.append(s);
            }
            const std::string& src = combined.empty() ? s : combined;
            std::string filtered = filter.filter(src.data(), src.size());
            if (!filtered.empty()) {
                vterm_->feed(filtered.data(), filtered.size());
            }
        }
        batch.pop_front();
    }
    if (pending_overflowed_) {
        vterm_->feed("\x1b[?6c", 5);
        pending_overflowed_ = false;
    }
    vterm_->flushDamage();
}

ScreenSnapshot TerminalSession::getSnapshot(int scroll_offset, int max_scrollback,
                                            int view_height) const {
    if (!vterm_ || !vterm_->isReady())
        return ScreenSnapshot();

    ScreenSnapshot full = vterm_->snapshot(max_scrollback);
    ScreenSnapshot result;
    result.rows = view_height > 0 ? view_height : full.rows;
    result.cols = full.cols;

    std::vector<std::vector<TerminalCell>> combined;
    for (const auto& line : full.scrollback)
        combined.push_back(line);
    for (const auto& line : full.visible)
        combined.push_back(line);

    size_t total = combined.size();
    if (total == 0)
        return result;

    size_t offset = static_cast<size_t>(std::max(0, scroll_offset));
    size_t start = (total > result.rows + offset) ? total - result.rows - offset : 0;
    size_t count = std::min(static_cast<size_t>(result.rows), total - start);

    for (size_t i = 0; i < count; i++)
        result.visible.push_back(combined[start + i]);

    const size_t full_cursor_row = full.scrollback.size() + static_cast<size_t>(full.cursor_row);
    if (full_cursor_row >= start && full_cursor_row < start + count) {
        result.cursor_row = static_cast<int>(full_cursor_row - start);
        result.cursor_col = full.cursor_col;
        result.cursor_visible = full.cursor_visible;
    } else {
        result.cursor_visible = false;
    }
    return result;
}

ScreenSnapshot TerminalSession::getFullSnapshot(int max_scrollback) const {
    if (!vterm_ || !vterm_->isReady())
        return ScreenSnapshot();
    return vterm_->snapshot(max_scrollback);
}
#endif

void TerminalSession::resize(int cols, int rows) {
    if (backend_)
        backend_->resize(cols, rows);
#ifdef BUILD_LIBVTERM_SUPPORT
    if (vterm_)
        vterm_->resize(rows, cols);
#endif
}

bool TerminalSession::isRunning() const {
    return backend_ && backend_->isRunning();
}

void TerminalSession::onPtyRead(const char* data, size_t len) {
#ifdef BUILD_LIBVTERM_SUPPORT
    if (len > 0) {
        // 追踪 DECCKM（应用光标键模式）：
        // ESC [ ? 1 h -> 开启（方向键应发 ESC O A/B/C/D）
        // ESC [ ? 1 l -> 关闭（方向键应发 ESC [ A/B/C/D）
        csi_mode_probe_buf_.append(data, len);
        while (true) {
            size_t esc = csi_mode_probe_buf_.find("\x1b[");
            if (esc == std::string::npos) {
                if (csi_mode_probe_buf_.size() > 64) {
                    csi_mode_probe_buf_.erase(0, csi_mode_probe_buf_.size() - 2);
                }
                break;
            }
            if (esc > 0) {
                csi_mode_probe_buf_.erase(0, esc);
            }
            if (csi_mode_probe_buf_.size() < 5) {
                break;
            }

            if (csi_mode_probe_buf_.rfind("\x1b[?1h", 0) == 0) {
                app_cursor_mode_.store(true, std::memory_order_relaxed);
                csi_mode_probe_buf_.erase(0, 5);
                continue;
            }
            if (csi_mode_probe_buf_.rfind("\x1b[?1l", 0) == 0) {
                app_cursor_mode_.store(false, std::memory_order_relaxed);
                csi_mode_probe_buf_.erase(0, 5);
                continue;
            }

            if (csi_mode_probe_buf_[0] == '\x1b' && csi_mode_probe_buf_[1] == '[') {
                bool done = false;
                for (size_t i = 2; i < csi_mode_probe_buf_.size(); ++i) {
                    const unsigned char ch = static_cast<unsigned char>(csi_mode_probe_buf_[i]);
                    if (ch >= 0x40 && ch <= 0x7e) {
                        csi_mode_probe_buf_.erase(0, i + 1);
                        done = true;
                        break;
                    }
                }
                if (!done) {
                    if (csi_mode_probe_buf_.size() > 64) {
                        csi_mode_probe_buf_.erase(0, csi_mode_probe_buf_.size() - 16);
                    }
                    break;
                }
            }
        }

        std::lock_guard<std::mutex> lock(pending_feed_mutex_);
        auto now = std::chrono::steady_clock::now();
        const size_t max_chunk = 256;
        if (!pending_chunks_.empty() && now - last_feed_push_ <= read_coalesce_window_) {
            std::string& tail = pending_chunks_.back();
            size_t space = (tail.size() < max_chunk) ? (max_chunk - tail.size()) : 0;
            if (space > 0) {
                size_t take = std::min(space, len);
                tail.append(data, take);
                data += take;
                len -= take;
            }
        }
        size_t added = 0;
        while (len > 0) {
            size_t take = std::min(max_chunk, len);
            pending_chunks_.emplace_back(data, take);
            data += take;
            len -= take;
            added += take;
        }
        pending_bytes_ += added;
        last_feed_push_ = now;
        if (pending_bytes_ > (1u << 20)) {
            pending_overflowed_ = true;
        }
    }
#else
    (void)data;
    (void)len;
#endif
    if (on_output_) {
        on_output_();
    }
}

void TerminalSession::onPtyExit(int exit_code) {
    if (on_exit_)
        on_exit_(exit_code);
}

} // namespace terminal
} // namespace features
} // namespace pnana
