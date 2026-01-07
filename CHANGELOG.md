# Changelog

This document records all important changes to the pnana project.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Added GitHub Actions workflows for automated building, testing, and releasing
- Added RELEASE.md template for generating standardized release notes

### Improved
- Optimized release process to support multiple package formats
- Added code quality checks and security scanning

## [0.0.4] - 2025-01-07

### Added

- Integrated LSP system, allowing users to perform code hints, diagnostics, and formatting
- Added cursor style configuration modification functionality
- Preliminary completion of plugin system, allowing users to enable/disable plugins during use
- Integrated FFmpeg to convert images to ASCII art and preview them

### Improved
- Added syntax highlighting for more languages (tree-sitter/built-in syntax highlighter)
- Provided more built-in themes for switching
- Optimized the theme style and functionality of the inline terminal, improved command parsing effects
- Optimized UI effects, improved interface UI and icon consistency
- Optimized file editing and browsing effects
- Optimized the SSH module written in Go (untested)

### Fixed

- Improved build configuration, allowing users to manually configure feature functions and reduce necessary dependencies
- Fixed configuration system, allowing users to update configurations
- Improved compilation and build related documentation to increase user onboarding speed

For detailed getting started documentation, refer to [QUICKSTART](QUICKSTART.md)


## [1.0.0] - 2023-XX-XX

### Added
- Modern terminal text editor based on FTXUI
- 28 built-in themes
- Three-column layout design
- Intuitive shortcut key system
- Multi-file support
- Syntax highlighting
- LSP support
- Lua plugin system
- SSH remote file editing
- Built-in terminal
- Image preview functionality
- Binary file viewer
- Search and replace functionality
- Split view
- Command palette
- File browser
- Encoding conversion support

### Improved
- Optimized memory usage
- Improved large file loading speed
- Enhanced user interface responsiveness

### Fixed
- Fixed display issues in certain terminals
- Fixed syntax highlighting problems for specific file formats

## [0.9.0] - 2023-XX-XX

### Added
- Initial version features
- Basic text editing functionality
- Simple theme system

---

## Version Notes

- **Major version**: Incompatible API changes
- **Minor version**: Backward-compatible functionality additions
- **Patch version**: Backward-compatible bug fixes
