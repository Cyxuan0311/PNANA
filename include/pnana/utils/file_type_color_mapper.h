#ifndef PNANA_UTILS_FILE_TYPE_COLOR_MAPPER_H
#define PNANA_UTILS_FILE_TYPE_COLOR_MAPPER_H

#include "ui/theme.h"
#include <string>

namespace pnana {
namespace utils {

/**
 * 文件类型颜色映射器
 * 根据文件类型和扩展名返回相应的显示颜色
 * 颜色选择符合文件类型的实际情况和常见约定
 */
class FileTypeColorMapper {
  public:
    explicit FileTypeColorMapper(const ui::Theme& theme);

    /**
     * 获取文件或目录的显示颜色
     * @param filename 文件名
     * @param is_directory 是否为目录
     * @return 显示颜色
     */
    ftxui::Color getFileColor(const std::string& filename, bool is_directory) const;

    /**
     * 根据文件类型获取颜色
     * @param file_type 文件类型（如 "cpp", "python", "javascript" 等）
     * @return 显示颜色
     */
    ftxui::Color getColorByFileType(const std::string& file_type) const;

  private:
    const ui::Theme& theme_;

    /**
     * 获取文件扩展名（小写）
     */
    static std::string getFileExtension(const std::string& filename);
};

} // namespace utils
} // namespace pnana

#endif // PNANA_UTILS_FILE_TYPE_COLOR_MAPPER_H
