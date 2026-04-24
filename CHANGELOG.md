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


## [0.0.6] - 2026-04-24


### Added

- Add a logo configuration option in the welcome page
- Add libvterm support for terminal emulation, users to use libvterm to emulate the terminal effect in the code area
- Add Chafa support for image compression, users to compress images in the code area using Chafa compression algorithm,if chafa not install ,the image will use local compression algorithm
- Some operation when user using will operate a toast to notify the user
- Add a CPP(Libssh2) backend for users to use SSH to connect to remote servers.Now user can choose GO or CPP backend to connect to remote servers.(Attention:GO transfer speed faster than CPP transfer speed)
- Add a history panel in command palette ,user can rollback the file content to previous previous versions
- Add a auto indent feature,users can automatically indent the code in the code area by pressing the "Tab" key or when enter to other line [SMART_INDENT](docs/SMART_INDENT.md)
- CMake build add multi architecture support,users can build the project for different architectures such as x86_64,arm64...

### Improved

- Optimize the fzf panel refresh speed, reducing the time required to refresh the file list
- Optimize the remote connection feature, users can preview remote images in the code area after connecting
- Optimize the remote transfer speed, users can upload and download files faster than before
- Optimize the program startup speed, reducing the time required to start the program
- Optimize the plugin api, improving the plugin development experience(Still in development)


### Fixed

- Fix the compile error on OpenBSD platform
- Fix release package issue,Users can download the release package from the release page

For detailed getting started documentation, refer to [QUICKSTART](QUICKSTART.md)


## [0.0.6 patch] - 2026-03-11 

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
