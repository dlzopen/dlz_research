#pragma once

// level_select_input.hpp — 关卡选择界面输入处理
// Level selection screen input handler

#include "platform/platform.hpp"

#include <cstdint>

namespace dlzr::presentation {

enum class LevelSelectAction {
    NONE,
    BACK,
    SELECT,
    SCROLL_UP,
    SCROLL_DOWN,
    MOVE_LEFT,
    MOVE_RIGHT,
    MOVE_UP,
    MOVE_DOWN,
};

class LevelSelectInput {
public:
    LevelSelectAction process_event(const platform::Event& event,
                                     int32_t& selected_level,
                                     int32_t& scroll_offset);

private:
    static constexpr int32_t COLS = 4;
    static constexpr int32_t ROWS = 8;
    static constexpr int32_t CELL_W = 120;
    static constexpr int32_t CELL_H = 40;
    static constexpr int32_t GRID_X = 80;
    static constexpr int32_t GRID_Y = 100;
};

}  // namespace dlzr::presentation
