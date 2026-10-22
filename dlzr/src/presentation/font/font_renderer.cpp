#include "presentation/font/font_renderer.hpp"

#include <cstring>
#include <filesystem>
#include <fstream>

namespace dlzr::presentation {

namespace {

std::vector<uint8_t> load_file(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return {};
    auto size = file.tellg();
    std::vector<uint8_t> data(static_cast<size_t>(size));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(data.data()),
              static_cast<std::streamsize>(size));
    return data;
}

std::string make_cache_key(std::string_view text, uint8_t r, uint8_t g, uint8_t b) {
    std::string key;
    key.reserve(text.size() + 4);
    key.append(text.data(), text.size());
    key.push_back(static_cast<char>(r));
    key.push_back(static_cast<char>(g));
    key.push_back(static_cast<char>(b));
    return key;
}

}  // namespace

bool FontRenderer::init(platform::Platform& platform,
                         const std::string& game_data_dir) {
    platform_ = &platform;

    namespace fs = std::filesystem;
    fs::path dir(game_data_dir);

    hzk16_ = load_file(dir / "HZK16");
    asc16_ = load_file(dir / "ASC16");

    if (hzk16_.empty()) {
        std::fprintf(stderr, "Warning: HZK16 not loaded\n");
    }
    if (asc16_.empty()) {
        std::fprintf(stderr, "Warning: ASC16 not loaded\n");
    }

    return true;
}

void FontRenderer::clear_cache() {
    for (auto& [key, cached] : texture_cache_) {
        if (cached.texture && platform_) {
            platform_->destroy_texture(cached.texture);
        }
    }
    texture_cache_.clear();
}

bool FontRenderer::lookup_gbk_glyph(uint8_t hi, uint8_t lo, size_t& out_offset) const {
    if (hzk16_.empty()) return false;

    // GB2312: hi 0xA1-0xFE, lo 0xA1-0xFE → zones 1-87, positions 1-94
    if (hi >= 0xA1 && hi <= 0xFE && lo >= 0xA1 && lo <= 0xFE) {
        int32_t zone = hi - 0xA0;  // 1-87
        int32_t pos = lo - 0xA0;   // 1-94
        out_offset = (static_cast<size_t>(zone - 1) * 94 + (pos - 1)) * 32;
        return out_offset + 32 <= hzk16_.size();
    }

    // GBK extension: hi 0x81-0xA0, lo 0x40-0xFE (excl 0x7F)
    // These map to GB2312 user-defined zones or are unavailable in HZK16
    // HZK16 only covers GB2312 zones 1-87, so GBK-only chars have no glyph
    return false;
}

void FontRenderer::render_glyph_16x16(uint8_t* dst, int32_t dst_w,
                                        int32_t gx, int32_t gy,
                                        const uint8_t* glyph_data,
                                        uint8_t r, uint8_t g, uint8_t b) {
    for (int row = 0; row < 16; ++row) {
        uint16_t bits = (static_cast<uint16_t>(glyph_data[row * 2]) << 8)
                      | glyph_data[row * 2 + 1];
        for (int col = 0; col < 16; ++col) {
            if (bits & (0x8000 >> col)) {
                int32_t px = gx + col;
                int32_t py = gy + row;
                size_t idx = (static_cast<size_t>(py) * dst_w + px) * 4;
                dst[idx + 0] = r;
                dst[idx + 1] = g;
                dst[idx + 2] = b;
                dst[idx + 3] = 255;
            }
        }
    }
}

void FontRenderer::render_ascii_glyph(uint8_t* dst, int32_t dst_w,
                                        int32_t gx, int32_t gy,
                                        uint8_t ch,
                                        uint8_t r, uint8_t g, uint8_t b) {
    if (asc16_.empty()) return;

    // ASC16 字形为 8x16 原生半角，不做横向放大。
    // 证据 FUN_004056ab.c: ASCII 字形 FUN_004052f3 步进 uVar1 += 8（8px 半角），
    // 与 GBK 的 uVar1 += 0x10（16px 全角）相对。
    size_t glyph_offset = static_cast<size_t>(ch) * 16;
    if (glyph_offset + 16 > asc16_.size()) return;

    const uint8_t* glyph = asc16_.data() + glyph_offset;
    for (int row = 0; row < 16; ++row) {
        uint8_t bits = glyph[row];
        for (int col = 0; col < 8; ++col) {
            if (bits & (0x80 >> col)) {
                int32_t px = gx + col;
                int32_t py = gy + row;
                size_t idx = (static_cast<size_t>(py) * dst_w + px) * 4;
                dst[idx + 0] = r;
                dst[idx + 1] = g;
                dst[idx + 2] = b;
                dst[idx + 3] = 255;
            }
        }
    }
}

