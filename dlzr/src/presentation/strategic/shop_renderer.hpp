#pragma once

// 商店渲染器 / Shop renderer

#include "core/data_model/game_data.hpp"
#include "core/data_model/types.hpp"
#include "platform/platform.hpp"
#include "presentation/font/font_renderer.hpp"

#include <cstdint>

namespace dlzr::presentation {

enum class ShopType {
    ARSENAL,
    GENERAL,
    MATERIAL
};

class ShopRenderer {
public:
    void init(platform::Platform& platform, FontRenderer& font);
    void render(const core::Town& town, ShopType shop_type,
                const core::GameData& game_data);

    int32_t item_at_position(int32_t screen_x, int32_t screen_y,
                             const core::Town& town, ShopType shop_type) const;

private:
    platform::Platform* platform_{nullptr};
    FontRenderer* font_{nullptr};

    static constexpr int32_t ROW_HEIGHT = 28;
    static constexpr int32_t LIST_START_Y = 50;
    static constexpr int32_t NAME_X = 20;
    static constexpr int32_t PRICE_X = 400;
};

}  // namespace dlzr::presentation
