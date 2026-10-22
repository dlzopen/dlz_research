#pragma once

// strategic_map_renderer.hpp — 战略地图渲染器
// Strategic map renderer with real assets
//
// 右上角"人物信息面板" 1:1 复刻 FUN_0041be02（与 battle_renderer::draw_info_panel
// 同一原函数、同一坐标/配色/字段布局）。坐标与调色板证据均来自 FUN_0041be02.c
// 及 battle_renderer.cpp::draw_info_panel 的逐行注释。

#include "app/resource/face_loader.hpp"
#include "core/data_model/game_data.hpp"
#include "platform/platform.hpp"
#include "presentation/font/font_renderer.hpp"
#include "presentation/image/image_cache.hpp"

#include <cstdint>
#include <unordered_map>

namespace dlzr::presentation {

class StrategicMapRenderer {
public:
    void init(platform::Platform& platform, FontRenderer& font, ImageCache& images,
              app::FaceLoader& face);

    void render(const core::GameData& game_data, int32_t current_town);

    int32_t town_at_position(int32_t screen_x, int32_t screen_y,
                             const core::GameData& game_data) const;

private:
    platform::Platform* platform_{nullptr};
    FontRenderer* font_{nullptr};
    ImageCache* images_{nullptr};
    app::FaceLoader* face_loader_{nullptr};

    std::unordered_map<int32_t, platform::TextureHandle> face_tex_cache_;

    void draw_town(int32_t index, int32_t x, int32_t y,
                   bool is_current, bool blink);

    // 人物信息面板（原版 FUN_0041be02）：显示指定单位的核心数值字段
    void draw_unit_panel(const core::GameData& game_data, int32_t unit_index);

    void draw_int(int32_t x, int32_t y, int32_t value,
                  uint8_t r, uint8_t g, uint8_t b);

    platform::TextureHandle get_face_texture(int32_t face_idx);
};

}  // namespace dlzr::presentation
