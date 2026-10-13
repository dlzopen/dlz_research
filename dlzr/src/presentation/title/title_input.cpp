#include "presentation/title/title_input.hpp"

namespace dlzr::presentation {

TitleAction TitleInput::process_event(const platform::Event& event,
                                       int32_t& selected_option) {
    if (event.type == platform::Event::Type::KeyDown) {
        if (event.key == 0x40000052 || event.key == 'w') {
            selected_option--;
            if (selected_option < 0) selected_option = 4;
        } else if (event.key == 0x40000051 || event.key == 's') {
            selected_option++;
            if (selected_option > 4) selected_option = 0;
        } else if (event.key == 0x0D || event.key == ' ') {
            switch (selected_option) {
            case 0: return TitleAction::NEW_GAME;
            case 1: return TitleAction::CONTINUE;
            case 2: return TitleAction::BATTLE_HERO;
            case 3: return TitleAction::SUPPORT_MODEL;
            case 4: return TitleAction::REST_TROOPS;
            }
        }
    }

    if (event.type == platform::Event::Type::MouseDown &&
        event.mouse_button == 1) {
        for (int i = 0; i < 5; ++i) {
            if (event.mouse_x >= MENU_X &&
                event.mouse_x <= MENU_X + MENU_WIDTH &&
                event.mouse_y >= MENU_Y[i] &&
                event.mouse_y <= MENU_Y[i] + MENU_HEIGHT) {
                selected_option = i;
                switch (i) {
                case 0: return TitleAction::NEW_GAME;
                case 1: return TitleAction::CONTINUE;
                case 2: return TitleAction::BATTLE_HERO;
                case 3: return TitleAction::SUPPORT_MODEL;
                case 4: return TitleAction::REST_TROOPS;
                }
            }
        }
    }

    return TitleAction::NONE;
}

}  // namespace dlzr::presentation
