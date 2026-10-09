// strategic_map_input.cpp — 战略地图输入处理实现

#include "presentation/strategic/strategic_map_input.hpp"
#include "presentation/strategic/strategic_map_renderer.hpp"

namespace dlzr::presentation {

MapInputResult StrategicMapInputHandler::process_event(
    const platform::Event& event,
    const core::GameData& game_data,
    int32_t current_town,
    const StrategicMapRenderer& renderer) {

    MapInputResult result;

    if (event.type == platform::Event::Type::MouseDown && event.mouse_button == 0) {
        int32_t town = renderer.town_at_position(event.mouse_x, event.mouse_y, game_data);
        if (town >= 0) {
            if (town == current_town) {
                result.action = MapAction::ENTER_TOWN;
            } else {
                result.action = MapAction::SELECT_TOWN;
            }
            result.town_index = town;
        }
    }

    return result;
}

}  // namespace dlzr::presentation
