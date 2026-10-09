// anm_loader.cpp — ANM/LPF RLE 动画解码实现
// ANM/LPF RLE animation decode implementation
//
// 参考 / Reference: scripts/export_anm.py, FFmpeg libavcodec/anm.c
//
// 文件布局 / File layout:
//   0x000: LPF header (variable)
//   0x100: Palette (256 × 4 bytes, BGR0, 6-bit per component)
//   0xB00: Page data (each page 0x10000 bytes apart)
//
// Delta 编码 / Delta encoding:
//   每帧基于前帧差异编码。Skip 操作保留前帧像素不变。
//   Each frame encoded as delta from previous. Skip ops leave prev pixels.

#include "app/resource/anm_loader.hpp"

#include "core/data_model/constants.hpp"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace dlzr::app {

core::Palette AnmLoader::read_palette_anm(const uint8_t* pal_data) {
    core::Palette pal;
    for (int i = 0; i < 256; ++i) {
        // ANM palette: 4 bytes per entry, BGR0 order, 6-bit per component
        uint8_t b6 = pal_data[i * 4 + 0];
        uint8_t g6 = pal_data[i * 4 + 1];
        uint8_t r6 = pal_data[i * 4 + 2];
        // pal_data[i * 4 + 3] is 0x00 (unused)
        pal.colors[i][0] = static_cast<uint8_t>(r6 * core::IMG_PALETTE_COMPONENT_SCALE);
        pal.colors[i][1] = static_cast<uint8_t>(g6 * core::IMG_PALETTE_COMPONENT_SCALE);
        pal.colors[i][2] = static_cast<uint8_t>(b6 * core::IMG_PALETTE_COMPONENT_SCALE);
    }
    return pal;
}

void AnmLoader::decode_record(const uint8_t* rec_data, size_t rec_size,
                               uint8_t* frame_buffer) {
    // RLE 解码器 / RLE decoder
    // 参考 export_anm.py _decode_anm_record()
    size_t pos = 0;
    int dst = 0;
    const int frame_size = core::ANM_FRAME_SIZE;

    while (pos < rec_size) {
        uint8_t type_byte = rec_data[pos++];
        int count = type_byte & 0x7F;
        int op_type = type_byte >> 7;  // 0 = normal, 1 = skip/extended

        if (count > 0) {
            if (op_type == 0) {
                // 0x01-0x7F: Copy count pixels from stream
                for (int i = 0; i < count && pos < rec_size; ++i, ++pos) {
                    if (dst < frame_size) {
                        frame_buffer[dst] = rec_data[pos];
                    }
                    ++dst;
                }
            } else {
                // 0x81-0xFF: Skip count pixels (delta: leave unchanged)
                dst += count;
            }
        } else {
            if (op_type == 0) {
                // 0x00 CC PP: Fill CC pixels with value PP
                if (pos + 1 >= rec_size) {
                    break;
                }
                count = rec_data[pos++];
                uint8_t pixel = rec_data[pos++];
                for (int i = 0; i < count; ++i) {
                    if (dst < frame_size) {
                        frame_buffer[dst] = pixel;
                    }
                    ++dst;
                }
            } else {
                // 0x80 + uint16 LE: Extended opcode
                if (pos + 1 >= rec_size) {
                    break;
                }
                uint16_t val;
                std::memcpy(&val, rec_data + pos, 2);
                pos += 2;

                count = val & 0x3FFF;
                int sub_type = val >> 14;

                if (count == 0) {
                    if (sub_type == 0) {
                        break;  // STOP
                    } else if (sub_type == 2) {
                        break;  // Break out of decode loop
                    } else {
                        continue;
                    }
                }

                uint8_t pixel = 0;
                if (sub_type == 3) {
                    if (pos >= rec_size) {
                        break;
                    }
                    pixel = rec_data[pos++];
                }

                if (sub_type == 1) {
                    count += 0x4000;
                }

                switch (sub_type) {
                    case 0:  // Extended skip
                    case 1:  // Large skip
                        dst += count;
                        break;
                    case 2:  // Extended copy
                        for (int i = 0; i < count && pos < rec_size; ++i, ++pos) {
                            if (dst < frame_size) {
                                frame_buffer[dst] = rec_data[pos];
                            }
                            ++dst;
                        }
                        break;
                    case 3:  // Extended fill
                        for (int i = 0; i < count; ++i) {
                            if (dst < frame_size) {
                                frame_buffer[dst] = pixel;
                            }
                            ++dst;
                        }
                        break;
                }
            }
        }
    }
}

