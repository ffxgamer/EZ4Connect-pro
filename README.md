# EZ4Connect Pro

EZ4Connect Pro 基于 [EZ4Connect](https://github.com/chenx-dust/EZ4Connect)，支持在主界面管理多条 VPN 配置。每条配置分别保存服务器地址、端口、账号、密码和认证方式。

在“VPN 配置”旁点击“＋”，输入名称并完成配置引导，即可新增一条独立配置。点击列表中的配置可直接切换；如果 VPN 正在连接，程序会先断开，再使用所选配置重新连接。点击某条配置旁的齿轮可编辑该配置；编辑时会先断开当前连接。取消新建引导不会创建配置。

Pro 保留原版的配置路径，因此两者会共用已有配置。旧版地址列表中当前选中的地址仍保留在原配置内，其他仅保存地址的记录不会自动变成独立账号配置。

本仓库通过 GitHub Actions 每 5 分钟检查上游 `master`：发现更新时先合并并运行 Linux 构建和测试，全部通过后自动推送到 Pro 的 `master`。如果合并冲突、测试失败、主分支在检查期间发生变化或分支保护不允许推送，工作流会失败并保留远端主分支，需要检查 Actions 日志并处理。GitHub 定时任务可能延迟，因此并非严格实时。工作流需要仓库内容写权限；如需触发其他依赖 push 事件的工作流，或分支保护要求特定身份，可以配置具备相应权限的 `UPSTREAM_SYNC_TOKEN` secret。运行中的 Pro 只检查本仓库发布版本，不会提示安装原版更新。

## 自动发布 Pro

将需要发布的代码提交并推送到本仓库后，创建并推送版本标签，例如：

```bash
git tag v1.0.0
git push origin v1.0.0
```

标签使用 `v主版本.次版本.修订版本` 格式，也支持 `v1.0.0-rc.1` 这类预发布版本。工作流会运行测试，构建 Windows、macOS 和 Linux 的 x64 与 arm64 共六个安装包，在全部成功后自动创建 GitHub Release 并上传附件；带后缀的版本会标记为预发布。重跑已有版本时会替换同名附件。Actions 需要仓库内容写入权限。

macOS 默认生成未签名、未公证的 DMG，安装时可能需要手动允许打开。如需签名和 Apple 公证，请在仓库 Actions secrets 中同时配置 `MACOS_CERT_P12`（P12 文件的 Base64 内容）、`MACOS_CERT_PASSWORD`、`MACOS_CERT_IDENTITY`、`APPLE_ID`、`APPLE_APP_SPECIFIC_PASSWORD` 和 `APPLE_TEAM_ID`。只配置其中一部分会使构建失败，避免误以为发布包已完成公证。

手动运行 Build 工作流可生成构建产物，但不会创建 Release；正式发布请推送版本标签。

以下为上游项目的说明。

## 上游 EZ4Connect

*前身为 HITsz Connect for Windows*

![Action](https://github.com/chenx-dust/EZ4Connect/actions/workflows/build.yml/badge.svg)
![Release](https://img.shields.io/github/v/release/chenx-dust/EZ4Connect)
![Downloads](https://img.shields.io/github/downloads/chenx-dust/EZ4Connect/total)
![License](https://img.shields.io/github/license/chenx-dust/EZ4Connect)

改进的 ZJU-Connect 图形界面

## 🎉 现已正式提供 aTrust 支持

如使用中遇到问题，可加入 ZJU-Connect 用户反馈 QQ 群 1037726410 交流。

<div align="center">
<img src="docs/main.png" width="600px">
</div>

## 使用方式

以下下载链接和文件名适用于上游 EZ4Connect；Pro 的发布版本请查看 [EZ4Connect Pro Releases](https://github.com/ffxgamer/EZ4Connect-pro/releases)。

上游 [Releases](https://github.com/chenx-dust/EZ4Connect/releases)：

- **Windows 用户**：下载 `EZ4Connect-vX.X.X-windows-ARCH.zip` ，解压至同一目录下，双击运行 `EZ4Connect.exe` ；
  - 如果遇到缺少 DLL 等问题，请先下载安装 Microsoft Visual C++ 可再发行程序包版本（[x64](https://aka.ms/vs/17/release/vc_redist.x64.exe) | [arm64](https://aka.ms/vs/17/release/vc_redist.arm64.exe)），再运行程序；
- **macOS 用户**：下载 `EZ4Connect-vX.X.X-macOS-ARCH.dmg` ，将 EZ4Connect 移动到应用程序目录中；
  - ~~如果遇到“Apple 无法检查 App 是否包含恶意软件”等报错，请参考 [Apple 支持](https://support.apple.com/zh-cn/guide/mac-help/mchleab3a043/mac) 进行操作。~~本软件已通过 Apple 官方公证，可直接运行；
- **Linux 用户**：下载 `EZ4Connect-vX.X.X-linux-ARCH.AppImage` ，赋予执行权限，运行即可；
  - AppImage x64 仅支持系统 `glibc >= 2.31` 的发行版，Ubuntu 22.04 及以上版本可以正常运行（受限于 GitHub Actions Runner）；
  - AppImage arm64 仅支持系统 `glibc >= 2.38` 的发行版，Ubuntu 24.04 及以上版本可以正常运行（受限于 Qt 官方：[参考](https://doc.qt.io/qt-6/supported-platforms.html)）；
  - Arch Linux 用户推荐使用 [AUR](https://aur.archlinux.org/packages/ez4connect) 安装；
  - 如果遇到因依赖问题无法运行的情况，请自行编译运行。

1. 跟随配置向导进行服务器与账户设置，或在“设置”中手动配置；

2. 在主界面中点击“连接服务器”。如果只需进行校园网页浏览，则选择“设置系统代理”后即可使用。

如果需要配合 Clash / Mihomo 进行高级的分流操作，可以参见： [高级使用方式](docs/ADVANCED_USAGE.md)

## 路线图

如有更多好的建议，可以在 Issue 中或是 OSA 群里提出！

- [X] 支持 macOS 系统
- [X] 支持 Linux 系统
- [X] 支持手动设置 Proxy Bypass
- [X] 上传 AUR 包
- [ ] 使用密钥链存储密码等信息

## 开发

项目的分层、依赖方向和新增代码归属规则见
[架构说明](docs/ARCHITECTURE.md)。

## 许可证

本项目遵循 [GNU General Public License Version 3](LICENSE) 开源。

## 致谢

- [Mythologyli/ZJU-Connect-for-Windows](https://github.com/Mythologyli/ZJU-Connect-for-Windows)
- [Mythologyli/zju-connect](https://github.com/Mythologyli/zju-connect)

> 欢迎加入 HITSZ 开源技术协会 [@hitszosa](https://github.com/hitszosa)
