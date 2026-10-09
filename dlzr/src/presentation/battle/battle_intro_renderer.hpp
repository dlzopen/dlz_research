#pragma once

// battle_intro_renderer.hpp — 战斗开场简报渲染器
// Battle intro/briefing screen renderer

#include "platform/platform.hpp"
#include "presentation/font/font_renderer.hpp"
#include "core/data_model/types.hpp"

#include <cstdint>

namespace dlzr::presentation {

class BattleIntroRenderer {
public:
    void init(platform::Platform& platform, FontRenderer& font);

    void render(const core::FightIntro& intro);

private:
    platform::Platform* platform_{nullptr};
    FontRenderer* font_{nullptr};

    static constexpr int32_t TITLE_Y = 80;
    static constexpr int32_t INFO_Y = 160;
    static constexpr int32_t LINE_HEIGHT = 32;
    static constexpr int32_t TEXT_X = 120;
};

}  // namespace dlzr::presentation
