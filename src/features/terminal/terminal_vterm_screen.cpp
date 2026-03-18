#ifdef BUILD_LIBVTERM_SUPPORT

#include "features/terminal/terminal_vterm_screen.h"
#include "utils/logger.h"
#include <cstdio>
#include <cstring>
#include <mutex>
#include <vterm.h>

// libvterm 版本兼容性层
// 旧版 (0.2.x): VTermColor 使用 red/green/blue 成员，无 rgb 联合体
// 新版 (0.3+): VTermColor 使用 rgb 联合体，有 vterm_screen_convert_color_to_rgb 等函数
#if !defined(VTERM_COLOR_IS_RGB)
// 旧版 libvterm: 定义兼容宏和函数
#define VTERM_COLOR_IS_DEFAULT_FG(c) ((c)->red == 0 && (c)->green == 0 && (c)->blue == 0)
#define VTERM_COLOR_IS_DEFAULT_BG(c) ((c)->red == 0 && (c)->green == 0 && (c)->blue == 0)
#define VTERM_COLOR_IS_RGB(c) ((c)->red != 0 || (c)->green != 0 || (c)->blue != 0)
#define VTERM_UNDERLINE_OFF 0
#define VTERM_HAS_OUTPUT_CALLBACK 0

inline void vterm_screen_convert_color_to_rgb(VTermScreen* screen, VTermColor* color) {
    (void)screen;
    (void)color;
}

#define VTERM_GET_COLOR_RED(c) ((c)->red)
#define VTERM_GET_COLOR_GREEN(c) ((c)->green)
#define VTERM_GET_COLOR_BLUE(c) ((c)->blue)
#else
// 新版 libvterm: 使用 rgb 联合体
#define VTERM_HAS_OUTPUT_CALLBACK 1

inline void vterm_screen_convert_color_to_rgb(VTermScreen* screen, VTermColor* color) {
    ::vterm_screen_convert_color_to_rgb(screen, color);
}

#define VTERM_GET_COLOR_RED(c) ((c)->rgb.red)
#define VTERM_GET_COLOR_GREEN(c) ((c)->rgb.green)
#define VTERM_GET_COLOR_BLUE(c) ((c)->rgb.blue)
#endif

// libvterm callbacks require C linkage for ABI compatibility
extern "C" {

static int damage_cb(VTermRect rect, void* user) {
    (void)rect;
    (void)user;
    return 0;
}

static int moverect_cb(VTermRect dest, VTermRect src, void* user) {
    (void)dest;
    (void)src;
    (void)user;
    return 0;
}

static int movecursor_cb(VTermPos pos, VTermPos oldpos, int visible, void* user) {
    (void)pos;
    (void)oldpos;
    auto* self = static_cast<pnana::features::terminal::VTermScreenModel*>(user);
    self->setCursorVisible(visible != 0);
    return 0;
}

static int settermprop_cb(VTermProp prop, VTermValue* val, void* user) {
    auto* self = static_cast<pnana::features::terminal::VTermScreenModel*>(user);
    if (prop == VTERM_PROP_CURSORVISIBLE) {
        self->setCursorVisible(val->boolean != 0);
    }
    return 0;
}

static int bell_cb(void* user) {
    (void)user;
    return 0;
}

static int resize_cb(int rows, int cols, void* user) {
    auto* self = static_cast<pnana::features::terminal::VTermScreenModel*>(user);
    self->onResize(rows, cols);
    return 0;
}

static int sb_pushline_cb(int cols, const VTermScreenCell* cells, void* user) {
    auto* self = static_cast<pnana::features::terminal::VTermScreenModel*>(user);
    self->onSbPushline(cols, cells);
    return 0;
}

static int sb_popline_cb(int cols, VTermScreenCell* cells, void* user) {
    (void)cols;
    (void)cells;
    (void)user;
    return 0;
}

#if VTERM_HAS_OUTPUT_CALLBACK
[[maybe_unused]] static void output_callback(const char* s, size_t len, void* user) {
    auto* self = static_cast<pnana::features::terminal::VTermScreenModel*>(user);
    self->onOutput(s, len);
}
#endif

} // extern "C"

