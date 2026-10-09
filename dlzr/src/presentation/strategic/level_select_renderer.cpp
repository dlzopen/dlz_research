// level_select_renderer.cpp — 关卡选择界面渲染器实现
// Level selection screen renderer implementation

#include "presentation/strategic/level_select_renderer.hpp"

#include "core/data_model/constants.hpp"

#include <cstdio>
#include <string>

namespace dlzr::presentation {

void LevelSelectRenderer::init(platform::Platform& platform, FontRenderer& font) {
    platform_ = &platform;
    font_ = &font;
}

void LevelSelectRenderer::render(const logic::GameStateManager& gsm,
                                  int32_t selected_level,
                                  int32_t scroll_offset) {
    if (!platform_) return;

    platform_->fill_rect(0, 0, 640, 480, 0, 0, 0);

    platform_->fill_rect(40, 30, 560, 420, 15, 12, 8);
    platform_->draw_rect(40, 30, 560, 420, 140, 120, 70);

    std::string title = "\xB9\xD8\xBF\xA8\xD1\xA1\xD4\xF1";  // 关卡选择
    int32_t title_chars = 4;
    int32_t title_w = title_chars * FontRenderer::CHAR_WIDTH * 2;
    font_->draw_text(320 - title_w / 2, 50, title, 255, 220, 100);

    const auto& game_data = gsm.game_data();

    int32_t visible_cells = COLS * ROWS;
    for (int i = 0; i < visible_cells; ++i) {
        int32_t display_idx = scroll_offset + i;
        if (display_idx >= core::MAX_FIGHTS) break;

        int32_t col = i % COLS;
        int32_t row = i / COLS;
        int32_t x = GRID_X + col * (CELL_W + 10);
        int32_t y = GRID_Y + row * (CELL_H + 8);

        bool completed = gsm.is_fight_completed(display_idx);
        bool unlocked = (display_idx == 0) ||
                        gsm.is_fight_completed(display_idx - 1);

        uint8_t bg_r = 30, bg_g = 25, bg_b = 15;
        uint8_t border_r = 80, border_g = 70, border_b = 50;
        uint8_t text_r = 150, text_g = 150, text_b = 150;

        if (completed) {
            bg_r = 20; bg_g = 40; bg_b = 20;
            border_r = 80; border_g = 180; border_b = 80;
            text_r = 150; text_g = 255; text_b = 150;
        } else if (unlocked) {
            bg_r = 40; bg_g = 35; bg_b = 20;
            border_r = 180; border_g = 150; border_b = 80;
            text_r = 255; text_g = 220; text_b = 150;
        } else {
            bg_r = 20; bg_g = 18; bg_b = 15;
            border_r = 50; border_g = 45; border_b = 40;
            text_r = 80; text_g = 80; text_b = 80;
        }

        platform_->fill_rect(x, y, CELL_W, CELL_H, bg_r, bg_g, bg_b);

        if (display_idx == selected_level) {
            platform_->draw_rect(x - 2, y - 2, CELL_W + 4, CELL_H + 4,
                                 255, 220, 100);
        }
        platform_->draw_rect(x, y, CELL_W, CELL_H, border_r, border_g, border_b);

        char level_buf[16];
        std::snprintf(level_buf, sizeof(level_buf), "%d", display_idx + 1);
        font_->draw_text(x + 10, y + 12, level_buf, text_r, text_g, text_b);

        if (completed) {
            std::string check = "\xA1\xD7";  // ✓ (using GBK checkmark)
            font_->draw_text(x + CELL_W - 30, y + 12, check, 100, 255, 100);
        } else if (!unlocked) {
            std::string lock = "\xCB\xF8";  // 锁 (lock)
            font_->draw_text(x + CELL_W - 30, y + 12, lock, 100, 100, 100);
        }
    }

    std::string hint = "\xC9\xCF\xCF\xC2\xB7\xAD\xD2\xB3 \xBB\xD8\xB3\xB5\xBC\xFC\xB7\xB5\xBB\xD8 \xC8\xB7\xC8\xCF\xBD\xF8\xC8\xEB";
    font_->draw_text(80, 430, hint, 150, 150, 150);
}

}  // namespace dlzr::presentation
