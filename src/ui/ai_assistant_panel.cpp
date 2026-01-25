#include "ui/ai_assistant_panel.h"
#ifdef BUILD_AI_CLIENT_SUPPORT
#include "features/ai_client/ai_client.h"
#endif
#include "ui/icons.h"
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <regex>
#include <sstream>

using namespace ftxui;

// Custom border decorator with theme color
static inline Decorator borderWithColor(Color border_color) {
    return [=](Element child) -> Element {
        return child | border | ftxui::color(border_color);
    };
}

// Custom borderRounded decorator with theme color
static inline Decorator borderRoundedWithColor(Color border_color) {
    return [=](Element child) -> Element {
        return child | borderRounded | ftxui::color(border_color);
    };
}

namespace pnana {
namespace ui {

AIAssistantPanel::AIAssistantPanel(Theme& theme)
    : theme_(theme), visible_(false), selected_message_index_(0), scroll_offset_(0),
      is_streaming_(false) {
    // 初始化组件
    input_component_ = Input(&current_input_, "Ask me anything about your code...");
    messages_component_ = Renderer([this] {
        return renderMessages();
    });
    action_buttons_component_ = Renderer([this] {
        return renderActionButtons();
    });

    // 创建主组件
    Components container_components = {messages_component_, Renderer([] {
                                           return separator();
                                       }),
                                       action_buttons_component_, Renderer([] {
                                           return separator();
                                       }),
                                       input_component_};
    auto container = Container::Vertical(container_components);

    main_component_ = Renderer(container, [this] {
        return render();
    });

    // 设置输入组件的回调
    input_component_ |= CatchEvent([this](Event event) {
        if (event == Event::Return) {
            submitMessage();
            return true;
        }
        if (event == Event::Escape) {
            hide();
            return true;
        }
        return false;
    });
}

Element AIAssistantPanel::render() {
    if (!visible_) {
        return text("");
    }

    auto& colors = theme_.getColors();

    Elements content;
    content.push_back(renderMessages());
    content.push_back(separator());
    content.push_back(renderActionButtons());
    content.push_back(separator());
    content.push_back(renderInput());

    return window(text(" AI Assistant ") | color(colors.success) | bold, vbox(std::move(content))) |
           size(WIDTH, GREATER_THAN, 80) | size(HEIGHT, GREATER_THAN, 30) |
           bgcolor(colors.background) | borderWithColor(colors.dialog_border);
}

Component AIAssistantPanel::getComponent() {
    return main_component_;
}

void AIAssistantPanel::show() {
    visible_ = true;
    current_input_.clear();
}

void AIAssistantPanel::hide() {
    visible_ = false;
}

void AIAssistantPanel::addMessage(const ChatMessage& message) {
    messages_.push_back(message);
    // 限制消息数量
    if (messages_.size() > MAX_VISIBLE_MESSAGES) {
        messages_.erase(messages_.begin());
    }
    // 自动滚动到底部
    scroll_offset_ = std::max(0, static_cast<int>(messages_.size()) - 20);
}

void AIAssistantPanel::addUserMessage(const std::string& content) {
    addMessage(ChatMessage(MessageType::USER, content));
}

void AIAssistantPanel::addAssistantMessage(const std::string& content, const std::string& model) {
    addMessage(ChatMessage(MessageType::ASSISTANT, content, model));
}

void AIAssistantPanel::addSystemMessage(const std::string& content) {
    addMessage(ChatMessage(MessageType::SYSTEM, content));
}

void AIAssistantPanel::addErrorMessage(const std::string& content) {
    addMessage(ChatMessage(MessageType::ERROR, content));
}

void AIAssistantPanel::clearMessages() {
    messages_.clear();
    scroll_offset_ = 0;
}

void AIAssistantPanel::startStreamingResponse(const std::string& model) {
    is_streaming_ = true;
    current_streaming_model_ = model;
    addMessage(ChatMessage(MessageType::ASSISTANT, "", model));
}

void AIAssistantPanel::appendStreamingContent(const std::string& content) {
    if (is_streaming_ && !messages_.empty()) {
        messages_.back().content += content;
        messages_.back().is_streaming = true;
    }
}

void AIAssistantPanel::finishStreamingResponse() {
    if (is_streaming_ && !messages_.empty()) {
        messages_.back().is_streaming = false;
        is_streaming_ = false;
        current_streaming_model_.clear();
        clearToolCalls(); // 清除工具调用显示
    }
}

// 工具调用显示管理
#ifdef BUILD_AI_CLIENT_SUPPORT
void AIAssistantPanel::addToolCall(const pnana::features::ai_client::ToolCall& tool_call) {
    current_tool_calls_.push_back(tool_call);
}
#endif // BUILD_AI_CLIENT_SUPPORT

void AIAssistantPanel::clearToolCalls() {
#ifdef BUILD_AI_CLIENT_SUPPORT
    current_tool_calls_.clear();
#else
    current_tool_calls_ = 0; // Reset placeholder
#endif
}

// 会话管理
void AIAssistantPanel::startNewConversation() {
    conversation_history_.clear();
    current_conversation_topic_.clear();
    conversation_turn_count_ = 0;
}

void AIAssistantPanel::addToConversationHistory(const std::string& user_message,
                                                const std::string& ai_response) {
    conversation_history_.emplace_back(user_message, ai_response);
    conversation_turn_count_++;

    // 更新对话主题（基于第一个用户消息）
    if (current_conversation_topic_.empty() && !user_message.empty()) {
        // 简单的主题提取：取前20个字符作为主题标识
        current_conversation_topic_ =
            user_message.substr(0, std::min(size_t(20), user_message.length()));
        if (user_message.length() > 20) {
            current_conversation_topic_ += "...";
        }
    }

    // 限制历史记录长度
    if (conversation_history_.size() > 10) {
        conversation_history_.erase(conversation_history_.begin());
    }
}

std::string AIAssistantPanel::getConversationSummary() const {
    if (conversation_history_.empty()) {
        return "No previous conversation.";
    }

    std::string summary =
        "Conversation summary (" + std::to_string(conversation_turn_count_) + " turns):\n";
    summary += "Topic: " +
               (current_conversation_topic_.empty() ? "General" : current_conversation_topic_) +
               "\n\n";

    for (size_t i = 0; i < conversation_history_.size(); ++i) {
        const auto& [user_msg, ai_response] = conversation_history_[i];
        summary += "Turn " + std::to_string(i + 1) + ":\n";
        summary += "User: " + user_msg.substr(0, 50) + (user_msg.length() > 50 ? "..." : "") + "\n";
        summary +=
            "AI: " + ai_response.substr(0, 50) + (ai_response.length() > 50 ? "..." : "") + "\n\n";
    }

    return summary;
}

bool AIAssistantPanel::isRelatedToCurrentTopic(const std::string& message) const {
    if (current_conversation_topic_.empty()) {
        return false;
    }

    // 简单的相关性检查：检查关键词重叠
    std::string lower_topic = current_conversation_topic_;
    std::string lower_message = message;
    std::transform(lower_topic.begin(), lower_topic.end(), lower_topic.begin(), ::tolower);
    std::transform(lower_message.begin(), lower_message.end(), lower_message.begin(), ::tolower);

    // 提取关键词（简化实现）
    std::vector<std::string> topic_words;
    std::vector<std::string> message_words;

    std::istringstream iss_topic(lower_topic);
    std::istringstream iss_message(lower_message);
    std::string word;

    while (iss_topic >> word) {
        if (word.length() > 3) { // 只考虑较长的词
            topic_words.push_back(word);
        }
    }

    while (iss_message >> word) {
        if (word.length() > 3) {
            message_words.push_back(word);
        }
    }

    // 检查是否有共同关键词
    for (const auto& topic_word : topic_words) {
        for (const auto& message_word : message_words) {
            if (topic_word == message_word) {
                return true;
            }
        }
    }

    return false;
}

void AIAssistantPanel::sendMessage(const std::string& message) {
    if (on_send_message_) {
        on_send_message_(message);
    }
}

Element AIAssistantPanel::renderMessages() {
    auto& colors = theme_.getColors();
    Elements message_elements;

    // 显示消息历史
    size_t start_idx = scroll_offset_;
    size_t end_idx = std::min(start_idx + 20, messages_.size());

    for (size_t i = start_idx; i < end_idx; ++i) {
        message_elements.push_back(renderMessage(messages_[i]));
        if (i < end_idx - 1) {
            message_elements.push_back(separatorLight());
        }
    }

    // 如果没有消息，显示欢迎信息
    if (messages_.empty()) {
        message_elements.push_back(
            vbox({hbox({text("Welcome to AI Assistant! ") | color(colors.success) | bold,
                        text(icons::CODE) | color(colors.success)}) |
                      center,
                  text(""), text("I can help you with:") | color(colors.comment),
                  text("• Code explanation and documentation"),
                  text("• Code generation and refactoring"), text("• Bug fixing and debugging"),
                  text("• File and codebase analysis"), text("• Code formatting and style"),
                  text("• Function extraction and optimization"),
                  text("• Running terminal commands"), text("• Smart search and navigation"),
                  text(""),
                  text("Commands: /help, /explain, /review, /format, /extract, /search, /run") |
                      color(colors.comment),
                  text("Conversation: /summary, /clear") | color(colors.comment),
                  text("Or just type natural language requests!") | color(colors.comment) | dim}) |
            center);
    }

    return yframe(vbox(std::move(message_elements))) | size(HEIGHT, EQUAL, 20);
}

Element AIAssistantPanel::renderMessage(const ChatMessage& message) {
    auto& colors = theme_.getColors();

    // 消息头部（时间戳和发送者）
    Elements header_elements;
    header_elements.push_back(text(message.timestamp) | color(colors.comment) | dim);

    std::string sender_icon;
    Color sender_color = colors.foreground;
    switch (message.type) {
        case MessageType::USER:
            sender_icon = icons::USER;
            sender_color = colors.function;
            break;
        case MessageType::ASSISTANT:
            sender_icon = icons::CODE;
            sender_color = colors.success;
            if (!message.model_used.empty()) {
                header_elements.push_back(text(" • " + message.model_used) | color(colors.comment) |
                                          dim);
            }
            break;
        case MessageType::SYSTEM:
            sender_icon = icons::INFO_CIRCLE;
            sender_color = colors.keyword;
            break;
        case MessageType::ERROR:
            sender_icon = icons::EXCLAMATION_CIRCLE;
            sender_color = colors.error;
            break;
    }

    header_elements.push_back(text(" " + sender_icon) | color(sender_color));

    Element header = hbox(std::move(header_elements));

    // 消息内容
    Elements content_elements;
    std::string content = message.content;

    // 处理流式输出
    if (message.is_streaming) {
        // 添加更明显的流式指示器
        static int cursor_frame = 0;
        cursor_frame = (cursor_frame + 1) % 4;
        std::string cursors[] = {"|", "/", "-", "\\"};
        content += " " + std::string(cursors[cursor_frame]);
    }

    // 简单的代码块检测和渲染
    if (content.find("```") != std::string::npos) {
        // 这里可以实现代码语法高亮
        content_elements.push_back(text(content) | color(colors.foreground));
    } else {
        content_elements.push_back(text(content) | color(colors.foreground));
    }

    Element message_content = vbox(std::move(content_elements)) | flex;

    Elements message_parts = {std::move(header), std::move(message_content)};

    // 如果是流式输出，添加状态指示器
    if (message.is_streaming) {
        Elements status_elements;
        status_elements.push_back(hbox({text("🤖 AI is thinking") | color(colors.comment) | dim,
                                        text("...") | color(colors.success) | bold}) |
                                  center);

        // 如果有工具调用信息，显示工具使用状态
#ifdef BUILD_AI_CLIENT_SUPPORT
        if (!current_tool_calls_.empty()) {
            status_elements.push_back(text("🔧 Using tools:") | color(colors.keyword) | center);
            for (const auto& tool_call : current_tool_calls_) {
                status_elements.push_back(
                    hbox(ftxui::Elements{
                        ftxui::text("  • ") | color(colors.comment),
                        ftxui::text(tool_call.function_name) | color(colors.function) | bold}) |
                    center);
            }
        }
#endif

        message_parts.push_back(vbox(std::move(status_elements)));
    }

    return vbox(std::move(message_parts));
}

Element AIAssistantPanel::renderInput() {
    auto& colors = theme_.getColors();

    return vbox(
        {text("Your message (or /help for commands):") | color(colors.comment),
         input_component_->Render() | bgcolor(colors.current_line) | color(colors.foreground),
         text("Enter: Send  •  /help: Commands  •  Esc: Close  •  ↑↓: Navigate") |
             color(colors.comment) | dim});
}

Element AIAssistantPanel::renderActionButtons() {
    auto& colors = theme_.getColors();

    Elements buttons;
    auto quick_actions = getQuickActions();

    for (size_t i = 0; i < quick_actions.size(); ++i) {
        std::string action = quick_actions[i];
        buttons.push_back(text(action) | color(colors.function) | bgcolor(colors.selection) |
                          center);
        if (i < quick_actions.size() - 1) {
            buttons.push_back(text(" "));
        }
    }

    return hbox(std::move(buttons)) | bgcolor(colors.background) |
           borderRoundedWithColor(colors.comment) | size(HEIGHT, EQUAL, 3);
}

std::vector<std::string> AIAssistantPanel::getQuickActions() const {
    return {"Explain Code", "Generate Function", "Refactor",
            "Add Comments", "Fix Errors",        "Optimize"};
}

#ifdef BUILD_AI_CLIENT_SUPPORT
// 获取工具定义列表
std::vector<pnana::features::ai_client::ToolDefinition> AIAssistantPanel::getToolDefinitions()
    const {
    using namespace pnana::features::ai_client;

    std::vector<ToolDefinition> tools;

    // 文件读取工具
    ToolDefinition read_file_tool;
    read_file_tool.name = "read_file";
    read_file_tool.description = "Read the contents of a file";
    read_file_tool.parameters = {
        {"file_path", "string", "Path to the file to read", true, nlohmann::json()},
        {"start_line", "integer", "Starting line number (optional)", false, nlohmann::json()},
        {"end_line", "integer", "Ending line number (optional)", false, nlohmann::json()}};
    tools.push_back(read_file_tool);

    // 文件搜索工具
    ToolDefinition grep_tool;
    grep_tool.name = "grep_search";
    grep_tool.description = "Search for text patterns in files using regex";
    grep_tool.parameters = {
        {"pattern", "string", "Regular expression pattern to search for", true, nlohmann::json()},
        {"file_path", "string", "Specific file to search in (optional)", false, nlohmann::json()},
        {"include_pattern", "string", "File pattern to include (e.g., *.cpp,*.h)", false,
         nlohmann::json()},
        {"case_sensitive", "boolean", "Whether the search should be case sensitive", false,
         nlohmann::json()}};
    tools.push_back(grep_tool);

    // 符号搜索工具
    ToolDefinition symbol_search_tool;
    symbol_search_tool.name = "find_symbol";
    symbol_search_tool.description = "Find symbol definitions and references in the codebase";
    symbol_search_tool.parameters = {
        {"symbol_name", "string", "Name of the symbol to find", true, nlohmann::json()},
        {"symbol_type", "string", "Type of symbol (function,class,variable)", false,
         nlohmann::json()},
        {"file_extension", "string", "File extension to search in", false, nlohmann::json()}};
    tools.push_back(symbol_search_tool);

    // 运行命令工具
    ToolDefinition run_command_tool;
    run_command_tool.name = "run_terminal_command";
    run_command_tool.description = "Execute a terminal command and return the output";
    run_command_tool.parameters = {
        {"command", "string", "The terminal command to execute", true, nlohmann::json()},
        {"working_directory", "string", "Working directory for the command (optional)", false,
         nlohmann::json()},
        {"timeout_seconds", "integer", "Command timeout in seconds", false, nlohmann::json()}};
    tools.push_back(run_command_tool);

    // 列出目录工具
    ToolDefinition list_dir_tool;
    list_dir_tool.name = "list_directory";
    list_dir_tool.description = "List contents of a directory";
    list_dir_tool.parameters = {
        {"directory_path", "string", "Path to the directory to list", true, nlohmann::json()},
        {"show_hidden", "boolean", "Whether to show hidden files", false, nlohmann::json()},
        {"recursive", "boolean", "Whether to list recursively", false, nlohmann::json()}};
    tools.push_back(list_dir_tool);

    // 代码分析工具
    ToolDefinition analyze_code_tool;
    analyze_code_tool.name = "analyze_code";
    analyze_code_tool.description = "Analyze code for potential issues, bugs, or improvements";
    analyze_code_tool.parameters = {
        {"code", "string", "The code to analyze", true, nlohmann::json()},
        {"language", "string", "Programming language of the code", false, nlohmann::json()},
        {"focus_areas", "array", "Specific areas to focus on (performance,security,style)", false,
         nlohmann::json()}};
    tools.push_back(analyze_code_tool);

    // 项目结构分析工具
    ToolDefinition project_structure_tool;
    project_structure_tool.name = "analyze_project_structure";
    project_structure_tool.description = "Analyze the project structure and provide insights";
    project_structure_tool.parameters = {
        {"root_path", "string", "Root path of the project", false, nlohmann::json()},
        {"max_depth", "integer", "Maximum directory depth to analyze", false, nlohmann::json()}};
    tools.push_back(project_structure_tool);

    // 代码生成工具
    ToolDefinition generate_code_tool;
    generate_code_tool.name = "generate_code";
    generate_code_tool.description = "Generate code based on description and context";
    generate_code_tool.parameters = {
        {"description", "string", "Description of what code to generate", true, nlohmann::json()},
        {"language", "string", "Programming language", false, nlohmann::json()},
        {"context_code", "string", "Surrounding context code", false, nlohmann::json()},
        {"style_guide", "string", "Coding style guidelines to follow", false, nlohmann::json()}};
    tools.push_back(generate_code_tool);

    // 代码重构工具
    ToolDefinition refactor_code_tool;
    refactor_code_tool.name = "refactor_code";
    refactor_code_tool.description = "Suggest refactoring improvements for code";
    refactor_code_tool.parameters = {
        {"code", "string", "The code to refactor", true, nlohmann::json()},
        {"refactor_type", "string", "Type of refactoring (extract_method,rename_variable,etc.)",
         false, nlohmann::json()},
        {"constraints", "string", "Any constraints or requirements", false, nlohmann::json()}};
    tools.push_back(refactor_code_tool);

    // 代码编辑工具
    ToolDefinition edit_file_tool;
    edit_file_tool.name = "edit_file";
    edit_file_tool.description = "Edit a file by replacing text at specific locations";
    edit_file_tool.parameters = {
        {"file_path", "string", "Path to the file to edit", true, nlohmann::json()},
        {"old_string", "string", "Text to replace", true, nlohmann::json()},
        {"new_string", "string", "Replacement text", true, nlohmann::json()},
        {"start_line", "integer", "Starting line number (optional)", false, nlohmann::json()},
        {"end_line", "integer", "Ending line number (optional)", false, nlohmann::json()}};
    tools.push_back(edit_file_tool);

    // 代码格式化工具
    ToolDefinition format_code_tool;
    format_code_tool.name = "format_code";
    format_code_tool.description = "Format code according to language-specific conventions";
    format_code_tool.parameters = {
        {"code", "string", "The code to format", true, nlohmann::json()},
        {"language", "string", "Programming language", true, nlohmann::json()},
        {"style", "string", "Formatting style (e.g., google, llvm, microsoft)", false,
         nlohmann::json()}};
    tools.push_back(format_code_tool);

    // 代码重构工具
    ToolDefinition extract_function_tool;
    extract_function_tool.name = "extract_function";
    extract_function_tool.description = "Extract selected code into a new function";
    extract_function_tool.parameters = {
        {"code", "string", "The code to extract", true, nlohmann::json()},
        {"function_name", "string", "Name for the new function", true, nlohmann::json()},
        {"language", "string", "Programming language", true, nlohmann::json()},
        {"parameters", "array", "Function parameters", false, nlohmann::json()}};
    tools.push_back(extract_function_tool);

    // 导入管理工具
    ToolDefinition manage_imports_tool;
    manage_imports_tool.name = "manage_imports";
    manage_imports_tool.description = "Add, remove, or organize import statements";
    manage_imports_tool.parameters = {
        {"file_path", "string", "Path to the file to modify", true, nlohmann::json()},
        {"imports_to_add", "array", "Import statements to add", false, nlohmann::json()},
        {"imports_to_remove", "array", "Import statements to remove", false, nlohmann::json()},
        {"language", "string", "Programming language", true, nlohmann::json()}};
    tools.push_back(manage_imports_tool);

    // 代码审查工具
    ToolDefinition code_review_tool;
    code_review_tool.name = "code_review";
    code_review_tool.description = "Perform automated code review and suggest improvements";
    code_review_tool.parameters = {
        {"code", "string", "The code to review", true, nlohmann::json()},
        {"language", "string", "Programming language", true, nlohmann::json()},
        {"focus_areas", "array", "Areas to focus on (performance,security,style,maintainability)",
         false, nlohmann::json()},
        {"severity_level", "string", "Minimum severity level to report (low,medium,high)", false,
         nlohmann::json()}};
    tools.push_back(code_review_tool);

    return tools;
}
#endif // BUILD_AI_CLIENT_SUPPORT

#ifdef BUILD_AI_CLIENT_SUPPORT
// 执行工具调用
pnana::features::ai_client::ToolCallResult AIAssistantPanel::executeToolCall(
    const pnana::features::ai_client::ToolCall& tool_call) {
    using namespace pnana::features::ai_client;

    ToolCallResult result;
    result.tool_call_id = tool_call.id;
    result.tool_name = tool_call.function_name;

    try {
        if (tool_call.function_name == "read_file") {
            std::string file_path = tool_call.arguments.value("file_path", "");
            int start_line = tool_call.arguments.value("start_line", 0);
            int end_line = tool_call.arguments.value("end_line", 0);

            if (file_path.empty()) {
                result.success = false;
                result.error_message = "file_path parameter is required";
                return result;
            }

            std::ifstream file(file_path);
            if (!file.is_open()) {
                result.success = false;
                result.error_message = "Failed to open file: " + file_path;
                return result;
            }

            std::string content((std::istreambuf_iterator<char>(file)),
                                std::istreambuf_iterator<char>());
            file.close();

            // 如果指定了行范围，提取相应行
            if (start_line > 0 || end_line > 0) {
                std::istringstream iss(content);
                std::string line;
                std::vector<std::string> lines;
                while (std::getline(iss, line)) {
                    lines.push_back(line);
                }

                int actual_start = std::max(0, start_line - 1); // 转换为0-based索引
                int actual_end = end_line > 0 ? std::min(static_cast<int>(lines.size()), end_line)
                                              : static_cast<int>(lines.size());

                std::string filtered_content;
                for (int i = actual_start; i < actual_end && i < static_cast<int>(lines.size());
                     ++i) {
                    if (i > actual_start)
                        filtered_content += "\n";
                    filtered_content += lines[i];
                }
                content = filtered_content;
            }

            result.success = true;
            result.result["content"] = content;
            result.result["file_path"] = file_path;
            if (start_line > 0)
                result.result["start_line"] = start_line;
            if (end_line > 0)
                result.result["end_line"] = end_line;

        } else if (tool_call.function_name == "grep_search") {
            std::string pattern = tool_call.arguments.value("pattern", "");
            std::string file_path = tool_call.arguments.value("file_path", "");
            bool case_sensitive = tool_call.arguments.value("case_sensitive", true);

            if (pattern.empty()) {
                result.success = false;
                result.error_message = "pattern parameter is required";
                return result;
            }

            // 这里实现搜索逻辑（简化版）
            std::vector<std::string> matches;
            if (!file_path.empty()) {
                // 搜索特定文件
                std::ifstream file(file_path);
                if (file.is_open()) {
                    std::string line;
                    int line_num = 1;
                    std::regex regex_pattern(pattern, case_sensitive
                                                          ? std::regex_constants::ECMAScript
                                                          : std::regex_constants::icase);
                    while (std::getline(file, line)) {
                        if (std::regex_search(line, regex_pattern)) {
                            matches.push_back("Line " + std::to_string(line_num) + ": " + line);
                        }
                        line_num++;
                    }
                }
            } else {
                // 搜索整个项目（简化实现）
                // 实际应该递归搜索目录
                result.success = false;
                result.error_message = "Directory search not implemented yet";
                return result;
            }

            result.success = true;
            result.result["matches"] = matches;
            result.result["pattern"] = pattern;

        } else if (tool_call.function_name == "run_terminal_command") {
            std::string command = tool_call.arguments.value("command", "");
            std::string working_dir = tool_call.arguments.value("working_directory", ".");

            if (command.empty()) {
                result.success = false;
                result.error_message = "command parameter is required";
                return result;
            }

            // 执行命令（简化实现）
            // 注意：实际应用中应该更安全地处理命令执行
            std::string full_command = "cd " + working_dir + " && " + command + " 2>&1";
            FILE* pipe = popen(full_command.c_str(), "r");
            if (!pipe) {
                result.success = false;
                result.error_message = "Failed to execute command";
                return result;
            }

            char buffer[128];
            std::string output;
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                output += buffer;
            }

            int exit_code = pclose(pipe);
            result.success = true;
            result.result["output"] = output;
            result.result["exit_code"] = exit_code;
            result.result["command"] = command;

        } else if (tool_call.function_name == "list_directory") {
            std::string dir_path = tool_call.arguments.value("directory_path", ".");
            bool show_hidden = tool_call.arguments.value("show_hidden", false);

            try {
                std::vector<std::string> entries;
                for (const auto& entry : std::filesystem::directory_iterator(dir_path)) {
                    std::string name = entry.path().filename().string();
                    if (!show_hidden && name.find(".") == 0) {
                        continue;
                    }
                    entries.push_back(entry.is_directory() ? "[DIR] " + name : "[FILE] " + name);
                }

                result.success = true;
                result.result["entries"] = entries;
                result.result["directory"] = dir_path;

            } catch (const std::exception& e) {
                result.success = false;
                result.error_message = std::string("Failed to list directory: ") + e.what();
            }

        } else if (tool_call.function_name == "analyze_code") {
            std::string code = tool_call.arguments.value("code", "");
            std::string language = tool_call.arguments.value("language", "auto");

            if (code.empty()) {
                result.success = false;
                result.error_message = "code parameter is required";
                return result;
            }

            // 简单的代码分析（可以扩展）
            std::vector<std::string> suggestions;

            // 检查一些基本问题
            if (code.find("TODO") != std::string::npos) {
                suggestions.push_back("Found TODO comments that should be addressed");
            }
            if (code.find("FIXME") != std::string::npos) {
                suggestions.push_back("Found FIXME comments indicating known issues");
            }
            if (code.find("console.log") != std::string::npos && language == "javascript") {
                suggestions.push_back(
                    "Consider removing console.log statements in production code");
            }

            result.success = true;
            result.result["suggestions"] = suggestions;
            result.result["language"] = language;
            result.result["code_length"] = code.length();

        } else if (tool_call.function_name == "find_symbol") {
            std::string symbol_name = tool_call.arguments.value("symbol_name", "");
            std::string symbol_type = tool_call.arguments.value("symbol_type", "");
            std::string file_extension = tool_call.arguments.value("file_extension", "");

            if (symbol_name.empty()) {
                result.success = false;
                result.error_message = "symbol_name parameter is required";
                return result;
            }

            // 使用grep搜索符号定义和引用
            std::vector<std::string> findings;
            std::string pattern = "\\b" + symbol_name + "\\b"; // 单词边界匹配

            // 搜索定义（类、函数定义等）
            std::vector<std::string> define_patterns;
            if (symbol_type == "function" || symbol_type.empty()) {
                define_patterns.push_back(symbol_name + "\\s*\\("); // 函数定义
            }
            if (symbol_type == "class" || symbol_type.empty()) {
                define_patterns.push_back("class\\s+" + symbol_name);  // 类定义
                define_patterns.push_back("struct\\s+" + symbol_name); // 结构体定义
            }
            if (symbol_type == "variable" || symbol_type.empty()) {
                define_patterns.push_back("\\b" + symbol_name + "\\s*="); // 变量赋值
                define_patterns.push_back("\\b" + symbol_name + "\\s*;"); // 变量声明
            }

            // 这里应该实现递归目录搜索，暂时用简化实现
            result.success = true;
            result.result["symbol_name"] = symbol_name;
            result.result["findings"] = findings; // 实际实现中会填充搜索结果
            result.result["note"] = "Symbol search implementation needs filesystem access";

        } else if (tool_call.function_name == "analyze_project_structure") {
            std::string root_path = tool_call.arguments.value("root_path", ".");
            int max_depth = tool_call.arguments.value("max_depth", 3);

            try {
                std::vector<std::string> structure;
                analyzeDirectoryStructure(root_path, structure, 0, max_depth);

                result.success = true;
                result.result["structure"] = structure;
                result.result["root_path"] = root_path;
                result.result["max_depth"] = max_depth;

            } catch (const std::exception& e) {
                result.success = false;
                result.error_message =
                    std::string("Failed to analyze project structure: ") + e.what();
            }

        } else if (tool_call.function_name == "generate_code") {
            std::string description = tool_call.arguments.value("description", "");
            std::string language = tool_call.arguments.value("language", "cpp");
            std::string context_code = tool_call.arguments.value("context_code", "");
            std::string style_guide = tool_call.arguments.value("style_guide", "");

            if (description.empty()) {
                result.success = false;
                result.error_message = "description parameter is required";
                return result;
            }

            // 这里应该调用代码生成逻辑，暂时返回占位符
            std::string generated_code = "// Generated code for: " + description + "\n";
            generated_code += "// Language: " + language + "\n";
            if (!style_guide.empty()) {
                generated_code += "// Style guide: " + style_guide + "\n";
            }
            generated_code += "\n// TODO: Implement the actual functionality\n";

            result.success = true;
            result.result["generated_code"] = generated_code;
            result.result["description"] = description;
            result.result["language"] = language;

        } else if (tool_call.function_name == "refactor_code") {
            std::string code = tool_call.arguments.value("code", "");
            std::string refactor_type = tool_call.arguments.value("refactor_type", "");
            std::string constraints = tool_call.arguments.value("constraints", "");

            if (code.empty()) {
                result.success = false;
                result.error_message = "code parameter is required";
                return result;
            }

            // 简单的重构建议
            std::vector<std::string> suggestions;

            if (code.find("if") != std::string::npos && code.find("else") != std::string::npos) {
                suggestions.push_back("Consider using early returns to reduce nesting");
            }
            if (code.length() > 1000) {
                suggestions.push_back(
                    "Consider breaking down this large code block into smaller functions");
            }
            if (code.find("magic number") != std::string::npos ||
                std::regex_search(code, std::regex("\\b\\d{2,}\\b"))) {
                suggestions.push_back("Consider extracting magic numbers into named constants");
            }

            result.success = true;
            result.result["suggestions"] = suggestions;
            result.result["refactor_type"] = refactor_type;
            result.result["constraints"] = constraints;

        } else if (tool_call.function_name == "edit_file") {
            std::string file_path = tool_call.arguments.value("file_path", "");
            std::string old_string = tool_call.arguments.value("old_string", "");
            std::string new_string = tool_call.arguments.value("new_string", "");
            // int start_line = tool_call.arguments.value("start_line", 0);
            // int end_line = tool_call.arguments.value("end_line", 0);

            if (file_path.empty() || old_string.empty()) {
                result.success = false;
                result.error_message = "file_path and old_string parameters are required";
                return result;
            }

            try {
                std::ifstream file(file_path);
                if (!file.is_open()) {
                    result.success = false;
                    result.error_message = "Failed to open file: " + file_path;
                    return result;
                }

                std::string content((std::istreambuf_iterator<char>(file)),
                                    std::istreambuf_iterator<char>());
                file.close();

                // 查找并替换文本
                size_t pos = content.find(old_string);
                if (pos == std::string::npos) {
                    result.success = false;
                    result.error_message = "Text to replace not found in file";
                    return result;
                }

                content.replace(pos, old_string.length(), new_string);

                // 写回文件
                std::ofstream out_file(file_path);
                if (!out_file.is_open()) {
                    result.success = false;
                    result.error_message = "Failed to write to file: " + file_path;
                    return result;
                }
                out_file << content;
                out_file.close();

                result.success = true;
                result.result["file_path"] = file_path;
                result.result["changes_made"] = 1;
                result.result["bytes_modified"] =
                    static_cast<int>(new_string.length() - old_string.length());

            } catch (const std::exception& e) {
                result.success = false;
                result.error_message = std::string("File edit failed: ") + e.what();
            }

        } else if (tool_call.function_name == "format_code") {
            std::string code = tool_call.arguments.value("code", "");
            std::string language = tool_call.arguments.value("language", "");
            std::string style = tool_call.arguments.value("style", "google");

            if (code.empty() || language.empty()) {
                result.success = false;
                result.error_message = "code and language parameters are required";
                return result;
            }

            // 简单的代码格式化（实际实现需要调用相应的格式化工具）
            std::string formatted_code = code;

            // 基础的C++/C风格格式化
            if (language == "cpp" || language == "c") {
                // 这里应该调用clang-format或其他格式化工具
                formatted_code = "// Formatted code (" + style + " style)\n" + code;
            } else if (language == "python") {
                // 这里应该调用black或其他Python格式化工具
                formatted_code = "# Formatted Python code\n" + code;
            }

            result.success = true;
            result.result["formatted_code"] = formatted_code;
            result.result["language"] = language;
            result.result["style"] = style;

        } else if (tool_call.function_name == "extract_function") {
            std::string code = tool_call.arguments.value("code", "");
            std::string function_name = tool_call.arguments.value("function_name", "");
            std::string language = tool_call.arguments.value("language", "");

            if (code.empty() || function_name.empty() || language.empty()) {
                result.success = false;
                result.error_message = "code, function_name, and language parameters are required";
                return result;
            }

            // 生成提取的函数代码
            std::string extracted_function;
            std::string function_call;

            if (language == "cpp") {
                extracted_function = "void " + function_name + "() {\n";
                extracted_function += "    " + code + "\n";
                extracted_function += "}\n";
                function_call = function_name + "();";
            } else if (language == "python") {
                extracted_function = "def " + function_name + "():\n";
                extracted_function += "    " + code + "\n";
                function_call = function_name + "()";
            } else {
                extracted_function = "// Extracted function: " + function_name + "\n" + code;
                function_call = function_name + "();";
            }

            result.success = true;
            result.result["extracted_function"] = extracted_function;
            result.result["function_call"] = function_call;
            result.result["function_name"] = function_name;
            result.result["language"] = language;

        } else if (tool_call.function_name == "manage_imports") {
            std::string file_path = tool_call.arguments.value("file_path", "");
            std::string language = tool_call.arguments.value("language", "");

            if (file_path.empty() || language.empty()) {
                result.success = false;
                result.error_message = "file_path and language parameters are required";
                return result;
            }

            // 这里应该实现导入管理逻辑
            // 暂时返回成功状态
            result.success = true;
            result.result["file_path"] = file_path;
            result.result["language"] = language;
            result.result["message"] = "Import management not fully implemented yet";

        } else if (tool_call.function_name == "code_review") {
            std::string code = tool_call.arguments.value("code", "");
            std::string language = tool_call.arguments.value("language", "");

            if (code.empty() || language.empty()) {
                result.success = false;
                result.error_message = "code and language parameters are required";
                return result;
            }

            std::vector<std::string> issues;

            // 基础的代码审查规则
            if (code.find("goto") != std::string::npos) {
                issues.push_back("Avoid using 'goto' statements");
            }
            if (code.length() > 2000) {
                issues.push_back("Function is too long, consider breaking it down");
            }
            if (std::regex_search(code, std::regex("\\bif\\s*\\("))) {
                if (!std::regex_search(code, std::regex("\\bif\\s*\\(\\s*"))) {
                    issues.push_back("Add space after 'if' keyword");
                }
            }
            if (language == "cpp" && code.find("using namespace std;") != std::string::npos) {
                issues.push_back("Avoid 'using namespace std;' in header files");
            }

            result.success = true;
            result.result["issues"] = issues;
            result.result["severity"] = issues.empty() ? "clean" : "needs_attention";
            result.result["language"] = language;

        } else {
            result.success = false;
            result.error_message = "Unknown tool: " + tool_call.function_name;
        }

    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = std::string("Tool execution failed: ") + e.what();
    }

