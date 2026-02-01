#include "utils/archive_validator.h"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace pnana {
namespace utils {

bool ArchiveValidator::readFileHeader(const std::string& file_path, unsigned char* buffer,
                                      size_t size) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    file.read(reinterpret_cast<char*>(buffer), size);
    bool success = (file.gcount() == static_cast<std::streamsize>(size));
    file.close();

    return success;
}

bool ArchiveValidator::checkFileSize(const std::string& file_path, size_t min_size) {
    try {
        if (!fs::exists(file_path) || !fs::is_regular_file(file_path)) {
            return false;
        }

        auto file_size = fs::file_size(file_path);
        return file_size >= min_size;
    } catch (const std::exception&) {
        return false;
    }
}

bool ArchiveValidator::validateZip(const std::string& archive_path) {
    // ZIP 文件最小大小：至少要有中央目录结束记录（22字节）
    if (!checkFileSize(archive_path, 22)) {
        return false;
    }

    unsigned char header[4];
    if (!readFileHeader(archive_path, header, 4)) {
        return false;
    }

    // ZIP 文件应该以 PK\x03\x04 (本地文件头) 或 PK\x05\x06 (中央目录结束) 或 PK\x07\x08
    // (数据描述符) 开头 PK = 0x50 0x4B
    bool is_valid = (header[0] == 0x50 && header[1] == 0x4B &&
                     (header[2] == 0x03 || header[2] == 0x05 || header[2] == 0x07) &&
                     (header[3] == 0x04 || header[3] == 0x06 || header[3] == 0x08));

    // 如果不是标准开头，检查文件末尾是否有中央目录结束记录
    if (!is_valid) {
        try {
            auto file_size = fs::file_size(archive_path);
            if (file_size >= 22) {
                std::ifstream file(archive_path, std::ios::binary);
                if (file.is_open()) {
                    file.seekg(file_size - 22);
                    unsigned char eocd[22];
                    file.read(reinterpret_cast<char*>(eocd), 22);
                    if (file.gcount() == 22) {
                        // 检查中央目录结束签名 (0x06054b50)
                        is_valid = (eocd[0] == 0x50 && eocd[1] == 0x4B && eocd[2] == 0x05 &&
                                    eocd[3] == 0x06);
                    }
                    file.close();
                }
            }
        } catch (const std::exception&) {
            return false;
        }
    }

    return is_valid;
}

bool ArchiveValidator::validateTar(const std::string& archive_path) {
    // TAR 文件最小大小：至少要有 512 字节的块
    if (!checkFileSize(archive_path, 512)) {
        return false;
    }

    unsigned char header[512];
    if (!readFileHeader(archive_path, header, 512)) {
        return false;
    }

    // TAR 文件格式：每个文件/目录都有一个 512 字节的头部
    // 检查前 512 字节是否看起来像 TAR 头部
    // TAR 头部通常包含文件名（前 100 字节）、文件大小（第 124-135 字节）等
    // 简单的检查：文件名区域不应该全是 0，且文件大小字段应该是有效的八进制数或全 0

    // 检查文件名区域（前 100 字节）是否至少有一个非空字符
    bool has_name = false;
    for (int i = 0; i < 100 && i < 512; i++) {
        if (header[i] != 0) {
            has_name = true;
            break;
        }
    }

    if (!has_name) {
        return false;
    }

    // 检查文件大小字段（位置 124-135，八进制 ASCII）
    // 这只是一个基本检查，真正的 TAR 文件验证更复杂
    return true;
}

bool ArchiveValidator::validateGzip(const std::string& archive_path) {
    // GZIP 文件最小大小：至少要有 10 字节的头部
    if (!checkFileSize(archive_path, 10)) {
        return false;
    }

    unsigned char header[10];
    if (!readFileHeader(archive_path, header, 10)) {
        return false;
    }

    // GZIP 文件以 \x1f\x8b 开头
    return (header[0] == 0x1F && header[1] == 0x8B);
}

bool ArchiveValidator::validateBzip2(const std::string& archive_path) {
    // BZIP2 文件最小大小：至少要有 10 字节的头部
    if (!checkFileSize(archive_path, 10)) {
        return false;
    }

    unsigned char header[10];
    if (!readFileHeader(archive_path, header, 10)) {
        return false;
    }

    // BZIP2 文件以 "BZ" 开头，后跟版本号
    return (header[0] == 'B' && header[1] == 'Z' && header[2] == 'h' && header[3] >= '0' &&
            header[3] <= '9');
}

