// warehouse_renderer.cpp — 仓库画面渲染器实现

#include "presentation/strategic/warehouse_renderer.hpp"

#include "core/data_model/constants.hpp"

#include <cstring>

namespace dlzr::presentation {

void WarehouseRenderer::init(platform::Platform& platform, FontRenderer& font) {
    platform_ = &platform;
    font_ = &font;
}

static std::string safe_array_str(const char* data, size_t max_len) {
    size_t len = 0;
    while (len < max_len && data[len] != '\0') ++len;
    return std::string(data, len);
}

void WarehouseRenderer::render(const core::GameData& game_data, int32_t scroll_offset) {
    if (!platform_) return;

    platform_->fill_rect(0, 0, 640, 480, 10, 10, 20);

    std::string title = "\xB2\xD6\xBF\xE2\xCE\xEF\xD7\xCA";  // 仓库物资
    int32_t title_chars = 0;
    size_t pos = 0;
    while (pos < title.size()) {
        uint8_t b = static_cast<uint8_t>(title[pos]);
        pos += (b < 0x80) ? 1 : 2;
        title_chars++;
    }
    int32_t title_w = title_chars * FontRenderer::CHAR_WIDTH;
    font_->draw_text((640 - title_w) / 2, 20, title, 255, 255, 100);

    platform_->fill_rect(30, 50, 580, 400, 20, 20, 40);
    platform_->draw_rect(30, 50, 580, 400, 120, 120, 160);

    std::string header_name = "\xC3\xFB\xB3\xC6";  // 名称
    std::string header_qty = "\xCA\xFD\xC1\xBF";    // 数量
    font_->draw_text(LIST_X, LIST_Y - 28, header_name, 200, 200, 160);
    font_->draw_text(LIST_X + 400, LIST_Y - 28, header_qty, 200, 200, 160);

    int32_t row = 0;
    int32_t skipped = 0;
    for (int32_t i = 0; i < core::MAX_WUPIN && row < VISIBLE_ROWS; ++i) {
        if (game_data.warehouse[i] == 0) continue;

        if (skipped < scroll_offset) {
            skipped++;
            continue;
        }

        int32_t row_y = LIST_Y + row * ROW_H;

        if (row % 2 == 0) {
            platform_->fill_rect(32, row_y - 2, 576, ROW_H, 30, 30, 50);
        }

        std::string item_name = safe_array_str(
            game_data.items[i].name.data(),
            game_data.items[i].name.size());
        font_->draw_text(LIST_X, row_y, item_name, 220, 220, 220);

        char qty_str[16];
        std::snprintf(qty_str, sizeof(qty_str), "%d", game_data.warehouse[i]);
        font_->draw_text(LIST_X + 400, row_y, qty_str, 255, 255, 100);

        row++;
    }

    if (row == 0) {
        std::string empty = "\xB2\xD6\xBF\xE2\xCE\xAA\xBF\xD5";  // 仓库为空
        int32_t empty_chars = 0;
        size_t epos = 0;
        while (epos < empty.size()) {
            uint8_t b = static_cast<uint8_t>(empty[epos]);
            epos += (b < 0x80) ? 1 : 2;
            empty_chars++;
        }
        int32_t empty_w = empty_chars * FontRenderer::CHAR_WIDTH;
        font_->draw_text((640 - empty_w) / 2, 200, empty, 180, 180, 180);
    }

    std::string hint = "ESC \xB7\xB5\xBB\xD8";  // ESC 返回
    font_->draw_text(30, 460, hint, 160, 160, 160);
}

int32_t WarehouseRenderer::item_at_position(int32_t screen_x, int32_t screen_y,
                                             const core::GameData& game_data,
                                             int32_t scroll_offset) const {
    if (screen_x < 30 || screen_x > 610) return -1;
    if (screen_y < LIST_Y) return -1;

    int32_t row = 0;
    int32_t skipped = 0;
    for (int32_t i = 0; i < core::MAX_WUPIN && row < VISIBLE_ROWS; ++i) {
        if (game_data.warehouse[i] == 0) continue;

        if (skipped < scroll_offset) {
            skipped++;
            continue;
        }

        int32_t row_y = LIST_Y + row * ROW_H;
        if (screen_y >= row_y && screen_y < row_y + ROW_H) {
            return i;
        }

        row++;
    }
    return -1;
}

}  // namespace dlzr::presentation
