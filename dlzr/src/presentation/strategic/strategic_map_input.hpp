#pragma once

// 战略地图输入处理 / Strategic map input handler
// 处理地图点击选择城镇 / Handles map clicks to select towns

#include "platform/platform.hpp"
#include "core/data_model/game_data.hpp"

#include <cstdint>

namespace dlzr::presentation {

class StrategicMapRenderer;  // Forward declaration

enum class MapAction {
    NONE,
    SELECT_TOWN,
    ENTER_TOWN,
    START_BATTLE
};

struct MapInputResult {
    MapAction action{MapAction::NONE};
    int32_t town_index{-1};
};

class StrategicMapInputHandler {
public:
    MapInputResult process_event(const platform::Event& event,
                                  const core::GameData& game_data,
                                  int32_t current_town,
                                  const presentation::StrategicMapRenderer& renderer);

private:
};

}  // namespace dlzr::presentation
