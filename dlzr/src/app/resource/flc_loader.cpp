#include "app/resource/flc_loader.hpp"

#include <cstring>
#include <fstream>
#include <stdexcept>

namespace dlzr::core {

namespace {

#pragma pack(push, 1)
struct FlcHeader {
    uint32_t size;
    uint16_t magic;
    uint16_t frames;
    uint16_t width;
    uint16_t height;
    uint16_t depth;
    uint16_t flags;
    uint32_t speed;
    uint16_t next_header;
    uint16_t frit;
    uint8_t reserved[82];
};

struct FlcFrameHeader {
    uint32_t size;
    uint16_t magic;  // 0xF1FA for frame
    uint16_t chunks;
    uint8_t reserved[8];
};

struct FlcChunkHeader {
    uint32_t size;
    uint16_t type;
};
#pragma pack(pop)

constexpr uint16_t FLC_MAGIC = 0xAF12;
constexpr uint16_t FRAME_MAGIC = 0xF1FA;

// Chunk types
constexpr uint16_t CHUNK_COLOR_256 = 4;   // 256-color palette
constexpr uint16_t CHUNK_DELTA_FLC = 7;   // Delta compression (FLC)
constexpr uint16_t CHUNK_COLOR_64 = 11;   // 64-level color palette
constexpr uint16_t CHUNK_DELTA_FLI = 12;  // Delta compression (FLI)
constexpr uint16_t CHUNK_BLACK = 13;      // Black frame
constexpr uint16_t CHUNK_BRUN = 15;       // Byte-run-length
constexpr uint16_t CHUNK_COPY = 16;       // Uncompressed copy

void decode_brun(const uint8_t* data, size_t size, uint8_t* pixels,
                 int32_t width, int32_t height) {
    const uint8_t* src = data;
    uint8_t* dst = pixels;

    for (int32_t y = 0; y < height; ++y) {
        int32_t x = 0;
        int32_t packets = *src++;

        for (int32_t p = 0; p < packets && x < width; ++p) {
            int8_t count = static_cast<int8_t>(*src++);

            if (count < 0) {
                // Run of same color
                count = -count;
                uint8_t color = *src++;
                for (int32_t i = 0; i < count && x < width; ++i, ++x) {
                    *dst++ = color;
                }
            } else {
                // Literal run
                for (int32_t i = 0; i < count && x < width; ++i, ++x) {
                    *dst++ = *src++;
                }
            }
        }
    }
}

void decode_delta_fli(const uint8_t* data, size_t size, uint8_t* pixels,
                      int32_t width, int32_t height) {
    const uint8_t* src = data;
    uint8_t* dst = pixels;

    // FLI delta compression
    int32_t y = 0;
    int16_t lines_to_skip = *reinterpret_cast<const int16_t*>(src);
    src += 2;
    y += lines_to_skip;

    int16_t lines_to_change = *reinterpret_cast<const int16_t*>(src);
    src += 2;

    for (int32_t i = 0; i < lines_to_change && y < height; ++i) {
        int16_t packets = *reinterpret_cast<const int16_t*>(src);
        src += 2;

        if (packets < 0) {
            // Skip lines
            if (packets == -1) {
                // Last line marker
                break;
            }
            y += -packets;
            continue;
        }

        int32_t x = 0;
        for (int32_t p = 0; p < packets && x < width; ++p) {
            int8_t count = static_cast<int8_t>(*src++);

            if (count < 0) {
                // Skip pixels
                x += -count;
            } else {
                // Copy pixels
                for (int32_t j = 0; j < count && x < width; ++j, ++x) {
                    dst[y * width + x] = *src++;
                }
            }
        }
        ++y;
    }
}

void decode_palette(const uint8_t* data, size_t size, uint8_t* palette, int levels) {
    const uint8_t* src = data;
    uint16_t start = *reinterpret_cast<const uint16_t*>(src);
    src += 2;
    uint16_t count = *reinterpret_cast<const uint16_t*>(src);
    src += 2;

    for (uint16_t i = 0; i < count; ++i) {
        uint16_t idx = start + i;
        if (idx < 256) {
            palette[idx * 3 + 0] = static_cast<uint8_t>((*src++) * (256 / levels));
            palette[idx * 3 + 1] = static_cast<uint8_t>((*src++) * (256 / levels));
            palette[idx * 3 + 2] = static_cast<uint8_t>((*src++) * (256 / levels));
        }
    }
}

}  // namespace

DecodedFlc FlcLoader::load_from_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open FLC file: " + path);
    }

    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> data(size);
    file.read(reinterpret_cast<char*>(data.data()), size);

    return load_from_memory(data.data(), data.size());
}

DecodedFlc FlcLoader::load_from_memory(const uint8_t* data, size_t size) {
    if (size < sizeof(FlcHeader)) {
        throw std::runtime_error("FLC file too small");
    }

    const auto* header = reinterpret_cast<const FlcHeader*>(data);

    if (header->magic != FLC_MAGIC) {
        throw std::runtime_error("Invalid FLC magic number");
    }

    DecodedFlc flc;
    flc.width = header->width;
    flc.height = header->height;
    flc.frame_count = header->frames;
    flc.speed = header->speed;

    // Allocate frames
    flc.frames.resize(flc.frame_count);
    for (auto& frame : flc.frames) {
        frame.pixels.resize(flc.width * flc.height, 0);
        frame.palette.resize(256 * 3, 0);
    }

    // Parse frames
    size_t offset = sizeof(FlcHeader);
    for (int32_t i = 0; i < flc.frame_count; ++i) {
        if (offset + sizeof(FlcFrameHeader) > size) {
            break;
        }

        const auto* frame_header = reinterpret_cast<const FlcFrameHeader*>(data + offset);

        if (frame_header->magic != FRAME_MAGIC) {
            break;
        }

        size_t frame_offset = offset + sizeof(FlcFrameHeader);
        size_t frame_end = offset + frame_header->size;

        // Process chunks
        for (uint16_t c = 0; c < frame_header->chunks; ++c) {
            if (frame_offset + sizeof(FlcChunkHeader) > frame_end) {
                break;
            }

            const auto* chunk = reinterpret_cast<const FlcChunkHeader*>(data + frame_offset);
            const uint8_t* chunk_data = data + frame_offset + sizeof(FlcChunkHeader);
            size_t chunk_data_size = chunk->size - sizeof(FlcChunkHeader);

            switch (chunk->type) {
            case CHUNK_COLOR_256:
                decode_palette(chunk_data, chunk_data_size, flc.frames[i].palette.data(), 256);
                break;

            case CHUNK_COLOR_64:
                decode_palette(chunk_data, chunk_data_size, flc.frames[i].palette.data(), 64);
                break;

            case CHUNK_BRUN:
                decode_brun(chunk_data, chunk_data_size, flc.frames[i].pixels.data(),
                           flc.width, flc.height);
                break;

            case CHUNK_DELTA_FLI:
                decode_delta_fli(chunk_data, chunk_data_size, flc.frames[i].pixels.data(),
                                flc.width, flc.height);
                break;

            case CHUNK_COPY:
                if (chunk_data_size >= static_cast<size_t>(flc.width * flc.height)) {
                    std::memcpy(flc.frames[i].pixels.data(), chunk_data,
                               flc.width * flc.height);
                }
                break;

            default:
                // Unknown chunk type, skip
                break;
            }

            frame_offset += chunk->size;
        }

        offset += frame_header->size;
    }

    return flc;
}

}  // namespace dlzr::core
