# pnana Release Workflow Tutorial

> [中文](RELEASE_WORKFLOW_TUTORIAL.md) | English

This tutorial is based on `.github/workflows` and describes how to use GitHub Actions for building, testing, and releasing pnana.

## Table of Contents

- [Workflow Overview](#workflow-overview)
- [Prerequisites](#prerequisites)
- [Release Process](#release-process)
- [Release Artifacts](#release-artifacts)
- [Build and Validation](#build-and-validation)
- [Configuration and Customization](#configuration-and-customization)
- [Troubleshooting](#troubleshooting)
- [Best Practices](#best-practices)

---

## Workflow Overview

| Workflow | File | Trigger | Description |
|----------|------|---------|-------------|
| **Release** | `release.yml` | Manual / Push CHANGELOG | Build, package, create GitHub Release |
| **Build and Test** | `build.yml` | push/PR → master, develop, fix/ci-actions | Compile, test, code quality, security scan |
| **Pre-Push Validation** | `pre-push-validation.yml` | Same as above | Full build validation; auto-create Issue on failure |

---

## Prerequisites

### 1. GitHub Permissions
- Repository write access
- `GITHUB_TOKEN` (provided automatically by Actions)
- Release workflow requires `contents: write` and `packages: write`

### 2. CHANGELOG.md Format

Follow [Keep a Changelog](https://keepachangelog.com/):

```markdown
## [Unreleased]

### Added
- New feature description

### Changed
- Change description

### Fixed
- Fix description

## [1.0.0] - 2025-XX-XX

### Added
- ...
```

Version extraction: matches the first `## [x.y.z]` (skips Unreleased).

### 3. Branch Setup
- **Release**: triggers only on `main` or `master`
- **Build / Pre-Push**: `master`, `develop`, `fix/ci-actions`
- Ensure branch protection rules allow workflow to create tags

---

## Release Process

### Option 1: Manual Trigger (workflow_dispatch)

1. Open repository → **Actions** → select **Release** workflow
2. Click **Run workflow**
3. Fill in parameters:
   - **version**: Version number (e.g., `1.0.0`), or leave empty to extract from CHANGELOG.md
   - **release_type**: `stable` (tag: v1.0.0) / `beta` (v1.0.0-beta) / `alpha` (v1.0.0-alpha)
   - **draft**: Whether to create a draft release
4. Click **Run workflow** to start

### Option 2: Auto Trigger (push)

Push CHANGELOG.md with version changes to `main` or `master` to trigger automatically:

```bash
# 1. Update CHANGELOG.md
# 2. Commit and push
git add CHANGELOG.md
git commit -m "Release version 1.1.0"
git push origin main
```

### Release Execution Steps

1. **prepare-release**: Extract version, generate tag, generate release notes
2. **build**: Build 6 package formats (skipped if tag already exists)
3. **create-release**: Create Git tag, create GitHub Release, upload all packages

---

## Release Artifacts

| Format | Example filename | Target |
|--------|------------------|--------|
| .deb | `pnana_1.0.0_amd64.deb` | Ubuntu / Debian |
| .rpm | `pnana_1.0.0_x86_64.rpm` | Red Hat / Fedora / CentOS |
| .tar.gz | `pnana-ubuntu22.04.tar.gz` | Generic Linux |
| .tar.xz | `pnana-ubuntu22.04.tar.xz` | Generic Linux |
| .tar.bz2 | `pnana-ubuntu22.04.tar.bz2` | Generic Linux |
| .zip | `pnana-ubuntu22.04.zip` | Generic Linux |

---

## Build and Validation

### Build and Test (build.yml)

- **Matrix**: ubuntu-latest × gcc/clang
- **Jobs**:
  - **build**: CMake + Ninja build, run ctest
  - **code-quality**: clang-format check
  - **security-scan**: Trivy vulnerability scanning

### Pre-Push Validation (pre-push-validation.yml)

- Full build validation with `./build.sh --clean`
- Runs `ctest` after successful build
- Creates an Issue with `build-failure`, `automated`, `high-priority` labels on failure

---

## Configuration and Customization

### Modify Release Build Options

Edit `.github/workflows/release.yml`:

```yaml
# Build type
env:
  BUILD_TYPE: Release

# CMake options (release build disables optional features)
-DBUILD_IMAGE_PREVIEW=OFF
-DBUILD_TREE_SITTER=OFF
-DBUILD_LUA=OFF
-DBUILD_GO=OFF
```

### Add More Package Formats

Add entries in `build` job's `strategy.matrix.include`:

```yaml
- package_type: TBZ2
  package_name: "pnana-ubuntu22.04.tar.bz2"
  cpack_generator: "TBZ2"
```

### Release Notes Generation

Release notes are auto-generated from the corresponding version's `### Added`, `### Changed`, `### Fixed` sections in CHANGELOG.md, plus installation and usage instructions.

---

## Troubleshooting

### "Could not extract version from CHANGELOG.md"
- **Cause**: Version format doesn't match `## [x.y.z]`
- **Fix**: Ensure a line like `## [1.0.0] - 2025-01-01` exists and is the first version block

### "Tag already exists"
- **Cause**: Tag for this version already exists
- **Fix**: Use a new version number or delete the existing tag (`git tag -d v1.0.0 && git push origin :refs/tags/v1.0.0`)

### Build failure
- **Cause**: Missing dependencies or CMake/build script errors
- **Fix**: Check `build.sh`, install dependencies, review Actions logs

### Release not created
- **Cause**: Workflow skipped (tag exists or not main/master branch)
- **Fix**: Verify branch, ensure CHANGELOG.md has changes, check permissions

### Pre-Push auto-creates Issue
- **Note**: Build failure automatically creates an Issue for tracking
- **Action**: Fix the build and close or address the Issue

---

## Best Practices

1. **Versioning**: Follow [Semantic Versioning](https://semver.org/)
2. **CHANGELOG**: Update promptly, keep format consistent, write clear descriptions
3. **Before release**: Use `draft: true` for a test release, verify, then publish
4. **Branch strategy**: Develop on feature branches, merge via PR, release from main
5. **Testing**: Ensure Build and Pre-Push pass before triggering Release

---

## Support

- View [GitHub Actions logs](https://github.com/{USERNAME}/pnana/actions)
- Refer to the troubleshooting section above
- Open a [GitHub Issue](https://github.com/{USERNAME}/pnana/issues)

---

*Last updated: 2025* | Document aligned with `.github/workflows` implementation
