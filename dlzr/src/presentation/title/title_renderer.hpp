#pragma once

#include "platform/platform.hpp"
#include "presentation/font/font_renderer.hpp"
#include "presentation/image/image_cache.hpp"

#include <cstdint>
#include <string>

namespace dlzr::presentation {

class TitleRenderer {
public:
    void init(platform::Platform& platform, FontRenderer& font,
              ImageCache& images, const std::string& game_data_dir);

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
    void load_background();
    void load_animation_frames();

    platform::Platform* platform_{nullptr};
    FontRenderer* font_{nullptr};
    ImageCache* images_{nullptr};
    std::string game_dir_;

    platform::TextureHandle bg_texture_{0};
    bool bg_loaded_{false};

    static constexpr int32_t ANIM_FRAME_COUNT = 30;
    static constexpr int32_t ANIM_FRAME_WIDTH = 56;
    static constexpr int32_t ANIM_FRAME_HEIGHT = 48;
    static constexpr int32_t ANIM_X = 250;
    static constexpr int32_t MENU_Y[5] = {60, 132, 204, 270, 336};

    platform::TextureHandle anim_frames_[ANIM_FRAME_COUNT]{};
    bool anim_loaded_{false};
    int32_t anim_frame_{0};
    uint32_t last_anim_time_{0};
};

}  // namespace dlzr::presentation
