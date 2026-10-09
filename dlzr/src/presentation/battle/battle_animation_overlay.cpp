// battle_animation_overlay.cpp — FLM 战斗动画叠加层实现

#include "presentation/battle/battle_animation_overlay.hpp"

#include <cstring>

namespace dlzr::presentation {

void BattleAnimationOverlay::init(platform::Platform& platform,
                                   const app::FlmLoader& flm) {
    platform_ = &platform;
    flm_ = &flm;
}

void BattleAnimationOverlay::play(int32_t flm_index,
                                   int32_t grid_x, int32_t grid_y,
                                   int32_t grid_origin_x,
                                   int32_t grid_origin_y,
                                   int32_t cell_size) {
    if (!platform_ || !flm_) return;

    const auto* anim = flm_->get_animation(flm_index);
    if (!anim) return;

    anim_ = anim;
    dest_x_ = grid_origin_x + grid_x * cell_size + (cell_size - app::FlmAnimation::WIDTH) / 2;
    dest_y_ = grid_origin_y + grid_y * cell_size + (cell_size - app::FlmAnimation::HEIGHT) / 2;

    if (!texture_) {
        texture_ = platform_->create_texture(
            app::FlmAnimation::WIDTH, app::FlmAnimation::HEIGHT);
    }

    current_frame_ = 0;
    playing_ = true;
    frame_start_time_ = platform_->ticks_ms();
    upload_frame(0);
}

void BattleAnimationOverlay::update() {
    if (!playing_ || !anim_) return;

    uint32_t now = platform_->ticks_ms();
    if (now - frame_start_time_ < FRAME_DURATION_MS) return;

    current_frame_++;
    if (current_frame_ >= app::FlmAnimation::FRAME_COUNT) {
        playing_ = false;
        return;
    }

    upload_frame(current_frame_);
    frame_start_time_ = now;
}

void BattleAnimationOverlay::render() {
    if (!platform_ || !texture_ || !playing_) return;
    platform_->draw_texture(texture_, dest_x_, dest_y_);
}

void BattleAnimationOverlay::upload_frame(int32_t frame_idx) {
    if (!anim_ || frame_idx < 0 || frame_idx >= app::FlmAnimation::FRAME_COUNT) return;

    const auto& frame = anim_->frames[frame_idx];
    platform_->update_texture(texture_, frame.rgba.data(),
                              app::FlmAnimation::WIDTH * 4);
}

}  // namespace dlzr::presentation