bool ArchiveValidator::validateXz(const std::string& archive_path) {
    // XZ 文件最小大小：至少要有 6 字节的头部
    if (!checkFileSize(archive_path, 6)) {
        return false;
    }

    unsigned char header[6];
    if (!readFileHeader(archive_path, header, 6)) {
        return false;
    }

    // XZ 文件以 \xfd 7z XZ \x00 开头
    return (header[0] == 0xFD && header[1] == 0x37 && header[2] == 0x7A && header[3] == 0x58 &&
            header[4] == 0x5A && header[5] == 0x00);
}

bool ArchiveValidator::validate7z(const std::string& archive_path) {
    // 7Z 文件最小大小：至少要有 6 字节的头部
    if (!checkFileSize(archive_path, 6)) {
        return false;
    }

    unsigned char header[6];
    if (!readFileHeader(archive_path, header, 6)) {
        return false;
    }

    // 7Z 文件以 "7z\xbc\xaf\x27\x1c" 开头
    return (header[0] == '7' && header[1] == 'z' && header[2] == 0xBC && header[3] == 0xAF &&
            header[4] == 0x27 && header[5] == 0x1C);
}

bool ArchiveValidator::validateRar(const std::string& archive_path) {
    // RAR 文件最小大小：至少要有 7 字节的头部
    if (!checkFileSize(archive_path, 7)) {
        return false;
    }

    unsigned char header[7];
    if (!readFileHeader(archive_path, header, 7)) {
        return false;
    }

    // RAR 文件以 "Rar!\x1a\x07" 开头（RAR 4.x）
    // 或 "Rar!\x1a\x07\x01\x00" 开头（RAR 5.x）
    bool is_rar4 = (header[0] == 'R' && header[1] == 'a' && header[2] == 'r' && header[3] == '!' &&
                    header[4] == 0x1A && header[5] == 0x07 && header[6] == 0x00);

    if (is_rar4) {
        return true;
    }

    // 检查 RAR 5.x 格式（需要更多字节）
    if (checkFileSize(archive_path, 8)) {
        unsigned char header8[8];
        if (readFileHeader(archive_path, header8, 8)) {
            return (header8[0] == 'R' && header8[1] == 'a' && header8[2] == 'r' &&
                    header8[3] == '!' && header8[4] == 0x1A && header8[5] == 0x07 &&
                    header8[6] == 0x01 && header8[7] == 0x00);
        }
    }

    return false;
}

bool ArchiveValidator::validateTarGz(const std::string& archive_path) {
    // TAR.GZ 文件是 GZIP 压缩的 TAR 文件
    // 检查 GZIP 头部
    return validateGzip(archive_path);
}

bool ArchiveValidator::validateTarBz2(const std::string& archive_path) {
    // TAR.BZ2 文件是 BZIP2 压缩的 TAR 文件
    // 检查 BZIP2 头部
    return validateBzip2(archive_path);
}

bool ArchiveValidator::validateTarXz(const std::string& archive_path) {
    // TAR.XZ 文件是 XZ 压缩的 TAR 文件
    // 检查 XZ 头部
    return validateXz(archive_path);
}

bool ArchiveValidator::validateArchive(const std::string& archive_path,
                                       const std::string& archive_type) {
    // 首先检查文件是否存在且可读
    try {
        if (!fs::exists(archive_path) || !fs::is_regular_file(archive_path)) {
            return false;
        }
    } catch (const std::exception&) {
        return false;
    }

    // 根据类型调用相应的验证函数
    if (archive_type == "zip") {
        return validateZip(archive_path);
    } else if (archive_type == "tar") {
        return validateTar(archive_path);
    } else if (archive_type == "gz") {
        return validateGzip(archive_path);
    } else if (archive_type == "bz2") {
        return validateBzip2(archive_path);
    } else if (archive_type == "xz") {
        return validateXz(archive_path);
    } else if (archive_type == "7z") {
        return validate7z(archive_path);
    } else if (archive_type == "rar") {
        return validateRar(archive_path);
    } else if (archive_type == "tar.gz" || archive_type == "tgz") {
        return validateTarGz(archive_path);
    } else if (archive_type == "tar.bz2" || archive_type == "tbz2") {
        return validateTarBz2(archive_path);
    } else if (archive_type == "tar.xz" || archive_type == "txz") {
        return validateTarXz(archive_path);
    }

    // 未知类型，返回 false
    return false;
}

} // namespace utils
} // namespace pnana
