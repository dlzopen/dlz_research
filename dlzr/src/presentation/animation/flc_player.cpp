#include "presentation/animation/flc_player.hpp"

namespace dlzr::presentation {

void FlcPlayer::init(platform::Platform& platform) {
    platform_ = &platform;
}

void FlcPlayer::set_animation(const core::DecodedFlc& flc) {
    flc_ = &flc;
    current_frame_ = 0;
    frame_timer_ = 0;
    finished_ = false;

    // Allocate RGBA buffer for conversion
    rgba_buffer_.resize(flc.width * flc.height * 4);
}

void FlcPlayer::play(int32_t x, int32_t y) {
    if (!flc_ || flc_->frames.empty()) {
        return;
    }

    dest_x_ = x;
    dest_y_ = y;
    playing_ = true;
    finished_ = false;
    current_frame_ = 0;
    frame_timer_ = 0;

    upload_frame(0);
}

void FlcPlayer::stop() {
    playing_ = false;
    finished_ = true;
}

void FlcPlayer::update(uint32_t delta_ms) {
    if (!playing_ || finished_ || !flc_) {
        return;
    }

    frame_timer_ += delta_ms;

    if (frame_timer_ >= static_cast<uint32_t>(flc_->speed)) {
        frame_timer_ -= flc_->speed;
        ++current_frame_;

        if (current_frame_ >= flc_->frames.size()) {
            finished_ = true;
            playing_ = false;
        } else {
            upload_frame(current_frame_);
        }
    }
}

void FlcPlayer::render() {
    if (!playing_ || !texture_) {
        return;
    }

    platform_->draw_texture(texture_, dest_x_, dest_y_);
}

void FlcPlayer::upload_frame(size_t frame_idx) {
    if (!flc_ || frame_idx >= flc_->frames.size()) {
        return;
    }

    const auto& frame = flc_->frames[frame_idx];
    const auto& palette = frame.palette;

    // Convert 8-bit palette indexed to RGBA
    for (int32_t i = 0; i < flc_->width * flc_->height; ++i) {
        uint8_t idx = frame.pixels[i];
        rgba_buffer_[i * 4 + 0] = palette[idx * 3 + 0];  // R
        rgba_buffer_[i * 4 + 1] = palette[idx * 3 + 1];  // G
        rgba_buffer_[i * 4 + 2] = palette[idx * 3 + 2];  // B
        rgba_buffer_[i * 4 + 3] = 255;                     // A
    }

    // Create or update texture
    if (!texture_) {
        texture_ = platform_->create_texture(flc_->width, flc_->height);
    }

    platform_->update_texture(texture_, rgba_buffer_.data(), rgba_buffer_.size());
}

}  // namespace dlzr::presentation
