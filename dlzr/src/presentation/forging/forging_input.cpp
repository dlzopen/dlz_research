// forging_input.cpp — 锻造界面输入处理实现
// Forging/crafting screen input handler implementation

#include "presentation/forging/forging_input.hpp"

namespace dlzr::presentation {

ForgingAction ForgingInput::process_event(const platform::Event& event,
                                           int32_t& selected_tab,
                                           int32_t& selected_item,
                                           int32_t& quantity) {
    if (event.type == platform::Event::Type::KeyDown) {
        if (event.key == 0x1B) {  // Escape
            return ForgingAction::BACK;
        }
        if (event.key == 0x0D) {  // Enter
            return ForgingAction::CONFIRM;
        }
        if (event.key == 0x40000052 || event.key == 'w') {  // Up
            selected_item -= 5;
            if (selected_item < 0) selected_item += 10;
            return ForgingAction::ITEM_SELECT;
        }
        if (event.key == 0x40000051 || event.key == 's') {  // Down
            selected_item = (selected_item + 5) % 10;
            return ForgingAction::ITEM_SELECT;
        }
        if (event.key == 0x40000050 || event.key == 'a') {  // Left
            selected_item--;
            if (selected_item < 0) selected_item = 9;
            return ForgingAction::ITEM_SELECT;
        }
        if (event.key == 0x4000004F || event.key == 'd') {  // Right
            selected_item = (selected_item + 1) % 10;
            return ForgingAction::ITEM_SELECT;
        }
        if (event.key == '+' || event.key == '=') {
            quantity++;
            return ForgingAction::QUANTITY_UP;
        }
        if (event.key == '-') {
            if (quantity > 1) quantity--;
            return ForgingAction::QUANTITY_DOWN;
        }
        if (event.key >= '1' && event.key <= '4') {
            selected_tab = event.key - '1';
            return ForgingAction::TAB_SWITCH;
        }
    }

    if (event.type == platform::Event::Type::MouseDown &&
        event.mouse_button == 1) {
        int32_t mx = event.mouse_x;
        int32_t my = event.mouse_y;

        for (int i = 0; i < 4; ++i) {
            int32_t x = TAB_X + (i % 2) * (TAB_W + TAB_GAP);
            int32_t y = TAB_Y + (i / 2) * (TAB_H + TAB_GAP);
            if (mx >= x && mx <= x + TAB_W && my >= y && my <= y + TAB_H) {
                selected_tab = i;
                return ForgingAction::TAB_SWITCH;
            }
        }

        for (int i = 0; i < 10; ++i) {
            int32_t col = i % COLS;
            int32_t row = i / COLS;
            int32_t x = GRID_X + col * (CELL_W + 4);
            int32_t y = GRID_Y + row * (CELL_H + 4);
            if (mx >= x && mx <= x + CELL_W && my >= y && my <= y + CELL_H) {
                selected_item = i;
                return ForgingAction::ITEM_SELECT;
            }
        }

        for (int i = 0; i < 4; ++i) {
            int32_t x = BTN_X + (i % 2) * (BTN_W + BTN_GAP);
            int32_t y = BTN_Y + (i / 2) * (BTN_H + BTN_GAP);
            if (mx >= x && mx <= x + BTN_W && my >= y && my <= y + BTN_H) {
                switch (i) {
                case 0: return ForgingAction::BACK;
                case 1: return ForgingAction::CONFIRM;
                case 2: return ForgingAction::QUANTITY_UP;
                case 3: return ForgingAction::QUANTITY_DOWN;
                }
            }
        }
    }

    return ForgingAction::NONE;
}

}  // namespace dlzr::presentation
