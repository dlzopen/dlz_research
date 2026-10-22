#pragma once

#include "platform/platform.hpp"

#include <cstdint>

namespace dlzr::presentation {

enum class TitleAction {
    NONE,
    NEW_GAME,       // 初出茅庐 → Mode 4 (CHARACTER_SELECT)
    CONTINUE,       // 继往开来 → Mode 5 (CONTINUE_GAME)
    BATTLE_HERO,    // 战斗英雄 → Mode 2 (TITLE_RANKING)
    SUPPORT_MODEL,  // 支前模范 → Mode 6 (VICTORY_ANIMATION/ranking)
    REST_TROOPS,    // 部队休整 → Mode 3 (TITLE_REST)
    EXIT,
};

class TitleInput {
public:
    TitleAction process_event(const platform::Event& event,
                               int32_t& selected_option);

private:
    // Click regions for 5 menu options (right side of 640×480 screen)
    // Exact positions from RE: FUN_0042c7d4
    static constexpr int32_t MENU_X = 315;
    static constexpr int32_t MENU_WIDTH = 232;
    static constexpr int32_t MENU_Y[5] = {60, 132, 204, 270, 336};
    static constexpr int32_t MENU_HEIGHT = 56;
};

}  // namespace dlzr::presentation
