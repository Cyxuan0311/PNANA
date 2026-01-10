#include "features/lsp/snippet_manager.h"
#include "core/editor.h" // 需要访问Editor类
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
    // C/C++ 代码片段
    std::vector<Snippet> cpp_snippets = {
        {"for", "for (auto& ${1:item} : ${2:container}) {\n    ${3:// code}\n}",
         "Range-based for loop"},
        {"fori", "for (size_t ${1:i} = 0; ${1:i} < ${2:n}; ++${1:i}) {\n    ${3:// code}\n}",
         "Index-based for loop"},
        {"if", "if (${1:condition}) {\n    ${2:// code}\n}", "If statement"},
        {"else", "else {\n    ${1:// code}\n}", "Else statement"},
        {"while", "while (${1:condition}) {\n    ${2:// code}\n}", "While loop"},
        {"class",
         "class ${1:ClassName} {\npublic:\n    ${1}(${2});\n    ~${1}();\n\nprivate:\n    ${3}\n};",
         "Class definition"},
        {"struct", "struct ${1:StructName} {\n    ${2}\n};", "Struct definition"},
        {"main", "int main(int argc, char* argv[]) {\n    ${1:// code}\n    return 0;\n}",
         "Main function"},
        {"include", "#include \"${1:header.h}\"", "Include header"},
        {"guard", "#ifndef ${1:HEADER_H}\n#define ${1:HEADER_H}\n\n${2}\n\n#endif // ${1:HEADER_H}",
         "Header guard"}};

    // Python 代码片段
    std::vector<Snippet> python_snippets = {
        {"def", "def ${1:function_name}(${2:parameters}):\n    ${3:pass}", "Function definition"},
        {"class",
         "class ${1:ClassName}:\n    def __init__(self${2:, parameters}):\n        ${3:pass}",
         "Class definition"},
        {"if", "if ${1:condition}:\n    ${2:pass}", "If statement"},
        {"elif", "elif ${1:condition}:\n    ${2:pass}", "Elif statement"},
        {"else", "else:\n    ${1:pass}", "Else statement"},
        {"for", "for ${1:item} in ${2:iterable}:\n    ${3:pass}", "For loop"},
        {"while", "while ${1:condition}:\n    ${2:pass}", "While loop"},
        {"try", "try:\n    ${1:pass}\nexcept ${2:Exception} as ${3:e}:\n    ${4:pass}",
         "Try-except block"},
        {"import", "import ${1:module}", "Import statement"},
        {"main", "if __name__ == \"__main__\":\n    ${1:main()}", "Main guard"}};

    // JavaScript/TypeScript 代码片段
    std::vector<Snippet> js_snippets = {
        {"function", "function ${1:functionName}(${2:parameters}) {\n    ${3:// code}\n}",
         "Function declaration"},
        {"arrow", "const ${1:functionName} = (${2:parameters}) => {\n    ${3:// code}\n};",
         "Arrow function"},
        {"if", "if (${1:condition}) {\n    ${2:// code}\n}", "If statement"},
        {"else", "else {\n    ${1:// code}\n}", "Else statement"},
        {"for", "for (let ${1:i} = 0; ${1:i} < ${2:array}.length; ${1:i}++) {\n    ${3:// code}\n}",
         "For loop"},
        {"foreach", "${1:array}.forEach((${2:item}) => {\n    ${3:// code}\n});", "ForEach loop"},
        {"console", "console.log(${1:message});", "Console log"},
        {"class",
         "class ${1:ClassName} {\n    constructor(${2:parameters}) {\n        ${3:// code}\n    "
         "}\n}",
         "Class definition"}};

    builtin_snippets_["cpp"] = cpp_snippets;
    builtin_snippets_["c++"] = cpp_snippets;
    builtin_snippets_["c"] = cpp_snippets;
    builtin_snippets_["python"] = python_snippets;
    builtin_snippets_["javascript"] = js_snippets;
    builtin_snippets_["typescript"] = js_snippets;

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

    pnana::core::Document* doc = editor.getCurrentDocument();
    size_t cursor_row = editor.cursor_row_;
    size_t cursor_col = editor.cursor_col_;

    // 插入代码片段主体
    doc->insertText(cursor_row, cursor_col, snippet.body);

    // 如果有占位符，设置第一个占位符为活动状态
    if (!snippet.placeholders.empty()) {
        // 这里可以实现占位符导航逻辑
        // 暂时只插入代码片段主体
        // TODO: 实现占位符跳转和编辑
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
