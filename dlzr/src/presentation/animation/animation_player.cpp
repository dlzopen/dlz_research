#include "presentation/animation/animation_player.hpp"

#include <cstring>

namespace dlzr::presentation {

void AnimationPlayer::init(platform::Platform& platform) {
    platform_ = &platform;
}

void AnimationPlayer::set_animation(const core::DecodedAnimation& anim) {
    animation_ = &anim;
    current_frame_ = 0;
    finished_ = false;

    if (!texture_ && platform_) {
        texture_ = platform_->create_texture(anim.width, anim.height);
        rgba_buffer_.resize(anim.width * anim.height * 4);
    }
}

void AnimationPlayer::play(int32_t x, int32_t y, float scale) {
    if (!animation_ || animation_->frames.empty()) {
        return;
    }

    dest_x_ = x;
    dest_y_ = y;
    scale_ = scale;
    current_frame_ = 0;
    playing_ = true;
    finished_ = false;
    frame_start_time_ = platform_->ticks_ms();

    upload_frame(0);
}

void AnimationPlayer::stop() {
    playing_ = false;
    finished_ = true;
}

void AnimationPlayer::update() {
    if (!playing_ || !animation_ || animation_->frames.empty()) {
        return;
    }

    uint32_t now = platform_->ticks_ms();
    uint32_t frame_duration = animation_->rate > 0 ? (1000 / animation_->rate) : 66;

    if (now - frame_start_time_ >= frame_duration) {
        current_frame_++;
        if (current_frame_ >= animation_->frames.size()) {
            playing_ = false;
            finished_ = true;
            return;
        }

        upload_frame(current_frame_);
        frame_start_time_ = now;
    }
}

void AnimationPlayer::render() {
    if (!platform_ || !texture_ || !playing_) {
        return;
    }

    if (scale_ == 1.0f) {
        platform_->draw_texture(texture_, dest_x_, dest_y_);
    } else {
        int32_t w = static_cast<int32_t>(animation_->width * scale_);
        int32_t h = static_cast<int32_t>(animation_->height * scale_);
        platform_->draw_texture_rect(texture_, dest_x_, dest_y_, w, h);
    }
}

void AnimationPlayer::upload_frame(size_t frame_idx) {
    if (!animation_ || frame_idx >= animation_->frames.size()) {
        return;
    }

    const auto& frame = animation_->frames[frame_idx];
    const auto& palette = animation_->palette;

    for (size_t i = 0; i < frame.pixels.size(); ++i) {
        uint8_t idx = frame.pixels[i];
        rgba_buffer_[i * 4 + 0] = palette.colors[idx][0];
        rgba_buffer_[i * 4 + 1] = palette.colors[idx][1];
        rgba_buffer_[i * 4 + 2] = palette.colors[idx][2];
        rgba_buffer_[i * 4 + 3] = 255;
    }

    platform_->update_texture(texture_, rgba_buffer_.data(),
                              animation_->width * 4);
}

}  // namespace dlzr::presentation
