#pragma once

#include "platform/platform.hpp"
#include "presentation/font/font_renderer.hpp"
#include "presentation/image/image_cache.hpp"
#include "app/resource/face_loader.hpp"
#include "app/resource/army_loader.hpp"

#include <cstdint>
#include <string>

namespace dlzr::presentation {

class CharacterSelectRenderer {
public:
    void init(platform::Platform& platform, FontRenderer& font,
              ImageCache& images, const app::FaceLoader& faces,
              const app::ArmyLoader& army);

    void render(int32_t selected_portrait, int32_t selected_formation,
                int32_t slider_value, int32_t difficulty,
                const std::string& player_name,
                const char* formation_names[4],
                int32_t hovered_button = -1, int32_t pressed_button = -1,
                bool naming_mode = false);

    // Trigger attack animation for selected formation
    void trigger_formation_anim(int32_t formation_index);

    // Update animation timer (call each frame with delta_ms)
    void update_anim(uint32_t delta_ms);

private:
    platform::Platform* platform_{nullptr};
    FontRenderer* font_{nullptr};
    ImageCache* images_{nullptr};
    const app::FaceLoader* faces_{nullptr};
    const app::ArmyLoader* army_{nullptr};

    platform::TextureHandle face_textures_[6]{0, 0, 0, 0, 0, 0};
    bool faces_loaded_{false};

    platform::TextureHandle formation_textures_[4]{0, 0, 0, 0};
    bool formations_loaded_{false};

    // Animation state for selected formation
    // Attack West frames: 14, 15 (alternating), loops 3 times
    int32_t anim_formation_{-1};          // Which formation is animating (-1 = none)
    int32_t anim_frame_{0};               // Current animation frame (0 or 1)
    int32_t anim_loop_count_{0};          // Number of completed loops
    uint32_t anim_timer_{0};              // Timer for frame switching
    bool anim_active_{false};             // Is animation playing
    static constexpr uint32_t ANIM_FRAME_INTERVAL = 150;  // ms between frames
    static constexpr int32_t ANIM_LOOP_COUNT = 3;           // Loop 3 times

    // Pre-cached textures for all 19 frames of each formation
    // [formation_index][frame_index]
    platform::TextureHandle formation_frame_textures_[4][19]{};
    bool formation_frames_loaded_{false};

    // Portrait positions (6 portraits, 80×100 each)
    static constexpr int32_t PORTRAIT_Y = 86;
    static constexpr int32_t PORTRAIT_X[6] = {40, 136, 232, 328, 424, 520};

    // Formation button positions (4 buttons, 48×48 each)
    static constexpr int32_t FORMATION_Y = 203;
    static constexpr int32_t FORMATION_X[4] = {163, 251, 339, 427};

    // Slider position
    static constexpr int32_t SLIDER_X = 255;
    static constexpr int32_t SLIDER_Y = 287;
    static constexpr int32_t SLIDER_WIDTH = 128;
    static constexpr int32_t SLIDER_HEIGHT = 16;

    // Difficulty bar position
    static constexpr int32_t DIFFICULTY_X = 249;
    static constexpr int32_t DIFFICULTY_Y = 371;
    static constexpr int32_t DIFFICULTY_WIDTH = 344;
    static constexpr int32_t DIFFICULTY_HEIGHT = 22;

    // Button positions (3 buttons)
    static constexpr int32_t BUTTON_Y = 425;
    static constexpr int32_t BUTTON_X[3] = {138, 275, 412};
    static constexpr int32_t BUTTON_WIDTH = 88;
    static constexpr int32_t BUTTON_HEIGHT = 32;
};

}  // namespace dlzr::presentation
