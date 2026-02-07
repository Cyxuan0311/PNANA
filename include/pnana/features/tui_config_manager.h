#ifndef PNANA_FEATURES_TUI_CONFIG_MANAGER_H
#define PNANA_FEATURES_TUI_CONFIG_MANAGER_H

#include <filesystem>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace pnana {
namespace features {

struct TUIConfig {
    std::string name;                      // 工具名称
    std::string display_name;              // 显示名称
    std::string description;               // 描述
    std::vector<std::string> config_paths; // 可能的配置文件路径
    std::string category;                  // 分类（terminal, editor, file_manager等）
};

// TUI配置文件管理器
class TUIConfigManager {
  public:
    TUIConfigManager();

    // 获取所有可用的TUI配置
    std::vector<TUIConfig> getAvailableTUIConfigs() const;

    // 检查配置文件是否存在
    bool configExists(const TUIConfig& config) const;

    // 获取第一个存在的配置文件路径
    std::string getFirstAvailableConfigPath(const TUIConfig& config) const;

    // 设置配置文件打开回调
    void setConfigOpenCallback(std::function<void(const std::string&)> callback);

    // 打开指定配置
    void openConfig(const TUIConfig& config);

  private:
    std::vector<TUIConfig> tui_configs_;
    std::function<void(const std::string&)> config_open_callback_;

    // 初始化TUI配置列表
    void initializeTUIConfigs();

    // 添加单个TUI配置
    void addTUIConfig(const std::string& name, const std::string& display_name,
                      const std::string& description, const std::vector<std::string>& paths,
                      const std::string& category = "");

    // 展开路径（处理~和环境变量）
    std::filesystem::path expandPath(const std::string& path) const;
};

} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_TUI_CONFIG_MANAGER_H
