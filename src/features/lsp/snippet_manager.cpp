#include "features/lsp/snippet_manager.h"
#include "core/editor.h" // 需要访问Editor类
#include "features/lsp/snippets/snippets_registry.h"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace pnana {
namespace features {

SnippetManager::SnippetManager() {
    initializeBuiltinSnippets();
}

void SnippetManager::initializeBuiltinSnippets() {
    // 从各个语言的代码片段文件中注册代码片段
    using namespace snippets;

    // 注册 C/C++ 代码片段
    std::vector<Snippet> cpp_snippets = getCppSnippets();
    builtin_snippets_["cpp"] = cpp_snippets;
    builtin_snippets_["c++"] = cpp_snippets;
    builtin_snippets_["c"] = cpp_snippets;

    // 注册 Python 代码片段
    std::vector<Snippet> python_snippets = getPythonSnippets();
    builtin_snippets_["python"] = python_snippets;

    // 注册 JavaScript 代码片段
    std::vector<Snippet> js_snippets = getJavaScriptSnippets();
    builtin_snippets_["javascript"] = js_snippets;
    builtin_snippets_["js"] = js_snippets;

    // 注册 TypeScript 代码片段
    std::vector<Snippet> ts_snippets = getTypeScriptSnippets();
    builtin_snippets_["typescript"] = ts_snippets;
    builtin_snippets_["ts"] = ts_snippets;

    // 注册 Rust 代码片段
    std::vector<Snippet> rust_snippets = getRustSnippets();
    builtin_snippets_["rust"] = rust_snippets;
    builtin_snippets_["rs"] = rust_snippets;

    // 注册 Go 代码片段
    std::vector<Snippet> go_snippets = getGoSnippets();
    builtin_snippets_["go"] = go_snippets;
    builtin_snippets_["golang"] = go_snippets;

    // 注册 Java 代码片段
    std::vector<Snippet> java_snippets = getJavaSnippets();
    builtin_snippets_["java"] = java_snippets;

    // 注册 Ruby 代码片段
    std::vector<Snippet> ruby_snippets = getRubySnippets();
    builtin_snippets_["ruby"] = ruby_snippets;
    builtin_snippets_["rb"] = ruby_snippets;

    // 注册 PHP 代码片段
    std::vector<Snippet> php_snippets = getPhpSnippets();
    builtin_snippets_["php"] = php_snippets;

    // 注册 C# 代码片段
    std::vector<Snippet> csharp_snippets = getCSharpSnippets();
    builtin_snippets_["csharp"] = csharp_snippets;
    builtin_snippets_["cs"] = csharp_snippets;
    builtin_snippets_["c#"] = csharp_snippets;

    // 注册 Kotlin 代码片段
    std::vector<Snippet> kotlin_snippets = getKotlinSnippets();
    builtin_snippets_["kotlin"] = kotlin_snippets;
    builtin_snippets_["kt"] = kotlin_snippets;

    // 注册 Swift 代码片段
    std::vector<Snippet> swift_snippets = getSwiftSnippets();
    builtin_snippets_["swift"] = swift_snippets;

    // 注册 Dart 代码片段
    std::vector<Snippet> dart_snippets = getDartSnippets();
    builtin_snippets_["dart"] = dart_snippets;

    // 注册 Shell/Bash 代码片段
    std::vector<Snippet> shell_snippets = getShellSnippets();
    builtin_snippets_["shellscript"] = shell_snippets;
    builtin_snippets_["bash"] = shell_snippets;
    builtin_snippets_["sh"] = shell_snippets;
    builtin_snippets_["zsh"] = shell_snippets;

    // 注册 SQL 代码片段
    std::vector<Snippet> sql_snippets = getSqlSnippets();
    builtin_snippets_["sql"] = sql_snippets;
    builtin_snippets_["mysql"] = sql_snippets;
    builtin_snippets_["postgresql"] = sql_snippets;
    builtin_snippets_["sqlite"] = sql_snippets;

    // 注册 HTML 代码片段
    std::vector<Snippet> html_snippets = getHtmlSnippets();
    builtin_snippets_["html"] = html_snippets;
    builtin_snippets_["xhtml"] = html_snippets;

    // 注册 CSS 代码片段
    std::vector<Snippet> css_snippets = getCssSnippets();
    builtin_snippets_["css"] = css_snippets;
    builtin_snippets_["scss"] = css_snippets;
    builtin_snippets_["sass"] = css_snippets;
    builtin_snippets_["less"] = css_snippets;

    // 注册 YAML 代码片段
    std::vector<Snippet> yaml_snippets = getYamlSnippets();
    builtin_snippets_["yaml"] = yaml_snippets;
    builtin_snippets_["yml"] = yaml_snippets;

    // 注册 Lua 代码片段
    std::vector<Snippet> lua_snippets = getLuaSnippets();
    builtin_snippets_["lua"] = lua_snippets;

    // 为每个代码片段解析占位符
    for (auto& [lang, snippets] : builtin_snippets_) {
        for (auto& snippet : snippets) {
            parseSnippetPlaceholders(snippet);
        }
    }
}

void SnippetManager::parseSnippetPlaceholders(Snippet& snippet) {
    snippet.placeholders.clear();

    std::string body = snippet.body;
    size_t pos = 0;

    while ((pos = body.find("${", pos)) != std::string::npos) {
        size_t end_pos = body.find("}", pos);
        if (end_pos != std::string::npos) {
            std::string placeholder_content = body.substr(pos + 2, end_pos - pos - 2);
            size_t colon_pos = placeholder_content.find(":");

            SnippetPlaceholder placeholder;
            if (colon_pos != std::string::npos) {
                placeholder.index = std::atoi(placeholder_content.substr(0, colon_pos).c_str());
                placeholder.default_value = placeholder_content.substr(colon_pos + 1);
            } else {
                placeholder.index = std::atoi(placeholder_content.c_str());
                placeholder.default_value = "";
            }

            placeholder.position = {pos, end_pos - pos + 1};
            snippet.placeholders.push_back(placeholder);

            pos = end_pos + 1;
        } else {
            break;
        }
    }
}

void SnippetManager::expandSnippet(const Snippet& snippet, core::Editor& editor) {
    if (!editor.getCurrentDocument()) {
        return;
    }

    LOG("[SNIPPET] expandSnippet: prefix='" + snippet.prefix +
        "' body.size=" + std::to_string(snippet.body.size()));

    pnana::core::Document* doc = editor.getCurrentDocument();
    size_t cursor_row = editor.cursor_row_;
    size_t cursor_col = editor.cursor_col_;

    struct Occ {
        int index = 0;
        size_t out_offset = 0;
        size_t len = 0;
    };

    // 0) 预处理：兼容 LSP snippet/newText 里常见的转义序列（\\n/\\t 等）
    // 说明：内置 snippet 在 C++ 字符串里通常已经是实际 '\n'，这里也能安全处理。
    auto unescapeSnippetText = [](const std::string& in) -> std::string {
        std::string out;
        out.reserve(in.size());
        for (size_t i = 0; i < in.size(); ++i) {
            char ch = in[i];
            if (ch == '\\' && i + 1 < in.size()) {
                char n = in[i + 1];
                switch (n) {
                    case 'n':
                        out.push_back('\n');
                        i++;
                        continue;
                    case 't':
                        out.push_back('\t');
                        i++;
                        continue;
                    case 'r':
                        out.push_back('\r');
                        i++;
                        continue;
                    case '\\':
                        out.push_back('\\');
                        i++;
                        continue;
                    default:
                        // unknown escape, keep backslash as-is
                        break;
                }
            }
            out.push_back(ch);
        }
        return out;
    };

    const std::string pre_body = unescapeSnippetText(snippet.body);
    if (pre_body != snippet.body) {
        LOG("[SNIPPET] unescape applied (body had escape sequences)");
    }

    // 1) 将 ${n:default} / ${n} 转换为 default 文本，并记录占位符在展开后文本中的位置
    std::string expanded;
    expanded.reserve(pre_body.size());
    std::vector<Occ> occs;

    const std::string& body = pre_body;
    size_t i = 0;
    while (i < body.size()) {
        if (i + 1 < body.size() && body[i] == '$' && body[i + 1] == '{') {
            size_t end = body.find('}', i + 2);
            if (end == std::string::npos) {
                // malformed placeholder, copy as-is
                expanded.push_back(body[i]);
                i++;
                continue;
            }
            std::string content = body.substr(i + 2, end - (i + 2));
            size_t colon = content.find(':');
            int idx = 0;
            std::string def;
            if (colon != std::string::npos) {
                idx = std::atoi(content.substr(0, colon).c_str());
                def = content.substr(colon + 1);
            } else {
                idx = std::atoi(content.c_str());
                def = "";
            }

            Occ occ;
            occ.index = idx;
            occ.out_offset = expanded.size();
            occ.len = def.size();
            occs.push_back(occ);

            expanded += def;
            i = end + 1;
        } else {
            expanded.push_back(body[i]);
            i++;
        }
    }

    // 1.5) 基础“格式化”：按当前行缩进对齐 snippet 的多行内容
    // 目标：插入 snippet 时不要把多行内容贴成“无缩进的裸文本”
    if (doc && cursor_row < doc->lineCount()) {
        const std::string& line = doc->getLine(cursor_row);
        // 取当前行的前导空白作为缩进（支持 tab/space 混用）
        std::string indent;
        for (char ch : line) {
            if (ch == ' ' || ch == '\t') {
                indent.push_back(ch);
            } else {
                break;
            }
        }
        if (!indent.empty()) {
            std::string indented;
            indented.reserve(expanded.size() + indent.size() * 4);
            for (size_t k = 0; k < expanded.size(); ++k) {
                char ch = expanded[k];
                indented.push_back(ch);
                if (ch == '\n' && k + 1 < expanded.size()) {
                    indented += indent;
                }
            }
            expanded.swap(indented);
            LOG("[SNIPPET] applied indent prefix len=" + std::to_string(indent.size()));
        }
    }

    // 2) 插入展开后的文本
    doc->insertText(cursor_row, cursor_col, expanded);
    LOG("[SNIPPET] inserted text len=" + std::to_string(expanded.size()));

    // 3) 将 out_offset 转换为 (row,col)，按 index 排序并启动 snippet session
    if (!occs.empty()) {
        auto offsetToPos = [&](size_t off) -> std::pair<size_t, size_t> {
            size_t r = cursor_row;
            size_t c = cursor_col;
            for (size_t k = 0; k < off && k < expanded.size(); ++k) {
                if (expanded[k] == '\n') {
                    r++;
                    c = 0;
                } else {
                    c++;
                }
            }
            return {r, c};
        };

        std::vector<core::Editor::SnippetPlaceholderRange> ranges;
        ranges.reserve(occs.size());
        for (const auto& occ : occs) {
            auto [r, c] = offsetToPos(occ.out_offset);
            core::Editor::SnippetPlaceholderRange range;
            range.row = r;
            range.col = c;
            range.len = occ.len;
            range.index = occ.index;
            // ignore index 0 placeholders (commonly cursor-only)
            if (range.index != 0) {
                ranges.push_back(range);
            }
        }

        std::sort(ranges.begin(), ranges.end(),
                  [](const core::Editor::SnippetPlaceholderRange& a,
                     const core::Editor::SnippetPlaceholderRange& b) {
                      if (a.index != b.index)
                          return a.index < b.index;
                      if (a.row != b.row)
                          return a.row < b.row;
                      return a.col < b.col;
                  });

        if (!ranges.empty()) {
            editor.startSnippetSession(std::move(ranges));
        }
    }

    // 更新UI
    editor.adjustCursor();
}

std::vector<Snippet> SnippetManager::getSnippetsForLanguage(const std::string& language_id) {
    std::vector<Snippet> all_snippets;

    // 获取内置代码片段
    auto builtin_it = builtin_snippets_.find(language_id);
    if (builtin_it != builtin_snippets_.end()) {
        all_snippets.insert(all_snippets.end(), builtin_it->second.begin(),
                            builtin_it->second.end());
    }

    // 获取用户自定义代码片段
    auto user_it = user_snippets_.find(language_id);
    if (user_it != user_snippets_.end()) {
        all_snippets.insert(all_snippets.end(), user_it->second.begin(), user_it->second.end());
    }

    return all_snippets;
}

void SnippetManager::loadUserSnippets(const std::string& language_id) {
    (void)language_id; // 暂时未使用，消除编译警告
    // TODO: 从配置文件加载用户自定义代码片段
    // 这里可以实现从JSON或YAML文件加载用户代码片段
}

void SnippetManager::saveUserSnippets(const std::string& language_id) {
    (void)language_id; // 暂时未使用，消除编译警告
    // TODO: 保存用户自定义代码片段到配置文件
}

void SnippetManager::registerSnippet(const std::string& language_id, const Snippet& snippet) {
    user_snippets_[language_id].push_back(snippet);
    parseSnippetPlaceholders(user_snippets_[language_id].back());
}

std::vector<Snippet> SnippetManager::findMatchingSnippets(const std::string& prefix,
                                                          const std::string& language_id) {
    std::vector<Snippet> matches;
    auto snippets = getSnippetsForLanguage(language_id);

    for (const auto& snippet : snippets) {
        if (snippet.prefix.find(prefix) == 0 || prefix.find(snippet.prefix) == 0) {
            matches.push_back(snippet);
        }
    }

    return matches;
}

} // namespace features
} // namespace pnana
