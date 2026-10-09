#pragma once

// warehouse_renderer.hpp — 仓库画面渲染器
// Warehouse/inventory screen renderer

#include "platform/platform.hpp"
#include "presentation/font/font_renderer.hpp"
#include "core/data_model/game_data.hpp"

#include <cstdint>

namespace dlzr::presentation {

class WarehouseRenderer {
public:
    void init(platform::Platform& platform, FontRenderer& font);

    void render(const core::GameData& game_data, int32_t scroll_offset);

private:
    platform::Platform* platform_{nullptr};
    FontRenderer* font_{nullptr};

    static constexpr int32_t LIST_X = 40;
    static constexpr int32_t LIST_Y = 60;
    static constexpr int32_t ROW_H = 24;
    static constexpr int32_t VISIBLE_ROWS = 15;
};

}  // namespace dlzr::presentation