void FontRenderer::render_text_to_buffer(uint8_t* buffer, int32_t buf_w,
                                          std::string_view text,
                                          uint8_t r, uint8_t g, uint8_t b) {
    int32_t cx = 0;
    int32_t cy = 0;
    size_t i = 0;

    while (i < text.size()) {
        uint8_t byte = static_cast<uint8_t>(text[i]);

        if (byte < 0x80) {
            if (byte >= 0x20) {
                // ASCII 半角 8px；仅当当前行确实放不下该字符（且不在行首）
                // 才换行。证据 FUN_004056ab.c：uVar1 += 8（半角）/ += 0x10（全角）。
                // 注意：换行判定必须用字符实际宽度，不能用全角 CHAR_WIDTH，
                // 否则以 8px 半角结尾的字符串会被误判为"放不下"而提前换行，
                // 把尾字符画到第 2 行(row 16+)，越界写穿单行 16 行高的缓冲。
                if (cx + ASCII_WIDTH > buf_w && cx > 0) {
                    cx = 0;
                    cy += CHAR_HEIGHT;
                }
                render_ascii_glyph(buffer, buf_w, cx, cy, byte, r, g, b);
            }
            cx += ASCII_WIDTH;
            ++i;
        } else if (i + 1 < text.size()) {
            uint8_t hi = byte;
            uint8_t lo = static_cast<uint8_t>(text[i + 1]);

            // 全角 16px；同理仅当放不下才换行（且不在行首）。
            if (cx + CHAR_WIDTH > buf_w && cx > 0) {
                cx = 0;
                cy += CHAR_HEIGHT;
            }
            size_t offset = 0;
            if (lookup_gbk_glyph(hi, lo, offset)) {
                render_glyph_16x16(buffer, buf_w, cx, cy,
                                    hzk16_.data() + offset, r, g, b);
            }
            cx += CHAR_WIDTH;
            i += 2;
        } else {
            ++i;
        }
    }
}

void FontRenderer::draw_text(int32_t x, int32_t y, std::string_view text,
                               uint8_t r, uint8_t g, uint8_t b) {
    if (!platform_ || text.empty()) return;

    int32_t text_w = 0, text_h = 0;
    measure_text(text, text_w, text_h);
    if (text_w <= 0 || text_h <= 0) return;

    std::string cache_key = make_cache_key(text, r, g, b);
    auto it = texture_cache_.find(cache_key);
    if (it != texture_cache_.end()) {
        platform_->draw_texture(it->second.texture, x, y);
        return;
    }

    if (texture_cache_.size() >= MAX_CACHE_ENTRIES) {
        clear_cache();
    }

    std::vector<uint8_t> buffer(static_cast<size_t>(text_w) * text_h * 4, 0);
    render_text_to_buffer(buffer.data(), text_w, text, r, g, b);

    auto tex = platform_->create_texture(text_w, text_h);
    if (tex) {
        platform_->update_texture(tex, buffer.data(), text_w * 4);
        platform_->draw_texture(tex, x, y);
        texture_cache_[cache_key] = {tex, text_w, text_h};
    }
}

void FontRenderer::draw_text_wrapped(int32_t x, int32_t y, int32_t max_width,
                                      std::string_view text,
                                      uint8_t r, uint8_t g, uint8_t b) {
    if (!platform_ || text.empty() || max_width <= 0) return;

    int32_t chars_per_line = max_width / CHAR_WIDTH;
    if (chars_per_line <= 0) return;

    int32_t line_w = chars_per_line * CHAR_WIDTH;
    int32_t cx = 0;
    int32_t cy = 0;
    size_t i = 0;

    while (i < text.size()) {
        size_t line_start = i;
        cx = 0;

        while (i < text.size() && cx < line_w) {
            uint8_t byte = static_cast<uint8_t>(text[i]);
            if (byte < 0x80) {
                if (byte == '\n') { i++; break; }
                cx += ASCII_WIDTH;   // 半角 8px。证据 FUN_004056ab.c。
                ++i;
            } else {
                cx += CHAR_WIDTH;
                i += 2;
            }
        }

        size_t line_len = i - line_start;
        std::string_view line = text.substr(line_start, line_len);

        int32_t lw = 0, lh = 0;
        measure_text(line, lw, lh);
        if (lw > 0 && lh > 0) {
            std::vector<uint8_t> buf(static_cast<size_t>(lw) * lh * 4, 0);
            render_text_to_buffer(buf.data(), lw, line, r, g, b);

            auto tex = platform_->create_texture(lw, lh);
            if (tex) {
                platform_->update_texture(tex, buf.data(), lw * 4);
                platform_->draw_texture(tex, x, y + cy);
                platform_->destroy_texture(tex);
            }
        }

        cy += CHAR_HEIGHT;
    }
}

void FontRenderer::measure_text(std::string_view text,
                                  int32_t& out_w, int32_t& out_h) const {
    int32_t max_w = 0;
    int32_t cx = 0;
    int32_t lines = 1;
    size_t i = 0;

    while (i < text.size()) {
        uint8_t byte = static_cast<uint8_t>(text[i]);
        if (byte < 0x80) {
            cx += ASCII_WIDTH;   // 半角 8px。证据 FUN_004056ab.c。
            ++i;
        } else {
            cx += CHAR_WIDTH;
            i += 2;
        }
        if (cx > max_w) max_w = cx;
    }

    out_w = max_w;
    out_h = lines * CHAR_HEIGHT;
}

}  // namespace dlzr::presentation
