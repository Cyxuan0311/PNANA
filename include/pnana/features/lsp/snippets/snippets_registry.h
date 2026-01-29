#ifndef PNANA_FEATURES_LSP_SNIPPETS_SNIPPETS_REGISTRY_H
#define PNANA_FEATURES_LSP_SNIPPETS_SNIPPETS_REGISTRY_H

#include "features/lsp/lsp_types.h"
#include <vector>

namespace pnana {
namespace features {
namespace snippets {

/**
 * 获取 C/C++ 代码片段
 */
std::vector<Snippet> getCppSnippets();

/**
 * 获取 Python 代码片段
 */
std::vector<Snippet> getPythonSnippets();

/**
 * 获取 JavaScript 代码片段
 */
std::vector<Snippet> getJavaScriptSnippets();

/**
 * 获取 TypeScript 代码片段
 */
std::vector<Snippet> getTypeScriptSnippets();

/**
 * 获取 Rust 代码片段
 */
std::vector<Snippet> getRustSnippets();

/**
 * 获取 Go 代码片段
 */
std::vector<Snippet> getGoSnippets();

/**
 * 获取 Java 代码片段
 */
std::vector<Snippet> getJavaSnippets();

/**
 * 获取 Ruby 代码片段
 */
std::vector<Snippet> getRubySnippets();

/**
 * 获取 PHP 代码片段
 */
std::vector<Snippet> getPhpSnippets();

} // namespace snippets
} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_LSP_SNIPPETS_SNIPPETS_REGISTRY_H
