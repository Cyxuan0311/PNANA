#ifndef PNANA_UTILS_ARCHIVE_VALIDATOR_H
#define PNANA_UTILS_ARCHIVE_VALIDATOR_H

#include <string>

namespace pnana {
namespace utils {

// 压缩文件格式验证器
// 通过检查文件的 magic number 来验证文件是否为有效的压缩文件
class ArchiveValidator {
  public:
    // 验证文件是否为指定类型的有效压缩文件
    // archive_path: 压缩文件路径
    // archive_type: 压缩文件类型 ("zip", "tar", "gz", "bz2", "xz", "7z", "rar", "tar.gz",
    // "tar.bz2", "tar.xz") 返回: true 如果是有效的压缩文件，false 否则
    static bool validateArchive(const std::string& archive_path, const std::string& archive_type);

    // 验证文件是否为有效的 ZIP 文件
    static bool validateZip(const std::string& archive_path);

    // 验证文件是否为有效的 TAR 文件
    static bool validateTar(const std::string& archive_path);

    // 验证文件是否为有效的 GZIP 文件
    static bool validateGzip(const std::string& archive_path);

    // 验证文件是否为有效的 BZIP2 文件
    static bool validateBzip2(const std::string& archive_path);

    // 验证文件是否为有效的 XZ 文件
    static bool validateXz(const std::string& archive_path);

    // 验证文件是否为有效的 7Z 文件
    static bool validate7z(const std::string& archive_path);

    // 验证文件是否为有效的 RAR 文件
    static bool validateRar(const std::string& archive_path);

    // 验证文件是否为有效的 TAR.GZ 文件
    static bool validateTarGz(const std::string& archive_path);

    // 验证文件是否为有效的 TAR.BZ2 文件
    static bool validateTarBz2(const std::string& archive_path);

    // 验证文件是否为有效的 TAR.XZ 文件
    static bool validateTarXz(const std::string& archive_path);

  private:
    // 读取文件的前 N 个字节
    static bool readFileHeader(const std::string& file_path, unsigned char* buffer, size_t size);

    // 检查文件大小是否足够
    static bool checkFileSize(const std::string& file_path, size_t min_size);
};

} // namespace utils
} // namespace pnana

#endif // PNANA_UTILS_ARCHIVE_VALIDATOR_H
