# pnana Release Workflow 使用教程

> 中文 | [English](RELEASE_WORKFLOW_TUTORIAL_EN.md)

本教程基于 `.github/workflows` 实现，介绍如何使用 GitHub Actions 进行 pnana 的构建、测试与发布。

## 📋 目录

- [Workflow 概览](#workflow-概览)
- [前置要求](#前置要求)
- [发布流程](#发布流程)
- [发布产物](#发布产物)
- [构建与验证](#构建与验证)
- [配置与自定义](#配置和自定义)
- [故障排除](#故障排除)
- [最佳实践](#最佳实践)

---

## Workflow 概览

| Workflow | 文件 | 触发条件 | 说明 |
|----------|------|----------|------|
| **Release** | `release.yml` | 手动 / 推送 CHANGELOG | 构建、打包、创建 GitHub Release |
| **Build and Test** | `build.yml` | push/PR → master, develop, fix/ci-actions | 编译、测试、代码格式、安全扫描 |
| **Pre-Push Validation** | `pre-push-validation.yml` | 同上 | 完整构建验证，失败时自动创建 Issue |

---

## 前置要求

### 1. GitHub 权限
- 仓库写权限（write access）
- `GITHUB_TOKEN`（Actions 自动提供）
- Release 需 `contents: write`、`packages: write`

### 2. CHANGELOG.md 格式

遵循 [Keep a Changelog](https://keepachangelog.com/) 格式：

```markdown
## [Unreleased]

### Added
- 新功能描述

### Changed
- 变更描述

### Fixed
- 修复描述

## [1.0.0] - 2025-XX-XX

### Added
- ...
```

版本提取规则：匹配第一个 `## [x.y.z]`（不含 Unreleased）。

### 3. 分支设置
- **Release**：仅在 `main`、`master` 分支触发
- **Build / Pre-Push**：`master`、`develop`、`fix/ci-actions`
- 确保分支保护规则允许 workflow 创建 tag

---

## 发布流程

### 方式一：手动触发（workflow_dispatch）

1. 打开仓库 → **Actions** → 选择 **Release** workflow
2. 点击 **Run workflow**
3. 填写参数：
   - **version**：版本号（如 `1.0.0`），留空则从 CHANGELOG.md 提取
   - **release_type**：`stable`（tag: v1.0.0） / `beta`（v1.0.0-beta） / `alpha`（v1.0.0-alpha）
   - **draft**：是否为草稿发布
4. 点击 **Run workflow** 开始执行

### 方式二：自动触发（push）

推送包含版本变更的 CHANGELOG.md 到 `main` 或 `master` 时自动触发：

```bash
# 1. 更新 CHANGELOG.md
# 2. 提交并推送
git add CHANGELOG.md
git commit -m "Release version 1.1.0"
git push origin main
```

### Release 执行步骤

1. **prepare-release**：提取版本、生成 tag、生成 release notes
2. **build**：构建 6 种包格式（若 tag 已存在则跳过）
3. **create-release**：创建 Git tag、创建 GitHub Release、上传所有包

---

## 发布产物

| 格式 | 文件名示例 | 适用场景 |
|------|-------------|----------|
| .deb | `pnana_1.0.0_amd64.deb` | Ubuntu / Debian |
| .rpm | `pnana_1.0.0_x86_64.rpm` | Red Hat / Fedora / CentOS |
| .tar.gz | `pnana-ubuntu22.04.tar.gz` | 通用 Linux |
| .tar.xz | `pnana-ubuntu22.04.tar.xz` | 通用 Linux |
| .tar.bz2 | `pnana-ubuntu22.04.tar.bz2` | 通用 Linux |
| .zip | `pnana-ubuntu22.04.zip` | 通用 Linux |

---

## 构建与验证

### Build and Test (build.yml)

- **矩阵**：ubuntu-latest × gcc/clang
- **Jobs**：
  - **build**：CMake + Ninja 构建，运行 ctest
  - **code-quality**：clang-format 检查
  - **security-scan**：Trivy 漏洞扫描

### Pre-Push Validation (pre-push-validation.yml)

- 使用 `./build.sh --clean` 进行完整编译验证
- 编译成功后运行 `ctest`
- 失败时自动创建带 `build-failure`、`automated`、`high-priority` 标签的 Issue

---

## 配置和自定义

### 修改 Release 构建选项

编辑 `.github/workflows/release.yml`：

```yaml
# 构建类型
env:
  BUILD_TYPE: Release

# CMake 选项（当前发布构建关闭部分可选功能）
-DBUILD_IMAGE_PREVIEW=OFF
-DBUILD_TREE_SITTER=OFF
-DBUILD_LUA=OFF
-DBUILD_GO=OFF
```

### 添加更多包格式

在 `build` job 的 `strategy.matrix.include` 中增加：

```yaml
- package_type: TBZ2
  package_name: "pnana-ubuntu22.04.tar.bz2"
  cpack_generator: "TBZ2"
```

### Release Notes 生成

Release notes 基于 CHANGELOG.md 中对应版本的 `### Added`、`### Changed`、`### Fixed` 等小节自动生成，并附加安装说明和 Usage 示例。

---

## 故障排除

### "Could not extract version from CHANGELOG.md"
- **原因**：版本格式不符合 `## [x.y.z]`
- **解决**：确保有类似 `## [1.0.0] - 2025-01-01` 的条目，且为第一个版本块

### "Tag already exists"
- **原因**：该版本的 tag 已存在
- **解决**：使用新版本号，或删除已有 tag（`git tag -d v1.0.0 && git push origin :refs/tags/v1.0.0`）

### 构建失败
- **原因**：依赖缺失或 CMake/构建脚本错误
- **解决**：检查 `build.sh`、安装依赖、查看 Actions 日志

### Release 未创建
- **原因**：workflow 被跳过（tag 已存在或非 main/master 分支）
- **解决**：确认分支、确认 CHANGELOG.md 有变更、检查权限

### Pre-Push 自动创建 Issue
- **说明**：构建失败时会在仓库中创建 Issue，便于追踪
- **处理**：修复后关闭或按 Issue 说明操作

---

## 最佳实践

1. **版本管理**：遵循 [Semantic Versioning](https://semver.org/)
2. **CHANGELOG**：及时更新、格式规范、描述清晰
3. **发布前**：先用 `draft: true` 创建草稿，验证后再正式发布
4. **分支策略**：在 feature 分支开发，通过 PR 合并到主分支，在主分支发布
5. **测试**：确保 Build/Pre-Push 均通过后再触发 Release

---

## 📞 支持

- 查看 [GitHub Actions 日志](https://github.com/{USERNAME}/pnana/actions)
- 参考本教程的故障排除部分
- 提交 [GitHub Issue](https://github.com/{USERNAME}/pnana/issues)

---

*最后更新: 2025年* | 文档与 `.github/workflows` 实现同步
