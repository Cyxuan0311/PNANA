# pnana 快速入门指南

## 🚀 5分钟上手

### 安装

```bash
# 编译项目
cd /path/to/pnana
./build.sh

# 运行pnana
./build/pnana/pnana
```

### 基本使用

#### 打开文件
```bash
pnana myfile.txt
```

#### 创建新文件
```bash
pnana
# 然后按 Ctrl+S 保存时输入文件名
```

#### 打开多个文件
```bash
pnana file1.txt file2.cpp file3.py
```

## ⌨️ 最常用的10个快捷键

1. **Ctrl+S** - 保存文件 💾
2. **Ctrl+Q** - 退出编辑器 🚪
3. **Ctrl+F** - 搜索文本 🔍
4. **Ctrl+G** - 跳转到指定行 🎯
5. **Ctrl+Z** - 撤销 ↩️
6. **Ctrl+Y** - 重做 ↪️
7. **Ctrl+C** - 复制 📋
8. **Ctrl+X** - 剪切 ✂️
9. **Ctrl+V** - 粘贴 📌
10. **Ctrl+A** - 全选 ✅

## 📝 编辑技巧

### 光标移动
- **箭头键** - 上下左右移动
- **Home/End** - 行首/行尾
- **PgUp/PgDn** - 上一页/下一页
- **Ctrl+Home** - 文件开头
- **Ctrl+End** - 文件结尾

### 选择文本
1. 按住Shift + 方向键 - 选择文本
2. Ctrl+A - 全选
3. 双击 - 选择单词（需要鼠标支持）

### 搜索
1. 按 **Ctrl+F** 进入搜索模式
2. 输入要搜索的文本
3. 按 **Enter** 开始搜索
4. 按 **F3** 查找下一个
5. 按 **Shift+F3** 查找上一个
6. 按 **Esc** 退出搜索

### 跳转到行
1. 按 **Ctrl+G**
2. 输入行号
3. 按 **Enter**

## 🎨 更换主题

在命令行启动时指定主题：
```bash
pnana --theme dracula myfile.txt
```

可用主题：
- `monokai` - 默认，经典的Monokai主题
- `dracula` - 流行的Dracula主题
- `solarized-dark` - Solarized深色
- `solarized-light` - Solarized浅色
- `onedark` - One Dark主题
- `nord` - 北欧风格主题

## 💡 实用技巧

### 1. 快速保存并退出
```
Ctrl+S (保存)
Ctrl+Q (退出)
```

### 2. 撤销多次操作
连续按 Ctrl+Z 可以撤销多次操作

### 3. 复制整行
将光标移到要复制的行，直接按 Ctrl+C

### 4. 删除整行
使用 Ctrl+X (剪切) 来删除一行

### 5. 重复行
使用 Ctrl+D 复制当前行

## 🆘 常见问题

### Q: 如何保存文件？
A: 按 Ctrl+S

### Q: 文件有修改但我想退出怎么办？
A: 先按 Ctrl+S 保存，然后按 Ctrl+Q 退出

### Q: 如何搜索文本？
A: 按 Ctrl+F，输入要搜索的内容，按 Enter

### Q: 如何撤销操作？
A: 按 Ctrl+Z

### Q: 如何跳转到某一行？
A: 按 Ctrl+G，输入行号，按 Enter

### Q: 底部的快捷键提示栏很有用！
A: 是的！帮助栏会根据当前模式显示最相关的快捷键

## 🔥 高级功能

### 搜索选项（即将推出）
- 区分大小写搜索
- 正则表达式搜索
- 全词匹配

### 多文件编辑（即将推出）
- 标签页切换
- 分屏编辑

### 语法高亮（即将推出）
- 自动识别文件类型
- 支持多种编程语言

## 📚 更多帮助

- 查看完整文档: [README.md](README.md)
- 键盘快捷键大全: 按 F1 或参考README
- 项目主页: https://github.com/yourrepo/pnana

---

**开始享受pnana带来的流畅编辑体验吧！** 🎉✨

