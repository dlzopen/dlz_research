#include "presentation/character_select/character_select_input.hpp"

namespace dlzr::presentation {

static int32_t calc_slider_value(int32_t mouse_x) {
    int32_t val = ((mouse_x - 269) * 40) / 100 + 60;
    if (val < 60) val = 60;
    if (val > 100) val = 100;
    return val;
}

static bool hit_button(int32_t mx, int32_t my, int32_t idx) {
    int32_t btn_y = 425 + idx;
    return mx >= 138 + idx * 137 && mx <= 138 + idx * 137 + 88 &&
           my >= btn_y && my <= btn_y + 32;
}

CharacterSelectAction CharacterSelectInput::process_event(
    const platform::Event& event,
    int32_t& selected_portrait,
    int32_t& selected_formation,
    int32_t& slider_value,
    int32_t& difficulty,
    std::string& player_name,
    int32_t& hovered_button,
    int32_t& pressed_button,
    bool& slider_dragging) {

    // MouseMove: update hover, handle slider drag
    if (event.type == platform::Event::Type::MouseMove) {
        hovered_button = -1;
        for (int i = 0; i < 3; ++i) {
            if (hit_button(event.mouse_x, event.mouse_y, i)) {
                hovered_button = i;
                break;
            }
        }
        // Slider drag
        if (slider_dragging) {
            slider_value = calc_slider_value(event.mouse_x);
            return CharacterSelectAction::ADJUST_SLIDER;
        }
        return CharacterSelectAction::NONE;
    }

    // MouseUp: release button or stop slider drag
    if (event.type == platform::Event::Type::MouseUp &&
        event.mouse_button == 1) {
        if (slider_dragging) {
            slider_dragging = false;
            return CharacterSelectAction::NONE;
        }
        if (pressed_button >= 0) {
            int32_t btn = pressed_button;
            pressed_button = -1;
            if (hit_button(event.mouse_x, event.mouse_y, btn)) {
                switch (btn) {
                case 0: return CharacterSelectAction::CHANGE;
                case 1: return CharacterSelectAction::CONFIRM;
                case 2: return CharacterSelectAction::CANCEL;
                }
            }
        }
        return CharacterSelectAction::NONE;
    }

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

        // Check slider click/drag start
        if (event.mouse_x >= SLIDER_X &&
            event.mouse_x <= SLIDER_X + SLIDER_WIDTH &&
            event.mouse_y >= SLIDER_Y &&
            event.mouse_y <= SLIDER_Y + SLIDER_HEIGHT) {
            slider_value = calc_slider_value(event.mouse_x);
            slider_dragging = true;
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

        // Check button clicks - set pressed state, don't trigger action yet
        for (int i = 0; i < 3; ++i) {
            if (hit_button(event.mouse_x, event.mouse_y, i)) {
                pressed_button = i;
                return CharacterSelectAction::NONE;
            }
        }
    }

    return CharacterSelectAction::NONE;
}

}  // namespace dlzr::presentation
