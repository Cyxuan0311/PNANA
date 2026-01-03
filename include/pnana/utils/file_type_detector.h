#ifndef PNANA_UTILS_FILE_TYPE_DETECTOR_H
#define PNANA_UTILS_FILE_TYPE_DETECTOR_H

#include <string>

namespace pnana {
namespace utils {

// 文件类型检测器
class FileTypeDetector {
  public:
    // 根据文件名和扩展名检测文件类型
    static std::string detectFileType(const std::string& filename, const std::string& extension);
};

} // namespace utils
} // namespace pnana

#endif // PNANA_UTILS_FILE_TYPE_DETECTOR_H