namespace pnana {
namespace features {
namespace terminal {

namespace {

// UTF-32 to UTF-8
void utf32_to_utf8(uint32_t codepoint, std::string& out) {
    if (codepoint < 0x80) {
        out += static_cast<char>(codepoint);
    } else if (codepoint < 0x800) {
        out += static_cast<char>(0xC0 | (codepoint >> 6));
        out += static_cast<char>(0x80 | (codepoint & 0x3F));
    } else if (codepoint < 0x10000) {
        out += static_cast<char>(0xE0 | (codepoint >> 12));
        out += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (codepoint & 0x3F));
    } else if (codepoint < 0x110000) {
        out += static_cast<char>(0xF0 | (codepoint >> 18));
        out += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (codepoint & 0x3F));
    }
}

} // namespace

TerminalCell::TerminalCell()
    : width(1), bold(false), underline(false), italic(false), blink(false), reverse(false),
      strike(false), fg_r(255), fg_g(255), fg_b(255), bg_r(255), bg_g(255), bg_b(255),
      fg_default(true), bg_default(true) {}

ScreenSnapshot::ScreenSnapshot()
    : rows(0), cols(0), cursor_row(0), cursor_col(0), cursor_visible(true) {}

VTermScreenModel::VTermScreenModel(int rows, int cols)
    : rows_(rows), cols_(cols), vt_(nullptr), screen_(nullptr), state_(nullptr),
      initialized_(false), cursor_visible_(true) {
    vt_ = vterm_new(rows, cols);
    if (!vt_)
        return;

    vterm_set_utf8(static_cast<VTerm*>(vt_), 1);

    screen_ = vterm_obtain_screen(static_cast<VTerm*>(vt_));
    if (!screen_)
        return;

    state_ = vterm_obtain_state(static_cast<VTerm*>(vt_));
    if (!state_)
        return;

#if VTERM_HAS_OUTPUT_CALLBACK
    vterm_output_set_callback(static_cast<VTerm*>(vt_), output_callback, this);
#endif

    vterm_screen_reset(static_cast<VTermScreen*>(screen_), 1);
    vterm_screen_enable_altscreen(static_cast<VTermScreen*>(screen_), 1);

    static const VTermScreenCallbacks kCallbacks = {
        damage_cb, moverect_cb, movecursor_cb,  settermprop_cb,
        bell_cb,   resize_cb,   sb_pushline_cb, sb_popline_cb,
    };
    vterm_screen_set_callbacks(static_cast<VTermScreen*>(screen_), &kCallbacks, this);

    if (vterm_screen_get_cbdata(static_cast<VTermScreen*>(screen_)) != this) {
        initialized_ = false;
        return;
    }

    initialized_ = true;
}

VTermScreenModel::~VTermScreenModel() {
    if (vt_) {
        vterm_free(static_cast<VTerm*>(vt_));
        vt_ = nullptr;
        screen_ = nullptr;
        state_ = nullptr;
    }
}

void VTermScreenModel::setOutputCallback(OutputCallback cb) {
    output_cb_ = std::move(cb);
}

void VTermScreenModel::setScreenCallbackData(void* user) {
    if (!screen_)
        return;
    static const VTermScreenCallbacks kCallbacks = {
        damage_cb, moverect_cb, movecursor_cb,  settermprop_cb,
        bell_cb,   resize_cb,   sb_pushline_cb, sb_popline_cb,
    };
    vterm_screen_set_callbacks(static_cast<VTermScreen*>(screen_), &kCallbacks, user);
}

void VTermScreenModel::onOutput(const char* data, size_t len) {
    if (output_cb_ && data && len > 0)
        output_cb_(data, len);
}

void VTermScreenModel::feed(const char* data, size_t len) {
    if (!vt_ || !initialized_) {
        return;
    }
    std::lock_guard<std::mutex> lock(vterm_mutex_);
    vterm_input_write(static_cast<VTerm*>(vt_), data, len);
}

void VTermScreenModel::flushDamage() {
    // 不调用 vterm_screen_flush_damage：damage 回调对我们无用，
    // 直接用 vterm_screen_get_cell 读取 cell 即可。
}

void VTermScreenModel::feedNoFlush(const char* data, size_t len) {
    feed(data, len);
}

bool VTermScreenModel::isReady() const {
    return initialized_ && vt_ != nullptr && screen_ != nullptr && state_ != nullptr;
}

int VTermScreenModel::getVTermVersion() {
#if defined(VTERM_VERSION)
    return VTERM_VERSION;
#else
    return 0;
#endif
}

void VTermScreenModel::resize(int rows, int cols) {
    if (rows <= 0 || cols <= 0)
        return;
    std::lock_guard<std::mutex> lock(vterm_mutex_);
    rows_ = rows;
    cols_ = cols;
    if (vt_)
        vterm_set_size(static_cast<VTerm*>(vt_), rows, cols);
}

void VTermScreenModel::sync_cell_from_vterm(int row, int col, TerminalCell& out) const {
    out = TerminalCell();
    if (!screen_)
        return;

    VTermPos pos = {row, col};
    VTermScreenCell vcell;
    if (vterm_screen_get_cell(static_cast<VTermScreen*>(screen_), pos, &vcell) != 1)
        return;

    out.width = vcell.width;
    for (int i = 0; i < VTERM_MAX_CHARS_PER_CELL && vcell.chars[i]; i++) {
        utf32_to_utf8(vcell.chars[i], out.text);
    }
    if (out.text.empty())
        out.text = " ";

    out.bold = !!(vcell.attrs.bold);
    out.underline = (vcell.attrs.underline != VTERM_UNDERLINE_OFF);
    out.italic = !!(vcell.attrs.italic);
    out.blink = !!(vcell.attrs.blink);
    out.reverse = !!(vcell.attrs.reverse);
    out.strike = !!(vcell.attrs.strike);

    VTermColor fg = vcell.fg;
    VTermColor bg = vcell.bg;
    VTermScreen* scr = static_cast<VTermScreen*>(screen_);

    if (VTERM_COLOR_IS_DEFAULT_FG(&fg)) {
        out.fg_default = true;
    } else {
        out.fg_default = false;
        vterm_screen_convert_color_to_rgb(scr, &fg);
        if (VTERM_COLOR_IS_RGB(&fg)) {
            out.fg_r = VTERM_GET_COLOR_RED(&fg);
            out.fg_g = VTERM_GET_COLOR_GREEN(&fg);
            out.fg_b = VTERM_GET_COLOR_BLUE(&fg);
        }
    }

    if (VTERM_COLOR_IS_DEFAULT_BG(&bg)) {
        out.bg_default = true;
    } else {
        out.bg_default = false;
        vterm_screen_convert_color_to_rgb(scr, &bg);
        if (VTERM_COLOR_IS_RGB(&bg)) {
            out.bg_r = VTERM_GET_COLOR_RED(&bg);
            out.bg_g = VTERM_GET_COLOR_GREEN(&bg);
            out.bg_b = VTERM_GET_COLOR_BLUE(&bg);
        }
    }

    if (vcell.attrs.bold) {
        if (out.fg_default && !out.bg_default) {
            out.fg_default = false;
            out.fg_r = out.bg_r;
            out.fg_g = out.bg_g;
            out.fg_b = out.bg_b;
        } else if (out.fg_default && out.bg_default) {
            out.fg_default = false;
            out.fg_r = 255;
            out.fg_g = 255;
            out.fg_b = 255;
        }
    }
}

void VTermScreenModel::append_scrollback_line(int cols, const void* vterm_cells) {
    const VTermScreenCell* cells = static_cast<const VTermScreenCell*>(vterm_cells);
    VTermScreen* scr = static_cast<VTermScreen*>(screen_);
    std::vector<TerminalCell> line;
    line.reserve(cols);
    for (int c = 0; c < cols; c++) {
        TerminalCell tc;
        const VTermScreenCell& vc = cells[c];
        tc.text.clear();
        for (int i = 0; i < VTERM_MAX_CHARS_PER_CELL && vc.chars[i]; i++) {
            utf32_to_utf8(vc.chars[i], tc.text);
        }
        if (tc.text.empty())
            tc.text = " ";
        tc.width = vc.width;
        tc.bold = !!(vc.attrs.bold);
        tc.underline = (vc.attrs.underline != VTERM_UNDERLINE_OFF);
        tc.italic = !!(vc.attrs.italic);
        tc.blink = !!(vc.attrs.blink);
        tc.reverse = !!(vc.attrs.reverse);
        tc.strike = !!(vc.attrs.strike);
        tc.fg_default = VTERM_COLOR_IS_DEFAULT_FG(&vc.fg);
        tc.bg_default = VTERM_COLOR_IS_DEFAULT_BG(&vc.bg);
        if (!tc.fg_default && scr) {
            VTermColor fg = vc.fg;
            vterm_screen_convert_color_to_rgb(scr, &fg);
            tc.fg_r = VTERM_GET_COLOR_RED(&fg);
            tc.fg_g = VTERM_GET_COLOR_GREEN(&fg);
            tc.fg_b = VTERM_GET_COLOR_BLUE(&fg);
        }
        if (!tc.bg_default && scr) {
            VTermColor bg = vc.bg;
            vterm_screen_convert_color_to_rgb(scr, &bg);
            tc.bg_r = VTERM_GET_COLOR_RED(&bg);
            tc.bg_g = VTERM_GET_COLOR_GREEN(&bg);
            tc.bg_b = VTERM_GET_COLOR_BLUE(&bg);
        }
        line.push_back(tc);
    }
    std::lock_guard<std::mutex> lock(scrollback_mutex_);
    scrollback_.push_back(std::move(line));
}

ScreenSnapshot VTermScreenModel::snapshot(int max_scrollback) const {
    static int snap_count = 0;
    ++snap_count;
    ScreenSnapshot snap;
    {
        std::lock_guard<std::mutex> lock(vterm_mutex_);
        snap.rows = rows_;
        snap.cols = cols_;
        snap.cursor_visible = cursor_visible_;

        if (!screen_ || !state_) {
            snap.visible.resize(rows_, std::vector<TerminalCell>(cols_, TerminalCell()));
            return snap;
        }

        VTermPos cursorpos;
        vterm_state_get_cursorpos(static_cast<VTermState*>(state_), &cursorpos);
        snap.cursor_row = cursorpos.row;
        snap.cursor_col = cursorpos.col;

        snap.visible.resize(rows_);
        for (int r = 0; r < rows_; r++) {
            snap.visible[r].resize(cols_);
            for (int c = 0; c < cols_; c++) {
                sync_cell_from_vterm(r, c, snap.visible[r][c]);
            }
        }
    }

    {
        std::lock_guard<std::mutex> lock(scrollback_mutex_);
        size_t sb_limit = static_cast<size_t>(max_scrollback);
        if (scrollback_.size() > sb_limit) {
            snap.scrollback.assign(scrollback_.end() - sb_limit, scrollback_.end());
        } else {
            snap.scrollback = scrollback_;
        }
    }

    return snap;
}

} // namespace terminal
} // namespace features
} // namespace pnana

#endif // BUILD_LIBVTERM_SUPPORT
