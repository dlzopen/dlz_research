#pragma once

// anm_loader.hpp — ANM/LPF RLE 动画加载器
// ANM/LPF RLE animation loader
//
// 格式 / Format:
//   LPF container → ANIM header → Palette(0x100) → Pages(0xB00+)
//   帧尺寸固定 320×200, 8-bit 调色板索引
//   Frame size fixed 320×200, 8-bit palette indices
//
// RLE 操作码 / RLE Opcodes:
//   0x01-0x7F: Copy N pixels from stream
//   0x81-0xFF: Skip N pixels (delta: leave unchanged from prev frame)
//   0x00 CC PP: Fill CC pixels with value PP
//   0x80 + uint16 LE: Extended opcode (upper 2 bits = sub_type, lower 14 = count)

#include "core/data_model/types.hpp"
#include "core/data_model/constants.hpp"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace dlzr::app {

class AnmLoader {
public:
    // 从文件路径加载 ANM 动画
    // Load ANM animation from file path
    static core::DecodedAnimation load(const std::filesystem::path& path);

    // 从内存缓冲区加载 ANM 动画
    // Load ANM animation from memory buffer
    static core::DecodedAnimation load(const uint8_t* data, size_t size);

    // 将单帧调色板索引转换为 RGBA8888
    // Convert single frame palette indices to RGBA8888
    static std::vector<uint8_t> frame_to_rgba(
        const core::DecodedAnimation& anim, size_t frame_idx);

private:
    static core::Palette read_palette_anm(const uint8_t* pal_data);
    static void decode_record(const uint8_t* rec_data, size_t rec_size,
                              uint8_t* frame_buffer);
};

}  // namespace dlzr::app
