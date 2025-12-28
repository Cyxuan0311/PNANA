# 撤销/重做功能说明

## Ctrl+Z 工作原理

### 1. 快捷键绑定

- **撤销**: `Ctrl+Z` → `KeyAction::UNDO`
- **重做**: `Ctrl+Y` 或 `Ctrl+Shift+Z` → `KeyAction::REDO`

### 2. 撤销栈机制

编辑器使用**撤销栈（Undo Stack）**来记录所有编辑操作：

#### DocumentChange 结构

```cpp
struct DocumentChange {
    enum class Type { INSERT, DELETE, REPLACE };
    Type type;              // 操作类型
    size_t row;             // 行号
    size_t col;             // 列号
    std::string old_content; // 修改前的内容
    std::string new_content; // 修改后的内容
};
```

#### 操作记录

每次编辑操作（插入、删除、替换）都会：
1. 创建 `DocumentChange` 记录
2. 推入 `undo_stack_`（撤销栈）
3. 清空 `redo_stack_`（重做栈）

### 3. 撤销流程

当按下 `Ctrl+Z` 时：

1. **事件处理** (`editor_input.cpp`)
   - `Ctrl+Z` 被解析为 `KeyAction::UNDO`
   - 调用 `ActionExecutor::execute(KeyAction::UNDO)`

2. **执行撤销** (`editor_edit.cpp`)
   ```cpp
   void Editor::undo() {
       size_t change_row, change_col;
       if (doc->undo(&change_row, &change_col)) {
           // 恢复光标位置到修改发生的位置
           cursor_row_ = change_row;
           cursor_col_ = change_col;
           adjustCursor();
           adjustViewOffset(); // 只在必要时
           setStatusMessage("Undone");
       }
   }
   ```

3. **文档撤销** (`document.cpp`)
   ```cpp
   bool Document::undo(size_t* out_row, size_t* out_col) {
       // 从撤销栈弹出最后一个操作
       DocumentChange change = undo_stack_.back();
       undo_stack_.pop_back();
       
       // 应用反向操作
       switch (change.type) {
           case INSERT:  // 删除插入的内容
           case DELETE:  // 恢复删除的内容
           case REPLACE: // 恢复原内容
       }
       
       // 将操作推入重做栈
       redo_stack_.push_back(change);
       
       // 返回修改位置
       *out_row = change.row;
       *out_col = change.col;
   }
   ```

### 4. 优化措施（减少刷新）

#### 问题
- 之前：每次撤销都会调用 `adjustViewOffset()`，即使光标在可见范围内
- 导致：不必要的视图刷新

#### 解决方案
```cpp
// 只在光标超出可见范围时才调整视图偏移
int screen_height = screen_.dimy() - 4;
if (cursor_row_ >= view_offset_row_ + screen_height || 
    cursor_row_ < view_offset_row_) {
    adjustViewOffset();
}
```

### 5. 撤销栈限制

- 最大撤销次数：`MAX_UNDO_STACK = 1000`
- 超过限制时，最旧的操作会被移除（FIFO）

### 6. 支持的操作类型

1. **INSERT（插入）**
   - 撤销：删除插入的内容
   - 光标位置：回到插入开始位置

2. **DELETE（删除）**
   - 撤销：恢复删除的内容
   - 光标位置：回到删除开始位置

3. **REPLACE（替换）**
   - 撤销：恢复原内容
   - 光标位置：回到替换开始位置

### 7. 重做机制

- 撤销的操作会被推入 `redo_stack_`（重做栈）
- 按 `Ctrl+Y` 可以重新应用被撤销的操作
- 重做后，操作会重新推入撤销栈

## 使用示例

```
1. 输入 "Hello" → 记录 INSERT 操作
2. 按 Ctrl+Z → 撤销，删除 "Hello"，光标回到插入位置
3. 按 Ctrl+Y → 重做，重新插入 "Hello"
```

## 注意事项

- 撤销栈在文件保存后不会清空（可以继续撤销保存前的操作）
- 新操作会清空重做栈
- 撤销/重做不会影响文件系统，只影响内存中的文档内容

