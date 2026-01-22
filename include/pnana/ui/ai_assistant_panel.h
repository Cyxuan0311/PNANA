#ifndef PNANA_UI_AI_ASSISTANT_PANEL_H
#define PNANA_UI_AI_ASSISTANT_PANEL_H

#include "features/ai_client/ai_client.h"
#include "features/ai_config/ai_config.h"
#include "ui/theme.h"
#include <chrono>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/elements.hpp>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace pnana {
namespace ui {

// 消息类型枚举
enum class MessageType { USER, ASSISTANT, SYSTEM, ERROR };

// 聊天消息结构
struct ChatMessage {
    MessageType type;
    std::string content;
    std::string timestamp;
    std::string model_used;    // 使用的AI模型
    bool is_streaming = false; // 是否正在流式输出

    ChatMessage(MessageType t, const std::string& c, const std::string& model = "")
        : type(t), content(c), model_used(model) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
        timestamp = ss.str();
    }
};

// AI编程助手面板
class AIAssistantPanel {
  public:
    AIAssistantPanel(Theme& theme);
    ~AIAssistantPanel() = default;

    // UI渲染
    ftxui::Element render();
    ftxui::Component getComponent();

    // 面板控制
    void show();
    void hide();
    bool isVisible() const {
        return visible_;
    }

    // 消息管理
    void addMessage(const ChatMessage& message);
    void addUserMessage(const std::string& content);
    void addAssistantMessage(const std::string& content, const std::string& model = "");
    void addSystemMessage(const std::string& content);
    void addErrorMessage(const std::string& content);
    void clearMessages();

    // 流式输出支持
    void startStreamingResponse(const std::string& model = "");
    void appendStreamingContent(const std::string& content);
    void finishStreamingResponse();

    // 工具调用显示
    void addToolCall(const pnana::features::ai_client::ToolCall& tool_call);
    void clearToolCalls();

    // 会话管理
    void startNewConversation();
    void addToConversationHistory(const std::string& user_message, const std::string& ai_response);
    std::string getConversationSummary() const;
    bool isRelatedToCurrentTopic(const std::string& message) const;

    // 代码操作
    void insertCodeAtCursor(const std::string& code);
    void replaceSelectedCode(const std::string& code);
    std::string getSelectedCode() const;
    std::string getCurrentFileContent() const;

    // 工具定义和执行
    std::vector<pnana::features::ai_client::ToolDefinition> getToolDefinitions() const;
    pnana::features::ai_client::ToolCallResult executeToolCall(
        const pnana::features::ai_client::ToolCall& tool_call);

    // 目录分析
    void analyzeDirectoryStructure(const std::string& path, std::vector<std::string>& structure,
                                   int depth, int max_depth) const;

    // 命令处理
    void handleCommand(const std::string& command);

    // AI功能
    void sendMessage(const std::string& message);
    void explainCode();
    void generateCode(const std::string& description);
    void refactorCode();
    void addComments();
    void fixErrors();

    // 回调设置
    void setOnSendMessage(std::function<void(const std::string&)> callback);
    void setOnInsertCode(std::function<void(const std::string&)> callback);
    void setOnReplaceCode(std::function<void(const std::string&)> callback);
    void setOnGetSelectedCode(std::function<std::string()> callback);
    void setOnGetCurrentFile(std::function<std::string()> callback);

    // 输入处理
    bool handleInput(ftxui::Event event);

  private:
    // UI组件
    ftxui::Component input_component_;
    ftxui::Component messages_component_;
    ftxui::Component action_buttons_component_;
    ftxui::Component main_component_;

    // 数据
    Theme& theme_;
    bool visible_;
    std::vector<ChatMessage> messages_;
    std::string current_input_;
    int selected_message_index_;
    int scroll_offset_;
    bool is_streaming_;
    std::string current_streaming_model_;
    std::vector<pnana::features::ai_client::ToolCall> current_tool_calls_;

    // 会话管理
    std::vector<std::pair<std::string, std::string>>
        conversation_history_; // user_message -> ai_response
    std::string current_conversation_topic_;
    int conversation_turn_count_;

    // 最大显示消息数
    static constexpr size_t MAX_VISIBLE_MESSAGES = 50;

    // 回调函数
    std::function<void(const std::string&)> on_send_message_;
    std::function<void(const std::string&)> on_insert_code_;
    std::function<void(const std::string&)> on_replace_code_;
    std::function<std::string()> on_get_selected_code_;
    std::function<std::string()> on_get_current_file_;

    // 私有方法
    ftxui::Element renderMessages();
    ftxui::Element renderInput();
    ftxui::Element renderActionButtons();
    ftxui::Element renderMessage(const ChatMessage& message);

    void scrollUp();
    void scrollDown();
    void submitMessage();

    // 预设提示词
    std::vector<std::string> getQuickActions() const;
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_AI_ASSISTANT_PANEL_H
