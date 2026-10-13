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
        NEW_GAME = 0,       // 初出茅庐
        CONTINUE = 1,       // 继往开来
        BATTLE_HERO = 2,    // 战斗英雄
        SUPPORT_MODEL = 3,  // 支前模范
        REST_TROOPS = 4,    // 部队休整
        OPTION_COUNT = 5,
    };

private:
    platform::Platform* platform_{nullptr};
    FontRenderer* font_{nullptr};
    ImageCache* images_{nullptr};
};

}  // namespace dlzr::presentation
