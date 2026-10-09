#include "presentation/dialogue/dialogue_renderer.hpp"

#include <algorithm>

namespace dlzr::presentation {

void DialogueRenderer::init(platform::Platform& platform, FontRenderer& font,
                              ImageCache& images) {
    platform_ = &platform;
    font_ = &font;
    images_ = &images;
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
        draw_face(face_rgba);
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
