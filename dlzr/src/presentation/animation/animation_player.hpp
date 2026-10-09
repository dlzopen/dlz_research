#pragma once

// animation_player.hpp — 动画播放器
// Animation player
//
// 渲染 ANM 动画帧，支持调色板转换和帧率控制。
// Renders ANM animation frames with palette conversion and frame rate control.

#include "app/resource/anm_loader.hpp"
#include "core/data_model/types.hpp"
#include "platform/platform.hpp"

#include <cstdint>

namespace dlzr::presentation {

class AnimationPlayer {
public:
    void init(platform::Platform& platform);

    // 设置要播放的动画 / Set animation to play
    void set_animation(const core::DecodedAnimation& anim);

    // 开始播放 / Start playback
    void play(int32_t x, int32_t y, float scale = 1.0f);

    // 停止播放 / Stop playback
    void stop();

    // 更新动画状态（每帧调用）/ Update animation state (call each frame)
    void update();

    // 渲染当前帧 / Render current frame
    void render();

    // 是否正在播放 / Is playing
    bool is_playing() const { return playing_; }

    // 是否播放完成 / Is finished
    bool is_finished() const { return finished_; }

private:
    platform::Platform* platform_{nullptr};
    const core::DecodedAnimation* animation_{nullptr};
    platform::TextureHandle texture_{0};
    std::vector<uint8_t> rgba_buffer_;

    int32_t dest_x_{0};
    int32_t dest_y_{0};
    float scale_{1.0f};
    size_t current_frame_{0};
    uint32_t frame_start_time_{0};
    bool playing_{false};
    bool finished_{false};

    void upload_frame(size_t frame_idx);
};

}  // namespace dlzr::presentation
