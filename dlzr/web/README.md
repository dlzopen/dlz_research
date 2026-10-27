# Web Deployment Guide / Web 部署指南

## Building for Web / 构建 Web 版本

### Prerequisites / 前提条件

1. **Emscripten SDK** - Install from https://emscripten.org/docs/getting_started/downloads.html
2. **Activate emsdk environment**:
   ```bash
   source /path/to/emsdk/emsdk_env.sh
   ```

### Build Steps / 构建步骤

```bash
# From dlzr/ directory
# Configure for Emscripten
emcmake cmake -B build-web -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build-web --config Release

# Output files:
# - build-web/bin/dlzr.html
# - build-web/bin/dlzr.js
# - build-web/bin/dlzr.wasm
# - build-web/bin/dlzr.data (if data/ directory exists)
```

## Custom HTML Shell / 自定义 HTML 模板

The web build uses a custom HTML shell (`web/shell.html`) that provides:

- **Loading screen** with progress bar
- **Responsive canvas** that maintains 640x480 aspect ratio
- **Bilingual UI** (Chinese + English)
- **Touch support** for mobile devices
- **Error handling** with user-friendly messages

Web 构建使用自定义 HTML 模板（`web/shell.html`），提供：

- **加载画面**带进度条
- **响应式画布**保持 640x480 宽高比
- **双语界面**（中文 + 英文）
- **触摸支持**适用于移动设备
- **错误处理**显示用户友好的消息

## Game Data for Web / Web 版游戏数据

For web deployment, game data must be preloaded:

对于 Web 部署，必须预加载游戏数据：

```bash
# Create data directory with required files
mkdir -p data
cp /path/to/DLZFULL/GAME.DAT data/
cp /path/to/DLZFULL/FIGHT.DAT data/
cp /path/to/DLZFULL/FACE.DAT data/
cp /path/to/DLZFULL/TOWN.DAT data/
cp /path/to/DLZFULL/WAVE.DAT data/
cp /path/to/DLZFULL/TXT.DAT data/
cp /path/to/DLZFULL/HZK16 data/
cp /path/to/DLZFULL/ASC16 data/
# ... copy other required files

# Rebuild to include data in .data file
cmake --build build-web --config Release
```

The CMake configuration will automatically package the `data/` directory into `dlzr.data` which is loaded at startup.

CMake 配置会自动将 `data/` 目录打包到 `dlzr.data` 文件中，在启动时加载。

## Deployment / 部署

### Option 1: Static Hosting / 静态托管

Upload these files to any static web host:

上传这些文件到任何静态网页托管：

- `dlzr.html` (rename to `index.html`)
- `dlzr.js`
- `dlzr.wasm`
- `dlzr.data`

**Required HTTP headers / 必需的 HTTP 头**:

```
Cross-Origin-Opener-Policy: same-origin
Cross-Origin-Embedder-Policy: require-corp
```

Example nginx config:

```nginx
location / {
    add_header Cross-Origin-Opener-Policy same-origin;
    add_header Cross-Origin-Embedder-Policy require-corp;
}
```

### Option 2: Local Testing / 本地测试

Use Python's HTTP server:

```bash
cd build-web/bin
python -m http.server 8000
# Open http://localhost:8000/dlzr.html
```

Note: Some browsers require the COOP/COEP headers even for local testing. Use Chrome with flags:

```bash
chrome --enable-features=SharedArrayBuffer
```

## Mobile Support / 移动设备支持

The web version includes touch controls:

- **Tap**: Select/confirm
- **Long press**: Context menu (where applicable)
- **Pinch**: Zoom (if implemented)

Web 版本包含触摸控制：

- **点击**：选择/确认
- **长按**：上下文菜单（如适用）
- **捏合**：缩放（如已实现）

## Performance Notes / 性能说明

- **Initial load**: ~50-100MB (WASM + data files)
- **Memory**: Requires ~200MB RAM
- **Recommended**: Modern browsers (Chrome 90+, Firefox 88+, Safari 15+)

- **初始加载**：约 50-100MB（WASM + 数据文件）
- **内存**：需要约 200MB RAM
- **推荐**：现代浏览器（Chrome 90+、Firefox 88+、Safari 15+）

## Troubleshooting / 故障排除

### "SharedArrayBuffer is not defined"
Add COOP/COEP headers to your web server (see Deployment section).

### "Out of memory"
Increase `INITIAL_MEMORY` in CMakeLists.txt or reduce data file size.

### Slow loading
Use a CDN or enable gzip compression on your web server for .js and .wasm files.

### "SharedArrayBuffer is not defined"
添加 COOP/COEP 头到你的网页服务器（见部署部分）。

### "内存不足"
在 CMakeLists.txt 中增加 `INITIAL_MEMORY` 或减小数据文件大小。

### 加载缓慢
使用 CDN 或在网页服务器上为 .js 和 .wasm 文件启用 gzip 压缩。
