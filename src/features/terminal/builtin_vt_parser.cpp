#include "features/terminal/builtin_vt_parser.h"
#include <cstring>

namespace pnana {
namespace features {
namespace terminal {

void BuiltinVtParser::init(BuiltinScreen* screen) {
    screen_ = screen;
    state_ = BuiltinParserState::Ground;
    charset_designate_ = -1;
    decaln_pending_ = 0;
    utf8_.init();
    std::memset(params_, 0, sizeof(params_));
    param_count_ = 0;
    param_idx_ = 0;
    param_has_val_ = 0;
    private_marker_ = 0;
    final_byte_ = 0;
    osc_len_ = 0;
    dcs_collect_len_ = 0;
}

void BuiltinVtParser::resetParams() {
    std::memset(params_, 0, sizeof(params_));
    param_count_ = 0;
    param_idx_ = 0;
    param_has_val_ = 0;
    private_marker_ = 0;
    final_byte_ = 0;
}

int BuiltinVtParser::getParam(int idx, int defval) {
    if (idx < param_count_) {
        int v = params_[idx];
        return v > 0 ? v : defval;
    }
    return defval;
}

int BuiltinVtParser::decSpecial(int cp) {
    switch (cp) {
        case '_':
            return 0x00A0;
        case '`':
            return 0x2666;
        case 'a':
            return 0x2592;
        case 'b':
            return 0x2409;
        case 'c':
            return 0x240C;
        case 'd':
            return 0x240D;
        case 'e':
            return 0x240A;
        case 'f':
            return 0x00B0;
        case 'g':
            return 0x00B1;
        case 'h':
            return 0x2424;
        case 'i':
            return 0x240B;
        case 'j':
            return 0x2518;
        case 'k':
            return 0x2510;
        case 'l':
            return 0x250C;
        case 'm':
            return 0x2514;
        case 'n':
            return 0x253C;
        case 'o':
            return 0x23BA;
        case 'p':
            return 0x23BB;
        case 'q':
            return 0x2500;
        case 'r':
            return 0x23BC;
        case 's':
            return 0x23BD;
        case 't':
            return 0x251C;
        case 'u':
            return 0x2524;
        case 'v':
            return 0x2534;
        case 'w':
            return 0x252C;
        case 'x':
            return 0x2502;
        case 'y':
            return 0x2264;
        case 'z':
            return 0x2265;
        case '{':
            return 0x03C0;
        case '|':
            return 0x2260;
        case '}':
            return 0x00A3;
        case '~':
            return 0x00B7;
        default:
            return cp;
    }
}

void BuiltinVtParser::dispatchCSI(int cmd) {
    BuiltinScreen* s = screen_;
    int pm = private_marker_;

    switch (cmd) {
        case 'A': {
            int n = getParam(0, 1);
            s->pending_wrap = 0;
            s->cursor_y -= n;
            if (s->cursor_y < 0)
                s->cursor_y = 0;
            break;
        }
        case 'B': {
            int n = getParam(0, 1);
            s->pending_wrap = 0;
            s->cursor_y += n;
            if (s->cursor_y >= s->rows())
                s->cursor_y = s->rows() - 1;
            break;
        }
        case 'C': {
            int n = getParam(0, 1);
            s->pending_wrap = 0;
            s->cursor_x += n;
            if (s->cursor_x >= s->cols())
                s->cursor_x = s->cols() - 1;
            break;
        }
        case 'D': {
            int n = getParam(0, 1);
            s->pending_wrap = 0;
            s->cursor_x -= n;
            if (s->cursor_x < 0)
                s->cursor_x = 0;
            break;
        }
        case 'E': {
            int n = getParam(0, 1);
            s->pending_wrap = 0;
            s->cursor_x = 0;
            s->cursor_y += n;
            if (s->cursor_y >= s->rows())
                s->cursor_y = s->rows() - 1;
            break;
        }
        case 'F': {
            int n = getParam(0, 1);
            s->pending_wrap = 0;
            s->cursor_x = 0;
            s->cursor_y -= n;
            if (s->cursor_y < 0)
                s->cursor_y = 0;
            break;
        }
        case 'G': {
            int n = getParam(0, 1);
            s->pending_wrap = 0;
            s->cursor_x = n - 1;
            if (s->cursor_x < 0)
                s->cursor_x = 0;
            if (s->cursor_x >= s->cols())
                s->cursor_x = s->cols() - 1;
            break;
        }
        case 'H':
        case 'f': {
            int row = getParam(0, 1);
            int col = getParam(1, 1);
            s->pending_wrap = 0;
            if (s->origin_mode) {
                s->cursor_y = s->scroll_top + row - 1;
            } else {
                s->cursor_y = row - 1;
            }
            s->cursor_x = col - 1;
            if (s->cursor_y < 0)
                s->cursor_y = 0;
            if (s->cursor_y >= s->rows())
                s->cursor_y = s->rows() - 1;
            if (s->cursor_x < 0)
                s->cursor_x = 0;
            if (s->cursor_x >= s->cols())
                s->cursor_x = s->cols() - 1;
            break;
        }
        case 'J': {
            int mode = getParam(0, 0);
            if (pm == '?')
                mode = getParam(0, 0);
            s->eraseDisplay(mode);
            break;
        }
        case 'K': {
            int mode = getParam(0, 0);
            if (pm == '?')
                mode = getParam(0, 0);
            s->eraseLine(mode);
            break;
        }
        case 'L': {
            int n = getParam(0, 1);
            s->insertLines(n);
            break;
        }
        case 'M': {
            int n = getParam(0, 1);
            s->deleteLines(n);
            break;
        }
        case 'P': {
            int n = getParam(0, 1);
            s->deleteChars(n);
            break;
        }
        case '@': {
            int n = getParam(0, 1);
            s->insertChars(n);
            break;
        }
        case 'X': {
            int n = getParam(0, 1);
            s->eraseChars(n);
            break;
        }
        case 'S': {
            int n = getParam(0, 1);
            s->scrollUp(n);
            break;
        }
        case 'T': {
            if (pm == 0 && param_count_ > 0 && params_[0] > 0) {
                int n = getParam(0, 1);
                s->scrollDown(n);
            }
            break;
        }
        case 'b': {
            if (param_count_ > 0 && params_[0] > 0) {
                int n = params_[0];
                int px = s->cursor_x - 1;
                if (px < 0)
                    px = 0;
                BuiltinCell* prev = s->cell(px, s->cursor_y);
                if (prev && !prev->isEmpty()) {
                    for (int i = 0; i < n; i++) {
                        s->putChar(prev->codepoint, prev->width);
                    }
                }
            }
            break;
        }
        case 'd': {
            int row = getParam(0, 1);
            s->pending_wrap = 0;
            s->cursor_y = row - 1;
            if (s->cursor_y < 0)
                s->cursor_y = 0;
            if (s->cursor_y >= s->rows())
                s->cursor_y = s->rows() - 1;
            break;
        }
        case 'm': {
            if (param_count_ == 0) {
                s->cur_fg = BUILTIN_COLOR_DEFAULT;
                s->cur_bg = BUILTIN_COLOR_DEFAULT;
                s->cur_flags = 0;
            }
            for (int i = 0; i < param_count_; i++) {
                int v = params_[i];
                if (v == 0) {
                    s->cur_fg = BUILTIN_COLOR_DEFAULT;
                    s->cur_bg = BUILTIN_COLOR_DEFAULT;
                    s->cur_flags = 0;
                } else if (v == 1) {
                    s->cur_flags |= BUILTIN_FLAG_BOLD;
                    s->cur_flags &= ~BUILTIN_FLAG_DIM;
                } else if (v == 2) {
                    s->cur_flags |= BUILTIN_FLAG_DIM;
                    s->cur_flags &= ~BUILTIN_FLAG_BOLD;
                } else if (v == 3) {
                    s->cur_flags |= BUILTIN_FLAG_ITALIC;
                } else if (v == 4) {
                    s->cur_flags |= BUILTIN_FLAG_UNDERLINE;
                } else if (v == 5) {
                    s->cur_flags |= BUILTIN_FLAG_BLINK;
                } else if (v == 7) {
                    s->cur_flags |= BUILTIN_FLAG_REVERSE;
                } else if (v == 8) {
                    // concealed
                } else if (v == 9) {
                    s->cur_flags |= BUILTIN_FLAG_STRIKE;
                } else if (v == 21) {
                    s->cur_flags |= BUILTIN_FLAG_UNDERLINE;
                } else if (v == 22) {
                    s->cur_flags &= ~(BUILTIN_FLAG_BOLD | BUILTIN_FLAG_DIM);
                } else if (v == 23) {
                    s->cur_flags &= ~BUILTIN_FLAG_ITALIC;
                } else if (v == 24) {
                    s->cur_flags &= ~BUILTIN_FLAG_UNDERLINE;
                } else if (v == 25) {
                    s->cur_flags &= ~BUILTIN_FLAG_BLINK;
                } else if (v == 27) {
                    s->cur_flags &= ~BUILTIN_FLAG_REVERSE;
                } else if (v == 28) {
                    // concealed off
                } else if (v == 29) {
                    s->cur_flags &= ~BUILTIN_FLAG_STRIKE;
                } else if (v == 53) {
                    s->cur_flags |= BUILTIN_FLAG_OVERLINE;
                } else if (v == 55) {
                    s->cur_flags &= ~BUILTIN_FLAG_OVERLINE;
                } else if (v >= 30 && v <= 37) {
                    s->cur_fg = builtinColor16(v - 30);
                } else if (v == 38) {
                    if (i + 2 < param_count_ && params_[i + 1] == 5) {
                        s->cur_fg = builtinColor256(params_[i + 2]);
                        i += 2;
                    } else if (i + 4 < param_count_ && params_[i + 1] == 2) {
                        s->cur_fg = builtinColorRGB(params_[i + 2], params_[i + 3], params_[i + 4]);
                        i += 4;
                    }
                } else if (v == 39) {
                    s->cur_fg = BUILTIN_COLOR_DEFAULT;
                } else if (v >= 40 && v <= 47) {
                    s->cur_bg = builtinColor16(v - 40);
                } else if (v == 48) {
                    if (i + 2 < param_count_ && params_[i + 1] == 5) {
                        s->cur_bg = builtinColor256(params_[i + 2]);
                        i += 2;
                    } else if (i + 4 < param_count_ && params_[i + 1] == 2) {
                        s->cur_bg = builtinColorRGB(params_[i + 2], params_[i + 3], params_[i + 4]);
                        i += 4;
                    }
                } else if (v == 49) {
                    s->cur_bg = BUILTIN_COLOR_DEFAULT;
                } else if (v >= 90 && v <= 97) {
                    s->cur_fg = builtinColor16(v - 90 + 8);
                } else if (v >= 100 && v <= 107) {
                    s->cur_bg = builtinColor16(v - 100 + 8);
                }
            }
            break;
        }
        case 'g': {
            int mode = getParam(0, 0);
            if (mode == 0 && s->cursor_x < s->cols()) {
                s->tab_stops[static_cast<size_t>(s->cursor_x)] = 0;
            } else if (mode == 3) {
                std::memset(s->tab_stops.data(), 0, static_cast<size_t>(s->cols()));
            }
            break;
        }
        case 'r': {
            int top = getParam(0, 1);
            int bot = getParam(1, s->rows());
            s->pending_wrap = 0;
            s->setScrollingRegion(top - 1, bot - 1);
            break;
        }
        case 's': {
            if (pm == 0) {
                s->cursor_saved_x = s->cursor_x;
                s->cursor_saved_y = s->cursor_y;
                s->saved_fg = s->cur_fg;
                s->saved_bg = s->cur_bg;
                s->saved_flags = s->cur_flags;
            }
            break;
        }
        case 'u': {
            if (pm == 0) {
                s->pending_wrap = 0;
                s->cursor_x = s->cursor_saved_x;
                s->cursor_y = s->cursor_saved_y;
                s->cur_fg = s->saved_fg;
                s->cur_bg = s->saved_bg;
                s->cur_flags = s->saved_flags;
            }
            break;
        }
        case 'h':
        case 'l': {
            int set = (cmd == 'h');
            if (pm == '?') {
                for (int i = 0; i < param_count_; i++) {
                    int v = params_[i];
                    if (v == 1) {        /* DECCKM */
                    } else if (v == 3) { /* DECCOLM */
                    } else if (v == 5) { /* DECSCNM */
                    } else if (v == 6) {
                        s->origin_mode = set;
                    } else if (v == 7) {
                        s->auto_wrap = set;
                    } else if (v == 12) { /* att610 */
                    } else if (v == 25) {
                        s->cursor_visible = set;
                    } else if (v == 1000 || v == 1002 || v == 1003 || v == 1004 || v == 1005 ||
                               v == 1006 || v == 1015) { /* mouse */
                    } else if (v == 1047) {
                        s->switchAltScreen(set != 0);
                    } else if (v == 1048) {
                        if (set) {
                            s->cursor_saved_x = s->cursor_x;
                            s->cursor_saved_y = s->cursor_y;
                        } else {
                            s->pending_wrap = 0;
                            s->cursor_x = s->cursor_saved_x;
                            s->cursor_y = s->cursor_saved_y;
                        }
                    } else if (v == 1049) {
                        if (set) {
                            s->cursor_saved_x = s->cursor_x;
                            s->cursor_saved_y = s->cursor_y;
                        }
                        s->switchAltScreen(set != 0);
                        if (!set) {
                            s->pending_wrap = 0;
                            s->cursor_x = s->cursor_saved_x;
                            s->cursor_y = s->cursor_saved_y;
                        }
                    } else if (v == 2004) { /* bracketed paste */
                    }
                }
            } else {
                for (int i = 0; i < param_count_; i++) {
                    int v = params_[i];
                    if (v == 4) {         /* IRM */
                    } else if (v == 20) { /* LNM */
                    }
                }
            }
            break;
        }
        default:
            break;
    }
}

void BuiltinVtParser::handleEsc(int c) {
    BuiltinScreen* s = screen_;

    switch (c) {
        case '7':
            s->cursor_saved_x = s->cursor_x;
            s->cursor_saved_y = s->cursor_y;
            s->saved_fg = s->cur_fg;
            s->saved_bg = s->cur_bg;
            s->saved_flags = s->cur_flags;
            s->cursor_saved_visible = s->cursor_visible;
            break;
        case '8':
            s->cursor_x = s->cursor_saved_x;
            s->cursor_y = s->cursor_saved_y;
            s->cur_fg = s->saved_fg;
            s->cur_bg = s->saved_bg;
            s->cur_flags = s->saved_flags;
            s->cursor_visible = s->cursor_saved_visible;
            break;
        case 'M':
            s->reverseIndex();
            break;
        case 'D':
            s->lineFeed();
            break;
        case 'E':
            s->cursor_x = 0;
            s->lineFeed();
            break;
        case 'H':
            if (s->cursor_x >= 0 && s->cursor_x < s->cols())
                s->tab_stops[static_cast<size_t>(s->cursor_x)] = 1;
            break;
        case 'c':
            s->eraseCells(0, 0, s->cols() - 1, s->rows() - 1);
            s->cursor_x = 0;
            s->cursor_y = 0;
            s->cur_fg = BUILTIN_COLOR_DEFAULT;
            s->cur_bg = BUILTIN_COLOR_DEFAULT;
            s->cur_flags = 0;
            s->scroll_top = 0;
            s->scroll_bottom = s->rows() - 1;
            s->charset_G[0] = 0;
            s->charset_G[1] = 0;
            s->charset_active = 0;
            s->origin_mode = 0;
            s->auto_wrap = 1;
            s->cursor_visible = 1;
            std::memset(s->tab_stops.data(), 0, static_cast<size_t>(s->cols()));
            for (int x = 0; x < s->cols(); x++)
                s->tab_stops[static_cast<size_t>(x)] = (x % 8 == 0) ? 1 : 0;
            break;
        default:
            break;
    }
}

void BuiltinVtParser::feedByte(uint8_t b) {
    BuiltinScreen* s = screen_;

    switch (state_) {
        case BuiltinParserState::Ground:
            if (b == 0x1B) {
                state_ = BuiltinParserState::Esc;
            } else if (utf8_.remaining > 0 || (b >= 0xA0 && b <= 0xBF)) {
                uint32_t cp;
                int result = utf8_.feed(b, cp);
                if (result == 1) {
                    if (s->charset_G[s->charset_active] == 1 && cp < 0x80) {
                        cp = static_cast<uint32_t>(decSpecial(static_cast<int>(cp)));
                    }
                    int w = builtinCharWidth(cp);
                    s->putChar(cp, w);
                } else if (result == -1) {
                    s->putChar(0xFFFD, 1);
                }
            } else if (b >= 0x80 && b <= 0x9F) {
                switch (b) {
                    case 0x84:
                        s->lineFeed();
                        break;
                    case 0x85:
                        s->cursor_x = 0;
                        s->lineFeed();
                        break;
                    case 0x88:
                        if (s->cursor_x < s->cols())
                            s->tab_stops[static_cast<size_t>(s->cursor_x)] = 1;
                        break;
                    case 0x8D:
                        s->reverseIndex();
                        break;
                    case 0x8E:
                        break;
                    case 0x8F:
                        break;
                    case 0x90:
                        dcs_collect_len_ = 0;
                        state_ = BuiltinParserState::DcsEntry;
                        break;
                    case 0x9A:
                        break;
                    case 0x9B:
                        resetParams();
                        state_ = BuiltinParserState::CsiEntry;
                        break;
                    case 0x9C:
                        state_ = BuiltinParserState::Ground;
                        break;
                    case 0x9D:
                        osc_len_ = 0;
                        state_ = BuiltinParserState::OscString;
                        break;
                    case 0x9E:
                        state_ = BuiltinParserState::SosPmApc;
                        break;
                    case 0x9F:
                        state_ = BuiltinParserState::SosPmApc;
                        break;
                    default:
                        break;
                }
            } else if (b <= 0x1F || b == 0x7F) {
                switch (b) {
                    case 0x00:
                        break;
                    case 0x05:
                        break;
                    case 0x07:
                        break;
                    case 0x08:
                        s->pending_wrap = 0;
                        if (s->cursor_x > 0)
                            s->cursor_x--;
                        break;
                    case 0x09: {
                        s->pending_wrap = 0;
                        int nx = s->cursor_x + 1;
                        while (nx < s->cols() && !s->tab_stops[static_cast<size_t>(nx)])
                            nx++;
                        if (nx >= s->cols())
                            nx = s->cols() - 1;
                        s->cursor_x = nx;
                        break;
                    }
                    case 0x0A:
                    case 0x0B:
                    case 0x0C:
                        s->pending_wrap = 0;
                        s->lineFeed();
                        break;
                    case 0x0D:
                        s->cursor_x = 0;
                        s->pending_wrap = 0;
                        break;
                    case 0x0E:
                        s->charset_active = 1;
                        break;
                    case 0x0F:
                        s->charset_active = 0;
                        break;
                    case 0x7F:
                        break;
                    default:
                        break;
                }
            } else {
                uint32_t cp;
                int result = utf8_.feed(b, cp);
                if (result == 1) {
                    if (s->charset_G[s->charset_active] == 1 && cp < 0x80) {
                        cp = static_cast<uint32_t>(decSpecial(static_cast<int>(cp)));
                    }
                    int w = builtinCharWidth(cp);
                    s->putChar(cp, w);
                } else if (result == -1) {
                    s->putChar(0xFFFD, 1);
                }
            }
            break;

        case BuiltinParserState::Esc:
            if (charset_designate_ >= 0) {
                int g = charset_designate_;
                charset_designate_ = -1;
                if (b == 'B')
                    s->charset_G[g] = 0;
                else if (b == '0')
                    s->charset_G[g] = 1;
                else if (b == 'A')
                    s->charset_G[g] = 0;
                else if (b == '1')
                    s->charset_G[g] = 0;
                else if (b == '2')
                    s->charset_G[g] = 0;
                state_ = BuiltinParserState::Ground;
            } else if (decaln_pending_) {
                decaln_pending_ = 0;
                if (b == '8') {
                    auto& buf = s->use_alt_screen ? s->alt_cells_ : s->cells_;
                    if (!buf.empty()) {
                        int stride = s->cols();
                        for (int y = 0; y < s->rows(); y++) {
                            for (int x = 0; x < s->cols(); x++) {
                                BuiltinCell& c = buf[static_cast<size_t>(y * stride + x)];
                                c.codepoint = 'E';
                                c.fg_color = BUILTIN_COLOR_DEFAULT;
                                c.bg_color = BUILTIN_COLOR_DEFAULT;
                                c.flags = 0;
                                c.width = 1;
                            }
                        }
                        s->markDirtyRange(0, s->rows() - 1);
                    }
                }
                state_ = BuiltinParserState::Ground;
            } else if (b == '[') {
                resetParams();
                state_ = BuiltinParserState::CsiEntry;
            } else if (b == ']') {
                osc_len_ = 0;
                state_ = BuiltinParserState::OscString;
            } else if (b == 'P') {
                dcs_collect_len_ = 0;
                state_ = BuiltinParserState::DcsEntry;
            } else if (b == 'X' || b == '^' || b == '_') {
                state_ = BuiltinParserState::SosPmApc;
            } else if (b == '(') {
                charset_designate_ = 0;
            } else if (b == ')') {
                charset_designate_ = 1;
            } else if (b == '*') {
                charset_designate_ = 2;
            } else if (b == '+') {
                charset_designate_ = 3;
            } else if (b == '#') {
                decaln_pending_ = 1;
            } else if (b == ' ') {
                state_ = BuiltinParserState::Ground;
            } else {
                handleEsc(static_cast<int>(b));
                state_ = BuiltinParserState::Ground;
            }
            break;

        case BuiltinParserState::CsiEntry:
            if (b >= 0x30 && b <= 0x39) {
                params_[0] = (b - 0x30);
                param_idx_ = 0;
                param_count_ = 1;
                param_has_val_ = 1;
                state_ = BuiltinParserState::CsiParam;
            } else if (b == ';') {
                param_idx_ = 1;
                param_count_ = 2;
                param_has_val_ = 0;
                state_ = BuiltinParserState::CsiParam;
            } else if (b >= 0x3C && b <= 0x3F) {
                private_marker_ = b;
                state_ = BuiltinParserState::CsiParam;
            } else if (b >= 0x40 && b <= 0x7E) {
                final_byte_ = b;
                dispatchCSI(static_cast<int>(b));
                state_ = BuiltinParserState::Ground;
            } else {
                state_ = BuiltinParserState::Ground;
            }
            break;

        case BuiltinParserState::CsiParam:
            if (b >= 0x30 && b <= 0x39) {
                if (!param_has_val_) {
                    params_[param_idx_] = 0;
                    param_has_val_ = 1;
                }
                params_[param_idx_] = params_[param_idx_] * 10 + (b - 0x30);
            } else if (b == ';') {
                param_idx_++;
                if (param_idx_ >= BUILTIN_MAX_PARAMS)
                    param_idx_ = BUILTIN_MAX_PARAMS - 1;
                param_count_ = param_idx_ + 1;
                param_has_val_ = 0;
            } else if (b >= 0x3C && b <= 0x3F) {
                private_marker_ = b;
            } else if (b == 0x20) {
                state_ = BuiltinParserState::CsiInter;
            } else if (b >= 0x40 && b <= 0x7E) {
                if (param_has_val_)
                    param_count_ = param_idx_ + 1;
                final_byte_ = b;
                dispatchCSI(static_cast<int>(b));
                state_ = BuiltinParserState::Ground;
            } else {
                state_ = BuiltinParserState::Ground;
            }
            break;

        case BuiltinParserState::CsiInter:
            if (b >= 0x30 && b <= 0x39) {
                state_ = BuiltinParserState::CsiParam;
            } else if (b >= 0x40 && b <= 0x7E) {
                dispatchCSI(static_cast<int>(b));
                state_ = BuiltinParserState::Ground;
            } else {
                state_ = BuiltinParserState::Ground;
            }
            break;

        case BuiltinParserState::OscString:
            if (b == 0x07 || b == 0x9C) {
                osc_buf_[osc_len_] = '\0';
                state_ = BuiltinParserState::Ground;
            } else if (b == 0x1B) {
                state_ = BuiltinParserState::Esc;
            } else if (b >= 0x20 && osc_len_ < BUILTIN_OSC_BUF_SIZE - 1) {
                osc_buf_[osc_len_++] = static_cast<char>(b);
            }
            break;

        case BuiltinParserState::DcsEntry:
        case BuiltinParserState::DcsParam:
        case BuiltinParserState::DcsData:
            if (b == 0x1B) {
                state_ = BuiltinParserState::DcsIgnore;
            } else if (b == 0x9C) {
                state_ = BuiltinParserState::Ground;
            } else if (b >= 0x40 && state_ == BuiltinParserState::DcsEntry) {
                state_ = BuiltinParserState::DcsData;
            } else if (b >= 0x30 && b <= 0x3B && state_ == BuiltinParserState::DcsEntry) {
                state_ = BuiltinParserState::DcsParam;
            }
            break;

        case BuiltinParserState::DcsIgnore:
            if (b == 0x9C || b == '\\') {
                state_ = BuiltinParserState::Ground;
            }
            break;

        case BuiltinParserState::SosPmApc:
            if (b == 0x1B) {
                // might be ST
            } else if (b == 0x9C || b == '\\') {
                state_ = BuiltinParserState::Ground;
            }
            break;

        default:
            state_ = BuiltinParserState::Ground;
            break;
    }
}

void BuiltinVtParser::feed(const uint8_t* data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        feedByte(data[i]);
    }
}

void BuiltinVtParser::feed(const std::string& data) {
    feed(reinterpret_cast<const uint8_t*>(data.data()), data.size());
}

} // namespace terminal
} // namespace features
} // namespace pnana
