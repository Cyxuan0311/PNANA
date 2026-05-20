#ifndef PNANA_FEATURES_TERMINAL_BUILTIN_VT_PARSER_H
#define PNANA_FEATURES_TERMINAL_BUILTIN_VT_PARSER_H

#include "features/terminal/builtin_screen.h"
#include <cstdint>
#include <string>

namespace pnana {
namespace features {
namespace terminal {

enum class BuiltinParserState {
    Ground = 0,
    Esc,
    CsiEntry,
    CsiParam,
    CsiInter,
    CsiIgnore,
    OscString,
    DcsEntry,
    DcsParam,
    DcsData,
    DcsIgnore,
    SosPmApc,
};

constexpr int BUILTIN_MAX_PARAMS = 32;
constexpr int BUILTIN_OSC_BUF_SIZE = 256;

struct Utf8Decoder {
    uint32_t codepoint = 0;
    int remaining = 0;

    void init() {
        codepoint = 0;
        remaining = 0;
    }

    int feed(uint8_t b, uint32_t& cp) {
        if (remaining == 0) {
            if (b < 0x80) {
                cp = static_cast<uint32_t>(b);
                return 1;
            }
            if ((b & 0xE0) == 0xC0) {
                codepoint = b & 0x1F;
                remaining = 1;
            } else if ((b & 0xF0) == 0xE0) {
                codepoint = b & 0x0F;
                remaining = 2;
            } else if ((b & 0xF8) == 0xF0) {
                codepoint = b & 0x07;
                remaining = 3;
            } else {
                return -1;
            }
            return 0;
        }
        if ((b & 0xC0) != 0x80) {
            remaining = 0;
            return -1;
        }
        codepoint = (codepoint << 6) | (b & 0x3F);
        remaining--;
        if (remaining == 0) {
            cp = codepoint;
            return 1;
        }
        return 0;
    }
};

class BuiltinVtParser {
  public:
    BuiltinVtParser() = default;

    void init(BuiltinScreen* screen);
    void feed(const uint8_t* data, size_t len);
    void feed(const std::string& data);

    BuiltinScreen* screen() {
        return screen_;
    }

  private:
    BuiltinScreen* screen_ = nullptr;
    BuiltinParserState state_ = BuiltinParserState::Ground;
    Utf8Decoder utf8_;

    int params_[BUILTIN_MAX_PARAMS] = {};
    int param_count_ = 0;
    int param_idx_ = 0;
    int param_has_val_ = 0;
    int private_marker_ = 0;
    int final_byte_ = 0;

    char osc_buf_[BUILTIN_OSC_BUF_SIZE] = {};
    int osc_len_ = 0;

    char dcs_collect_[8] = {};
    int dcs_collect_len_ = 0;

    int charset_designate_ = -1;
    int decaln_pending_ = 0;

    void resetParams();
    int getParam(int idx, int defval);
    void dispatchCSI(int cmd);
    void handleEsc(int c);
    static int decSpecial(int cp);
    void feedByte(uint8_t b);
};

} // namespace terminal
} // namespace features
} // namespace pnana

#endif
