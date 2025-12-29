<div align="center">

<img src="resources/logo.png" alt="pnana logo" width="200">

# pnana - Modern Terminal Text Editor

![pnana](https://img.shields.io/badge/pnana-v0.0.3-brightgreen)
![C++](https://img.shields.io/badge/C++-17-blue)
![FTXUI](https://img.shields.io/badge/FTXUI-Terminal%20UI-orange)
![License](https://img.shields.io/badge/license-MIT-green)

[中文](README.md) | **English**

**pnana** is a modern terminal text editor built with FTXUI, inspired by Nano, Micro, and Sublime Text. It provides a friendly user interface, intuitive keyboard shortcuts, and powerful editing features.

</div>

## 📸 Demo

<div align="center">

<img src="resources/demo.png" alt="pnana demo" width="800">

</div>

## ✨ Core Features

### 🎨 Beautiful and Friendly Interface
- **6 Beautiful Themes**: Monokai (default), Dracula, Solarized Dark/Light, OneDark, Nord
- **Three-Column Layout**: Top menu bar, middle editing area, bottom help bar
- **Smart Status Bar**: Displays file information, cursor position, encoding, and modification status
- **Line Numbers**: Switch between absolute and relative line numbers
- **Current Line Highlighting**: Clearly identifies the editing position

### ⌨️ Modern Keyboard Shortcuts
Abandoning the learning curve of traditional Vim, pnana adopts intuitive shortcuts from modern editors. Use familiar standard shortcuts like `Ctrl+S` to save, `Ctrl+Z` to undo, with zero learning curve.

**Complete Shortcut List**: See [Keyboard Shortcuts Documentation](docs/KEYBINDINGS.md)

### 📝 Powerful Editing Features

#### Multi-File Support
- **Tab System**: Open multiple files simultaneously
- **Split Editing**: Horizontal/vertical split screens
- **Quick Switching**: Fast file switching with keyboard and mouse

#### Smart Editing
- **Auto Indent**: Intelligent indentation based on file type
- **Bracket Matching**: Auto-complete brackets and quotes
- **Multi-Cursor Editing**: Edit multiple positions simultaneously (planned)
- **Column Selection**: Hold Alt for column selection
- **Smart Undo/Redo**: Unlimited undo/redo

#### Search and Replace
- **Regular Expressions**: Support for regex search
- **Case Sensitive**: Optional case matching
- **Batch Replace**: Replace all matches at once
- **Live Preview**: Real-time search result highlighting

#### Syntax Highlighting
Supports multiple programming languages: C/C++, Python, JavaScript/TypeScript, Java, Go, Rust, Ruby, PHP, HTML/CSS, JSON, XML, Markdown, Shell, SQL, YAML, TOML

### 🔧 Configuration System
Simple JSON configuration file supporting themes, fonts, indentation, and other settings.

**Detailed Configuration Guide**: See [Configuration Documentation](docs/CONFIGURATION.md)

## 🚀 Quick Start

### Build Requirements
- C++17 or higher compiler
- CMake 3.10+
- FTXUI library (included in third-party)

### Build and Install

```bash
# Clone repository
cd /path/to/pnana

# Build project
./build.sh

# Run pnana
./build/pnana/pnana

# Or install to system
cd build
sudo make install
pnana filename.txt
```

### Usage Examples

```bash
# Start blank editor
pnana

# Open single file
pnana file.txt

# Open multiple files
pnana file1.txt file2.cpp file3.py

# Specify config file
pnana --config ~/.config/pnana/custom.json

# Use specific theme
pnana --theme dracula file.txt

# Open in read-only mode
pnana --readonly file.txt
```

## 📖 Documentation

Detailed documentation and guides are available in the [docs](docs/) folder:

- **[Keyboard Shortcuts Reference](docs/KEYBINDINGS.md)** - Complete shortcut list and usage instructions
- **[Configuration Documentation](docs/CONFIGURATION.md)** - Detailed configuration options and examples
- **[Menu Functions](docs/MENU.md)** - Detailed menu bar function descriptions
- **[Theme Documentation](docs/THEMES.md)** - Introduction and preview of all themes
- **[Development Roadmap](docs/ROADMAP.md)** - Version plans and feature roadmap
- **[Product Comparison](docs/COMPARISON.md)** - Detailed comparison with similar products


## 💡 Why Choose pnana?

1. **Zero Learning Curve**: Use familiar Ctrl shortcuts, no need to memorize complex commands
2. **Ready to Use**: Get an excellent editing experience without configuration
3. **Modern Design**: Beautiful UI and comfortable color schemes
4. **Lightweight and Efficient**: Terminal-based, low resource usage, fast startup
5. **Feature Complete**: Feature set comparable to GUI editors

## 🤝 Comparison with Similar Products

| Feature | pnana | Nano | Micro | Vim/Neovim |
|---------|-------|------|-------|------------|
| Learning Curve | Low | Low | Low | High |
| Modern UI | ✅ | ❌ | ✅ | Requires config |
| Mouse Support | ✅ | ⚠️ | ✅ | Requires config |
| Syntax Highlighting | ✅ | ⚠️ | ✅ | ✅ |
| Multi-File | ✅ | ❌ | ✅ | ✅ |
| Plugin System | Planned | ❌ | ✅ | ✅ |
| Simple Configuration | ✅ | ✅ | ✅ | ❌ |

**Detailed Comparison**: See [Product Comparison Documentation](docs/COMPARISON.md)

## 📚 References and Inspiration

This project is inspired by the following excellent projects:
- [Nano](https://www.nano-editor.org/) - Simple and easy-to-use terminal editor
- [Micro](https://micro-editor.github.io/) - Modern terminal editor
- [Sublime Text](https://www.sublimetext.com/) - Classic text editor
- [VS Code](https://code.visualstudio.com/) - Modern IDE
- [FTXUI](https://github.com/ArthurSonzogni/FTXUI) - Powerful terminal UI library

## 📝 License

This project is licensed under the MIT License - see the LICENSE file for details.


## 🌟 Star History

If this project helps you, please give it a star⭐️!

---


