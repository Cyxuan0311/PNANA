// Ctrl+/ 注释切换：事件处理在 core，注释类型与字符由 features 负责
#include "core/document.h"
#include "core/editor.h"
#include "features/comment/comment_syntax.h"

namespace pnana {
namespace core {

void Editor::toggleComment() {
    Document* doc = getCurrentDocument();
    if (!doc)
        return;

    auto& lines = doc->getLines();
    if (lines.empty())
        return;

    std::string file_type = getFileType();

    // 确定要注释的行范围：有选择时注释选中行，否则注释当前行
    size_t start_row;
    size_t end_row;
    if (selection_active_) {
        start_row = std::min(selection_start_row_, cursor_row_);
        end_row = std::max(selection_start_row_, cursor_row_);
        if (end_row >= lines.size())
            end_row = lines.size() - 1;
    } else {
        start_row = cursor_row_;
        end_row = cursor_row_;
        if (cursor_row_ >= lines.size())
            return;
    }

    int total_col_offset = 0; // 仅单行时用于光标列偏移

    for (size_t r = start_row; r <= end_row; ++r) {
        std::string& line = lines[r];
        auto [new_line, col_offset] = features::comment::toggleCommentForLine(line, file_type);
        line = new_line;
        if (start_row == end_row) {
            total_col_offset = col_offset;
        }
    }

    // 单行：调整光标列
    if (start_row == end_row) {
        int new_col = static_cast<int>(cursor_col_) + total_col_offset;
        cursor_col_ = (new_col >= 0) ? static_cast<size_t>(new_col) : 0;
    } else {
        // 多行：清除选择，光标置于选区末尾
        selection_active_ = false;
    }

    doc->setModified(true);
    setStatusMessage("Comment toggled");
}

} // namespace core
} // namespace pnana
