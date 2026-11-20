# nnam

[English](README.md) · 简体中文

用 [Asuka](https://asuka.egoist.dev/zh/docs.md) 编写的网易云音乐 NCM 桌面导出工具。nnam 在本地解密 `.ncm` 文件，保留其中的 FLAC 或 MP3 压缩音频帧，无需重新编码。

nnam 是独立的第三方工具，与网易云音乐无关联。请尊重音乐及封面版权，仅处理你依法有权解密和导出的本地文件，并遵守适用法律及相关服务条款。请勿未经授权复制、分享或分发导出内容。使用者应自行确认所需权限，并对解密、导出及后续使用行为负责。

## 功能

- 预览歌名、歌手、专辑和音频格式，导出时迁移可用标签及内嵌的 JPEG 或 PNG 封面。
- 支持文件选择、Finder 拖放和「打开方式」。
- 批量导出显示实时进度，保留目标目录中的已有文件，并逐项报告成功、失败和标签警告。

## 快速开始

在 macOS Apple Silicon 上从源码运行需要 Asuka 0.2.11 和 Xcode 命令行工具。可用 `xcode-select --install` 安装命令行工具。下载或克隆仓库后，在项目根目录运行以下命令。

```sh
curl -fsSL https://asuka.egoist.dev/install.sh | ASUKA_VERSION=0.2.11 sh
asuka dev
```

## 使用方法

- **单文件。** 点击 **选择 NCM 文件…**（⌘O）。确认曲目信息后，点击 **导出 FLAC…** 或 **导出 MP3…**（⌘S）并选择保存位置。导出后可以在 Finder 中显示文件。
- **批量。** 点击 **批量导出…**（⌘B）或拖入多个 `.ncm` 文件，再选择输出目录。「正在批量导出」显示已处理数、成功与失败数，以及当前处理的文件。失败也计入已处理数，带标签警告的导出计为成功。完成后按选取顺序显示结果，重名时会追加 ` (2)`、` (3)` 等后缀。

支持 NCM 中的 FLAC 和标准码率的 CBR、VBR MP3。无法识别格式或确认音频边界时会显示错误。歌曲信息缺失或无法读取时，只要音频通过校验，仍可导出音频和可用封面。详见[格式校验与标签写入](docs/architecture.md#格式校验与标签写入)。

## 开发和构建

```sh
asuka check --json
asuka test --json
asuka test --json converter
asuka build -p mac
```

macOS Apple Silicon 构建输出到 `dist/darwin-arm64/`。模块职责、回归测试和性能基准见[开发文档](docs/README.md)。

## 贡献

欢迎提交 Issue 或 Pull Request。报告问题时请附上 macOS 和 Asuka 版本、复现步骤及完整错误信息。请勿在 Issue 或 Pull Request 中上传无权分发的音频或封面。提交代码前请运行与改动相关的[回归测试](docs/development.md#回归测试)。

## 许可证

本项目采用 [MIT 许可证](LICENSE)。
