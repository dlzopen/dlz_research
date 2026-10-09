#pragma once

// strategic_map_renderer.hpp — 战略地图渲染器
// Strategic map renderer with real assets

#include "core/data_model/game_data.hpp"
#include "platform/platform.hpp"
#include "presentation/font/font_renderer.hpp"
#include "presentation/image/image_cache.hpp"

#include <cstdint>

namespace dlzr::presentation {

class StrategicMapRenderer {
public:
    void init(platform::Platform& platform, FontRenderer& font, ImageCache& images);

    void render(const core::GameData& game_data, int32_t current_town);

    int32_t town_at_position(int32_t screen_x, int32_t screen_y,
                             const core::GameData& game_data) const;

private:
    platform::Platform* platform_{nullptr};
    FontRenderer* font_{nullptr};
    ImageCache* images_{nullptr};

    void draw_town(int32_t index, int32_t x, int32_t y, bool is_current);
    void draw_paths();
};

}  // namespace dlzr::presentation
