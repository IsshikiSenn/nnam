# nnam

English · [简体中文](README.zh.md)

nnam is a desktop tool for exporting NetEase Cloud Music NCM files, built with [Asuka](https://github.com/egoist/asuka). It decrypts `.ncm` files locally and exports their original FLAC or MP3 audio without re-encoding.

nnam is an independent third-party tool with no affiliation with NetEase Cloud Music. Please respect the copyright of music and cover art. Only process local files that you are legally permitted to decrypt and export, and follow applicable laws and terms of service. Copying, sharing, or distributing exported content requires authorization. You are responsible for confirming that you have the necessary permissions and for how you decrypt, export, and use the resulting files.

## Features

- Preview the track title, artist, album, and audio format. Export available tags and embedded JPEG or PNG cover art.
- Open files with the file picker, by dragging them from Finder, or with Finder’s **Open With** menu.
- Track batch exports in real time, preserve existing files in the destination, and see which files succeeded, failed, or had tag warnings.

## Quick start

Running from source on macOS with Apple silicon requires Asuka 0.2.11 and the Xcode Command Line Tools. Install the command-line tools with `xcode-select --install`. Download or clone this repository, then run the following commands from its root.

```sh
curl -fsSL https://asuka.egoist.dev/install.sh | ASUKA_VERSION=0.2.11 sh
asuka dev
```

## Usage

- **Single file.** Click **选择 NCM 文件…** (Open NCM File, ⌘O). Review the track details, then click **导出 FLAC…** or **导出 MP3…** (Export FLAC or MP3, ⌘S) and choose where to save the file. You can reveal the exported file in Finder.
- **Batch export.** Click **批量导出…** (Batch Export, ⌘B) or drag in multiple `.ncm` files, then choose a destination folder. The progress panel shows the processed, successful, and failed counts, along with the files currently being processed. Failed files count toward the processed total, and exports with tag warnings count as successful. Results appear in selection order. If a filename is already taken, nnam tries suffixes such as ` (2)` and ` (3)`.

nnam supports FLAC and CBR or VBR MP3 with standard MPEG bitrates inside NCM files. It reports an error if it cannot identify the format or confirm the audio boundaries. If track metadata is missing or unreadable, nnam can still export valid audio and any available cover art. See [format validation and tag writing](docs/architecture.md#格式校验与标签写入) (Chinese).

## Development and build

```sh
asuka check --json
asuka test --json
asuka test --json converter
asuka build -p mac
```

macOS builds for Apple silicon are written to `dist/darwin-arm64/`. See the [developer documentation](docs/README.md) (Chinese) for module responsibilities, regression tests, and benchmarks.

## Contributing

Issues and pull requests are welcome. Bug reports should include your macOS and Asuka versions, steps to reproduce, and the full error message. Please only attach audio or cover art that you have permission to distribute. Before opening a pull request, run the relevant [regression tests](docs/development.md#回归测试) (Chinese).

## License

This project is licensed under the [MIT License](LICENSE).
