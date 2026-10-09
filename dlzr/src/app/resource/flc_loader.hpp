#pragma once

// flc_loader.hpp — FLC 动画加载器
// FLC animation loader
//
// 解码 Autodesk FLC 格式动画文件（magic 0xAF12）。
// Decodes Autodesk FLC format animation files.

#include <cstdint>
#include <string>
#include <vector>

namespace dlzr::core {

struct FlcFrame {
    std::vector<uint8_t> pixels;  // 640×480 像素数据 (8-bit palette index)
    std::vector<uint8_t> palette; // 256 × 3 bytes (RGB)
};

struct DecodedFlc {
    int32_t width{0};
    int32_t height{0};
    int32_t frame_count{0};
    int32_t speed{0};  // 帧间隔（毫秒）/ Frame interval in milliseconds
    std::vector<FlcFrame> frames;
};

class FlcLoader {
public:
    // 从文件加载 FLC / Load FLC from file
    static DecodedFlc load_from_file(const std::string& path);

    // 从内存加载 FLC / Load FLC from memory
    static DecodedFlc load_from_memory(const uint8_t* data, size_t size);

private:
    static void decode_frame(const uint8_t* frame_data, size_t frame_size,
                             FlcFrame& frame, int32_t width, int32_t height);
};

}  // namespace dlzr::core
