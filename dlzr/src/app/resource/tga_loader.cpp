// tga_loader.cpp — TGA 8-bit indexed image loader
// Standard TGA type 1: 18-byte header, 256-entry 24-bit palette, bottom-to-top pixels.

#include "app/resource/tga_loader.hpp"

#include <cstring>
#include <fstream>
#include <stdexcept>

namespace dlzr::app {

core::Palette TgaLoader::read_palette(const uint8_t* pal_data, int count) {
    core::Palette pal;
    for (int i = 0; i < count && i < 256; ++i) {
        // TGA palette: BGR order, 8-bit per component
        uint8_t b = pal_data[i * 3 + 0];
        uint8_t g = pal_data[i * 3 + 1];
        uint8_t r = pal_data[i * 3 + 2];
        pal.colors[i][0] = r;
        pal.colors[i][1] = g;
        pal.colors[i][2] = b;
    }
    return pal;
}

core::DecodedImage TgaLoader::load(const uint8_t* data, size_t size) {
    if (size < 18) {
        throw std::runtime_error("TGA data too small for header");
    }

    uint8_t id_len = data[0];
    uint8_t cmap_type = data[1];
    uint8_t img_type = data[2];

    if (cmap_type != 1 || img_type != 1) {
        throw std::runtime_error("TGA: only type 1 (indexed) supported");
    }

    uint16_t cm_length = static_cast<uint16_t>(data[5] | (data[6] << 8));
    uint8_t cm_bits = data[7];
    uint16_t width = static_cast<uint16_t>(data[12] | (data[13] << 8));
    uint16_t height = static_cast<uint16_t>(data[14] | (data[15] << 8));
    uint8_t descriptor = data[17];

    bool top_down = (descriptor & 0x20) != 0;

    size_t offset = 18 + id_len;

    core::DecodedImage img;
    img.width = width;
    img.height = height;

    // Read palette
    if (cm_length > 0 && cm_bits == 24) {
        size_t pal_size = static_cast<size_t>(cm_length) * 3;
        if (offset + pal_size > size) {
            throw std::runtime_error("TGA: palette exceeds file size");
        }
        img.palette = read_palette(data + offset, cm_length);
        offset += pal_size;
    }

    // Read pixel data
    size_t pixel_count = static_cast<size_t>(width) * height;
    if (offset + pixel_count > size) {
        throw std::runtime_error("TGA: pixel data exceeds file size");
    }

    img.pixels.resize(pixel_count);

    if (top_down) {
        std::memcpy(img.pixels.data(), data + offset, pixel_count);
    } else {
        // TGA stores bottom-to-top, flip to top-to-bottom
        for (uint16_t y = 0; y < height; ++y) {
            std::memcpy(
                img.pixels.data() + static_cast<size_t>(y) * width,
                data + offset + static_cast<size_t>(height - 1 - y) * width,
                width);
        }
    }

    return img;
}

core::DecodedImage TgaLoader::load(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open TGA: " + path.string());
    }

    auto file_size = file.tellg();
    std::vector<uint8_t> buffer(static_cast<size_t>(file_size));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()),
              static_cast<std::streamsize>(file_size));

    return load(buffer.data(), buffer.size());
}

std::vector<uint8_t> TgaLoader::to_rgba(const core::DecodedImage& img) {
    size_t pixel_count = static_cast<size_t>(img.width) * img.height;
    std::vector<uint8_t> rgba(pixel_count * 4);

    for (size_t i = 0; i < pixel_count; ++i) {
        uint8_t idx = img.pixels[i];
        rgba[i * 4 + 0] = img.palette.colors[idx][0];
        rgba[i * 4 + 1] = img.palette.colors[idx][1];
        rgba[i * 4 + 2] = img.palette.colors[idx][2];
        rgba[i * 4 + 3] = 255;
    }

    return rgba;
}

}  // namespace dlzr::app
