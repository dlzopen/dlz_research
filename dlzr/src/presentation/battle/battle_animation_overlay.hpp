#pragma once

// battle_animation_overlay.hpp — FLM 战斗动画叠加层
// FLM battle animation overlay
//
// 在战斗网格指定位置播放 FLM 动画（攻击/技能/地雷引爆）。
// Plays FLM animations at grid positions (attack/skill/mine trigger).

#include "app/resource/flm_loader.hpp"
#include "platform/platform.hpp"

#include <cstdint>

namespace dlzr::presentation {

class BattleAnimationOverlay {
public:
    void init(platform::Platform& platform, const app::FlmLoader& flm);

    void play(int32_t flm_index, int32_t grid_x, int32_t grid_y,
              int32_t grid_origin_x, int32_t grid_origin_y, int32_t cell_size);

    void update();
    void render();

    bool is_playing() const { return playing_; }

private:
    platform::Platform* platform_{nullptr};
    const app::FlmLoader* flm_{nullptr};
    platform::TextureHandle texture_{0};

    const app::FlmAnimation* anim_{nullptr};
    int32_t current_frame_{0};
    uint32_t frame_start_time_{0};
    bool playing_{false};

    int32_t dest_x_{0};
    int32_t dest_y_{0};

    static constexpr uint32_t FRAME_DURATION_MS = 66;

    void upload_frame(int32_t frame_idx);
};

}  // namespace dlzr::presentation
