#include "presentation/title/title_input.hpp"

namespace dlzr::presentation {

TitleAction TitleInput::process_event(const platform::Event& event,
                                       int32_t& selected_option) {
    if (event.type == platform::Event::Type::KeyDown) {
        if (event.key == 0x40000052 || event.key == 'w') {
            selected_option--;
            if (selected_option < 0) selected_option = 3;
        } else if (event.key == 0x40000051 || event.key == 's') {
            selected_option++;
            if (selected_option > 3) selected_option = 0;
        } else if (event.key == 0x0D || event.key == ' ') {
            switch (selected_option) {
            case 0: return TitleAction::NEW_GAME;
            case 1: return TitleAction::LOAD_GAME;
            case 2: return TitleAction::CYCLE_DIFFICULTY;
            case 3: return TitleAction::EXIT;
            }
        }
    }

    if (event.type == platform::Event::Type::MouseDown &&
        event.mouse_button == 1) {
        for (int i = 0; i < 4; ++i) {
            int32_t y = MENU_Y + i * MENU_SPACING;
            if (event.mouse_x >= MENU_X - MENU_WIDTH / 2 &&
                event.mouse_x <= MENU_X + MENU_WIDTH / 2 &&
                event.mouse_y >= y &&
                event.mouse_y <= y + MENU_HEIGHT) {
                selected_option = i;
                switch (i) {
                case 0: return TitleAction::NEW_GAME;
                case 1: return TitleAction::LOAD_GAME;
                case 2: return TitleAction::CYCLE_DIFFICULTY;
                case 3: return TitleAction::EXIT;
                }
            }
        }
    }

    return TitleAction::NONE;
}

}  // namespace dlzr::presentation