    return result;
}

// 递归分析目录结构
void AIAssistantPanel::analyzeDirectoryStructure(const std::string& path,
                                                 std::vector<std::string>& structure, int depth,
                                                 int max_depth) const {
    if (depth >= max_depth)
        return;

    try {
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            if (entry.is_directory()) {
                std::string indent(depth * 2, ' ');
                structure.push_back(indent + "[DIR] " + entry.path().filename().string());
                analyzeDirectoryStructure(entry.path().string(), structure, depth + 1, max_depth);
            } else {
                std::string indent(depth * 2, ' ');
                structure.push_back(indent + "[FILE] " + entry.path().filename().string());
            }
        }
    } catch (const std::exception&) {
        // 忽略访问错误
    }
}
#endif // BUILD_AI_CLIENT_SUPPORT

bool AIAssistantPanel::handleInput(Event event) {
    if (!visible_)
        return false;

    // 处理快捷键
    if (event == Event::F1) {
        explainCode();
        return true;
    }
    if (event == Event::F2) {
        generateCode("Generate a function");
        return true;
    }
    if (event == Event::F3) {
        refactorCode();
        return true;
    }

    // 让输入组件处理其他事件
    return main_component_->OnEvent(event);
}

void AIAssistantPanel::submitMessage() {
    if (!current_input_.empty()) {
        std::string message = current_input_;

        // 处理特殊命令
        if (message.find("/") == 0) {
            handleCommand(message);
        } else {
            // 检查是否是跟进问题
            bool is_followup = isRelatedToCurrentTopic(message) && conversation_turn_count_ > 0;

            // 添加上下文信息到消息
            std::string enhanced_message = message;
            if (is_followup) {
                enhanced_message = "[Follow-up question in ongoing conversation]\n" + message;
            }

            // 添加简短的对话历史上下文（如果有的话）
            if (conversation_turn_count_ > 0) {
                std::string context_hint =
                    "\n\n[Conversation context: " + std::to_string(conversation_turn_count_) +
                    " previous exchanges]";
                enhanced_message += context_hint;
            }

            // 普通消息，直接发送给AI
            addUserMessage(message);
            sendMessage(enhanced_message);
        }

        current_input_.clear();
    }
}

