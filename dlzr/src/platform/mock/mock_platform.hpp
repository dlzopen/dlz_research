#pragma once

// mock_platform.hpp — 模拟平台后端（用于测试）
// Mock platform backend (for testing)
//
// 无实际渲染/音频的模拟实现，用于单元测试。
// Mock implementation without actual rendering/audio, for unit testing.

#include "platform/platform.hpp"

#include <unordered_map>
#include <vector>

namespace dlzr::platform {

class MockPlatform : public Platform {
public:
    MockPlatform() = default;
    ~MockPlatform() override = default;

    bool init(const PlatformConfig& config) override;
    void shutdown() override;

    bool poll_event(Event& out_event) override;

    void clear(uint8_t r, uint8_t g, uint8_t b) override;
    void present() override;

    TextureHandle create_texture(int32_t width, int32_t height) override;
    void update_texture(TextureHandle tex,
                        const uint8_t* rgba_data,
                        int32_t pitch) override;
    void draw_texture(TextureHandle tex,
                      int32_t dst_x, int32_t dst_y) override;
    void draw_texture_rect(TextureHandle tex,
                           int32_t dst_x, int32_t dst_y,
                           int32_t dst_w, int32_t dst_h) override;
    void draw_texture_sub(TextureHandle tex,
                          int32_t src_x, int32_t src_y,
                          int32_t src_w, int32_t src_h,
                          int32_t dst_x, int32_t dst_y) override;
    void destroy_texture(TextureHandle tex) override;

    void draw_rect(int32_t x, int32_t y, int32_t w, int32_t h,
                   uint8_t r, uint8_t g, uint8_t b) override;
    void fill_rect(int32_t x, int32_t y, int32_t w, int32_t h,
                   uint8_t r, uint8_t g, uint8_t b) override;

    AudioHandle play_sound(const uint8_t* pcm_data, size_t size,
                           int32_t sample_rate = 22050) override;
    void stop_sound(AudioHandle handle) override;
    bool is_sound_playing(AudioHandle handle) override;

    bool play_music(const std::string& path, bool loop = true) override;
    void stop_music() override;
    void set_music_volume(int32_t volume) override;

    void delay_ms(uint32_t ms) override;
    uint32_t ticks_ms() override;

    void start_text_input() override;
    void stop_text_input() override;

    // 测试辅助 / Test helpers
    int32_t clear_count() const { return clear_count_; }
    int32_t present_count() const { return present_count_; }
    int32_t texture_count() const { return static_cast<int32_t>(textures_.size()); }

private:
    struct TextureData {
        int32_t width;
        int32_t height;
        std::vector<uint8_t> data;
    };

    std::unordered_map<TextureHandle, TextureData> textures_;
    TextureHandle next_texture_{1};
    AudioHandle next_sound_{1};
    uint32_t current_time_{0};

    int32_t clear_count_{0};
    int32_t present_count_{0};
};

}  // namespace dlzr::platform
