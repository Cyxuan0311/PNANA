# PNANA 上线到各包管理器方案

本文档说明如何将 PNANA 发布到 **apt**（Debian/Ubuntu）、**yum/dnf**（Fedora/RHEL/CentOS）、**yay**（Arch AUR）和 **Homebrew**（macOS/Linux），便于用户通过系统包管理器安装。

你已在 `build/packages` 通过 CPack 生成了 DEB、RPM、TXZ 等包，下面按各平台分别说明发布路径与步骤。

---

## 一、APT（Debian / Ubuntu）

**适用**：`apt install pnana`（Ubuntu、Debian、Linux Mint、Pop!_OS 等）

### 方式 A：Ubuntu PPA（推荐，免费且官方）

1. **注册 Launchpad 并配置 PGP**
   - 注册：https://launchpad.net/
   - 创建 PGP 密钥并上传到 Launchpad：https://help.launchpad.net/YourAccount/ImportingYourPGPKey

2. **创建 PPA**
   - 打开 https://launchpad.net/people/+me
   - 创建 PPA，例如：`ppa:your-username/pnana`

3. **准备源码包（需生成 .dsc + 源码 tarball）**
   - 若项目无 `debian/` 目录，可先用 `dh_make` 生成模板，或手写：
   - 在项目根目录创建 `debian/`，至少包含：
     - `debian/changelog`（版本与变更）
     - `debian/control`（依赖、描述）
     - `debian/rules`（构建规则，可调用 CMake）
     - `debian/compat`（如 `10`）
   - 构建源码包：
     ```bash
     dpkg-buildpackage -S -us -uc
     ```
   - 会生成 `../pnana_0.0.4-1.dsc` 和 `../pnana_0.0.4.orig.tar.gz`

4. **上传到 PPA**
   ```bash
   dput ppa:your-username/pnana pnana_0.0.4-1_source.changes
   ```

5. **用户安装**
   ```bash
   sudo add-apt-repository ppa:your-username/pnana
   sudo apt update
   sudo apt install pnana
   ```

### 方式 B：自建 APT 仓库（适合有服务器的情况）

1. 在服务器上创建目录结构，例如：
   ```text
   /var/www/apt-repo/
   ├── conf/
   │   └── distributions   # 定义 Origin、Suite、Components 等
   ├── pool/
   │   └── main/
   │       └── p/
   │           └── pnana/
   │               └── pnana_0.0.4_amd64.deb
   ```

2. 用 `reprepro` 或 `aptly` 导入 DEB 并生成仓库元数据。

3. 用户添加源后 `apt update && apt install pnana`。

你现有的 `build/packages/` 下生成的 `.deb` 可直接用于方式 B；方式 A 需要上述 Debian 源码包流程。

---

## 二、YUM/DNF（Fedora / RHEL / CentOS）

**适用**：`dnf install pnana` 或 `yum install pnana`

### 方式 A：Fedora COPR（推荐，免费）

1. **注册 COPR**
   - https://copr.fedorainfracloud.org/ 用 Fedora 账号登录。

2. **创建项目**
   - New Project → 填写名称（如 `pnana`）、选择 Fedora 版本与架构。

3. **提供 SRPM 或源码**
   - **选项 1**：上传 CPack 生成的 `.rpm`（可直接用于“从二进制包构建”的流程，若 COPR 支持）。
   - **选项 2（推荐）**：在仓库中提供 `pnana.spec` 和源码 tarball，让 COPR 在云端用 `rpmbuild` 从源码构建。  
     你的 spec 在：`build/packages/_CPack_Packages/Linux/RPM/SPECS/pnana.spec`，可复制到项目根目录的 `rpm/` 或 `packaging/`，并改为从 GitHub Release 的源码 tarball 构建（用 `Source0: https://github.com/.../archive/v%{version}.tar.gz` 等）。

