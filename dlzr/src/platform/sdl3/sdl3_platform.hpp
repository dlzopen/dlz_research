#pragma once

#include "platform/platform.hpp"

#include <SDL3/SDL.h>

// Workaround: SDL_ALIGNED missing from SDL3 release-3.2.x
#ifndef SDL_ALIGNED
#  if defined(_MSC_VER)
#    define SDL_ALIGNED(x) __declspec(align(x))
#  elif defined(__GNUC__) || defined(__clang__)
#    define SDL_ALIGNED(x) __attribute__((aligned(x)))
#  else
#    define SDL_ALIGNED(x)
#  endif
#endif

#ifdef DLZR_HAS_SDL3_MIXER
#include <SDL3_mixer/SDL_mixer.h>
#endif

#include <unordered_map>
#include <vector>

namespace dlzr::platform {

class Sdl3Platform : public Platform {
public:
    Sdl3Platform() = default;
    ~Sdl3Platform() override;

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
    void draw_line(int32_t x1, int32_t y1, int32_t x2, int32_t y2,
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

private:
    SDL_Window* window_{nullptr};
    SDL_Renderer* renderer_{nullptr};
    SDL_AudioDeviceID audio_device_{0};
#ifdef DLZR_HAS_SDL3_MIXER
    Mix_Music* current_music_{nullptr};
#else
    void* current_music_{nullptr};
#endif
    std::unordered_map<TextureHandle, SDL_Texture*> textures_;
    TextureHandle next_texture_{1};

    // 延迟销毁：SDL3 D3D11 后端在 present 时把"已销毁"纹理加入 zombie 列表并立即释放。
    // 若同一帧内刚绘制过的纹理被立即销毁，present 提交的渲染批次仍引用它，
    // 释放即触发 RtlFreeHeap 堆损坏崩溃。故实际释放延后 2 帧，待在途批次退役。
    std::vector<SDL_Texture*> deferred_destroy_[3];
    int destroy_gen_{0};

    void flush_deferred_destroy();

    struct SoundInstance {
        SDL_AudioStream* stream{nullptr};
        bool playing{false};
    };
    std::unordered_map<AudioHandle, SoundInstance> sounds_;
    AudioHandle next_sound_{1};
};

}  // namespace dlzr::platform
