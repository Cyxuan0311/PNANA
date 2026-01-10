#ifndef PNANA_FEATURES_LSP_SNIPPET_MANAGER_H
#define PNANA_FEATURES_LSP_SNIPPET_MANAGER_H

#include "features/lsp/lsp_types.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace pnana {

namespace core {
// 前向声明Editor类以避免循环依赖
class Editor;
} // namespace core

class Editor;

namespace features {

/**
 * 代码片段管理器
 * 负责加载、管理和展开代码片段
 */
class SnippetManager {
  public:
    SnippetManager();
    ~SnippetManager() = default;

    // 禁用拷贝构造和赋值
    SnippetManager(const SnippetManager&) = delete;
    SnippetManager& operator=(const SnippetManager&) = delete;

    // 代码片段展开
    void expandSnippet(const Snippet& snippet, core::Editor& editor);

    // 获取指定语言的代码片段
    std::vector<Snippet> getSnippetsForLanguage(const std::string& language_id);

    // 加载用户自定义代码片段
    void loadUserSnippets(const std::string& language_id);

    // 保存用户自定义代码片段
    void saveUserSnippets(const std::string& language_id);

    // 注册新的代码片段
    void registerSnippet(const std::string& language_id, const Snippet& snippet);

    // 查找匹配的代码片段
    std::vector<Snippet> findMatchingSnippets(const std::string& prefix,
                                              const std::string& language_id);

  private:
    // 内置代码片段存储
    std::map<std::string, std::vector<Snippet>> builtin_snippets_;

    // 用户自定义代码片段存储
    std::map<std::string, std::vector<Snippet>> user_snippets_;

    // 初始化内置代码片段
    void initializeBuiltinSnippets();

    // 解析代码片段占位符
    void parseSnippetPlaceholders(Snippet& snippet);
};

} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_LSP_SNIPPET_MANAGER_H