4. **在 COPR 中“Build”**
   - 添加 Build 时选择“从 SRPM/源码 + spec”或上传 spec + 源码，触发构建。

5. **用户安装（Fedora）**
   ```bash
   sudo dnf copr enable your-username/pnana
   sudo dnf install pnana
   ```

RHEL/CentOS 若该 COPR 支持对应发行版，也可用同样方式启用并安装。

### 方式 B：自建 YUM/DNF 仓库

1. 将 CPack 生成的 `.rpm` 放到目录，例如 `~/rpm-repo/RPMS/x86_64/`。
2. 在该目录执行：
   ```bash
   createrepo .
   ```
3. 用 Nginx/Apache 或 `file://` 暴露该目录，在 `/etc/yum.repos.d/` 添加 `.repo` 指向该 URL。
4. 用户执行 `dnf install pnana` 即可。

---

## 三、YAY / AUR（Arch Linux）

**适用**：`yay -S pnana`（或 `paru -S pnana` 等 AUR 助手）

AUR 不直接分发二进制，而是提供 **PKGBUILD**，用户在本地从源码或你提供的二进制 URL 构建/安装。

### 步骤

1. **注册 AUR 账号**
   - https://aur.archlinux.org/ 注册并配置 SSH 或 HTTPS。

2. **克隆 AUR 仓库（若已存在则更新）**
   ```bash
   git clone ssh://aur@aur.archlinux.org/pnana.git aur-pnana
   cd aur-pnana
   ```
   若包名尚未被占用，需先在 AUR 网站“Submit Package”并填包名，再克隆空仓库或带 PKGBUILD 的仓库。

3. **编写或更新 PKGBUILD**
   - 下面给出一个基于 **GitHub Release 源码 tarball** 的示例（与 CPack 生成的 TXZ 无直接依赖，AUR 更倾向源码构建）：

   创建文件：`PKGBUILD`（放在 `aur-pnana/` 下）：

   ```bash
   # Maintainer: Your Name <your.email@example.com>

   pkgname=pnana
   pkgver=0.0.4
   pkgrel=1
   pkgdesc="Modern Terminal Text Editor"
   arch=(x86_64)
   url="https://github.com/Cyxuan0311/PNANA"
   license=(MIT)
   depends=(gcc-libs cmake ftxui-component ftxui-dom ftxui-screen)
   makedepends=(git cmake)
   source=("$pkgname-$pkgver.tar.gz::https://github.com/Cyxuan0311/PNANA/archive/refs/tags/v$pkgver.tar.gz")
   sha256sums=(SKIP)  # 首次提交可 SKIP，之后用 updpkgsums 生成

   build() {
     cd "PNANA-$pkgver"
     cmake -B build -DCMAKE_INSTALL_PREFIX=/usr
     cmake --build build
   }

   package() {
     cd "PNANA-$pkgver"
     DESTDIR="$pkgdir" cmake --install build
     # 若有配置文件等，可在此 install 到 $pkgdir
   }
   ```

   - 若你希望 AUR 直接安装 **预编译的 TXZ**（较少见），可把 `source` 改为指向 GitHub Release 上的 `.tar.xz`，并在 `package()` 里解压到 `$pkgdir`。通常 AUR 更推荐源码构建以便适配 Arch 的库版本。

4. **提交到 AUR**
   ```bash
   makepkg --printsrcinfo > .SRCINFO
   git add PKGBUILD .SRCINFO
   git commit -m "Upgrade to 0.0.4"
   git push origin master
   ```

5. **用户安装**
   ```bash
   yay -S pnana
   ```

注意：Arch 的 `ftxui` 包名可能是 `ftxui-component`、`ftxui-dom`、`ftxui-screen`，需根据 Arch 官方/社区仓库实际包名调整 `depends` 和 `makedepends`。

---

## 四、Homebrew（macOS / Linux）

**适用**：`brew install pnana`

### 方式 A：个人 Tap（推荐，审核简单）

