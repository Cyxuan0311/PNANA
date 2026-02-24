#include "features/terminal/terminal_line_buffer.h"

namespace pnana {
namespace features {
namespace terminal {

void PendingLineBuffer::feed(const std::string& chunk) {
    for (size_t i = 0; i < chunk.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(chunk[i]);
        if (state_ == State::Escape) {
            feedEscape(c);
            continue;
        } else if (state_ == State::SkipOne) {
            state_ = State::Normal;
            continue;
        } else if (state_ == State::OSC) {
            if (c == '\x07') {
                state_ = State::Normal;
            }
            continue;
        } else if (state_ == State::CSI) {
            feedCSI(c);
        } else if (state_ == State::AfterBackspace) {
            if (c == '\x07') {
                continue;
            }
            if (c >= 0x20 && c != 0x7f) {
                replace_buf_ += static_cast<char>(c);
            } else {
                flushReplace();
                if (c == 0x08) {
                    bool was_at_end = (cursor_pos_ >= line_.size());
                    cursor_pos_ = cursor_pos_ > 0 ? cursor_pos_ - 1 : 0;
                    backspace_at_end_ = was_at_end;
                    state_ = State::AfterBackspace;
                } else if (c == '\r') {
                    line_.clear();
                    cursor_pos_ = 0;
                    state_ = State::Normal;
                } else if (c == '\x1b') {
                    state_ = State::Escape;
                } else if (c == '\n') {
                    state_ = State::Normal;
                } else {
                    state_ = State::Normal;
                    feedChar(c);
                }
            }
        } else {
            // 忽略 0x07 (BEL)，避免 bracketed paste 等产生的杂音干扰
            if (c == '\x07') {
                continue;
            }
            // readline 在引号内输入时 echo 为 char+"\b，其中的 " 是视觉反馈，\b 仅回退该视觉字符。
            // 跳过 "\b 中的 " 和 \b，不插入 " 也不移动光标，避免累积多余引号。
            if (c == '"' && i + 1 < chunk.size() &&
                static_cast<unsigned char>(chunk[i + 1]) == 0x08) {
                i++; // 消耗 \b，两者都不影响 line/cursor
                continue;
            }
            if (c == 0x08) {
                bool was_at_end = (cursor_pos_ >= line_.size());
                cursor_pos_ = cursor_pos_ > 0 ? cursor_pos_ - 1 : 0;
                replace_buf_.clear();
                state_ = State::AfterBackspace;
                backspace_at_end_ = was_at_end;
            } else if (c == '\r') {
                // \r 通常表示“回到行首并准备重绘”（历史切换、命令补全等），清空以便新内容替换
                line_.clear();
                cursor_pos_ = 0;
            } else if (c == '\x1b') {
                state_ = State::Escape;
            } else if (c == '\n') {
                // 换行不改变 line/cursor，由调用方按 \n 分行
            } else if (c >= 0x20 && c != 0x7f) {
                feedChar(c);
            }
        }
    }
    flushReplace();
}

void PendingLineBuffer::flushReplace() {
    if (!replace_buf_.empty()) {
        line_ = line_.substr(0, cursor_pos_) + replace_buf_;
        cursor_pos_ = line_.size();
        replace_buf_.clear();
    } else if (backspace_at_end_) {
        int cnt = pending_backspace_count_ ? pending_backspace_count_->load() : 0;
        if (cnt > 0) {
            pending_backspace_count_->fetch_sub(1);
            line_ = line_.substr(0, cursor_pos_);
        }
    }
    backspace_at_end_ = false;
    state_ = State::Normal;
}

void PendingLineBuffer::reset() {
    line_.clear();
    cursor_pos_ = 0;
    replace_buf_.clear();
    state_ = State::Normal;
    csi_buf_.clear();
    backspace_at_end_ = false;
}

void PendingLineBuffer::feedChar(unsigned char c) {
    if (cursor_pos_ >= line_.size()) {
        line_ += static_cast<char>(c);
        cursor_pos_ = line_.size();
    } else {
        line_.insert(cursor_pos_, 1, static_cast<char>(c));
        cursor_pos_++;
    }
}

void PendingLineBuffer::feedEscape(unsigned char c) {
    if (c == '[') {
        state_ = State::CSI;
        csi_buf_ = "[";
    } else if (c == ']') {
        state_ = State::OSC;
    } else if (c == '(' || c == ')' || c == 'O' || c == 'N' || c == ' ') {
        // 两字节序列如 \x1b(0, \x1b)0 等，跳过下一字节
        state_ = State::SkipOne;
    } else {
        state_ = State::Normal;
    }
}

void PendingLineBuffer::feedCSI(unsigned char c) {
    if (c >= 0x40 && c <= 0x7e) {
        if (c == 'D') {
            cursor_pos_ = cursor_pos_ > 0 ? cursor_pos_ - 1 : 0;
        } else if (c == 'C') {
            cursor_pos_ = cursor_pos_ < line_.size() ? cursor_pos_ + 1 : cursor_pos_;
        } else if (c == 'A' || c == 'B') {
            // 上下方向键：历史命令切换时 readline 会先发 erase+重绘，此处仅忽略
        } else if (c == 'H' || c == '~') {
            if (csi_buf_ == "[" || csi_buf_ == "[1") {
                cursor_pos_ = 0;
            }
        } else if (c == 'F') {
            cursor_pos_ = line_.size();
        } else if (c == 'K') {
            // EL: 0=从光标到行末, 1=从行首到光标, 2=整行（历史切换时 readline 常用）
            int param = 0;
            for (size_t j = 1; j < csi_buf_.size(); ++j) {
                if (csi_buf_[j] >= '0' && csi_buf_[j] <= '9') {
                    param = param * 10 + (csi_buf_[j] - '0');
                } else if (csi_buf_[j] == ';') {
                    break;
                }
            }
            if (param == 0) {
                // \x1b[K 在光标移动时也会被发送，若直接截断会导致后面的字符消失且无法还原。
                // 仅在 param==2（整行清除，历史切换）时修改 line_，param 0/1 仅作显示用，不删内容
            } else if (param == 1) {
                // 从行首到光标：也不修改 line_，避免光标移动时误删
            } else if (param == 2) {
                line_.clear();
                cursor_pos_ = 0;
            }
        }
        state_ = State::Normal;
        csi_buf_.clear();
    } else {
        csi_buf_ += static_cast<char>(c);
    }
}

} // namespace terminal
} // namespace features
} // namespace pnana
