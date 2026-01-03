#ifndef PNANA_FEATURES_SYNTAX_HIGHLIGHTER_SYNTAX_HIGHLIGHTER_TREE_SITTER_H
#define PNANA_FEATURES_SYNTAX_HIGHLIGHTER_SYNTAX_HIGHLIGHTER_TREE_SITTER_H

#include "ui/theme.h"
#include <ftxui/dom/elements.hpp>
#include <string>
#include <vector>
#include <memory>
#include <map>

// Tree-sitter 头文件包含
#include <tree_sitter/api.h>

namespace pnana {
namespace features {

// Tree-sitter 语法高亮器
class SyntaxHighlighterTreeSitter {
public:
    explicit SyntaxHighlighterTreeSitter(ui::Theme& theme);
    ~SyntaxHighlighterTreeSitter();
    
    // 禁用拷贝构造和赋值
    SyntaxHighlighterTreeSitter(const SyntaxHighlighterTreeSitter&) = delete;
    SyntaxHighlighterTreeSitter& operator=(const SyntaxHighlighterTreeSitter&) = delete;
    
    // 设置文件类型
    void setFileType(const std::string& file_type);
    
    // 高亮一行代码（使用增量解析）
    ftxui::Element highlightLine(const std::string& line);
    
    // 高亮多行代码（更高效）
    ftxui::Element highlightLines(const std::vector<std::string>& lines);
    
    // 重置解析器状态
    void reset();
    
    // 检查是否支持指定文件类型
    bool supportsFileType(const std::string& file_type) const;
    
private:
    ui::Theme& theme_;
    TSParser* parser_;
    TSLanguage* current_language_;
    std::string current_file_type_;
    
    // 语言映射：文件类型 -> Tree-sitter 语言
    std::map<std::string, TSLanguage*> language_map_;
    
    // 初始化语言映射
    void initializeLanguages();
    
    // 获取 Tree-sitter 语言
    TSLanguage* getLanguageForFileType(const std::string& file_type);
    
    // 将 Tree-sitter 节点类型映射到颜色
    ftxui::Color getColorForNodeType(const std::string& node_type) const;
    
    // 解析代码并生成高亮元素
    ftxui::Element parseAndHighlight(const std::string& code);
    
    // 遍历语法树并生成高亮元素
    void traverseTree(TSNode node, const std::string& source, 
                     std::vector<ftxui::Element>& elements, 
                     size_t& current_pos) const;
    
    // 获取节点文本
    std::string getNodeText(TSNode node, const std::string& source) const;
};

} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_SYNTAX_HIGHLIGHTER_SYNTAX_HIGHLIGHTER_TREE_SITTER_H

