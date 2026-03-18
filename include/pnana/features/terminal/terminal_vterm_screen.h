#ifndef PNANA_FEATURES_TERMINAL_VTERM_SCREEN_H
#define PNANA_FEATURES_TERMINAL_VTERM_SCREEN_H

#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

#ifdef BUILD_LIBVTERM_SUPPORT
#include <vterm.h>

namespace pnana {
namespace features {
namespace terminal {

// 单个 cell 的渲染属性（UI 无关，供 ftxui 等渲染）
struct TerminalCell {
    std::string text; // UTF-8 字符（含宽字符）
    int width;        // 显示宽度（1 或 2，用于双宽字符）
    bool bold;
    bool underline;
    bool italic;
    bool blink;
    bool reverse;
    bool strike;
    uint8_t fg_r, fg_g, fg_b; // 前景色 RGB，255 表示默认
    uint8_t bg_r, bg_g, bg_b; // 背景色 RGB，255 表示默认
    bool fg_default;
    bool bg_default;

    TerminalCell();
};

// 屏幕快照：可见区域 + 滚动缓冲 + 光标
struct ScreenSnapshot {
    int rows;
    int cols;
    std::vector<std::vector<TerminalCell>> visible;    // 当前可见区域 [row][col]
    std::vector<std::vector<TerminalCell>> scrollback; // 滚动历史（从旧到新）
    int cursor_row;
    int cursor_col;
    bool cursor_visible;

    ScreenSnapshot();
};

// libvterm 屏幕模型封装：解析 VT 流，维护 cell 缓冲与 scrollback
class VTermScreenModel {
  public:
    explicit VTermScreenModel(int rows = 24, int cols = 80);
    ~VTermScreenModel();

    VTermScreenModel(const VTermScreenModel&) = delete;
    VTermScreenModel& operator=(const VTermScreenModel&) = delete;

    // 喂入 PTY 输出的字节流
    void feed(const char* data, size_t len);
    // 喂入但不立即 flush（用于批量喂入后统一 flush，减少回调次数）
    void feedNoFlush(const char* data, size_t len);
    void flushDamage();

    // 调整终端尺寸（同时需在 PTY 侧调用 setTerminalSize）
    void resize(int rows, int cols);

    // 获取当前屏幕快照（含 scrollback 行数）
    ScreenSnapshot snapshot(int max_scrollback = 1000) const;

    bool isReady() const;

    static int getVTermVersion();

    // 获取当前行列数
    int getRows() const {
        return rows_;
    }
    int getCols() const {
        return cols_;
    }

    // 注册 PTY 写回回调：libvterm 处理终端查询（ESC[c / ESC[5n 等）时会生成响应，
    // 必须通过此回调写回 PTY，否则输出缓冲区满后 vterm_input_write 会挂死。
    using OutputCallback = std::function<void(const char* data, size_t len)>;
    void setOutputCallback(OutputCallback cb);
    void setScreenCallbackData(void* user);

    // 供 libvterm 回调使用
    void setCursorVisible(bool v) {
        cursor_visible_ = v;
    }
    void onResize(int rows, int cols) {
        updateSize(rows, cols);
    }
    void onSbPushline(int cols, const void* cells) {
        append_scrollback_line(cols, cells);
    }
    void onOutput(const char* data, size_t len);

  private:
    int rows_;
    int cols_;
    void* vt_;     // VTerm*
    void* screen_; // VTermScreen*
    void* state_;  // VTermState*
    bool initialized_;
    mutable std::mutex vterm_mutex_;
    mutable std::mutex scrollback_mutex_;
    mutable std::vector<std::vector<TerminalCell>> scrollback_;
    bool cursor_visible_;
    OutputCallback output_cb_; // 写回 PTY 的回调（用于 DA 等终端响应）

    void sync_cell_from_vterm(int row, int col, TerminalCell& out) const;
    void append_scrollback_line(int cols, const void* vterm_cells);
    void updateSize(int rows, int cols) {
        rows_ = rows;
        cols_ = cols;
    }
};

} // namespace terminal
} // namespace features
} // namespace pnana

#endif // BUILD_LIBVTERM_SUPPORT

#endif // PNANA_FEATURES_TERMINAL_VTERM_SCREEN_H
