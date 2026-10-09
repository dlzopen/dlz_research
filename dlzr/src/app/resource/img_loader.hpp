#pragma once

// img_loader.hpp — IMG 8-bit 调色板索引图像加载器
// IMG 8-bit palette-indexed image loader
//
// 格式 / Format:
//   [Header: 16B][Pixels: W×H bytes][Palette: 768B]
//   Header: width_m1(2), height_m1(2) at offsets 0x06, 0x08
//   Palette: 256 × 3 bytes RGB, 6-bit per component (×4 → 8-bit)

#include "core/data_model/types.hpp"
#include "core/data_model/constants.hpp"

#include <cstdint>
#include <filesystem>
#include <string>

namespace dlzr::app {

class ImgLoader {
public:
    // 从文件路径加载 IMG
    // Load IMG from file path
    static core::DecodedImage load(const std::filesystem::path& path);

    // 从内存缓冲区加载 IMG
    // Load IMG from memory buffer
    static core::DecodedImage load(const uint8_t* data, size_t size);

    // 将调色板索引图像转换为 RGBA8888
    // Convert palette-indexed image to RGBA8888
    static std::vector<uint8_t> to_rgba(const core::DecodedImage& img);

private:
    static core::Palette read_palette(const uint8_t* pal_data);
};

}  // namespace dlzr::app
