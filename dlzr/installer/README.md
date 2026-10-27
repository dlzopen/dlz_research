# Windows Installer Build Guide / Windows 安装程序构建指南

## Prerequisites / 前提条件

1. **Inno Setup 6.x** - Download from https://jrsoftware.org/isinfo.php
2. **dlzr built** - Run `cmake --build build --config Release` first

## Building the Installer / 构建安装程序

### Method 1: Command Line / 命令行方式

```bash
# From dlzr/ directory
iscc installer\dlzr_installer.iss
```

The installer will be created in `installer\installer_output\dlzr_setup_1.0.0.exe`

### Method 2: Inno Setup Compiler GUI / 图形界面方式

1. Open `installer\dlzr_installer.iss` in Inno Setup Compiler
2. Click Build → Compile (or press F9)
3. Installer will be created in `installer\installer_output\`

## Installer Features / 安装程序特性

- **Bilingual**: Chinese Simplified + English
- **Checks for game data**: Warns if GAME.DAT is missing
- **Creates shortcuts**: Start menu + optional desktop icon
- **Post-install instructions**: Tells user where to copy game data
- **Supports background music**: Copies music/ directory if present

## Game Data Files / 游戏数据文件

The installer does NOT include original game data. Users must copy these files manually:

安装程序不包含原版游戏数据。用户必须手动复制以下文件：

### Required / 必需
- GAME.DAT
- FIGHT.DAT  
- FACE.DAT
- TOWN.DAT
- WAVE.DAT
- TXT.DAT
- HZK16, ASC16 (font files)

### Optional / 可选
- `music/` directory with track0.mp3, track1.mp3, etc.

## Customization / 自定义

Edit `installer\dlzr_installer.iss` to change:

- `#define MyAppVersion` - Version number
- `OutputBaseFilename` - Installer filename
- `SetupIconFile` - Installer icon (currently empty)
- Language files - Add/remove languages

## Notes / 注意事项

1. Build in **Release** mode for distribution:
   ```bash
   cmake --build build --config Release
   ```

2. Update the script to use `Release` paths instead of `Debug`

3. The installer uses LZMA2 ultra compression for smallest size

4. For portable distribution, just zip the `build\bin\Release\` directory + game data
