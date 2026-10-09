#pragma once

// forging_input.hpp — 锻造界面输入处理
// Forging/crafting screen input handler

#include "platform/platform.hpp"

#include <cstdint>

namespace dlzr::presentation {

enum class ForgingAction {
    NONE,
    BACK,
    CONFIRM,
    TAB_SWITCH,
    ITEM_SELECT,
    QUANTITY_UP,
    QUANTITY_DOWN,
};

class ForgingInput {
public:
    ForgingAction process_event(const platform::Event& event,
                                 int32_t& selected_tab,
                                 int32_t& selected_item,
                                 int32_t& quantity);

private:
    static constexpr int32_t TAB_X = 60;
    static constexpr int32_t TAB_Y = 80;
    static constexpr int32_t TAB_W = 80;
    static constexpr int32_t TAB_H = 30;
    static constexpr int32_t TAB_GAP = 10;

    static constexpr int32_t GRID_X = 220;
    static constexpr int32_t GRID_Y = 80;
    static constexpr int32_t CELL_W = 48;
    static constexpr int32_t CELL_H = 48;
    static constexpr int32_t COLS = 5;

    static constexpr int32_t BTN_X = 420;
    static constexpr int32_t BTN_Y = 360;
    static constexpr int32_t BTN_W = 80;
    static constexpr int32_t BTN_H = 30;
    static constexpr int32_t BTN_GAP = 10;
};

}  // namespace dlzr::presentation
