#pragma once

// level_select_renderer.hpp — 关卡选择界面渲染器
// Level selection screen renderer

#include "logic/game_state_manager.hpp"
#include "platform/platform.hpp"
#include "presentation/font/font_renderer.hpp"

#include <cstdint>

namespace dlzr::presentation {

class LevelSelectRenderer {
public:
    void init(platform::Platform& platform, FontRenderer& font);

    void render(const logic::GameStateManager& gsm, int32_t selected_level,
                int32_t scroll_offset);

private:
    platform::Platform* platform_{nullptr};
    FontRenderer* font_{nullptr};

    static constexpr int32_t COLS = 4;
    static constexpr int32_t ROWS = 8;
    static constexpr int32_t CELL_W = 120;
    static constexpr int32_t CELL_H = 40;
    static constexpr int32_t GRID_X = 80;
    static constexpr int32_t GRID_Y = 100;
};

}  // namespace dlzr::presentation
