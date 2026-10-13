// shop_renderer.cpp — 商店渲染实现

#include "presentation/strategic/shop_renderer.hpp"

#include <cstdio>

namespace dlzr::presentation {

void ShopRenderer::init(platform::Platform& platform, FontRenderer& font) {
    platform_ = &platform;
    font_ = &font;
}

static void format_price(char* buf, size_t buf_size, int32_t price) {
    std::snprintf(buf, buf_size, "%d\xBD\xF0", price);
}

void ShopRenderer::render(const core::Town& town, ShopType shop_type,
                           const core::GameData& game_data) {
    platform_->clear(40, 40, 60);

    const char* title = "\xBE\xFC\xD0\xB5\xB3\xA7";
    uint8_t title_r = 80, title_g = 60, title_b = 40;
    if (shop_type == ShopType::GENERAL) {
        title = "\xD4\xD3\xBB\xF5\xB5\xEA";
        title_r = 60; title_g = 80; title_b = 40;
    } else if (shop_type == ShopType::MATERIAL) {
        title = "\xB2\xC4\xC1\xCF\xB5\xEA";
        title_r = 40; title_g = 60; title_b = 80;
    }
    platform_->fill_rect(0, 0, 640, 36, title_r, title_g, title_b);
    font_->draw_text(10, 8, title, 255, 255, 255);

    platform_->fill_rect(NAME_X - 4, 40, 560, 2, 180, 180, 180);

    font_->draw_text(NAME_X, 44, "\xC3\xFB\xB3\xC6", 200, 200, 150);
    font_->draw_text(PRICE_X, 44, "\xBC\xDB\xB8\xF1", 200, 200, 150);
    font_->draw_text(QTY_X, 44, "\xD2\xD3\xD3\xD0", 200, 200, 150);

    int32_t gold = static_cast<int32_t>(game_data.warehouse[0]);
    char gold_buf[48];
    std::snprintf(gold_buf, sizeof(gold_buf), "\xBD\xF0\xC7\xAE: %d", gold);
    font_->draw_text(420, 8, gold_buf, 255, 220, 100);

    const int32_t* items = nullptr;
    int32_t count = 0;

    if (shop_type == ShopType::ARSENAL) {
        items = town.goods_arsenal.data();
        count = 12;
    } else if (shop_type == ShopType::GENERAL) {
        items = town.goods_general.data();
        count = 12;
    } else {
        items = town.goods_material.data();
        count = 12;
    }

    int32_t displayed = 0;
    for (int32_t i = 0; i < count; ++i) {
        if (items[i] < 0) break;

        int32_t y = LIST_START_Y + displayed * ROW_HEIGHT;

        uint8_t row_r = 50, row_g = 50, row_b = 60;
        if (displayed % 2 == 1) {
            row_r = 55; row_g = 55; row_b = 68;
        }
        platform_->fill_rect(NAME_X - 4, y, 560, ROW_HEIGHT, row_r, row_g, row_b);

        const char* name = "";
        int32_t price = 0;

        if (shop_type == ShopType::MATERIAL) {
            int32_t idx = items[i];
            if (idx >= 0 && idx < core::MAX_DILEI) {
                name = game_data.mines[idx].name.data();
                price = game_data.mines[idx].price;
            }
        } else {
            int32_t idx = items[i];
            if (idx >= 0 && idx < core::MAX_WUPIN) {
                name = game_data.items[idx].name.data();
                price = game_data.items[idx].price;
            }
        }

        font_->draw_text(NAME_X, y + 4, name, 255, 255, 220);

        char price_buf[32];
        format_price(price_buf, sizeof(price_buf), price);
        uint8_t pr = 255, pg = 220, pb = 100;
        if (static_cast<int32_t>(game_data.warehouse[0]) < price) {
            pr = 180; pg = 80; pb = 80;
        }
        font_->draw_text(PRICE_X, y + 4, price_buf, pr, pg, pb);

        int32_t owned = 0;
        if (shop_type == ShopType::MATERIAL) {
            int32_t wh_slot = core::MAX_WUPIN - core::MAX_DILEI + items[i];
            if (wh_slot >= 0 && wh_slot < static_cast<int32_t>(game_data.warehouse.size())) {
                owned = game_data.warehouse[wh_slot];
            }
        } else {
            int32_t wh_slot = items[i];
            if (wh_slot >= 0 && wh_slot < static_cast<int32_t>(game_data.warehouse.size())) {
                owned = game_data.warehouse[wh_slot];
            }
        }
        char qty_buf[16];
        std::snprintf(qty_buf, sizeof(qty_buf), "%d", owned);
        font_->draw_text(QTY_X, y + 4, qty_buf, 200, 255, 200);

        ++displayed;
    }

    platform_->fill_rect(0, 450, 640, 30, 50, 50, 50);
    font_->draw_text(10, 456, "\xD7\xF3\xBC\xFC\xB9\xBA\xC2\xF2  \xD3\xD2\xBC\xFC\xCD\xCB\xB3\xF6", 200, 200, 200);
}

int32_t ShopRenderer::item_at_position(int32_t screen_x, int32_t screen_y,
                                        const core::Town& town, ShopType shop_type) const {
    const int32_t* items = nullptr;
    int32_t count = 0;

    if (shop_type == ShopType::ARSENAL) {
        items = town.goods_arsenal.data();
        count = 12;
    } else if (shop_type == ShopType::GENERAL) {
        items = town.goods_general.data();
        count = 12;
    } else {
        items = town.goods_material.data();
        count = 12;
    }

    int32_t displayed = 0;
    for (int32_t i = 0; i < count; ++i) {
        if (items[i] < 0) break;

        int32_t y = LIST_START_Y + displayed * ROW_HEIGHT;

        if (screen_x >= NAME_X - 4 && screen_x < NAME_X - 4 + 560 &&
            screen_y >= y && screen_y < y + ROW_HEIGHT) {
            return items[i];
        }

        ++displayed;
    }

    return -1;
}

}  // namespace dlzr::presentation
