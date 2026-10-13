#pragma once

// platform.hpp — 平台抽象层接口
// Platform abstraction layer interface
//
// 所有平台相关操作的抽象接口。SDL3、Web 等后端各自实现。
// Abstract interface for all platform operations.
// SDL3, Web, etc. each provide their own implementation.

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace dlzr::platform {

struct PlatformConfig {
    std::string title{"dlzr"};
    int32_t width{640};
    int32_t height{480};

    // Display options
    bool fullscreen{false};
    bool vsync{true};
    int32_t render_scale{1};  // 整数缩放 / Integer scaling (1x, 2x, 3x, etc.)
    bool auto_quality{true};  // 自动质量调整 / Automatic quality adjustment
};

// 平台事件 / Platform event
struct Event {
    enum class Type {
        None,
        Quit,
        KeyDown,
        KeyUp,
        MouseDown,
        MouseUp,
        MouseMove,
        TouchDown,
        TouchUp,
        TouchMove,
        GamepadButtonDown,
        GamepadButtonUp,
        GamepadAxis,
        MouseWheel,
    };

    Type type{Type::None};
    int32_t key{0};
    int32_t mouse_x{0};
    int32_t mouse_y{0};
    int32_t mouse_button{0};

    // Touch events
    int32_t touch_id{0};
    float touch_x{0.0f};
    float touch_y{0.0f};
    float touch_pressure{0.0f};

    // Gamepad events
    int32_t gamepad_button{0};
    float gamepad_axis_value{0.0f};
    int32_t gamepad_axis{0};

    // Mouse wheel
    int32_t wheel_x{0};
    int32_t wheel_y{0};
};

// 纹理句柄 / Texture handle
using TextureHandle = uint32_t;

// 平台抽象接口 / Platform abstract interface
class Platform {
public:
    virtual ~Platform() = default;

    virtual bool init(const PlatformConfig& config) = 0;
    virtual void shutdown() = 0;

    virtual bool poll_event(Event& out_event) = 0;

    virtual void clear(uint8_t r, uint8_t g, uint8_t b) = 0;
    virtual void present() = 0;

    virtual TextureHandle create_texture(int32_t width, int32_t height) = 0;
    virtual void update_texture(TextureHandle tex,
                                const uint8_t* rgba_data,
                                int32_t pitch) = 0;
    virtual void draw_texture(TextureHandle tex,
                              int32_t dst_x, int32_t dst_y) = 0;
    virtual void draw_texture_rect(TextureHandle tex,
                                   int32_t dst_x, int32_t dst_y,
                                   int32_t dst_w, int32_t dst_h) = 0;
    virtual void destroy_texture(TextureHandle tex) = 0;

    virtual void draw_rect(int32_t x, int32_t y, int32_t w, int32_t h,
                           uint8_t r, uint8_t g, uint8_t b) = 0;
    virtual void fill_rect(int32_t x, int32_t y, int32_t w, int32_t h,
                           uint8_t r, uint8_t g, uint8_t b) = 0;
    virtual void draw_line(int32_t x1, int32_t y1, int32_t x2, int32_t y2,
                           uint8_t r, uint8_t g, uint8_t b) = 0;

    // 音频 / Audio
    using AudioHandle = uint32_t;
    virtual AudioHandle play_sound(const uint8_t* pcm_data, size_t size,
                                   int32_t sample_rate = 22050) = 0;
    virtual void stop_sound(AudioHandle handle) = 0;
    virtual bool is_sound_playing(AudioHandle handle) = 0;

    // 背景音乐 / Background music
    virtual bool play_music(const std::string& path, bool loop = true) = 0;
    virtual void stop_music() = 0;
    virtual void set_music_volume(int32_t volume) = 0;  // 0-128

    virtual void delay_ms(uint32_t ms) = 0;
    virtual uint32_t ticks_ms() = 0;
};

std::unique_ptr<Platform> create_sdl3_platform();

}  // namespace dlzr::platform
