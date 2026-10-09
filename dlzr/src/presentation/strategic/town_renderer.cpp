// town_renderer.cpp — 城镇场景渲染实现

#include "presentation/strategic/town_renderer.hpp"

namespace dlzr::presentation {

static const char* BUILDING_NAMES[6] = {
    "\xD7\xA1\xD5\xAC",       // 住宅 (House)
    "\xD6\xB8\xBB\xD3\xB2\xBF", // 指挥部 (HQ)
    "\xBE\xFC\xD0\xB5\xB3\xA7", // 军械厂 (Arsenal)
    "\xD4\xD3\xBB\xF5\xB5\xEA", // 杂货店 (Store)
    "\xB3\xF6\xB3\xC7",       // 出城 (Exit)
    "\xB9\xA4\xD7\xF7\xCA\xD2", // 工作室 (Workshop)
};

void TownRenderer::init(platform::Platform& platform, FontRenderer& font,
                         ImageCache& images) {
    platform_ = &platform;
    font_ = &font;
    images_ = &images;
}

void TownRenderer::render(const core::Town& town, int32_t town_index) {
    const char* bg_files[] = {"BG01.IMG", "BG02.IMG", "BG03.IMG"};
    int32_t bg_idx = town_index % 3;
    images_->draw_image(bg_files[bg_idx], 0, 0);

    platform_->fill_rect(0, 0, 640, 30, 40, 40, 60);

    if (town.sound1_id >= 0) {
        platform_->fill_rect(10, 4, 20, 20, 80, 120, 80);
    }
    if (town.sound2_id >= 0) {
        platform_->fill_rect(36, 4, 20, 20, 80, 120, 80);
    }
    if (town.sound3_id >= 0) {
        platform_->fill_rect(62, 4, 20, 20, 80, 120, 80);
    }

    for (int i = 0; i < 6; ++i) {
        int32_t nx = town.name_x[i];
        int32_t ny = town.name_y[i];
        if ((nx == 0 && ny == 0) || nx == 999) {
            continue;
        }
        draw_building(i, nx, ny, town.click_x[i], town.click_y[i]);
    }

    platform_->fill_rect(0, 400, 640, 80, 30, 30, 30);

    int32_t x = 10;
    for (int i = 0; i < 12 && town.goods_arsenal[i] >= 0; ++i) {
        platform_->fill_rect(x, 410, 24, 24, 150, 100, 50);
        x += 28;
    }

    x = 10;
    for (int i = 0; i < 12 && town.goods_general[i] >= 0; ++i) {
        platform_->fill_rect(x, 440, 24, 24, 100, 150, 50);
        x += 28;
    }

    x = 10;
    for (int i = 0; i < 12 && town.goods_material[i] >= 0; ++i) {
        platform_->fill_rect(x, 470, 24, 24, 50, 100, 150);
        x += 28;
    }
}

void TownRenderer::draw_building(int32_t index, int32_t name_x, int32_t name_y,
                                  int32_t click_x, int32_t click_y) {
    int32_t bx = name_x + click_x;
    int32_t by = name_y + click_y;
    platform_->fill_rect(bx, by, 60, 40, 120, 100, 70);
    platform_->draw_rect(bx, by, 60, 40, 200, 200, 200);

    if (font_ && index < 6) {
        int32_t tw = 0, th = 0;
        font_->measure_text(BUILDING_NAMES[index], tw, th);
        font_->draw_text(name_x - 2, name_y - 18, BUILDING_NAMES[index],
                         255, 255, 200);
    }
}

int32_t TownRenderer::building_at_position(int32_t screen_x, int32_t screen_y,
                                            const core::Town& town) const {
    for (int i = 0; i < 6; ++i) {
        int32_t nx = town.name_x[i];
        int32_t ny = town.name_y[i];
        if ((nx == 0 && ny == 0) || nx == 999) {
            continue;
        }

        int32_t bx = nx + town.click_x[i];
        int32_t by = ny + town.click_y[i];

        if (screen_x >= bx && screen_x < bx + 60 &&
            screen_y >= by && screen_y < by + 40) {
            return i;
        }
    }
    return -1;
}

}  // namespace dlzr::presentation
