#include "ui/binary_file_view.h"
#include "ui/icons.h"
#include <ftxui/dom/elements.hpp>
#include <filesystem>

using namespace ftxui;

namespace pnana {
namespace ui {

BinaryFileView::BinaryFileView(Theme& theme) : theme_(theme) {
}

void BinaryFileView::setFilePath(const std::string& filepath) {
    filepath_ = filepath;
}

Element BinaryFileView::render() {
    auto& colors = theme_.getColors();
    
    Elements content;
    
    // 空行
    content.push_back(text(""));
    content.push_back(text(""));
    
    // 警告图标和标题
    content.push_back(
        hbox({
            text("  "),
            text(icons::WARNING) | color(colors.warning) | bold,
            text("  Binary File Detected") | color(colors.warning) | bold
        }) | center
    );
    
    content.push_back(text(""));
    content.push_back(text(""));
    
    // 文件路径信息
    std::string filename = filepath_;
    try {
        std::filesystem::path path(filepath_);
        filename = path.filename().string();
    } catch (...) {
        // 如果路径解析失败，使用原始路径
    }
    
    content.push_back(
        hbox({
            text("  "),
            text(icons::FILE) | color(colors.comment),
            text("  File: ") | color(colors.foreground),
            text(filename) | color(colors.function) | bold
        }) | center
    );
    
    content.push_back(text(""));
    
    // 完整路径（如果与文件名不同）
    if (filepath_ != filename) {
        content.push_back(
            hbox({
                text("  "),
                text("Path: ") | color(colors.comment) | dim,
                text(filepath_) | color(colors.comment) | dim
            }) | center
        );
        content.push_back(text(""));
    }
    
    // 分隔线
    content.push_back(
        text("─────────────────────────────────────────────────") 
        | color(colors.comment) | dim | center
    );
    
    content.push_back(text(""));
    content.push_back(text(""));
    
    // 说明信息
    content.push_back(
        text("This file appears to be a binary file and cannot") 
        | color(colors.foreground) | center
    );
    content.push_back(
        text("be opened in text editor mode.") 
        | color(colors.foreground) | center
    );
    
    content.push_back(text(""));
    content.push_back(text(""));
    
    // 建议信息
    content.push_back(
        hbox({
            text(icons::BULB) | color(colors.success),
            text(" Suggestions:") | color(colors.success) | bold
        }) | center
    );
    
    content.push_back(text(""));
    
    Elements suggestions;
    suggestions.push_back(
        hbox({
            text("  • "),
            text("Use a specialized tool for this file type") | color(colors.foreground)
        })
    );
    suggestions.push_back(
        hbox({
            text("  • "),
            text("Check if the file has a text-based format") | color(colors.foreground)
        })
    );
    suggestions.push_back(
        hbox({
            text("  • "),
            text("Open with a hex editor if you need to view raw bytes") | color(colors.foreground)
        })
    );
    
    for (const auto& suggestion : suggestions) {
        content.push_back(suggestion | center);
    }
    
    content.push_back(text(""));
    content.push_back(text(""));
    
    // 底部提示
    content.push_back(
        text("─────────────────────────────────────────────────") 
        | color(colors.comment) | dim | center
    );
    content.push_back(
        text("Press Ctrl+O to open another file") 
        | color(colors.comment) | dim | center
    );
    
    content.push_back(text(""));
    content.push_back(text(""));
    
    return vbox(content) | center | flex | bgcolor(colors.background);
}

} // namespace ui
} // namespace pnana

