#pragma once

#include "platform/platform.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace dlzr::presentation {

class FontRenderer {
public:
    bool init(platform::Platform& platform, const std::string& game_data_dir);

    void draw_text(int32_t x, int32_t y, std::string_view text,
                   uint8_t r = 255, uint8_t g = 255, uint8_t b = 255);

    void draw_text_wrapped(int32_t x, int32_t y, int32_t max_width,
                           std::string_view text,
                           uint8_t r = 255, uint8_t g = 255, uint8_t b = 255);

    void measure_text(std::string_view text, int32_t& out_w, int32_t& out_h) const;

    void clear_cache();

    static constexpr int32_t CHAR_WIDTH = 16;
    static constexpr int32_t CHAR_HEIGHT = 16;

private:
    struct CachedText {
        platform::TextureHandle texture{0};
        int32_t width{0};
        int32_t height{0};
    };

    platform::Platform* platform_{nullptr};
    std::vector<uint8_t> hzk16_;
    std::vector<uint8_t> asc16_;
    std::unordered_map<std::string, CachedText> texture_cache_;
    static constexpr size_t MAX_CACHE_ENTRIES = 256;

    void render_glyph_16x16(uint8_t* dst, int32_t dst_w,
                             int32_t gx, int32_t gy,
                             const uint8_t* glyph_data,
                             uint8_t r, uint8_t g, uint8_t b);

    void render_ascii_glyph(uint8_t* dst, int32_t dst_w,
                             int32_t gx, int32_t gy,
                             uint8_t ch,
                             uint8_t r, uint8_t g, uint8_t b);

    void render_text_to_buffer(uint8_t* buffer, int32_t buf_w,
                                std::string_view text,
                                uint8_t r, uint8_t g, uint8_t b);

    bool lookup_gbk_glyph(uint8_t hi, uint8_t lo, size_t& out_offset) const;
};

}  // namespace dlzr::presentation
