#pragma once

#include "platform/platform.hpp"
#include "presentation/font/font_renderer.hpp"
#include "presentation/image/image_cache.hpp"

#include <cstdint>
#include <string>

namespace dlzr::presentation {

// 头像表情 / Face expression
enum class FaceExpression {
    NEUTRAL,
    HAPPY,
    ANGRY,
    SAD,
    SURPRISED,
};

class DialogueRenderer {
public:
    void init(platform::Platform& platform, FontRenderer& font,
              ImageCache& images);

    void render(const std::string& speaker_name,
                const std::string& text,
                int32_t visible_chars,
                const uint8_t* face_rgba = nullptr);

    // 更新动画状态 / Update animation state (call each frame)
    void update_animation(int32_t delta_ms, bool text_is_scrolling);

    // 设置表情 / Set expression
    void set_expression(FaceExpression expr) { expression_ = expr; }

    // 获取当前头像帧（含动画） / Get current face frame with animation applied
    // Returns modified RGBA buffer with blink/expression applied
    const uint8_t* get_animated_face(const uint8_t* base_face_rgba);

    static constexpr int32_t BOX_X = 32;
    static constexpr int32_t BOX_Y = 360;
    static constexpr int32_t BOX_W = 576;
    static constexpr int32_t BOX_H = 100;
    static constexpr int32_t TEXT_X = BOX_X + 16;
    static constexpr int32_t TEXT_Y = BOX_Y + 32;

    static constexpr int32_t FACE_X = 4;
    static constexpr int32_t FACE_Y = 340;
    static constexpr int32_t FACE_W = 80;
    static constexpr int32_t FACE_H = 100;

private:
    platform::Platform* platform_{nullptr};
    FontRenderer* font_{nullptr};
    ImageCache* images_{nullptr};

    platform::TextureHandle face_texture_{0};

    // 动画状态 / Animation state
    int32_t blink_timer_{0};           // 眨眼计时器 / Blink timer (ms)
    bool eyes_closed_{false};          // 眼睛是否闭合 / Are eyes closed
    int32_t mouth_frame_{0};           // 嘴部动画帧 / Mouth animation frame (0-2)
    FaceExpression expression_{FaceExpression::NEUTRAL};

    // 动画缓冲 / Animation buffer
    std::array<uint8_t, FACE_W * FACE_H * 4> animated_face_{};

    static constexpr int32_t BLINK_INTERVAL = 3000;  // 每3秒眨眼 / Blink every 3s
    static constexpr int32_t BLINK_DURATION = 150;   // 眨眼持续150ms / Blink lasts 150ms

    void draw_face(const uint8_t* face_rgba);
    void apply_blink(uint8_t* rgba);
    void apply_expression(uint8_t* rgba);
    void apply_mouth(uint8_t* rgba, int32_t frame);
};

}  // namespace dlzr::presentation
