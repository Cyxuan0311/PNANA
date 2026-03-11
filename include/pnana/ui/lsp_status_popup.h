#ifndef PNANA_UI_LSP_STATUS_POPUP_H
#define PNANA_UI_LSP_STATUS_POPUP_H

#include "features/lsp/lsp_server_manager.h"
#include "ui/theme.h"
#include <atomic>
#include <condition_variable>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace pnana {
namespace ui {

/** LSP 连接状态弹窗：左侧列表（语言/服务器/连接状态），右侧选中项详情 */
class LspStatusPopup {
  public:
    explicit LspStatusPopup(Theme& theme);

    void open();
    void close();
    bool isOpen() const {
        return is_open_;
    }

    /** 设置数据源（每次打开或渲染时调用，获取当前 LSP 状态快照） */
    void setStatusProvider(std::function<std::vector<features::LspStatusEntry>()> provider);
    // 设置 pid 提供器：通过语言 ID 返回服务器进程 PID（或 -1）
    void setPidProvider(std::function<int(const std::string&)> provider);

    bool handleInput(ftxui::Event event);
    ftxui::Element render();

  private:
    Theme& theme_;
    bool is_open_ = false;
    std::function<std::vector<features::LspStatusEntry>()> status_provider_;
    std::vector<features::LspStatusEntry> entries_; // 当前快照
    size_t selected_index_ = 0;
    size_t scroll_offset_ = 0;
    static constexpr size_t list_display_count_ = 16;

    void refreshEntries();
    ftxui::Element renderTitle() const;
    ftxui::Element renderLeftList() const;
    ftxui::Element renderRightDetail() const;
    ftxui::Element renderHelpBar() const;

    // 用于获取对应语言服务器的 PID（可为空）
    std::function<int(const std::string&)> pid_provider_;

    // 异步内存读取相关
    mutable std::string mem_text_;
    mutable std::string cpu_text_;
    mutable std::mutex mem_mutex_;
    std::thread mem_thread_;
    std::atomic<bool> mem_thread_running_{false};
    std::condition_variable mem_cv_;

    // 保护 entries_ 和 selected_index_ 的互斥量，避免渲染线程与后台线程数据竞争
    mutable std::mutex entries_mutex_;
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_LSP_STATUS_POPUP_H
