#pragma once

#include "platform/platform.hpp"

#include <cstdint>
#include <string>

namespace dlzr::presentation {

enum class CharacterSelectAction {
    NONE,
    SELECT_PORTRAIT,
    SELECT_FORMATION,
    ADJUST_SLIDER,
    SELECT_DIFFICULTY,
    EDIT_NAME,
    CONFIRM,
    CANCEL,
    CHANGE,
};

class CharacterSelectInput {
public:
    CharacterSelectAction process_event(const platform::Event& event,
                                         int32_t& selected_portrait,
                                         int32_t& selected_formation,
                                         int32_t& slider_value,
                                         int32_t& difficulty,
                                         std::string& player_name,
                                         int32_t& hovered_button,
                                         int32_t& pressed_button,
                                         bool& slider_dragging);

private:
    // Portrait click regions
    static constexpr int32_t PORTRAIT_Y = 86;
    static constexpr int32_t PORTRAIT_X[6] = {40, 136, 232, 328, 424, 520};
    static constexpr int32_t PORTRAIT_SIZE = 80;

    // Formation button click regions
    static constexpr int32_t FORMATION_Y = 203;
    static constexpr int32_t FORMATION_X[4] = {163, 251, 339, 427};
    static constexpr int32_t FORMATION_SIZE = 48;

    // Slider click region (RE: clickable start=269, width=0x80=128)
    static constexpr int32_t SLIDER_X = 269;
    static constexpr int32_t SLIDER_Y = 287;
    static constexpr int32_t SLIDER_WIDTH = 128;
    static constexpr int32_t SLIDER_HEIGHT = 16;

    // Difficulty bar click region
    static constexpr int32_t DIFFICULTY_X = 249;
    static constexpr int32_t DIFFICULTY_Y = 371;
    static constexpr int32_t DIFFICULTY_WIDTH = 344;
    static constexpr int32_t DIFFICULTY_HEIGHT = 22;

    // Button click regions (RE: Y positions 425, 426, 427)
    static constexpr int32_t BUTTON_Y_BASE = 425;
    static constexpr int32_t BUTTON_X[3] = {138, 275, 412};
    static constexpr int32_t BUTTON_WIDTH = 88;
    static constexpr int32_t BUTTON_HEIGHT = 32;
};

}  // namespace dlzr::presentation
