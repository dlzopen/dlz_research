# Cutscene System / 过场动画系统

## Current Implementation / 当前实现

The remake uses text-based cutscenes rendered with the font system. The original game (1998) used MPEG-1 video files stored on the CD-ROM for cutscenes.

复刻版使用字体系统渲染基于文本的过场动画。原版游戏（1998年）使用存储在 CD-ROM 上的 MPEG-1 视频文件作为过场动画。

## Original Game Data / 原版游戏数据

The original game's MPEG files were **4-byte stubs** in the game directory that pointed to actual video content on the CD-ROM. These stubs contain no video data themselves.

原版游戏的 MPEG 文件是游戏目录中的 **4字节桩**，指向 CD-ROM 上的实际视频内容。这些桩文件本身不包含视频数据。

## Video Playback Support / 视频播放支持

Full MPEG video playback is not currently implemented due to:

1. **Heavy dependency**: Requires FFmpeg/libavcodec (~10MB+ binary size)
2. **Original data unavailable**: MPEG files were CD-ROM stubs, not in DAT files
3. **Complexity**: Video decoding, audio sync, and frame rendering add significant complexity

当前未实现完整的 MPEG 视频播放，原因如下：

1. **重量级依赖**：需要 FFmpeg/libavcodec（二进制大小约 10MB+）
2. **原始数据不可用**：MPEG 文件是 CD-ROM 桩，不在 DAT 文件中
3. **复杂性**：视频解码、音频同步和帧渲染增加了显著的复杂性

## Future Enhancement / 未来增强

To add video cutscene support:

1. Add FFmpeg as an optional dependency (CMake option `DLZR_USE_FFMPEG`)
2. Create `VideoPlayer` class in `presentation/video/`
3. Support loading external MP4/WEBM files from `cutscenes/` directory
4. Implement frame-by-frame decoding and rendering
5. Synchronize audio track with video frames

要添加视频过场动画支持：

1. 将 FFmpeg 添加为可选依赖（CMake 选项 `DLZR_USE_FFMPEG`）
2. 在 `presentation/video/` 中创建 `VideoPlayer` 类
3. 支持从 `cutscenes/` 目录加载外部 MP4/WEBM 文件
4. 实现逐帧解码和渲染
5. 同步音频轨道与视频帧

Example structure:

```
DLZFULL/
├── ...
└── cutscenes/
    ├── opening.mp4      - Game opening / 游戏开场
    ├── ending00.mp4     - Perfect ending / 完美结局
    ├── ending01.mp4     - 柳忻 ending / 柳忻 结局
    └── ...
```

## Current Alternative / 当前替代方案

The text-based opening sequence provides narrative context using GBK-encoded Chinese text, displayed scene-by-scene with auto-advance or manual progression. This is lightweight, requires no external dependencies, and works on all platforms including web (Emscripten).

基于文本的开场序列使用 GBK 编码的中文文本提供叙事背景，逐场景显示，支持自动推进或手动操作。这是轻量级的，不需要外部依赖，并且可以在所有平台上工作，包括 Web（Emscripten）。
