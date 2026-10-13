#pragma once

#include "platform/platform.hpp"

#include <cstdint>

namespace dlzr::presentation {

enum class TitleAction {
    NONE,
    NEW_GAME,
    LOAD_GAME,
    CYCLE_DIFFICULTY,
    EXIT,
};

class TitleInput {
public:
    TitleAction process_event(const platform::Event& event,
                               int32_t& selected_option);

private:
    static constexpr int32_t MENU_X = 320;
    static constexpr int32_t MENU_Y = 520;
    static constexpr int32_t MENU_SPACING = 25;
    static constexpr int32_t MENU_WIDTH = 160;
    static constexpr int32_t MENU_HEIGHT = 25;
};

}  // namespace dlzr::presentation
