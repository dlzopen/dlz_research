#pragma once

// flc_player.hpp — FLC 动画播放器
// FLC animation player
//
// 渲染 FLC 动画帧，支持调色板转换和帧率控制。
// Renders FLC animation frames with palette conversion and frame rate control.

#include "app/resource/flc_loader.hpp"
#include "platform/platform.hpp"

#include <cstdint>

namespace dlzr::presentation {

class FlcPlayer {
public:
    void init(platform::Platform& platform);

    // 设置要播放的动画 / Set animation to play
    void set_animation(const core::DecodedFlc& flc);

    // 开始播放 / Start playback
    void play(int32_t x = 0, int32_t y = 0);

    // 停止播放 / Stop playback
    void stop();

    // 更新动画状态（每帧调用）/ Update animation state (call each frame)
    void update(uint32_t delta_ms);

    // 渲染当前帧 / Render current frame
    void render();

    // 是否正在播放 / Is playing
    bool is_playing() const { return playing_; }

    // 是否播放完成 / Is finished
    bool is_finished() const { return finished_; }

private:
    platform::Platform* platform_{nullptr};
    const core::DecodedFlc* flc_{nullptr};
    platform::TextureHandle texture_{0};
    std::vector<uint8_t> rgba_buffer_;

    int32_t dest_x_{0};
    int32_t dest_y_{0};
    size_t current_frame_{0};
    uint32_t frame_timer_{0};
    bool playing_{false};
    bool finished_{false};

    void upload_frame(size_t frame_idx);
};

}  // namespace dlzr::presentation
