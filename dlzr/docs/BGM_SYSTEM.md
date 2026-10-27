# BGM System / 背景音乐系统

## Overview / 概述

The remake supports background music playback via SDL3_mixer. The original game used CD audio for BGM, which is not available in the DAT files.

复刻版通过 SDL3_mixer 支持背景音乐播放。原版游戏使用 CD 音频作为背景音乐，这些内容不在 DAT 文件中。

## How to Add Music / 如何添加音乐

Create a `music/` subdirectory in your game data folder and add music files:

在游戏数据文件夹中创建 `music/` 子目录并添加音乐文件：

```
DLZFULL/
├── FACE.DAT
├── FIGHT.DAT
├── GAME.DAT
├── ...
└── music/
    ├── track0.mp3  (or .ogg) - Title screen / 标题画面
    ├── track1.mp3  (or .ogg) - Strategic map / 战略地图
    ├── track2.mp3  (or .ogg) - Town / 城镇
    ├── track3.mp3  (or .ogg) - Battle / 战斗
    └── ...
```

## Supported Formats / 支持的格式

- MP3 (MPEG Audio Layer 3)
- OGG (Ogg Vorbis)
- WAV (uncompressed)
- FLAC (if enabled in SDL3_mixer build)

## Track Mapping / 音轨映射

The game calls `audio.play_bgm(track_id)` with these track IDs:

游戏调用 `audio.play_bgm(track_id)`，音轨 ID 如下：

- Track 0: Title screen / 标题画面
- Track 1: Strategic map / 战略地图  
- Track 2: Town / 城镇
- Track 3: Battle (various) / 战斗（多种）

You can rip the original CD audio tracks or use any music files you prefer.

你可以从原版 CD 提取音轨，或使用你喜欢的任何音乐文件。

## Implementation Notes / 实现说明

- SDL3_mixer is fetched automatically via CMake FetchContent on first build
- Music files are loaded on-demand and streamed (not fully loaded into memory)
- Looping is enabled by default for all BGM tracks
- Volume can be adjusted via `AudioManager::set_music_volume(0-128)`

- SDL3_mixer 在首次构建时通过 CMake FetchContent 自动获取
- 音乐文件按需加载并流式播放（不完全加载到内存）
- 所有背景音乐默认启用循环
- 可通过 `AudioManager::set_music_volume(0-128)` 调整音量
