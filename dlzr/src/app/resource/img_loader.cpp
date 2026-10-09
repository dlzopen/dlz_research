// img_loader.cpp — IMG 格式解码实现
// IMG format decode implementation
//
// 格式布局 / Layout:
//   Offset 0x00: magic(4) + pad(2) + width_m1(2) + height_m1(2) + pad(2) + pad(4) = 16B header
//   Offset 0x10: pixel data (width × height bytes, palette indices)
//   After pixels: 768 bytes palette (256 × RGB, 6-bit per component)
//
// 调色板转换 / Palette conversion:
//   6-bit component × 4 → 8-bit component (range 0-252)

#include "app/resource/img_loader.hpp"

#include "core/data_model/types.hpp"
#include "core/data_model/constants.hpp"

#include <cstring>
#include <fstream>
#include <stdexcept>

namespace dlzr::app {

core::Palette ImgLoader::read_palette(const uint8_t* pal_data) {
    core::Palette pal;
    for (int i = 0; i < 256; ++i) {
        // IMG palette: 3 bytes per entry, RGB order, 6-bit per component
        uint8_t r6 = pal_data[i * 3 + 0];
        uint8_t g6 = pal_data[i * 3 + 1];
        uint8_t b6 = pal_data[i * 3 + 2];
        pal.colors[i][0] = static_cast<uint8_t>(r6 * core::IMG_PALETTE_COMPONENT_SCALE);
        pal.colors[i][1] = static_cast<uint8_t>(g6 * core::IMG_PALETTE_COMPONENT_SCALE);
        pal.colors[i][2] = static_cast<uint8_t>(b6 * core::IMG_PALETTE_COMPONENT_SCALE);
    }
    return pal;
}

core::DecodedImage ImgLoader::load(const uint8_t* data, size_t size) {
    if (size < core::IMG_HEADER_SIZE) {
        throw std::runtime_error("IMG data too small for header");
    }

    // Parse header
    // Offsets: width_m1 at +0x06, height_m1 at +0x08
    uint16_t width_m1, height_m1;
    std::memcpy(&width_m1, data + 0x06, 2);
    std::memcpy(&height_m1, data + 0x08, 2);

    uint16_t width = width_m1 + 1;
    uint16_t height = height_m1 + 1;
    size_t pixel_count = static_cast<size_t>(width) * height;
    size_t pixel_offset = core::IMG_HEADER_SIZE;
    size_t palette_offset = pixel_offset + pixel_count;

    if (palette_offset + core::IMG_PALETTE_SIZE > size) {
        throw std::runtime_error("IMG data too small for pixel + palette data");
    }

    core::DecodedImage img;
    img.width = width;
    img.height = height;
    img.pixels.assign(data + pixel_offset,
                      data + pixel_offset + pixel_count);
    img.palette = read_palette(data + palette_offset);

    return img;
}

core::DecodedImage ImgLoader::load(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open IMG: " + path.string());
    }

    auto file_size = file.tellg();
    std::vector<uint8_t> buffer(static_cast<size_t>(file_size));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()),
              static_cast<std::streamsize>(file_size));

    return load(buffer.data(), buffer.size());
}

std::vector<uint8_t> ImgLoader::to_rgba(const core::DecodedImage& img) {
    size_t pixel_count = static_cast<size_t>(img.width) * img.height;
    std::vector<uint8_t> rgba(pixel_count * 4);

    for (size_t i = 0; i < pixel_count; ++i) {
        uint8_t idx = img.pixels[i];
        rgba[i * 4 + 0] = img.palette.colors[idx][0];  // R
        rgba[i * 4 + 1] = img.palette.colors[idx][1];  // G
        rgba[i * 4 + 2] = img.palette.colors[idx][2];  // B
        rgba[i * 4 + 3] = 255;                          // A
    }

    return rgba;
}

}  // namespace dlzr::app
