#ifndef PNANA_UI_AI_CONFIG_DIALOG_H
#define PNANA_UI_AI_CONFIG_DIALOG_H

#include "features/ai_config/ai_config.h"
#include "ui/theme.h"
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <functional>
#include <string>
#include <vector>

namespace pnana {
namespace ui {

// AI配置对话框类
class AIConfigDialog {
  public:
    AIConfigDialog(Theme& theme);

    void open();
    void close();
    bool isVisible() const {
        return visible_;
    }

    // 处理输入事件
    bool handleInput(ftxui::Event event);

    // 渲染UI
    ftxui::Element render();

    // 回调函数
    std::function<void()> on_save_;
    std::function<void()> on_cancel_;

  private:
    Theme& theme_;
    pnana::features::ai_config::AIConfig& ai_config_;
    bool visible_;
    int selected_option_;

    // 当前编辑的配置
    pnana::features::ai_config::AIProviderConfig current_config_;

    // UI状态
    int provider_index_;
    int api_key_index_;
    int endpoint_index_;
    int model_index_;
    int max_tokens_index_;
    int temperature_index_;

    // 可用选项
    std::vector<std::string> available_providers_;
    std::vector<pnana::features::ai_config::AIModel> available_models_;

    // 渲染各个组件
    ftxui::Element renderTitle() const;
    ftxui::Element renderProviderSelector() const;
    ftxui::Element renderApiKeyField() const;
    ftxui::Element renderEndpointField() const;
    ftxui::Element renderModelSelector() const;
    ftxui::Element renderMaxTokensField() const;
    ftxui::Element renderTemperatureField() const;
    ftxui::Element renderButtons() const;

    // 导航方法
    void selectNext();
    void selectPrevious();
    void apply();
    void cancel();

    // 输入处理
    void handleMaxTokensInput(const std::string& input);
    void handleTemperatureInput(const std::string& input);
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_AI_CONFIG_DIALOG_H
