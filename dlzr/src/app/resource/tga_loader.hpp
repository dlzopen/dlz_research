// tga_loader.hpp — TGA 8-bit indexed image loader
// Standard TGA type 1 (uncompressed color-mapped)
//
// Format:
//   [Header: 18B][Palette: 768B (256×24-bit RGB)][Pixels: W×H bottom-to-top]

#pragma once

#include "core/data_model/types.hpp"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace dlzr::app {

class TgaLoader {
public:
    static core::DecodedImage load(const std::filesystem::path& path);
    static core::DecodedImage load(const uint8_t* data, size_t size);
    static std::vector<uint8_t> to_rgba(const core::DecodedImage& img);

private:
    static core::Palette read_palette(const uint8_t* pal_data, int count);
};

}  // namespace dlzr::app