// 处理命令
void AIAssistantPanel::handleCommand(const std::string& command) {
    std::string cmd = command.substr(1); // 移除开头的'/'

    if (cmd == "help") {
        addSystemMessage(R"(
Available commands:
/help - Show this help message
/clear - Clear chat history and start new conversation
/summary - Show conversation summary

Code Analysis:
/explain - Explain selected code
/review - Review selected code for issues
/search <pattern> - Search for pattern in codebase

Code Manipulation:
/refactor - Refactor selected code
/extract - Extract selected code into a function
/format - Format selected code
/generate <description> - Generate code based on description

System:
/run <command> - Run terminal command

You can also just type natural language messages and the AI will use tools automatically.
)");
    } else if (cmd == "clear") {
        clearMessages();
        startNewConversation();
        addSystemMessage("Chat history and conversation context cleared.");
    } else if (cmd == "summary") {
        std::string summary = getConversationSummary();
        addSystemMessage(summary);
    } else if (cmd == "explain") {
        explainCode();
    } else if (cmd == "refactor") {
        refactorCode();
    } else if (cmd.find("generate ") == 0) {
        std::string description = cmd.substr(9);
        if (!description.empty()) {
            generateCode(description);
        } else {
            addSystemMessage("Usage: /generate <description>");
        }
    } else if (cmd.find("search ") == 0) {
        std::string pattern = cmd.substr(7);
        if (!pattern.empty()) {
            std::string prompt =
                "Search for the pattern '" + pattern + "' in the codebase and show me the results.";
            addUserMessage("Search: " + pattern);
            sendMessage(prompt);
        } else {
            addSystemMessage("Usage: /search <pattern>");
        }
    } else if (cmd.find("run ") == 0) {
        std::string terminal_cmd = cmd.substr(4);
        if (!terminal_cmd.empty()) {
            std::string prompt =
                "Run the terminal command '" + terminal_cmd + "' and show me the output.";
            addUserMessage("Run: " + terminal_cmd);
            sendMessage(prompt);
        } else {
            addSystemMessage("Usage: /run <command>");
        }
    } else if (cmd == "format") {
        std::string selected_code = on_get_selected_code_ ? on_get_selected_code_() : "";
        if (!selected_code.empty()) {
            std::string prompt =
                "Format this code according to best practices:\n\n" + selected_code;
            addUserMessage("Format selected code");
            sendMessage(prompt);
        } else {
            addSystemMessage("Please select some code first to format.");
        }
    } else if (cmd == "review") {
        std::string selected_code = on_get_selected_code_ ? on_get_selected_code_() : "";
        if (!selected_code.empty()) {
            std::string prompt = "Review this code and suggest improvements:\n\n" + selected_code;
            addUserMessage("Review selected code");
            sendMessage(prompt);
        } else {
            addSystemMessage("Please select some code first to review.");
        }
    } else if (cmd == "extract") {
        std::string selected_code = on_get_selected_code_ ? on_get_selected_code_() : "";
        if (!selected_code.empty()) {
            std::string prompt =
                "Extract this code into a separate function with an appropriate name:\n\n" +
                selected_code;
            addUserMessage("Extract function from selected code");
            sendMessage(prompt);
        } else {
            addSystemMessage("Please select some code first to extract.");
        }
    } else {
        addSystemMessage("Unknown command: " + cmd + ". Type /help for available commands.");
    }
}

