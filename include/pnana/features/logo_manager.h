#ifndef PNANA_FEATURES_LOGO_MANAGER_H
#define PNANA_FEATURES_LOGO_MANAGER_H

#include <string>
#include <vector>

namespace pnana {
namespace core {
struct CustomLogoConfig;
}

namespace features {

// Logo 样式项：id 用于配置存储，display_name 用于菜单显示
struct LogoStyleEntry {
    std::string id;
    std::string display_name;
};

// Logo 逻辑：提供多种 logo 样式及对应文本行，供欢迎界面与 Logo 菜单使用
class LogoManager {
  public:
    LogoManager() = default;

    // 设置来自配置文件的自定义 Logo（会覆盖上一轮注入）
    static void setCustomLogos(const std::vector<core::CustomLogoConfig>& custom_logos);

    // 获取所有可用样式（内置 + 自定义，id + 显示名）
    static std::vector<LogoStyleEntry> getAvailableStyles();

    // 根据样式 id 获取 logo 文本行（用于渲染）；若 id 未知则回退到 block
    static std::vector<std::string> getLogoLines(const std::string& style_id);

    // 校验样式 id 是否有效
    static bool isValidStyle(const std::string& style_id);
};

} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_LOGO_MANAGER_H
