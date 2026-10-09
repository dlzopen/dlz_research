// promotion_renderer.cpp — 转职界面渲染器实现
// Class promotion screen renderer implementation

#include "presentation/promotion/promotion_renderer.hpp"

#include "core/data_model/constants.hpp"

#include <cstdio>
#include <string>

namespace dlzr::presentation {

void PromotionRenderer::init(platform::Platform& platform, FontRenderer& font) {
    platform_ = &platform;
    font_ = &font;
}

void PromotionRenderer::render(const logic::GameStateManager& gsm,
                                const logic::PromotionSystem& promotion,
                                int32_t selected_char) {
    if (!platform_) return;

    platform_->fill_rect(0, 0, 640, 480, 0, 0, 0);
    platform_->fill_rect(20, 20, 600, 440, 15, 12, 8);
    platform_->draw_rect(20, 20, 600, 440, 140, 120, 70);

    std::string title = "\xD7\xAA\xD6\xB0";  // 转职
    font_->draw_text(280, 40, title, 255, 220, 100);

    const auto& game_data = gsm.game_data();

    int32_t char_count = 0;
    for (int i = 0; i < core::MAX_PEOPLE; ++i) {
        const auto& person = game_data.people[i];
        if (person.enemy != 0) continue;
        if (person.level <= 0) continue;

        int32_t col = char_count % COLS;
        int32_t row = char_count / COLS;
        int32_t x = GRID_X + col * (CHAR_W + 20);
        int32_t y = GRID_Y + row * (CHAR_H + 8);

        bool can_promote = promotion.can_promote(i);
        bool is_selected = (i == selected_char);

        uint8_t bg_r = 30, bg_g = 25, bg_b = 15;
        uint8_t border_r = 80, border_g = 70, border_b = 50;

        if (can_promote) {
            bg_r = 40; bg_g = 35; bg_b = 20;
            border_r = 180; border_g = 150; border_b = 80;
        }

        platform_->fill_rect(x, y, CHAR_W, CHAR_H, bg_r, bg_g, bg_b);

        if (is_selected) {
            platform_->draw_rect(x - 2, y - 2, CHAR_W + 4, CHAR_H + 4,
                                 255, 220, 100);
        }
        platform_->draw_rect(x, y, CHAR_W, CHAR_H, border_r, border_g, border_b);

        std::string name(person.name.data());
        font_->draw_text(x + 8, y + 8, name, 220, 200, 150);

        char level_buf[32];
        std::snprintf(level_buf, sizeof(level_buf), "Lv%d", person.level);
        font_->draw_text(x + 140, y + 8, level_buf, 180, 180, 180);

        int32_t class_idx = person.class_type;
        if (class_idx >= 0 && class_idx < core::MAX_CLASS_TYPES) {
            std::string class_name(game_data.class_types[class_idx].name.data());
            font_->draw_text(x + 190, y + 8, class_name, 150, 200, 150);
        }

        if (can_promote) {
            std::string arrow = "\xA1\xFA";  // →
            font_->draw_text(x + CHAR_W - 30, y + 8, arrow, 255, 200, 100);
        }

        ++char_count;
    }

    if (selected_char >= 0 && selected_char < core::MAX_PEOPLE) {
        const auto& person = game_data.people[selected_char];
        int32_t target = promotion.get_promotion_target(selected_char);
        int32_t req_item = promotion.get_required_item(selected_char);

        std::string info_y = "\xD2\xAA\xC7\xF3\xB5\xC8\xBC\xD6\xCA" "10" "\xBC\xB6";  // 要求等级10级
        font_->draw_text(40, 400, info_y, 180, 180, 180);

        if (target >= 0 && target < core::MAX_CLASS_TYPES) {
            std::string target_name = "\xD7\xAA\xD6\xB0\xC4\xBF\xB1\xEA\xA3\xBA";  // 转职目标：
            target_name += game_data.class_types[target].name.data();
            font_->draw_text(40, 420, target_name, 220, 200, 150);
        }

        if (req_item >= 0) {
            std::string item_name = "\xD0\xE8\xD2\xAA\xCE\xEF\xC6\xB7\xA3\xBA";  // 需要物品：
            if (req_item < core::MAX_WUPIN) {
                item_name += std::string(game_data.items[req_item].name.data());
            }
            font_->draw_text(320, 420, item_name, 200, 180, 120);
        }
    }

    std::string hint = "\xC8\xB7\xC8\xCF\xD7\xAA\xD6\xB0 ESC\xB7\xB5\xBB\xD8";  // 确认转职 ESC返回
    font_->draw_text(40, 450, hint, 150, 150, 150);
}

int32_t PromotionRenderer::char_at_position(int32_t mx, int32_t my,
                                              const core::GameData& game_data) const {
    if (mx < GRID_X || my < GRID_Y) return -1;

    int32_t col = (mx - GRID_X) / (CHAR_W + 20);
    int32_t row = (my - GRID_Y) / (CHAR_H + 8);

    if (col < 0 || col >= COLS) return -1;
    if (row < 0) return -1;

    int32_t local_x = (mx - GRID_X) % (CHAR_W + 20);
    int32_t local_y = (my - GRID_Y) % (CHAR_H + 8);
    if (local_x > CHAR_W || local_y > CHAR_H) return -1;

    int32_t char_idx = row * COLS + col;

    int32_t count = 0;
    for (int i = 0; i < core::MAX_PEOPLE; ++i) {
        if (game_data.people[i].enemy != 0) continue;
        if (game_data.people[i].level <= 0) continue;
        if (count == char_idx) return i;
        ++count;
    }

    return -1;
}

}  // namespace dlzr::presentation
