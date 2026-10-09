// town_input.cpp — 城镇输入处理实现

#include "presentation/strategic/town_input.hpp"
#include "presentation/strategic/town_renderer.hpp"

namespace dlzr::presentation {

TownInputResult TownInputHandler::process_event(
    const platform::Event& event,
    const core::Town& town,
    const TownRenderer& renderer) {

    TownInputResult result;

    if (event.type == platform::Event::Type::MouseDown && event.mouse_button == 0) {
        int32_t building = renderer.building_at_position(event.mouse_x, event.mouse_y, town);
        if (building >= 0) {
            result.action = TownAction::ENTER_BUILDING;
            result.building_index = building;
        }
    }

    if (event.type == platform::Event::Type::MouseDown && event.mouse_button == 1) {
        result.action = TownAction::EXIT_TOWN;
    }

    return result;
}

}  // namespace dlzr::presentation
