#pragma once

// equipment_renderer.hpp — 装备/物品画面渲染器
// Equipment/inventory screen renderer

#include "platform/platform.hpp"
#include "presentation/font/font_renderer.hpp"
#include "core/data_model/game_data.hpp"

#include <cstdint>

namespace dlzr::presentation {

class EquipmentRenderer {
public:
    void init(platform::Platform& platform, FontRenderer& font);

    void render(const core::GameData& game_data, int32_t selected_char);

    int32_t char_at_position(int32_t screen_x, int32_t screen_y,
                             const core::GameData& game_data) const;

    static constexpr int32_t CHAR_LIST_X = 20;
    static constexpr int32_t CHAR_LIST_Y = 60;
    static constexpr int32_t CHAR_ROW_H = 28;
    static constexpr int32_t INFO_X = 200;
    static constexpr int32_t INFO_Y = 60;
    static constexpr int32_t ITEM_START_Y = 180;
    static constexpr int32_t ITEM_ROW_H = 24;

private:
    platform::Platform* platform_{nullptr};
    FontRenderer* font_{nullptr};
};

}  // namespace dlzr::presentation
