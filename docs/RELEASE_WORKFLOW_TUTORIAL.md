# PNANA Release Workflow 使用教程

本教程详细介绍如何使用 GitHub Actions 自动发布工作流来发布 PNANA 项目。

## 📋 前置要求

### 1. GitHub 权限
- 需要对仓库有写权限（write access）
- 需要 `GITHUB_TOKEN` 权限（自动提供）

### 2. CHANGELOG.md 格式
确保 CHANGELOG.md 遵循以下格式：
```markdown
## [Unreleased]

### Added
- 新功能描述

### Changed
- 变更描述

### Fixed
- 修复描述

## [1.0.0] - 2023-XX-XX
...
```

### 3. 分支设置
- 主分支应该是 `main` 或 `master`
- 确保分支保护规则允许 workflow 创建 tags

## 🚀 发布方式

### 方式一：手动触发发布

#### 步骤 1: 访问 GitHub Actions
1. 打开你的 GitHub 仓库
2. 点击 **Actions** 标签
3. 在左侧找到 **Release** workflow

#### 步骤 2: 手动触发
1. 点击 **Run workflow** 按钮
2. 填写发布参数：
   - **Version**: 版本号（如 1.0.0），留空则从 CHANGELOG.md 提取
   - **Release type**: 发布类型
     - `stable`: 稳定版 (tag: v1.0.0)
     - `beta`: 测试版 (tag: v1.0.0-beta)
     - `alpha`: 开发版 (tag: v1.0.0-alpha)
   - **Draft**: 是否创建草稿发布

#### 步骤 3: 确认发布
1. 点击 **Run workflow**
2. workflow 将自动：
   - 提取版本信息
   - 构建项目
   - 生成 release notes
   - 创建 Git tag
   - 发布到 GitHub Releases

### 方式二：自动触发发布

#### 通过 CHANGELOG.md 触发
当你推送包含版本更新的 CHANGELOG.md 到主分支时，workflow 会自动触发。

#### 准备步骤：
1. 更新 CHANGELOG.md，添加新版本：
   ```markdown
   ## [1.1.0] - 2023-12-07

   ### Added
   - 新功能

   ### Changed
   - 改进功能

   ## [Unreleased]
   ```
2. 提交并推送：
   ```bash
   git add CHANGELOG.md
   git commit -m "Release version 1.1.0"
   git push origin main
   ```

## 📦 发布产物

Workflow 会自动生成以下发布产物：

### Ubuntu/Debian (.deb)
- 文件名: `pnana_{VERSION}_amd64.deb`
- 适用于: Ubuntu 18.04+, Debian 10+

### 通用 Linux (.tar.gz)
- 文件名: `pnana-ubuntu22.04.tar.gz`
- 适用于: 大多数 Linux 发行版

## 📋 Release Notes 格式

自动生成的 Release Notes 包含：

### 标准格式
```markdown
# Release v1.0.0

## New Features
- 功能描述

## Improvements
- 改进描述

## Fixes
- 修复描述

## Installation

### Ubuntu/Debian (.deb)
```bash
wget https://github.com/{USERNAME}/pnana/releases/download/v1.0.0/pnana_1.0.0_amd64.deb
sudo dpkg -i pnana_1.0.0_amd64.deb
sudo apt-get install -f  # If there are dependency issues
```

### Generic Linux (.tar.gz)
```bash
wget https://github.com/{USERNAME}/pnana/releases/download/v1.0.0/pnana-ubuntu22.04.tar.gz
tar -xzf pnana-ubuntu22.04.tar.gz
cd package
sudo ./install.sh
```

## Usage
```bash
# Start blank editor
pnana

# Open file
pnana filename.txt

# View help
pnana --help
```

## What's Changed
See [CHANGELOG.md](https://github.com/{USERNAME}/pnana/blob/main/CHANGELOG.md) for full details.
```

## 🔧 配置和自定义

### 修改构建配置
如果需要修改构建参数，编辑 `.github/workflows/release.yml`：

```yaml
env:
  BUILD_TYPE: Release  # 可以改为 Debug
```

### 添加更多构建目标
在 `strategy.matrix` 中添加新的包格式：

```yaml
matrix:
  include:
    - package_type: deb
      package_name: "pnana_${{ needs.prepare-release.outputs.version }}_amd64.deb"
    - package_type: rpm
      package_name: "pnana-${{ needs.prepare-release.outputs.version }}-x86_64.rpm"
```

### 修改触发条件
添加更多触发条件：

```yaml
on:
  push:
    tags:
      - 'v*'
  release:
    types: [published]
```

## 🐛 故障排除

### 常见问题

#### 1. "Could not extract version from CHANGELOG.md"
**原因**: CHANGELOG.md 格式不正确
**解决**:
- 确保版本格式为 `## [x.y.z]`
- 检查语法是否正确

#### 2. "Tag already exists"
**原因**: 该版本的 tag 已存在
**解决**:
- 使用不同版本号
- 或者删除已存在的 tag

#### 3. 构建失败
**原因**: 依赖缺失或构建脚本错误
**解决**:
- 检查 `build.sh` 脚本
- 验证所有依赖都已安装
- 查看 workflow 日志获取详细错误信息

#### 4. Release 没有创建
**原因**: workflow 被跳过
**解决**:
- 检查分支名称（必须是 main 或 master）
- 确保 CHANGELOG.md 有实际变更
- 验证 workflow 权限

### 查看日志
1. 进入 **Actions** 标签
2. 点击运行的 workflow
3. 查看每个 job 的详细日志

### 手动修复
如果 workflow 失败，你可以：
1. 手动创建 tag: `git tag v1.0.0 && git push origin v1.0.0`
2. 手动构建: `./build.sh --clean --install`
3. 手动创建 release 并上传文件

## 📊 Workflow 状态检查

### 成功标志
- ✅ 所有 jobs 显示绿色
- ✅ Git tag 已创建
- ✅ GitHub Release 已发布
- ✅ 下载链接可用

### 验证发布
```bash
# 检查版本
pnana --version

# 验证安装
which pnana
pnana --help
```

## 🎯 最佳实践

### 1. 版本管理
- 使用语义化版本 (Semantic Versioning)
- 主版本：不兼容的 API 变更
- 次版本：向下兼容的功能新增
- 修订版本：向下兼容的 bug 修复

### 2. CHANGELOG.md 维护
- 及时更新变更日志
- 使用标准格式
- 保持清晰简洁的描述

### 3. 分支策略
- 在 feature 分支开发
- 通过 Pull Request 合并到主分支
- 在主分支进行发布

### 4. 测试发布
- 先用 `draft: true` 创建草稿
- 验证所有功能正常
- 确认文档和安装说明正确

### 5. 发布频率
- 稳定版：按需发布
- 测试版：每个月或重要功能完成后
- 开发版：每周或持续集成

## 📞 支持

如果遇到问题：
1. 查看 [GitHub Issues](https://github.com/{USERNAME}/pnana/issues)
2. 检查 workflow 日志
3. 参考本教程的故障排除部分

## 🔄 更新 Workflow

当需要更新 workflow 时：
1. 编辑 `.github/workflows/release.yml`
2. 测试更改（建议在测试分支上）
3. 提交到主分支
4. 验证新版本工作正常

---

*最后更新: 2024年* | *版本: 1.0.0*
