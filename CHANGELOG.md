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

## [0.0.5 patch] - 2026-03-11 

- Fix the version display issue on the welcome page
- Fix the segmentation fault caused by race conditions in multi-threaded resource acquisition when switching documents
- Optimize the border color effect of the diagnostic pop-up and symbol pop-up


## [0.0.5] - 2026-03-11

### Added

- Add an online Git feature that allows users to open the Git panel from the command palette to perform operations.
- Add an fzf-like feature that enables users to type "fzf" in the command palette to open the fzf panel for quickly viewing file contents and opening files.
- Add a TUI configuration quick access panel, where users can type "tui" in the command palette to quickly open the configuration files of relevant tools in the system for editing in the code area.

### Improved

- Allow users to open files with ellipsis by pressing the "." symbol in the file list (#23)
- Optimize the configuration system and update the relevant documentation (#26)
- Improve the output message effect in the CMake build system (#27)
- Add more online themes and refactor the UI effects
- Redesign the effect of the online terminal (to be verified and optimized)
- Optimize the SSH connection effect under the GO backend, which allows users to establish SSH connections by configuring corresponding parameters (GO dependencies need to be installed)

### Fixed

- Fix the issue where the screen flickers when users press CTRL+Z to undo actions in the code area
- Fix the problem where the entire terminal freezes when users press CTRL+Z in the file list area

### contributor

- @barracuda156  (#19)

For detailed getting started documentation, refer to [QUICKSTART](QUICKSTART.md)


## [0.0.4] - 2026-01-07

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


---

## Version Notes

- **Major version**: Incompatible API changes
- **Minor version**: Backward-compatible functionality additions
- **Patch version**: Backward-compatible bug fixes
