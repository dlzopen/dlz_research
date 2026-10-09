// level_select_input.cpp — 关卡选择界面输入处理实现
// Level selection screen input handler implementation

#include "presentation/strategic/level_select_input.hpp"
#include "core/data_model/constants.hpp"

namespace dlzr::presentation {

LevelSelectAction LevelSelectInput::process_event(const platform::Event& event,
                                                    int32_t& selected_level,
                                                    int32_t& scroll_offset) {
    if (event.type == platform::Event::Type::KeyDown) {
        if (event.key == 0x1B) {  // Escape
            return LevelSelectAction::BACK;
        }
        if (event.key == 0x0D) {  // Enter
            return LevelSelectAction::SELECT;
        }
        if (event.key == 0x40000052 || event.key == 'w') {  // Up
            if (selected_level >= COLS) {
                selected_level -= COLS;
                if (selected_level < scroll_offset) {
                    scroll_offset = selected_level;
                }
            }
            return LevelSelectAction::MOVE_UP;
        }
        if (event.key == 0x40000051 || event.key == 's') {  // Down
            if (selected_level + COLS < core::MAX_FIGHTS) {
                selected_level += COLS;
                if (selected_level >= scroll_offset + COLS * ROWS) {
                    scroll_offset += COLS;
                }
            }
            return LevelSelectAction::MOVE_DOWN;
        }
        if (event.key == 0x40000050 || event.key == 'a') {  // Left
            if (selected_level % COLS > 0) {
                --selected_level;
            }
            return LevelSelectAction::MOVE_LEFT;
        }
        if (event.key == 0x4000004F || event.key == 'd') {  // Right
            if (selected_level % COLS < COLS - 1 &&
                selected_level + 1 < core::MAX_FIGHTS) {
                ++selected_level;
            }
            return LevelSelectAction::MOVE_RIGHT;
        }
        if (event.key == 0x40000055) {  // Page Up
            scroll_offset -= COLS * ROWS;
            if (scroll_offset < 0) scroll_offset = 0;
            if (selected_level >= scroll_offset + COLS * ROWS) {
                selected_level = scroll_offset + COLS * ROWS - 1;
            }
            return LevelSelectAction::SCROLL_UP;
        }
        if (event.key == 0x40000056) {  // Page Down
            scroll_offset += COLS * ROWS;
            int32_t max_scroll = core::MAX_FIGHTS - COLS * ROWS;
            if (max_scroll < 0) max_scroll = 0;
            if (scroll_offset > max_scroll) scroll_offset = max_scroll;
            return LevelSelectAction::SCROLL_DOWN;
        }
    }

    if (event.type == platform::Event::Type::MouseDown &&
        event.mouse_button == 1) {
        int32_t mx = event.mouse_x;
        int32_t my = event.mouse_y;

        for (int i = 0; i < COLS * ROWS; ++i) {
            int32_t col = i % COLS;
            int32_t row = i / COLS;
            int32_t x = GRID_X + col * (CELL_W + 10);
            int32_t y = GRID_Y + row * (CELL_H + 8);

            if (mx >= x && mx <= x + CELL_W && my >= y && my <= y + CELL_H) {
                int32_t level = scroll_offset + i;
                if (level < core::MAX_FIGHTS) {
                    selected_level = level;
                    return LevelSelectAction::SELECT;
                }
            }
        }
    }

    if (event.type == platform::Event::Type::MouseDown &&
        event.mouse_button == 3) {
        return LevelSelectAction::BACK;
    }

    return LevelSelectAction::NONE;
}

}  // namespace dlzr::presentation