// 快捷操作
void AIAssistantPanel::explainCode() {
    std::string selected_code = getSelectedCode();
    if (!selected_code.empty()) {
        std::string prompt = "Please explain this code:\n\n" + selected_code;
        addUserMessage("Explain selected code");
        sendMessage(prompt);
    } else {
        addSystemMessage("Please select some code first to explain.");
    }
}

void AIAssistantPanel::generateCode(const std::string& description) {
    std::string prompt = "Generate code based on this description: " + description;
    addUserMessage("Generate code: " + description);
    sendMessage(prompt);
}

void AIAssistantPanel::refactorCode() {
    std::string selected_code = getSelectedCode();
    if (!selected_code.empty()) {
        std::string prompt =
            "Please refactor this code to make it cleaner and more efficient:\n\n" + selected_code;
        addUserMessage("Refactor selected code");
        sendMessage(prompt);
    } else {
        addSystemMessage("Please select some code first to refactor.");
    }
}

void AIAssistantPanel::addComments() {
    std::string selected_code = getSelectedCode();
    if (!selected_code.empty()) {
        std::string prompt = "Please add comprehensive comments to this code:\n\n" + selected_code;
        addUserMessage("Add comments to selected code");
        sendMessage(prompt);
    } else {
        addSystemMessage("Please select some code first to add comments.");
    }
}

