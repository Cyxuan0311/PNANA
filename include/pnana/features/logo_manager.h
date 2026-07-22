#ifndef PNANA_FEATURES_LOGO_MANAGER_H
#define PNANA_FEATURES_LOGO_MANAGER_H

#include <ftxui/screen/color.hpp>
#include <string>
#include <vector>

namespace pnana {
namespace core {
struct CustomLogoConfig;
}

namespace features {

struct LogoStyleEntry {
    std::string id;
    std::string display_name;
};

class LogoManager {
  public:
    LogoManager() = default;

    static void setCustomLogos(const std::vector<core::CustomLogoConfig>& custom_logos);
    static std::vector<LogoStyleEntry> getAvailableStyles();
    static std::vector<std::string> getLogoLines(const std::string& style_id);
    static bool isValidStyle(const std::string& style_id);

    // 生成逐字符平滑渐变颜色序列
    static std::vector<ftxui::Color> generateSmoothGradient(
        const std::vector<ftxui::Color>& palette, int num_rows, int num_cols);
};

} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_LOGO_MANAGER_H
