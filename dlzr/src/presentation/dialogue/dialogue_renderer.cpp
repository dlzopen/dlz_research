#include "presentation/dialogue/dialogue_renderer.hpp"

#include <algorithm>
#include <cstring>

namespace dlzr::presentation {

void DialogueRenderer::init(platform::Platform& platform, FontRenderer& font,
                              ImageCache& images) {
    platform_ = &platform;
    font_ = &font;
    images_ = &images;
    blink_timer_ = 0;
    eyes_closed_ = false;
    mouth_frame_ = 0;
    expression_ = FaceExpression::NEUTRAL;
}

void DialogueRenderer::update_animation(int32_t delta_ms, bool text_is_scrolling) {
    // 眨眼计时 / Blink timer
    blink_timer_ += delta_ms;
    if (blink_timer_ >= BLINK_INTERVAL) {
        eyes_closed_ = true;
        if (blink_timer_ >= BLINK_INTERVAL + BLINK_DURATION) {
            eyes_closed_ = false;
            blink_timer_ = 0;
        }
    }

    // 嘴部动画（文字滚动时）/ Mouth animation (when text is scrolling)
    if (text_is_scrolling) {
        mouth_frame_ = (mouth_frame_ + 1) % 3;
    } else {
        mouth_frame_ = 0;
    }
}

const uint8_t* DialogueRenderer::get_animated_face(const uint8_t* base_face_rgba) {
    if (!base_face_rgba) return nullptr;

    // 复制基础头像到动画缓冲 / Copy base face to animation buffer
    std::memcpy(animated_face_.data(), base_face_rgba, FACE_W * FACE_H * 4);

    // 应用动画效果 / Apply animation effects
    apply_blink(animated_face_.data());
    apply_expression(animated_face_.data());
    apply_mouth(animated_face_.data(), mouth_frame_);

    return animated_face_.data();
}

void DialogueRenderer::apply_blink(uint8_t* rgba) {
    if (!eyes_closed_) return;

    // 简化眨眼效果：将眼睛区域（上半部分中间）变暗
    // Simplified blink: darken eye region (upper-middle area)
    int32_t eye_y_start = FACE_H / 4;
    int32_t eye_y_end = FACE_H / 3;
    int32_t eye_x_start = FACE_W / 4;
    int32_t eye_x_end = FACE_W * 3 / 4;

    for (int32_t y = eye_y_start; y < eye_y_end; ++y) {
        for (int32_t x = eye_x_start; x < eye_x_end; ++x) {
            int32_t idx = (y * FACE_W + x) * 4;
            // 将RGB减半（变暗）/ Halve RGB (darken)
            rgba[idx + 0] /= 2;
            rgba[idx + 1] /= 2;
            rgba[idx + 2] /= 2;
        }
    }
}

void DialogueRenderer::apply_expression(uint8_t* rgba) {
    // 表情效果：简单色调偏移
    // Expression effects: simple tint overlay
    if (expression_ == FaceExpression::NEUTRAL) return;

    uint8_t tint_r = 0, tint_g = 0, tint_b = 0;
    float tint_strength = 0.15f;

    switch (expression_) {
        case FaceExpression::HAPPY:
            tint_r = 40; tint_g = 20; tint_b = 0;  // 暖色调 / Warm tint
            break;
        case FaceExpression::ANGRY:
            tint_r = 50; tint_g = 0; tint_b = 0;    // 红色调 / Red tint
            break;
        case FaceExpression::SAD:
            tint_r = 0; tint_g = 0; tint_b = 40;    // 蓝色调 / Blue tint
            break;
        case FaceExpression::SURPRISED:
            tint_r = 30; tint_g = 30; tint_b = 0;   // 黄色调 / Yellow tint
            break;
        default:
            return;
    }

    // 应用色调到整个头像 / Apply tint to entire face
    for (int32_t i = 0; i < FACE_W * FACE_H; ++i) {
        int32_t idx = i * 4;
        rgba[idx + 0] = std::min(255, static_cast<int32_t>(rgba[idx + 0] + tint_r * tint_strength * 10));
        rgba[idx + 1] = std::min(255, static_cast<int32_t>(rgba[idx + 1] + tint_g * tint_strength * 10));
        rgba[idx + 2] = std::min(255, static_cast<int32_t>(rgba[idx + 2] + tint_b * tint_strength * 10));
    }
}

void DialogueRenderer::apply_mouth(uint8_t* rgba, int32_t frame) {
    if (frame == 0) return;  // 静止状态 / Rest position

    // 简化嘴部动画：改变嘴部区域亮度
    // Simplified mouth animation: change brightness in mouth region
    int32_t mouth_y = FACE_H * 2 / 3;
    int32_t mouth_x_start = FACE_W * 2 / 5;
    int32_t mouth_x_end = FACE_W * 3 / 5;

    int32_t brightness_offset = (frame == 1) ? 20 : -10;

    for (int32_t y = mouth_y; y < mouth_y + 8 && y < FACE_H; ++y) {
        for (int32_t x = mouth_x_start; x < mouth_x_end; ++x) {
            int32_t idx = (y * FACE_W + x) * 4;
            rgba[idx + 0] = static_cast<uint8_t>(
                std::clamp(static_cast<int32_t>(rgba[idx + 0]) + brightness_offset, 0, 255));
            rgba[idx + 1] = static_cast<uint8_t>(
                std::clamp(static_cast<int32_t>(rgba[idx + 1]) + brightness_offset, 0, 255));
            rgba[idx + 2] = static_cast<uint8_t>(
                std::clamp(static_cast<int32_t>(rgba[idx + 2]) + brightness_offset, 0, 255));
        }
    }
}

void DialogueRenderer::draw_face(const uint8_t* face_rgba) {
    if (!face_rgba || !platform_) return;

    if (face_texture_) {
        platform_->destroy_texture(face_texture_);
    }

    face_texture_ = platform_->create_texture(FACE_W, FACE_H);
    if (!face_texture_) return;

    platform_->update_texture(face_texture_, face_rgba, FACE_W * 4);
    platform_->draw_texture(face_texture_, FACE_X, FACE_Y);

    platform_->draw_rect(FACE_X - 1, FACE_Y - 1, FACE_W + 2, FACE_H + 2,
                         180, 180, 220);
}

void DialogueRenderer::render(const std::string& speaker_name,
                                const std::string& text,
                                int32_t visible_chars,
                                const uint8_t* face_rgba) {
    if (!platform_) return;

    if (face_rgba) {
        const uint8_t* animated = get_animated_face(face_rgba);
        draw_face(animated);
    }

    platform_->fill_rect(BOX_X, BOX_Y, BOX_W, BOX_H, 0, 0, 40);
    platform_->draw_rect(BOX_X, BOX_Y, BOX_W, BOX_H, 180, 180, 220);
    platform_->draw_rect(BOX_X + 2, BOX_Y + 2, BOX_W - 4, BOX_H - 4,
                         100, 100, 160);

    if (!speaker_name.empty()) {
        font_->draw_text(BOX_X + 16, BOX_Y + 8, speaker_name,
                         255, 255, 100);
    }

    if (!text.empty() && visible_chars > 0) {
        size_t char_count = 0;
        size_t byte_pos = 0;
        while (byte_pos < text.size() && char_count < static_cast<size_t>(visible_chars)) {
            uint8_t b = static_cast<uint8_t>(text[byte_pos]);
            if (b < 0x80) {
                byte_pos++;
            } else {
                byte_pos += 2;
            }
            char_count++;
        }

        std::string visible_text = text.substr(0, byte_pos);

        int32_t max_line_w = BOX_W - 32;
        int32_t chars_per_line = max_line_w / FontRenderer::CHAR_WIDTH;

        int32_t cx = 0, cy = 0;
        size_t i = 0;
        while (i < visible_text.size()) {
            uint8_t b = static_cast<uint8_t>(visible_text[i]);
            int32_t char_bytes = (b < 0x80) ? 1 : 2;
            if (i + char_bytes > visible_text.size()) break;

            std::string_view ch(&visible_text[i], char_bytes);

            int32_t draw_x = TEXT_X + cx * FontRenderer::CHAR_WIDTH;
            int32_t draw_y = TEXT_Y + cy * FontRenderer::CHAR_HEIGHT;

            font_->draw_text(draw_x, draw_y, ch, 255, 255, 255);

            cx++;
            if (cx >= chars_per_line) {
                cx = 0;
                cy++;
            }
            i += char_bytes;
        }
    }
}

}  // namespace dlzr::presentation
