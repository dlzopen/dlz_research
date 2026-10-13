#pragma once

#include "platform/platform.hpp"

#include <cstdint>

namespace dlzr::presentation {

enum class TitleAction {
    NONE,
    NEW_GAME,       // 初出茅庐
    CONTINUE,       // 继往开来
    BATTLE_HERO,    // 战斗英雄
    SUPPORT_MODEL,  // 支前模范
    REST_TROOPS,    // 部队休整
    EXIT,
};

class TitleInput {
public:
    TitleAction process_event(const platform::Event& event,
                               int32_t& selected_option);

private:
    // Click regions for 5 menu options (right side of 640×480 screen)
    static constexpr int32_t MENU_X = 350;
    static constexpr int32_t MENU_WIDTH = 250;
    static constexpr int32_t MENU_Y[5] = {50, 130, 210, 290, 370};
    static constexpr int32_t MENU_HEIGHT = 60;
};

}  // namespace dlzr::presentation