void AIAssistantPanel::fixErrors() {
    std::string current_file = getCurrentFileContent();
    if (!current_file.empty()) {
        std::string prompt =
            "Please analyze this code and suggest fixes for any errors or potential issues:\n\n" +
            current_file;
        addUserMessage("Analyze and fix errors in current file");
        sendMessage(prompt);
    } else {
        addSystemMessage("No file content available to analyze.");
    }
}

// 代码操作辅助方法
void AIAssistantPanel::insertCodeAtCursor(const std::string& code) {
    if (on_insert_code_) {
        on_insert_code_(code);
    }
}

void AIAssistantPanel::replaceSelectedCode(const std::string& code) {
    if (on_replace_code_) {
        on_replace_code_(code);
    }
}

std::string AIAssistantPanel::getSelectedCode() const {
    return on_get_selected_code_ ? on_get_selected_code_() : "";
}

std::string AIAssistantPanel::getCurrentFileContent() const {
    return on_get_current_file_ ? on_get_current_file_() : "";
}

// 回调设置
void AIAssistantPanel::setOnSendMessage(std::function<void(const std::string&)> callback) {
    on_send_message_ = callback;
}

void AIAssistantPanel::setOnInsertCode(std::function<void(const std::string&)> callback) {
    on_insert_code_ = callback;
}

void AIAssistantPanel::setOnReplaceCode(std::function<void(const std::string&)> callback) {
    on_replace_code_ = callback;
}

void AIAssistantPanel::setOnGetSelectedCode(std::function<std::string()> callback) {
    on_get_selected_code_ = callback;
}

void AIAssistantPanel::setOnGetCurrentFile(std::function<std::string()> callback) {
    on_get_current_file_ = callback;
}

} // namespace ui
} // namespace pnana
