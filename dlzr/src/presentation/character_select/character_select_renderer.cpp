#include "presentation/character_select/character_select_renderer.hpp"

#include "heartbeat.hpp"

#include <cstdio>

namespace dlzr::presentation {

void CharacterSelectRenderer::init(platform::Platform& platform,
                                    FontRenderer& font,
                                    ImageCache& images,
                                    const app::FaceLoader& faces,
                                    const app::ArmyLoader& army) {
    platform_ = &platform;
    font_ = &font;
    images_ = &images;
    faces_ = &faces;
    army_ = &army;
}

void CharacterSelectRenderer::trigger_formation_anim(int32_t formation_index) {
    if (formation_index < 0 || formation_index >= 4) return;
    anim_formation_ = formation_index;
    anim_frame_ = 0;
    anim_loop_count_ = 0;
    anim_timer_ = 0;
    anim_active_ = true;
}

void CharacterSelectRenderer::update_anim(uint32_t delta_ms) {
    if (!anim_active_) return;

    anim_timer_ += delta_ms;
    if (anim_timer_ >= ANIM_FRAME_INTERVAL) {
        anim_timer_ = 0;
        anim_frame_++;
        // Attack West has 2 frames (14, 15), loop 3 times then stop
        if (anim_frame_ >= 2) {
            anim_frame_ = 0;
            anim_loop_count_++;
            if (anim_loop_count_ >= ANIM_LOOP_COUNT) {
                anim_active_ = false;
                anim_formation_ = -1;
            }
        }
    }
}

void CharacterSelectRenderer::render(int32_t selected_portrait,
                                      int32_t selected_formation,
                                      int32_t slider_value,
                                      int32_t difficulty,
                                      const std::string& player_name,
                                      const char* formation_names[4],
                                      int32_t hovered_button,
                                      int32_t pressed_button,
                                      bool naming_mode) {
    if (!platform_) return;

    dlzr::hb_step("CS:render_start");

    // Background (640×480)
    images_->draw_image("MENU01.IMG", 0, 0);

    // Draw 6 character portraits from FACE.DAT (indices 103-108)
    if (!faces_loaded_) {
        static constexpr int32_t FACE_OFFSET = 0x67;
        for (int i = 0; i < 6; ++i) {
            const uint8_t* face_rgba = faces_->get_face_rgba(FACE_OFFSET + i);
            if (face_rgba) {
                face_textures_[i] = platform_->create_texture(80, 100);
                if (face_textures_[i]) {
                    platform_->update_texture(face_textures_[i], face_rgba, 80 * 4);
                }
            }
        }
        faces_loaded_ = true;
    }
    dlzr::hb_step("CS:faces_loaded");
    for (int i = 0; i < 6; ++i) {
        if (face_textures_[i]) {
            platform_->draw_texture(face_textures_[i], PORTRAIT_X[i], PORTRAIT_Y);
        }
    }

    // Draw selection highlight on selected portrait
    if (selected_portrait >= 0 && selected_portrait < 6) {
        platform_->draw_rect(PORTRAIT_X[selected_portrait] - 2,
                            PORTRAIT_Y - 2, 84, 104, 255, 255, 0);
    }

    // Draw 4 formation icons from ARMY.DAT
    // RE: class indices [1,4,8,11] → ARMY.DAT entries [30,40,32,54]
    static constexpr int32_t FORMATION_ENTRIES[4] = {30, 40, 32, 54};
    
    // Load all 19 frames for each formation if not already loaded
    if (army_ && !formation_frames_loaded_) {
        for (int i = 0; i < 4; ++i) {
            for (int f = 0; f < 19; ++f) {
                const uint8_t* frame_rgba = army_->get_frame_rgba(FORMATION_ENTRIES[i], f);
                if (frame_rgba) {
                    formation_frame_textures_[i][f] = platform_->create_texture(48, 48);
                    if (formation_frame_textures_[i][f]) {
                        platform_->update_texture(formation_frame_textures_[i][f], frame_rgba, 48 * 4);
                    }
                }
            }
        }
        formation_frames_loaded_ = true;
    }
    dlzr::hb_step("CS:formation_frames_loaded");
    
    // Draw formations - use animation frame for selected formation if animating
    for (int i = 0; i < 4; ++i) {
        int32_t frame_to_draw = 0;  // Default: idle left (frame 0)
        
        // If this formation is animating, use attack west frames (14, 15)
        if (anim_active_ && anim_formation_ == i) {
            frame_to_draw = 14 + anim_frame_;  // Frame 14 or 15
        }
        
        if (formation_frame_textures_[i][frame_to_draw]) {
            platform_->draw_texture(formation_frame_textures_[i][frame_to_draw],
                                   FORMATION_X[i], FORMATION_Y);
        }
    }

    // Draw formation selection highlight
    if (selected_formation >= 0 && selected_formation < 4) {
        platform_->draw_rect(FORMATION_X[selected_formation] - 2,
                            FORMATION_Y - 2, 52, 52, 255, 255, 0);
    }

    // Draw formation class names below each icon (RE: y=0xff=255)
    if (formation_names) {
        for (int i = 0; i < 4; ++i) {
            font_->draw_text(FORMATION_X[i], 255, formation_names[i], 255, 255, 255);
        }
    }

    // Draw slider bar (RE: bar at (255,287), 128×16)
    // Palette: 0xED = RGB(72,89,198) blue (filled), 0xB7 = RGB(255,0,0) red (unfilled)
    int32_t filled_w = ((slider_value - 60) * SLIDER_WIDTH) / 40;
    if (filled_w < 0) filled_w = 0;
    if (filled_w > SLIDER_WIDTH) filled_w = SLIDER_WIDTH;
    platform_->fill_rect(SLIDER_X, SLIDER_Y, filled_w, SLIDER_HEIGHT,
                        72, 89, 198);
    platform_->fill_rect(SLIDER_X + filled_w, SLIDER_Y,
                        SLIDER_WIDTH - filled_w, SLIDER_HEIGHT,
                        255, 0, 0);

    // Draw slider thumb from MENU30.IMG (RE: thumb at ((value-60)*100/40 + 261, 283), 17×25, transparent color 0xF0)
    int32_t thumb_x = ((slider_value - 60) * 100) / 40 + 261;
    images_->draw_image_with_transparent_color("MENU30.IMG", thumb_x, 283, 0xF0);

    // Draw slider numeric values
    // Intelligence (智力) = 160 - value, at (217, 287)
    // Strength (武力) = value, at (392, 287)
    char intel_str[8], str_str[8];
    std::snprintf(intel_str, sizeof(intel_str), "%d", 160 - slider_value);
    std::snprintf(str_str, sizeof(str_str), "%d", slider_value);
    font_->draw_text(217, 287, intel_str, 255, 255, 255);
    font_->draw_text(392, 287, str_str, 255, 255, 255);

    // Draw difficulty zone highlight
    // RE: clear entire bar to 0xF0 (black), then fill from left with 0xB7 (red)
    // Palette: 0xF0 = RGB(0,0,0) black, 0xB7 = RGB(255,0,0) red
    // RE zone width = 0xac = 172 (NOT 344/3=114)
    static constexpr int32_t DIFFICULTY_ZONE_WIDTH = 172;
    platform_->fill_rect(DIFFICULTY_X, DIFFICULTY_Y, DIFFICULTY_WIDTH, DIFFICULTY_HEIGHT,
                        0, 0, 0);
    if (difficulty == 1) {
        platform_->fill_rect(DIFFICULTY_X, DIFFICULTY_Y,
                            DIFFICULTY_ZONE_WIDTH, DIFFICULTY_HEIGHT,
                            255, 0, 0);
    } else if (difficulty == 2) {
        platform_->fill_rect(DIFFICULTY_X, DIFFICULTY_Y,
                            DIFFICULTY_WIDTH, DIFFICULTY_HEIGHT,
                            255, 0, 0);
    }

    // Draw difficulty text label (RE: at (499, 331))
    // 简单/普通/困难
    const char* difficulty_labels[3] = {
        "\xBC\xF2\xB5\xA5",  // 简单
        "\xC6\xD5\xCD\xA8",  // 普通
        "\xC0\xA7\xC4\xD1",  // 困难
    };
    if (difficulty >= 0 && difficulty < 3) {
        font_->draw_text(499, 331, difficulty_labels[difficulty], 255, 255, 255);
    }

    // Draw player name (GBK encoded) at RE position (108, 374)
    if (!player_name.empty()) {
        font_->draw_text(108, 374, player_name.c_str(), 255, 255, 255);
    }

    // Draw cursor when in naming mode
    if (naming_mode) {
        int32_t name_width = 0;
        if (!player_name.empty()) {
            int32_t h;
            font_->measure_text(player_name.c_str(), name_width, h);
        }
        // Draw blinking cursor (simple vertical line)
        int32_t cursor_x = 108 + name_width + 2;
        int32_t cursor_y = 374;
        int32_t cursor_height = 16;
        platform_->fill_rect(cursor_x, cursor_y, 2, cursor_height, 255, 255, 255);
    }

    // Draw 3 buttons (RE: positions (138,425), (275,426), (412,427))
    // BUTTON22.IMG: 88×64, normal at (0,0,88,32), pressed at (0,32,88,32)
    static constexpr int32_t BTN_Y[3] = {425, 426, 427};
    const char* button_labels[3] = {
        "\xB8\xFC\xB8\xC4",  // 更改
        "\xC8\xB7\xB6\xA8",  // 确定
        "\xC8\xA1\xCF\xFB",  // 取消
    };
    for (int i = 0; i < 3; ++i) {
        bool is_pressed = (pressed_button == i);
        if (is_pressed) {
            images_->draw_image("BUTTON22.IMG", BUTTON_X[i], BTN_Y[i],
                               0, 32, 88, 32);
        } else {
            images_->draw_image("BUTTON22.IMG", BUTTON_X[i], BTN_Y[i],
                               0, 0, 88, 32);
        }
        int32_t text_w, text_h;
        font_->measure_text(button_labels[i], text_w, text_h);
        int32_t text_x = BUTTON_X[i] + (BUTTON_WIDTH - text_w) / 2;
        int32_t text_y = BTN_Y[i] + (BUTTON_HEIGHT - text_h) / 2;
        font_->draw_text(text_x, text_y, button_labels[i], 255, 255, 255);
    }
    dlzr::hb_step("CS:render_done");
}

}  // namespace dlzr::presentation
