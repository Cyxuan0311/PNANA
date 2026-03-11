#ifndef PNANA_UTILS_FILE_TYPE_ICON_MAPPER_H
#define PNANA_UTILS_FILE_TYPE_ICON_MAPPER_H

#include "ui/icons.h"
#include <map>
#include <string>

namespace pnana {
namespace utils {

// 文件类型图标映射器
class FileTypeIconMapper {
  public:
    FileTypeIconMapper();

    // 获取文件类型的图标
    std::string getIcon(const std::string& file_type) const;

    // 设置自定义图标映射（用于配置）
    void setCustomIcon(const std::string& file_type, const std::string& icon);

    // 清除自定义图标映射
    void clearCustomIcons();

  private:
    // 自定义图标映射（优先级最高）
    std::map<std::string, std::string> custom_icons_;

    // 默认图标映射（通过 icons.h 的 getFileTypeIcon 函数）
};

// 根据文件名和扩展名获取图标（与文件列表、状态栏共用同一套逻辑，保证显示一致）
// mapper 可为空，为空时仅使用 icons.h 内置映射，不应用自定义配置
std::string getIconForFile(const std::string& filename, std::string ext,
                           const FileTypeIconMapper* mapper = nullptr);

} // namespace utils
} // namespace pnana

#endif // PNANA_UTILS_FILE_TYPE_ICON_MAPPER_H
