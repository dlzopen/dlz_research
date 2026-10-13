#pragma once

#include "platform/platform.hpp"
#include "presentation/font/font_renderer.hpp"
#include "presentation/image/image_cache.hpp"
#include "app/resource/face_loader.hpp"

#include <cstdint>
#include <string>

namespace dlzr::presentation {

class CharacterSelectRenderer {
public:
    void init(platform::Platform& platform, FontRenderer& font,
              ImageCache& images, const app::FaceLoader& faces);

    void render(int32_t selected_portrait, int32_t selected_formation,
                int32_t slider_value, int32_t difficulty,
                const std::string& player_name);

private:
    platform::Platform* platform_{nullptr};
    FontRenderer* font_{nullptr};
    ImageCache* images_{nullptr};
    const app::FaceLoader* faces_{nullptr};

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
