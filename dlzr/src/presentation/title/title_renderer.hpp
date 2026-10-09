#pragma once

#include "platform/platform.hpp"
#include "presentation/font/font_renderer.hpp"
#include "presentation/image/image_cache.hpp"

#include <cstdint>

namespace dlzr::presentation {

class TitleRenderer {
public:
    void init(platform::Platform& platform, FontRenderer& font,
              ImageCache& images);

    void render(int32_t selected_option, int32_t difficulty_level = 1);

    enum Option {
        NEW_GAME = 0,
        LOAD_GAME = 1,
        DIFFICULTY = 2,
        EXIT = 3,
        OPTION_COUNT = 4,
    };

private:
    platform::Platform* platform_{nullptr};
    FontRenderer* font_{nullptr};
    ImageCache* images_{nullptr};

    static constexpr int32_t MENU_X = 240;
    static constexpr int32_t MENU_Y = 280;
    static constexpr int32_t MENU_SPACING = 40;
};

}  // namespace dlzr::presentation
