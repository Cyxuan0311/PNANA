// Ctrl+/ 注释切换：事件处理在 core，注释类型与字符由 utils 负责
#include "core/document.h"
#include "core/editor.h"
#include "utils/comment_syntax.h"

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

    std::string old_lines_content;
    for (size_t r = start_row; r <= end_row; ++r) {
        old_lines_content += lines[r];
        if (r < end_row)
            old_lines_content += "\n";
    }

    int total_col_offset = 0;

    for (size_t r = start_row; r <= end_row; ++r) {
        std::string& line = lines[r];
        auto [new_line, col_offset] = utils::toggleCommentForLine(line, file_type);
        line = new_line;
        if (start_row == end_row) {
            total_col_offset = col_offset;
        }
    }

    std::string new_lines_content;
    for (size_t r = start_row; r <= end_row; ++r) {
        new_lines_content += lines[r];
        if (r < end_row)
            new_lines_content += "\n";
    }

    doc->pushChange(DocumentChange(DocumentChange::Type::COMMENT_TOGGLE, start_row, end_row,
                                   old_lines_content, new_lines_content, DocumentChange::MOVE_TAG));

    if (start_row == end_row) {
        int new_col = static_cast<int>(cursor_col_) + total_col_offset;
        cursor_col_ = (new_col >= 0) ? static_cast<size_t>(new_col) : 0;
    } else {
        selection_active_ = false;
    }

    doc->setModified(true);
    setStatusMessage("Comment toggled");
}

} // namespace core
} // namespace pnana