1. **在 GitHub 创建仓库**  
   例如：`github.com/Cyxuan0311/homebrew-pnana`（名称为 `homebrew-pnana` 或 `homebrew-tap`）。

2. **编写 Formula**
   - 在仓库中创建：`Formula/pnana.rb`  
   - 内容示例（从 GitHub Release 的源码 tarball 构建）：

   ```ruby
   class Pnana < Formula
     desc "Modern Terminal Text Editor"
     homepage "https://github.com/Cyxuan0311/PNANA"
     url "https://github.com/Cyxuan0311/PNANA/archive/refs/tags/v0.0.4.tar.gz"
     sha256 "<从下载的 tarball 运行 openssl dgst -sha256 得到>"
     license "MIT"
     depends_on "cmake" => :build
     depends_on "ftxui"

     def install
       system "cmake", "-S", ".", "-B", "build", "-DCMAKE_INSTALL_PREFIX=#{prefix}"
       system "cmake", "--build", "build"
       system "cmake", "--install", "build"
     end

     test do
       assert_match "pnana", shell_output("#{bin}/pnana --version 2>&1", 1).strip
     end
   end
   ```

   - 若你提供的是 macOS 预编译二进制，可改为 `url` 指向 zip/tar.gz，在 `install` 里只做解压和 `bin.install "pnana"` 等。

3. **推送并让用户使用**
   ```bash
   brew tap Cyxuan0311/pnana
   brew install pnana
   ```
   （Tap 名以你实际 GitHub 账号和仓库名为准，如 `Cyxuan0311/homebrew-pnana` → `brew tap Cyxuan0311/pnana`）

### 方式 B：进入 homebrew-core（可选，审核严格）

- 在 https://github.com/Homebrew/homebrew-core 提 PR，新增 `Formula/p/pnana.rb`。
- 需满足 Homebrew 的命名、依赖、测试等规范，审核周期较长。多数项目先做个人 Tap，再考虑进 core。

---

## 五、发布前检查清单

| 项目           | 说明 |
|----------------|------|
| 版本号         | 在 `cmake/PackageConfig.cmake` 和各平台包元数据中统一（如 0.0.4）。 |
| GitHub Release | 建议每个版本打 tag（如 `v0.0.4`），并上传对应源码 tarball 和 DEB/RPM/TXZ，便于 AUR/COPR/Homebrew 引用。 |
| 依赖           | 各平台依赖名不同（如 ftxui、cmake），在 PKGBUILD、Formula、spec、debian/control 中写对。 |
| 安装后路径     | 确保 `pnana` 在 PATH 中（如 `/usr/bin` 或 Homebrew 的 `bin`）。 |
| 许可           | 仓库中包含 LICENSE 文件，与各包声明的 license 一致（如 MIT）。 |

---

## 六、建议的发布顺序与优先级

1. **GitHub Release**  
   打 tag、上传 CPack 生成的 `.deb`、`.rpm`、`.tar.xz` 等，并附简短 Release Notes。

2. **AUR（yay）**  
   只需 PKGBUILD + .SRCINFO，流程简单，用户量大，优先做。

3. **Homebrew Tap**  
   一个 Formula 即可，适合 macOS 用户。

4. **Ubuntu PPA**  
   需要 Debian 打包知识，但一旦建好，后续主要是改 changelog 和上传。

5. **Fedora COPR**  
   用 spec + 源码或 SRPM 在 COPR 构建，再 `dnf copr enable` 即可安装。

6. **自建 apt/yum 仓库**  
   在有服务器且需要完全自主控制时再考虑。

按上述顺序推进，即可逐步覆盖 apt、yum、yay、brew 四大类使用场景。若你提供当前 CMake/CPack 的版本号与依赖列表，我可以按你仓库结构再写一份可直接拷贝使用的 `debian/` 初版或 AUR PKGBUILD/Homebrew Formula 终版。
