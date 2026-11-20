# 开发指南

环境要求见[快速开始](../README.zh.md#快速开始)。编辑 `.asuka` 文件前运行 `asuka guide`，遇到诊断时用 `asuka explain <code>` 查询。Asuka 按包运行测试，在仓库根目录分别运行[应用和 `converter` 包的检查与测试](../README.zh.md#开发和构建)。

## 回归测试

转换测试在代码中构造容器、音频帧和标签。按改动选择相关测试，在项目根目录运行。

| 改动 | 测试覆盖 |
| --- | --- |
| NCM 解析和解密 | [`ncm_test.asuka`](../converter/ncm_test.asuka) 覆盖 AES、填充、段截断和任意切分时的密钥流对齐。[`audio_source_test.asuka`](../converter/audio_source_test.asuka) 用独立固定密文覆盖非零起点、密钥流回绕、跨分块、短尾和越界复制。 |
| 音频校验和标签 | [`audio_validation_test.asuka`](../converter/audio_validation_test.asuka) 覆盖音频边界与截断，并通过 worker 检查 MP3 标签处置和警告、FLAC 注释与图片迁移，以及压缩帧的字节一致性。另由 [`music_metadata_test.asuka`](../converter/music_metadata_test.asuka) 覆盖歌手字段和 ID3v2.3 文本编码。 |
| 流式 I/O 和文件发布 | [`stream_io_test.asuka`](../converter/stream_io_test.asuka) 覆盖带 ID3 的 MP3 从非零音频偏移开始解密、连续完整块和末尾短块、带预留空间的跨窗口封面，以及源文件变化、目标发布失败和 FLAC 中途写入失败。失败回归检查已有导出文件和临时文件清理。 |
| 批量调度 | [`batch_export_test.asuka`](../converter/batch_export_test.asuka) 用固定完成时刻覆盖并发升降、旧档收尾、失败样本和回退后的派发限制，并通过异步调度和真实 worker 分别覆盖进度、结果顺序、单项失败和重名发布。 |
| 工作区流程 | [`app_test.asuka`](../app_test.asuka) 覆盖同名进度项、取消目录选择和单曲保存、批量结果及返回原曲目、换曲读取失败、整批失败，以及系统打开无效文件后仍可访问成功批次的导出目录。 |

在 macOS 上单独验证流式和批量导出时，可以按测试名称筛选。

```sh
asuka test -p mac --json --run 'NCM worker' converter
asuka test -p mac --json --run 'Batch export' converter
asuka test -p mac --json --run 'Batch progress'
asuka test -p mac --json --run 'Batch export UI'
```

修改界面时，可用 `asuka dev -p mac` 检查原生控件。将窗口缩到 320 点宽，分别查看导入、曲目和批量结果，确认底部按钮换行后仍可操作，长文件名和反馈可滚动查看。使用 Tab 或文件菜单检查键盘操作，并在系统浅色、深色和减少动态效果设置下检查界面。

### 样本核对

选取有权处理的 NCM 与对应的参考 FLAC，用批量导出保存到新文件名。对参考和导出文件分别运行以下命令，比较歌名、歌手、专辑，以及压缩音频和 JPEG 封面的 SHA-256。样本应带有内嵌 JPEG 封面。

```sh
file='/path/to/track.flac'
ffprobe -v error -show_entries format_tags -of json "$file"
ffmpeg -v error -i "$file" -map 0:a:0 -c:a copy -f hash -hash sha256 -
ffmpeg -v error -i "$file" -map 0:v:0 -c:v copy -f hash -hash sha256 -
```

## 属性测试与基准

属性测试覆盖 NCM 段截断和不同切分点下的解密一致性。分块解密基准测量 64 KiB 音频块。

```sh
asuka test --fuzz --fuzztime 10s converter
asuka test --bench --json --run '64 KiB' converter
```

### 批量导出实测

以下结果来自 Apple M5 Pro、内置 SSD 和 Asuka 0.2.11。测试用同一份约 43 MiB、内含 FLAC 的 NCM 样本制作多个不同文件名的输入，通过临时命令行入口调用 `exportNcmBatch`，交替运行各版本并取三轮中位数。每轮写入一个新的本地目录。

| 对照 | 条件 | 批量耗时中位数 |
| --- | --- | --- |
| 解密缓冲区复用前 → 后 | 12 个热缓存样本，固定 2 并发 | 3.59 秒 → 2.92 秒，耗时减少约 19% |
| 固定并发与动态并发 | 32 个热缓存样本，均启用缓冲区复用 | 固定 2 并发 7.75 秒，动态 2–4 并发 4.91 秒，固定 4 并发 4.02 秒 |

临时微基准对照了每块重新分配的旧实现和预热后的复用缓冲区。单个完整 64 KiB 块从 17 次分配、累计约 1.06 MiB 降到 1 次分配、约 64 KiB。

32 文件批次的动态并发在三轮热缓存测试中都升至 4，耗时比固定 2 并发低约 37%。未主动预热输入的单轮对照为 5.20 秒和 7.86 秒，6 文件批次保持 2 并发。外置盘尚待验证。样本导出的歌曲标签、JPEG 封面和压缩音频均与参考 FLAC 核对一致。
