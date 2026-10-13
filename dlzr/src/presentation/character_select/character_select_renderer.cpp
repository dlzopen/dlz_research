#include "presentation/character_select/character_select_renderer.hpp"

namespace dlzr::presentation {

void CharacterSelectRenderer::init(platform::Platform& platform,
                                    FontRenderer& font,
                                    ImageCache& images,
                                    const app::FaceLoader& faces) {
    platform_ = &platform;
    font_ = &font;
    images_ = &images;
    faces_ = &faces;
}

void CharacterSelectRenderer::render(int32_t selected_portrait,
                                      int32_t selected_formation,
                                      int32_t slider_value,
                                      int32_t difficulty,
                                      const std::string& player_name) {
    if (!platform_) return;

    // Background (640×480)
    images_->draw_image("MENU01.IMG", 0, 0);

    // Draw 6 character portraits from FACE.DAT
    for (int i = 0; i < 6; ++i) {
        const uint8_t* face_rgba = faces_->get_face_rgba(i);
        if (face_rgba) {
            auto tex = platform_->create_texture(80, 100);
            if (tex) {
                platform_->update_texture(tex, face_rgba, 80 * 4);
                platform_->draw_texture(tex, PORTRAIT_X[i], PORTRAIT_Y);
            }
        }
    }

    // Draw selection highlight on selected portrait
    if (selected_portrait >= 0 && selected_portrait < 6) {
        platform_->draw_rect(PORTRAIT_X[selected_portrait] - 2,
                            PORTRAIT_Y - 2, 84, 104, 255, 255, 0);
    }

    // Draw formation selection highlight
    if (selected_formation >= 0 && selected_formation < 4) {
        platform_->draw_rect(FORMATION_X[selected_formation] - 2,
                            FORMATION_Y - 2, 52, 52, 255, 255, 0);
    }

    // Draw slider thumb position
    int32_t thumb_x = SLIDER_X + ((slider_value - 60) * SLIDER_WIDTH) / 40;
    platform_->fill_rect(thumb_x - 4, SLIDER_Y - 4, 8, SLIDER_HEIGHT + 8,
                        255, 255, 255);

    // Draw difficulty selection highlight
    int32_t zone_width = DIFFICULTY_WIDTH / 3;
    platform_->draw_rect(DIFFICULTY_X + difficulty * zone_width,
                        DIFFICULTY_Y, zone_width, DIFFICULTY_HEIGHT,
                        255, 255, 0);

    // Draw player name (GBK encoded)
    if (!player_name.empty()) {
        font_->draw_text(50, 340, player_name.c_str(), 255, 255, 255);
    }

    // Draw 3 buttons with text (BUTTON22.IMG has 2 states: 88×32 each)
    // Button 0: 更改, Button 1: 确定, Button 2: 取消
    const char* button_labels[3] = {
        "\xB8\xFC\xB8\xC4",  // 更改
        "\xC8\xB7\xB6\xA8",  // 确定
        "\xC8\xA1\xCF\xFB",  // 取消
    };
    for (int i = 0; i < 3; ++i) {
        // Draw button background (use first 32px of BUTTON22.IMG)
        images_->draw_image("BUTTON22.IMG", BUTTON_X[i], BUTTON_Y, 88, 32);
        // Draw button text centered
        int32_t text_w, text_h;
        font_->measure_text(button_labels[i], text_w, text_h);
        int32_t text_x = BUTTON_X[i] + (BUTTON_WIDTH - text_w) / 2;
        int32_t text_y = BUTTON_Y + (BUTTON_HEIGHT - text_h) / 2;
        font_->draw_text(text_x, text_y, button_labels[i], 255, 255, 255);
    }
}

}  // namespace dlzr::presentation