core::DecodedAnimation AnmLoader::load(const uint8_t* data, size_t size) {
    if (size < 0x100) {
        throw std::runtime_error("ANM data too small");
    }

    // Verify LPF magic
    uint32_t magic;
    std::memcpy(&magic, data, 4);
    if (magic != 0x2046504C) {  // "LPF "
        throw std::runtime_error("Invalid LPF magic");
    }

    // Parse LPF header
    uint16_t pages_num;
    std::memcpy(&pages_num, data + 0x06, 2);

    // Read palette from offset 0x100 (256 × 4 bytes BGR0)
    core::DecodedAnimation anim;
    anim.width = core::ANM_WIDTH;
    anim.height = core::ANM_HEIGHT;
    anim.palette = read_palette_anm(data + core::ANM_PALETTE_OFFSET);

    // Collect all records from all pages
    struct Record {
        int global_index;
        std::vector<uint8_t> data;
    };
    std::vector<Record> all_records;

    for (int pg = 0; pg < pages_num; ++pg) {
        size_t page_offset = core::ANM_PAGE_DATA_OFFSET
                           + static_cast<size_t>(pg) * core::ANM_PAGE_SIZE;

        if (page_offset + 8 > size) {
            break;
        }

        uint16_t base_rec, nb_recs, page_data_size, pad;
        std::memcpy(&base_rec, data + page_offset, 2);
        std::memcpy(&nb_recs, data + page_offset + 2, 2);
        std::memcpy(&page_data_size, data + page_offset + 4, 2);
        std::memcpy(&pad, data + page_offset + 6, 2);

        if (page_data_size == 0) {
            continue;
        }

        if (nb_recs > 0) {
            // Read record sizes
            std::vector<uint16_t> rec_sizes(nb_recs);
            size_t sizes_offset = page_offset + 8;
            for (int i = 0; i < nb_recs; ++i) {
                if (sizes_offset + 2 > size) {
                    break;
                }
                std::memcpy(&rec_sizes[i], data + sizes_offset, 2);
                sizes_offset += 2;
            }

            // Read each record
            size_t data_offset = page_offset + 8 + nb_recs * 2;
            for (int i = 0; i < nb_recs; ++i) {
                if (data_offset + rec_sizes[i] > size) {
                    break;
                }
                const uint8_t* rec_start = data + data_offset;
                size_t rec_size = rec_sizes[i];

                // Skip 4-byte record header (42 00 01 00) if present
                if (rec_size >= 4
                    && rec_start[0] == 0x42 && rec_start[1] == 0x00
                    && rec_start[2] == 0x01 && rec_start[3] == 0x00) {
                    rec_start += 4;
                    rec_size -= 4;
                }

                Record rec;
                rec.global_index = base_rec + i;
                rec.data.assign(rec_start, rec_start + rec_size);
                all_records.push_back(std::move(rec));

                data_offset += rec_sizes[i];
            }
        } else {
            // nb_recs == 0: single record
            size_t rec_start_offset = page_offset + 8;
            if (rec_start_offset + page_data_size > size) {
                continue;
            }
            const uint8_t* rec_start = data + rec_start_offset;
            size_t rec_size = page_data_size;

            if (rec_size >= 4
                && rec_start[0] == 0x42 && rec_start[1] == 0x00
                && rec_start[2] == 0x01 && rec_start[3] == 0x00) {
                rec_start += 4;
                rec_size -= 4;
            }

            Record rec;
            rec.global_index = base_rec;
            rec.data.assign(rec_start, rec_start + rec_size);
            all_records.push_back(std::move(rec));
        }
    }

    // Sort records by global index
    std::sort(all_records.begin(), all_records.end(),
              [](const Record& a, const Record& b) {
                  return a.global_index < b.global_index;
              });

    // Decode frames with delta encoding
    std::vector<uint8_t> prev_frame(core::ANM_FRAME_SIZE, 0);

    for (const auto& rec : all_records) {
        core::DecodedFrame frame;
        frame.pixels = prev_frame;  // Start from previous frame

        decode_record(rec.data.data(), rec.data.size(), frame.pixels.data());

        anim.frames.push_back(std::move(frame));
        prev_frame = anim.frames.back().pixels;
    }

    return anim;
}

core::DecodedAnimation AnmLoader::load(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open ANM: " + path.string());
    }

    auto file_size = file.tellg();
    std::vector<uint8_t> buffer(static_cast<size_t>(file_size));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()),
              static_cast<std::streamsize>(file_size));

    return load(buffer.data(), buffer.size());
}

std::vector<uint8_t> AnmLoader::frame_to_rgba(
    const core::DecodedAnimation& anim, size_t frame_idx) {
    if (frame_idx >= anim.frames.size()) {
        throw std::out_of_range("frame_idx out of range");
    }

    const auto& frame = anim.frames[frame_idx];
    size_t pixel_count = static_cast<size_t>(anim.width) * anim.height;
    std::vector<uint8_t> rgba(pixel_count * 4);

    for (size_t i = 0; i < pixel_count; ++i) {
        uint8_t idx = frame.pixels[i];
        rgba[i * 4 + 0] = anim.palette.colors[idx][0];
        rgba[i * 4 + 1] = anim.palette.colors[idx][1];
        rgba[i * 4 + 2] = anim.palette.colors[idx][2];
        rgba[i * 4 + 3] = 255;
    }

    return rgba;
}

}  // namespace dlzr::app
