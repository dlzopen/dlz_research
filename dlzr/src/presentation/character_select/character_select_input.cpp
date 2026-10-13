#include "presentation/character_select/character_select_input.hpp"

namespace dlzr::presentation {

CharacterSelectAction CharacterSelectInput::process_event(
    const platform::Event& event,
    int32_t& selected_portrait,
    int32_t& selected_formation,
    int32_t& slider_value,
    int32_t& difficulty,
    std::string& player_name) {

    if (event.type == platform::Event::Type::MouseDown &&
        event.mouse_button == 1) {

        // Check portrait clicks
        for (int i = 0; i < 6; ++i) {
            if (event.mouse_x >= PORTRAIT_X[i] &&
                event.mouse_x <= PORTRAIT_X[i] + PORTRAIT_SIZE &&
                event.mouse_y >= PORTRAIT_Y &&
                event.mouse_y <= PORTRAIT_Y + 100) {
                selected_portrait = i;
                return CharacterSelectAction::SELECT_PORTRAIT;
            }
        }

        // Check formation button clicks
        for (int i = 0; i < 4; ++i) {
            if (event.mouse_x >= FORMATION_X[i] &&
                event.mouse_x <= FORMATION_X[i] + FORMATION_SIZE &&
                event.mouse_y >= FORMATION_Y &&
                event.mouse_y <= FORMATION_Y + FORMATION_SIZE) {
                selected_formation = i;
                return CharacterSelectAction::SELECT_FORMATION;
            }
        }

        // Check slider click
        if (event.mouse_x >= SLIDER_X &&
            event.mouse_x <= SLIDER_X + SLIDER_WIDTH &&
            event.mouse_y >= SLIDER_Y &&
            event.mouse_y <= SLIDER_Y + SLIDER_HEIGHT) {
            slider_value = ((event.mouse_x - SLIDER_X) * 40) / SLIDER_WIDTH + 60;
            if (slider_value < 60) slider_value = 60;
            if (slider_value > 100) slider_value = 100;
            return CharacterSelectAction::ADJUST_SLIDER;
        }

        // Check difficulty bar click
        if (event.mouse_x >= DIFFICULTY_X &&
            event.mouse_x <= DIFFICULTY_X + DIFFICULTY_WIDTH &&
            event.mouse_y >= DIFFICULTY_Y &&
            event.mouse_y <= DIFFICULTY_Y + DIFFICULTY_HEIGHT) {
            int32_t zone_width = DIFFICULTY_WIDTH / 3;
            difficulty = (event.mouse_x - DIFFICULTY_X) / zone_width;
            if (difficulty < 0) difficulty = 0;
            if (difficulty > 2) difficulty = 2;
            return CharacterSelectAction::SELECT_DIFFICULTY;
        }

        // Check button clicks
        for (int i = 0; i < 3; ++i) {
            if (event.mouse_x >= BUTTON_X[i] &&
                event.mouse_x <= BUTTON_X[i] + BUTTON_WIDTH &&
                event.mouse_y >= BUTTON_Y &&
                event.mouse_y <= BUTTON_Y + BUTTON_HEIGHT) {
                switch (i) {
                case 0: return CharacterSelectAction::CHANGE;
                case 1: return CharacterSelectAction::CONFIRM;
                case 2: return CharacterSelectAction::CANCEL;
                }
            }
        }
    }

    return CharacterSelectAction::NONE;
}

}  // namespace dlzr::presentation
