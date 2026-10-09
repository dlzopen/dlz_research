#pragma once

// forging_renderer.hpp — 锻造界面渲染器
// Forging/crafting screen renderer

#include "logic/forging/forging_system.hpp"
#include "platform/platform.hpp"
#include "presentation/font/font_renderer.hpp"
#include "presentation/image/image_cache.hpp"

#include <cstdint>

namespace dlzr::presentation {

class ForgingRenderer {
public:
    void init(platform::Platform& platform, FontRenderer& font, ImageCache& images);

    void render(int32_t selected_tab, int32_t selected_item,
                int32_t quantity, const logic::ForgingSystem& forging);

private:
    platform::Platform* platform_{nullptr};
    FontRenderer* font_{nullptr};
    ImageCache* images_{nullptr};

    void render_tabs(int32_t selected_tab);
    void render_item_grid(int32_t selected_tab, int32_t selected_item);
    void render_quantity_display(int32_t quantity, int32_t price);
    void render_material_list(int32_t selected_item,
                               const logic::ForgingSystem& forging);
    void render_buttons();
};

}  // namespace dlzr::presentation
