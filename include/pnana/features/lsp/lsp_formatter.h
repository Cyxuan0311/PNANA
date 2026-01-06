#ifndef PNANA_FEATURES_LSP_FORMATTER_H
#define PNANA_FEATURES_LSP_FORMATTER_H

#include <memory>
#include <string>
#include <vector>

namespace pnana {
namespace features {

class LspClient;
class LspServerManager;

/**
 * LSP 代码格式化器
 * 负责管理和执行代码格式化操作
 */
class LspFormatter {
  public:
    explicit LspFormatter(LspServerManager* lsp_manager);
    ~LspFormatter();

    /**
     * 获取目录中所有支持格式化的文件
     * @param directory_path 目录路径
     * @return 支持格式化的文件路径列表
     */
    std::vector<std::string> getSupportedFilesInDirectory(const std::string& directory_path);

    /**
     * 检查文件类型是否支持格式化
     * @param file_type 文件类型
     * @return 是否支持
     */
    bool isFileTypeSupported(const std::string& file_type) const;

    /**
     * 格式化单个文件
     * @param file_path 文件路径
     * @return 是否成功
     */
    bool formatFile(const std::string& file_path);

    /**
     * 格式化多个文件
     * @param file_paths 文件路径列表
     * @return 是否全部成功
     */
    bool formatFiles(const std::vector<std::string>& file_paths);

    /**
     * 尝试使用命令行工具格式化
     * @param file_path 文件路径
     * @param original_content 原始内容
     * @return 格式化后的内容，失败返回空字符串
     */
    std::string tryCommandLineFormat(const std::string& file_path,
                                     const std::string& original_content);

    /**
     * 尝试使用clang-format格式化C/C++文件
     * @param file_path 文件路径
     * @return 格式化后的内容，失败返回空字符串
     */
    std::string tryClangFormat(const std::string& file_path);

    /**
     * 获取文件显示名称（仅文件名部分）
     * @param file_path 文件路径
     * @return 文件名
     */
    std::string getFileDisplayName(const std::string& file_path) const;

    /**
     * 获取相对于基准路径的文件路径
     * @param file_path 文件路径
     * @param base_path 基准路径
     * @return 相对路径
     */
    std::string getFileRelativePath(const std::string& file_path,
                                    const std::string& base_path) const;

    /**
     * 将文件路径转换为URI格式
     * @param filepath 文件路径
     * @return URI字符串
     */
    std::string filepathToUri(const std::string& filepath) const;

  private:
    LspServerManager* lsp_manager_;
};

} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_LSP_FORMATTER_H
