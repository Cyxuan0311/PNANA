# pnana Development Roadmap

> [中文](ROADMAP.md) | English

This document outlines pnana's development plans and feature roadmap. It is updated as the project evolves.

## Table of Contents

- [Version History](#version-history)
- [Planned Versions](#planned-versions)
- [Future Outlook](#future-outlook)
- [Priority Levels](#priority-levels)
- [Contributing](#contributing)

---

## Version History

### v0.0.4 (Current)

**Release Date**: 2025-01-07

**Completed Features**:

- [x] Core Editing
  - Text input, cursor movement, selection
  - Undo/redo, basic edit operations
- [x] Multi-file & Split View
  - Tabs, multi-document switching
  - Horizontal/vertical split (Ctrl+L)
  - Focus navigation between split regions
- [x] Search
  - Text search, regex support
  - Search result highlighting
- [x] Syntax Highlighting
  - Multi-language (tree-sitter + built-in)
  - File type detection
- [x] Themes & Configuration
  - Multiple built-in themes
  - JSON config, CLI arguments
- [x] LSP Support
  - Code completion, real-time diagnostics
  - Go to definition, find references
  - Symbol search, code formatting
- [x] Lua Plugin System
  - Plugin API, events, commands, keymaps
  - Plugin management (enable/disable)
  - Theme extension, status bar beautify
- [x] Terminal Integration
  - Built-in terminal
  - Command execution, output display
- [x] File Browser
  - Directory tree, file navigation
- [x] Todo Panel
  - Todo management, due reminders
- [x] Image Preview
  - FFmpeg ASCII art preview
- [x] SSH Module
  - Go-based SSH connection (testing in progress)

---

## Planned Versions

### v0.1 (Planned)

**Estimated**: 2025 Q2

**Planned Features**:

- [ ] Syntax Highlighting Enhancement
  - More accurate syntax recognition
  - Additional languages
  - Custom syntax rules
- [ ] Multi-cursor Editing
  - Multi-cursor support
  - Edit multiple positions simultaneously
  - Column selection improvements
- [ ] Code Folding
  - Block folding
  - Fold/unfold shortcuts
  - Fold state persistence
- [ ] Bracket Matching
  - Matching bracket highlight
  - Bracket jump
  - Auto-complete brackets and quotes
- [ ] File Tree Sidebar Enhancement
  - Improved directory tree
  - New, delete, rename operations

**Priority**: High

---

### v0.2 (Planned)

**Estimated**: 2025 Q3

**Planned Features**:

- [ ] Git Integration
  - Git status display
  - Diff viewing
  - Commit history
  - Basic Git operations
- [ ] LSP Enhancement
  - Code refactoring
  - Code Actions
  - Workspace symbol search
- [ ] Plugin System Enhancement
  - Plugin configuration UI
  - More APIs
- [ ] Auto Indent
  - Smart indent by file type

**Priority**: Medium

---

### v0.3 (Planned)

**Estimated**: 2025 Q4

**Planned Features**:

- [ ] Performance Optimization
  - Large file handling
  - Rendering and memory optimization
- [ ] User Experience
  - Error messages and feedback
  - UI polish
- [ ] Documentation & Testing
  - API documentation
  - Unit and integration tests

**Priority**: Medium

---

## Future Outlook

### v1.0 (Mid-term)

**Goal**: More complete IDE experience

- [ ] Project Management
  - Project templates
  - Project configuration
  - Dependency management
- [ ] Debugger Integration
  - Breakpoints, variable inspection
  - Call stack, step debugging
- [ ] Remote Editing
  - SSH improvements
  - Remote file editing
  - Remote terminal

### v2.0+ (Long-term)

- [ ] Collaborative Editing
  - Real-time collaboration
  - Multi-user editing
- [ ] Cross-platform & i18n
  - Windows / macOS optimization
  - Multi-language UI
- [ ] Cloud Sync
  - Config and plugin sync
- [ ] AI Integration
  - Enhanced code completion
  - Code suggestions and fixes

---

## Priority Levels

| Priority | Description |
|----------|-------------|
| **High** | Core features, key UX, stability |
| **Medium** | Enhanced features, dev productivity, ecosystem |
| **Low** | Long-term goals, experimental features |

---

## Contributing

Contributions are welcome:

1. **Check Issues**: Find open tasks
2. **Pick a Task**: Choose a feature or bug
3. **Submit PR**: Follow contribution guidelines
4. **Discuss**: Join feature discussions in Issues

### How to Contribute

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/AmazingFeature`
3. Commit changes: `git commit -m 'Add some AmazingFeature'`
4. Push: `git push origin feature/AmazingFeature`
5. Open a Pull Request

---

## Feedback and Suggestions

- **Submit Issue**: Open an Issue on GitHub
- **Feature Request**: Use the Feature Request label
- **Bug Report**: Use the Bug label with details

---

**Note**: The roadmap may change based on user feedback and development progress. Check [GitHub Issues](https://github.com/{USERNAME}/pnana/issues) and [Releases](https://github.com/{USERNAME}/pnana/releases) for the latest updates.
