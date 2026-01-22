#include "ui/ai_config_dialog.h"
#include "features/ai_config/ai_config.h"
#include "ui/icons.h"
#include <algorithm>
#include <sstream>

using namespace ftxui;

// Custom border decorator with theme color
static inline Decorator borderWithColor(Color border_color) {
    return [=](Element child) -> Element {
        return child | border | ftxui::color(border_color);
    };
}

namespace pnana {
namespace ui {

AIConfigDialog::AIConfigDialog(Theme& theme)
    : theme_(theme), ai_config_(pnana::features::ai_config::AIConfig::getInstance()),
      visible_(false), selected_option_(0), provider_index_(0), api_key_index_(1),
      endpoint_index_(2), model_index_(3), max_tokens_index_(4), temperature_index_(5) {
    // 初始化可用选项
    available_providers_ = ai_config_.getAvailableProviders();
}

void AIConfigDialog::open() {
    visible_ = true;
    selected_option_ = 0;
    current_config_ = ai_config_.getCurrentConfig();
    available_models_ = ai_config_.getAvailableModels(current_config_.name);
}

void AIConfigDialog::close() {
    visible_ = false;
}

bool AIConfigDialog::handleInput(Event event) {
    if (!visible_)
        return false;

    if (event == Event::Escape) {
        cancel();
        return true;
    }

    if (event == Event::Return) {
        apply();
        return true;
    }

    if (event == Event::ArrowUp || event == Event::Character('k')) {
        selectPrevious();
        return true;
    }

    if (event == Event::ArrowDown || event == Event::Character('j')) {
        selectNext();
        return true;
    }

    if (event == Event::ArrowLeft || event == Event::Character('h')) {
        if (selected_option_ == provider_index_ && !available_providers_.empty()) {
            // 切换到上一个提供商
            auto it = std::find(available_providers_.begin(), available_providers_.end(),
                                current_config_.name);
            if (it != available_providers_.begin()) {
                current_config_ = ai_config_.getProviderConfig(*(--it));
                available_models_ = ai_config_.getAvailableModels(current_config_.name);
            }
        } else if (selected_option_ == model_index_ && !available_models_.empty()) {
            // 切换到上一个模型
            auto it = std::find_if(available_models_.begin(), available_models_.end(),
                                   [this](const auto& model) {
                                       return model.id == current_config_.model;
                                   });
            if (it != available_models_.begin()) {
                --it;
                current_config_.model = it->id;
            }
        }
        return true;
    }

    if (event == Event::ArrowRight || event == Event::Character('l')) {
        if (selected_option_ == provider_index_ && !available_providers_.empty()) {
            // 切换到下一个提供商
            auto it = std::find(available_providers_.begin(), available_providers_.end(),
                                current_config_.name);
            if (it != available_providers_.end() && ++it != available_providers_.end()) {
                current_config_ = ai_config_.getProviderConfig(*it);
                available_models_ = ai_config_.getAvailableModels(current_config_.name);
            }
        } else if (selected_option_ == model_index_ && !available_models_.empty()) {
            // 切换到下一个模型
            auto it = std::find_if(available_models_.begin(), available_models_.end(),
                                   [this](const auto& model) {
                                       return model.id == current_config_.model;
                                   });
            if (it != available_models_.end() && ++it != available_models_.end()) {
                current_config_.model = it->id;
            }
        }
        return true;
    }

    // 处理文本输入
    if (event.is_character()) {
        std::string ch = event.character();
        if (selected_option_ == api_key_index_) {
            current_config_.api_key += ch;
        } else if (selected_option_ == endpoint_index_) {
            current_config_.base_url += ch;
        } else if (selected_option_ == max_tokens_index_) {
            handleMaxTokensInput(ch);
        } else if (selected_option_ == temperature_index_) {
            handleTemperatureInput(ch);
        }
        return true;
    }

    if (event == Event::Backspace) {
        if (selected_option_ == api_key_index_ && !current_config_.api_key.empty()) {
            current_config_.api_key.pop_back();
        } else if (selected_option_ == endpoint_index_ && !current_config_.base_url.empty()) {
            current_config_.base_url.pop_back();
        } else if (selected_option_ == max_tokens_index_) {
            std::string tokens_str = std::to_string(current_config_.max_tokens);
            if (!tokens_str.empty()) {
                tokens_str.pop_back();
                if (tokens_str.empty()) {
                    current_config_.max_tokens = 0;
                } else {
                    try {
                        current_config_.max_tokens = std::stoi(tokens_str);
                    } catch (...) {
                        current_config_.max_tokens = 0;
                    }
                }
            }
        } else if (selected_option_ == temperature_index_) {
            std::string temp_str = std::to_string(current_config_.temperature);
            if (!temp_str.empty()) {
                temp_str.pop_back();
                if (temp_str.empty() || temp_str == ".") {
                    current_config_.temperature = 0.0f;
                } else {
                    try {
                        current_config_.temperature = std::stof(temp_str);
                    } catch (...) {
                        current_config_.temperature = 0.0f;
                    }
                }
            }
        }
        return true;
    }

    return false;
}

Element AIConfigDialog::render() {
    if (!visible_)
        return text("");

    const auto& colors = theme_.getColors();
    Elements content;

    content.push_back(renderTitle());
    content.push_back(separator());

    content.push_back(text(""));
    content.push_back(renderProviderSelector());
    content.push_back(renderApiKeyField());
    content.push_back(renderEndpointField());
    content.push_back(renderModelSelector());
    content.push_back(renderMaxTokensField());
    content.push_back(renderTemperatureField());
    content.push_back(text(""));

    content.push_back(separator());
    content.push_back(renderButtons());

    return window(text(""), vbox(content)) | size(WIDTH, EQUAL, 90) | size(HEIGHT, EQUAL, 22) |
           bgcolor(colors.dialog_bg) | borderWithColor(colors.dialog_border);
}

Element AIConfigDialog::renderTitle() const {
    const auto& colors = theme_.getColors();
    return hbox({text(" "), text(pnana::ui::icons::SETTINGS) | color(colors.success),
                 text(" AI Configuration "), text(" ")}) |
           bold | bgcolor(colors.dialog_title_bg) | color(colors.dialog_title_fg) | center;
}

Element AIConfigDialog::renderProviderSelector() const {
    const auto& colors = theme_.getColors();
    bool is_selected = (selected_option_ == provider_index_);

    // 显示当前提供商和导航箭头
    auto current_provider = current_config_.name;
    Elements provider_elements;
    auto it = std::find(available_providers_.begin(), available_providers_.end(), current_provider);
    bool can_go_left = (it != available_providers_.begin());
    bool can_go_right = (it != available_providers_.end() && ++it != available_providers_.end());

    if (can_go_left) {
        provider_elements.push_back(text("◀ ") | color(colors.comment));
    } else {
        provider_elements.push_back(text("  "));
    }

    provider_elements.push_back(text(current_provider) |
                                color(is_selected ? colors.foreground : colors.comment));

    if (can_go_right) {
        provider_elements.push_back(text(" ▶") | color(colors.comment));
    }

    return hbox({text("  "), (is_selected ? text("► ") | color(colors.success) : text("  ")),
                 text("Provider: ") | color(is_selected ? colors.foreground : colors.comment),
                 hbox(provider_elements) |
                     (is_selected ? bgcolor(colors.selection) : bgcolor(colors.background)),
                 filler()}) |
           (is_selected ? bgcolor(colors.selection) : bgcolor(colors.background));
}

Element AIConfigDialog::renderApiKeyField() const {
    const auto& colors = theme_.getColors();
    bool is_selected = (selected_option_ == api_key_index_);

    std::string display_key = current_config_.api_key;
    if (display_key.length() > 60) {
        display_key = display_key.substr(0, 57) + "...";
    }

    return hbox({text("  "), (is_selected ? text("► ") | color(colors.success) : text("  ")),
                 text("API Key: ") | color(is_selected ? colors.foreground : colors.comment),
                 text(display_key + (is_selected ? "_" : "")) |
                     color(is_selected ? colors.foreground : colors.comment) |
                     (is_selected ? bgcolor(colors.selection) : bgcolor(colors.background)),
                 filler()}) |
           (is_selected ? bgcolor(colors.selection) : bgcolor(colors.background));
}

Element AIConfigDialog::renderEndpointField() const {
    const auto& colors = theme_.getColors();
    bool is_selected = (selected_option_ == endpoint_index_);

    return hbox({text("  "), (is_selected ? text("► ") | color(colors.success) : text("  ")),
                 text("Base URL: ") | color(is_selected ? colors.foreground : colors.comment),
                 text(current_config_.base_url + (is_selected ? "_" : "")) |
                     color(is_selected ? colors.foreground : colors.comment) |
                     (is_selected ? bgcolor(colors.selection) : bgcolor(colors.background)),
                 filler()}) |
           (is_selected ? bgcolor(colors.selection) : bgcolor(colors.background));
}

Element AIConfigDialog::renderModelSelector() const {
    const auto& colors = theme_.getColors();
    bool is_selected = (selected_option_ == model_index_);

    // 找到当前模型
    auto current_model = current_config_.model;
    Elements model_elements;

    // 查找当前模型在可用模型列表中的位置
    auto it = std::find_if(available_models_.begin(), available_models_.end(),
                           [&current_model](const auto& model) {
                               return model.id == current_model;
                           });

    bool can_go_left = (it != available_models_.begin());
    bool can_go_right = (it != available_models_.end() && ++it != available_models_.end());

    if (can_go_left) {
        model_elements.push_back(text("◀ ") | color(colors.comment));
    } else {
        model_elements.push_back(text("  "));
    }

    // 显示模型名称
    std::string display_name = current_model;
    auto model_it = std::find_if(available_models_.begin(), available_models_.end(),
                                 [&current_model](const auto& model) {
                                     return model.id == current_model;
                                 });
    if (model_it != available_models_.end()) {
        display_name = model_it->name;
    }

    model_elements.push_back(text(display_name) |
                             color(is_selected ? colors.foreground : colors.comment));

    if (can_go_right) {
        model_elements.push_back(text(" ▶") | color(colors.comment));
    }

    return hbox({text("  "), (is_selected ? text("► ") | color(colors.success) : text("  ")),
                 text("Model: ") | color(is_selected ? colors.foreground : colors.comment),
                 hbox(model_elements) |
                     (is_selected ? bgcolor(colors.selection) : bgcolor(colors.background)),
                 filler()}) |
           (is_selected ? bgcolor(colors.selection) : bgcolor(colors.background));
}

Element AIConfigDialog::renderMaxTokensField() const {
    const auto& colors = theme_.getColors();
    bool is_selected = (selected_option_ == max_tokens_index_);

    std::string tokens_display =
        std::to_string(current_config_.max_tokens) + (is_selected ? "_" : "");

    return hbox({text("  "), (is_selected ? text("► ") | color(colors.success) : text("  ")),
                 text("Max Tokens: ") | color(is_selected ? colors.foreground : colors.comment),
                 text(tokens_display) | color(is_selected ? colors.foreground : colors.comment) |
                     (is_selected ? bgcolor(colors.selection) : bgcolor(colors.background)),
                 filler()}) |
           (is_selected ? bgcolor(colors.selection) : bgcolor(colors.background));
}

Element AIConfigDialog::renderTemperatureField() const {
    const auto& colors = theme_.getColors();
    bool is_selected = (selected_option_ == temperature_index_);

    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << current_config_.temperature;
    std::string temp_display = ss.str() + (is_selected ? "_" : "");

    return hbox({text("  "), (is_selected ? text("► ") | color(colors.success) : text("  ")),
                 text("Temperature: ") | color(is_selected ? colors.foreground : colors.comment),
                 text(temp_display) | color(is_selected ? colors.foreground : colors.comment) |
                     (is_selected ? bgcolor(colors.selection) : bgcolor(colors.background)),
                 filler()}) |
           (is_selected ? bgcolor(colors.selection) : bgcolor(colors.background));
}

Element AIConfigDialog::renderButtons() const {
    const auto& colors = theme_.getColors();

    Elements buttons;
    buttons.push_back(text("  "));

    // Help text
    buttons.push_back(text("↑↓") | color(colors.helpbar_key) | bold);
    buttons.push_back(text(": Navigate  "));
    buttons.push_back(text("←→") | color(colors.helpbar_key) | bold);
    buttons.push_back(text(": Change  "));
    buttons.push_back(text("Enter") | color(colors.helpbar_key) | bold);
    buttons.push_back(text(": Save  "));
    buttons.push_back(text("Esc") | color(colors.helpbar_key) | bold);
    buttons.push_back(text(": Cancel"));

    return hbox(buttons) | bgcolor(colors.helpbar_bg) | color(colors.helpbar_fg) | dim;
}

void AIConfigDialog::selectNext() {
    selected_option_ = (selected_option_ + 1) % 6;
}

void AIConfigDialog::selectPrevious() {
    selected_option_ = (selected_option_ - 1 + 6) % 6;
}

void AIConfigDialog::apply() {
    if (ai_config_.validateConfig(current_config_)) {
        ai_config_.setCurrentConfig(current_config_);
        if (on_save_) {
            on_save_();
        }
        close();
    } else {
        // 配置无效，可以在这里显示错误信息
        // 暂时只关闭对话框
        close();
    }
}

void AIConfigDialog::cancel() {
    if (on_cancel_) {
        on_cancel_();
    }
    close();
}

void AIConfigDialog::handleMaxTokensInput(const std::string& input) {
    if (input.length() == 1) {
        char c = input[0];
        if (std::isdigit(c)) {
            std::string current = std::to_string(current_config_.max_tokens);
            current += c;
            try {
                current_config_.max_tokens = std::stoi(current);
                if (current_config_.max_tokens > 32768)
                    current_config_.max_tokens = 32768; // 限制最大值
            } catch (...) {
                // 忽略无效输入
            }
        }
    }
}

void AIConfigDialog::handleTemperatureInput(const std::string& input) {
    if (input.length() == 1) {
        char c = input[0];
        if (std::isdigit(c) || c == '.') {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(2) << current_config_.temperature;
            std::string current = ss.str();
            current += c;
            try {
                float temp = std::stof(current);
                if (temp >= 0.0f && temp <= 2.0f) {
                    current_config_.temperature = temp;
                }
            } catch (...) {
                // 忽略无效输入
            }
        }
    }
}

} // namespace ui
} // namespace pnana
