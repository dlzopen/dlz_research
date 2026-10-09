#pragma once

// 城镇输入处理 / Town input handler
// 处理建筑点击 / Handles building clicks

#include "platform/platform.hpp"
#include "core/data_model/types.hpp"

#include <cstdint>

namespace dlzr::presentation {

class TownRenderer;  // Forward declaration

enum class TownAction {
    NONE,
    ENTER_BUILDING,
    EXIT_TOWN
};

struct TownInputResult {
    TownAction action{TownAction::NONE};
    int32_t building_index{-1};  // 0-5
};

class TownInputHandler {
public:
    TownInputResult process_event(const platform::Event& event,
                                   const core::Town& town,
                                   const TownRenderer& renderer);

private:
};

}  // namespace dlzr::presentation
