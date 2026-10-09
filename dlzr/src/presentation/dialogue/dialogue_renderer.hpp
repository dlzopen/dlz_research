#pragma once

#include "platform/platform.hpp"
#include "presentation/font/font_renderer.hpp"
#include "presentation/image/image_cache.hpp"

#include <cstdint>
#include <string>

namespace dlzr::presentation {

class DialogueRenderer {
public:
    void init(platform::Platform& platform, FontRenderer& font,
              ImageCache& images);

    void render(const std::string& speaker_name,
                const std::string& text,
                int32_t visible_chars,
                const uint8_t* face_rgba = nullptr);

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

    void draw_face(const uint8_t* face_rgba);
};

}  // namespace dlzr::presentation
