# DLZ Web Build (Emscripten)

This directory contains the web build configuration for running DLZ in browsers.

## Prerequisites

1. Install Emscripten SDK:
```bash
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source emsdk_env.sh
```

2. Ensure you have the game data files in `dlzr/data/` directory.

## Building

### Quick build
```bash
cd dlzr
./build_web.sh
```

### Debug build
```bash
./build_web.sh debug
```

### Release build (optimized)
```bash
./build_web.sh release
```

## Running locally

After building, the output will be in `build_web/bin/`:
- `dlzr.html` - Main HTML file
- `dlzr.js` - JavaScript glue code
- `dlzr.wasm` - WebAssembly binary
- `dlzr.data` - Preloaded game assets

To test locally:
```bash
cd build_web/bin
python -m http.server 8000
```

Then open http://localhost:8000/dlzr.html in your browser.

**Note**: You must use a web server. Opening the HTML file directly won't work due to CORS restrictions.

## Browser Compatibility

- Chrome/Edge 90+
- Firefox 88+
- Safari 15+

Requires WebAssembly and WebGL2 support.

## Performance Notes

- Initial load: ~2-3 seconds (64MB WASM + assets)
- Runtime: 60 FPS on modern hardware
- Memory: ~128MB peak usage
- Audio: WebAudio API (may require user interaction to start)

## Deployment

For production deployment:

1. Build release version:
```bash
./build_web.sh release
```

2. Upload to web server:
```bash
build_web/bin/
├── dlzr.html
├── dlzr.js
├── dlzr.wasm
└── dlzr.data
```

3. Configure server to serve WASM with correct MIME type:
```
Content-Type: application/wasm
```

4. Enable gzip/brotli compression for `.js` and `.wasm` files.

## Troubleshooting

### "Out of memory" errors
Increase initial memory in CMakeLists.txt:
```cmake
"SHELL:-s INITIAL_MEMORY=134217728"  # 128MB
```

### Assets not loading
Ensure `data/` directory exists and contains game files before building.

### Audio not working
Browser autoplay policies require user interaction. Click/tap to start audio.

### Slow performance
- Use release build
- Disable browser extensions
- Update graphics drivers
- Try hardware-accelerated browser

## Technical Details

- **SDL3**: Built-in Emscripten support
- **Assets**: Preloaded via `--preload-file`
- **Memory**: 64MB initial, grows as needed
- **Exceptions**: Enabled (slight performance cost)
- **Threading**: Disabled (single-threaded for compatibility)

## Future Enhancements

- [ ] Service worker for offline play
- [ ] IndexedDB for save games
- [ ] Touch controls for mobile
- [ ] Progressive asset loading
- [ ] Web Workers for background tasks
