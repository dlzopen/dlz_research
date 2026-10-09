#pragma once

// 城镇场景渲染器 / Town scene renderer

#include "core/data_model/types.hpp"
#include "platform/platform.hpp"
#include "presentation/font/font_renderer.hpp"
#include "presentation/image/image_cache.hpp"

#include <cstdint>

namespace dlzr::presentation {

class TownRenderer {
public:
    void init(platform::Platform& platform, FontRenderer& font, ImageCache& images);
    void render(const core::Town& town, int32_t town_index);

    int32_t building_at_position(int32_t screen_x, int32_t screen_y,
                                 const core::Town& town) const;

private:
    platform::Platform* platform_{nullptr};
    FontRenderer* font_{nullptr};
    ImageCache* images_{nullptr};

    void draw_building(int32_t index, int32_t name_x, int32_t name_y,
                       int32_t click_x, int32_t click_y);
};

}  // namespace dlzr::presentation
