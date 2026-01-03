#ifndef PNANA_FEATURES_COMMAND_PALETTE_H
#define PNANA_FEATURES_COMMAND_PALETTE_H

#include <ftxui/dom/elements.hpp>
#include <functional>
#include <string>
#include <vector>

namespace pnana {
namespace features {

// 命令信息结构
struct Command {
    std::string id;                    // 命令ID（唯一标识）
    std::string name;                  // 命令名称（用于显示）
    std::string description;           // 命令描述
    std::vector<std::string> keywords; // 搜索关键词
    std::function<void()> execute;     // 执行函数

    Command(const std::string& i, const std::string& n, const std::string& d,
            const std::vector<std::string>& keys, std::function<void()> exec)
        : id(i), name(n), description(d), keywords(keys), execute(exec) {}
};

// 命令面板类
class CommandPalette {
  public:
    CommandPalette();

    // 注册命令
    void registerCommand(const Command& command);

    // 打开命令面板
    void open();

    // 关闭命令面板
    void close();

    // 检查是否打开
    bool isOpen() const {
        return is_open_;
    }

    // 处理输入
    void handleInput(const std::string& input);

    // 处理键盘事件（返回是否已处理）
    bool handleKeyEvent(const std::string& key);

    // 渲染命令面板
    ftxui::Element render();

    // 执行选中的命令
    void executeSelected();

    // 获取当前输入
    std::string getInput() const {
        return input_;
    }

    // 获取过滤后的命令列表
    std::vector<Command> getFilteredCommands() const {
        return filtered_commands_;
    }

    // 获取当前选中的索引
    size_t getSelectedIndex() const {
        return selected_index_;
    }

  private:
    bool is_open_;
    std::string input_;
    std::vector<Command> commands_;
    std::vector<Command> filtered_commands_;
    size_t selected_index_;

    // 过滤命令
    void filterCommands();

    // 检查命令是否匹配
    bool matchesCommand(const Command& cmd, const std::string& query) const;

    // 移动到下一个命令
    void selectNext();

    // 移动到上一个命令
    void selectPrevious();
};

} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_COMMAND_PALETTE_H
