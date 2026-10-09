// equipment_renderer.cpp — 装备/物品画面渲染器实现

#include "presentation/strategic/equipment_renderer.hpp"

#include "core/data_model/constants.hpp"

#include <cstring>

namespace dlzr::presentation {

void EquipmentRenderer::init(platform::Platform& platform, FontRenderer& font) {
    platform_ = &platform;
    font_ = &font;
}

static std::string safe_array_str(const char* data, size_t max_len) {
    size_t len = 0;
    while (len < max_len && data[len] != '\0') ++len;
    return std::string(data, len);
}

void EquipmentRenderer::render(const core::GameData& game_data, int32_t selected_char) {
    if (!platform_) return;

    platform_->fill_rect(0, 0, 640, 480, 10, 10, 20);

    std::string title = "\xC8\xCB\xCE\xEF\xD7\xB0\xB1\xB8";  // 人物装备
    int32_t title_chars = 0;
    size_t pos = 0;
    while (pos < title.size()) {
        uint8_t b = static_cast<uint8_t>(title[pos]);
        pos += (b < 0x80) ? 1 : 2;
        title_chars++;
    }
    int32_t title_w = title_chars * FontRenderer::CHAR_WIDTH;
    font_->draw_text((640 - title_w) / 2, 20, title, 255, 255, 100);

    platform_->fill_rect(10, 50, 170, 400, 20, 20, 40);
    platform_->draw_rect(10, 50, 170, 400, 120, 120, 160);

    int32_t char_y = CHAR_LIST_Y;
    for (int32_t i = 0; i < core::MAX_PEOPLE; ++i) {
        const auto& p = game_data.people[i];
        if (p.enemy != 0) continue;
        if (p.level == 0 && p.name[0] == '\0') continue;

        uint8_t r = 200, g = 200, b = 200;
        if (i == selected_char) {
            platform_->fill_rect(12, char_y - 2, 166, CHAR_ROW_H, 60, 60, 100);
            r = 255; g = 255; b = 100;
        }

        std::string name = safe_array_str(p.name.data(), p.name.size());
        font_->draw_text(CHAR_LIST_X, char_y, name, r, g, b);
        char_y += CHAR_ROW_H;
    }

    if (selected_char >= 0 && selected_char < core::MAX_PEOPLE) {
        const auto& p = game_data.people[selected_char];

        platform_->fill_rect(190, 50, 440, 400, 20, 20, 40);
        platform_->draw_rect(190, 50, 440, 400, 120, 120, 160);

        std::string name = safe_array_str(p.name.data(), p.name.size());
        font_->draw_text(INFO_X, INFO_Y, name, 255, 255, 100);

        char lvl_str[32];
        std::snprintf(lvl_str, sizeof(lvl_str), "Lv.%d", p.level);
        font_->draw_text(INFO_X + 200, INFO_Y, lvl_str, 200, 255, 200);

        std::string cls_label = "\xB1\xF8\xD6\xD6\xA3\xBA";  // 兵种：
        font_->draw_text(INFO_X, INFO_Y + 36, cls_label, 180, 180, 200);

        char cls_str[16];
        std::snprintf(cls_str, sizeof(cls_str), "%d", p.class_type);
        font_->draw_text(INFO_X + 7 * FontRenderer::CHAR_WIDTH, INFO_Y + 36,
                         cls_str, 220, 220, 220);

        std::string str_label = "\xCE\xE4\xC1\xA6\xA3\xBA";  // 武力：
        font_->draw_text(INFO_X, INFO_Y + 68, str_label, 180, 180, 200);
        char str_val[16];
        std::snprintf(str_val, sizeof(str_val), "%d", p.strength);
        font_->draw_text(INFO_X + 5 * FontRenderer::CHAR_WIDTH, INFO_Y + 68,
                         str_val, 255, 200, 100);

        std::string int_label = "\xD6\xC7\xC1\xA6\xA3\xBA";  // 智力：
        font_->draw_text(INFO_X + 200, INFO_Y + 68, int_label, 180, 180, 200);
        char int_val[16];
        std::snprintf(int_val, sizeof(int_val), "%d", p.intelligence);
        font_->draw_text(INFO_X + 200 + 5 * FontRenderer::CHAR_WIDTH, INFO_Y + 68,
                         int_val, 100, 200, 255);

        std::string item_header = "\xCE\xEF\xC6\xB7\xC0\xB8:";  // 物品栏:
        font_->draw_text(INFO_X, ITEM_START_Y - 28, item_header, 200, 200, 160);

        for (int32_t s = 0; s < 8; ++s) {
            int32_t row_y = ITEM_START_Y + s * ITEM_ROW_H;
            char slot_label[16];
            std::snprintf(slot_label, sizeof(slot_label), "%d.", s + 1);
            font_->draw_text(INFO_X, row_y, slot_label, 160, 160, 160);

            if (p.items[s] != 0) {
                int32_t item_idx = p.items[s];
                if (item_idx >= 0 && item_idx < core::MAX_WUPIN) {
                    std::string item_name = safe_array_str(
                        game_data.items[item_idx].name.data(),
                        game_data.items[item_idx].name.size());
                    font_->draw_text(INFO_X + 30, row_y, item_name, 220, 220, 220);
                }
            }
        }

        std::string mine_header = "\xB5\xD8\xC0\xD7\xC0\xB8:";  // 地雷栏:
        font_->draw_text(INFO_X + 300, ITEM_START_Y - 28, mine_header, 200, 160, 160);

        for (int32_t s = 0; s < 8; ++s) {
            int32_t row_y = ITEM_START_Y + s * ITEM_ROW_H;
            char slot_label[16];
            std::snprintf(slot_label, sizeof(slot_label), "%d.", s + 1);
            font_->draw_text(INFO_X + 300, row_y, slot_label, 160, 160, 160);

            if (p.mines[s] != 0) {
                int32_t mine_idx = p.mines[s];
                if (mine_idx >= 0 && mine_idx < core::MAX_DILEI) {
                    std::string mine_name = safe_array_str(
                        game_data.mines[mine_idx].name.data(),
                        game_data.mines[mine_idx].name.size());
                    font_->draw_text(INFO_X + 330, row_y, mine_name, 220, 220, 220);
                }
            }
        }
    }
}

int32_t EquipmentRenderer::char_at_position(int32_t screen_x, int32_t screen_y,
                                             const core::GameData& game_data) const {
    if (screen_x < 10 || screen_x > 180) return -1;
    if (screen_y < CHAR_LIST_Y) return -1;

    int32_t char_y = CHAR_LIST_Y;
    for (int32_t i = 0; i < core::MAX_PEOPLE; ++i) {
        const auto& p = game_data.people[i];
        if (p.enemy != 0) continue;
        if (p.level == 0 && p.name[0] == '\0') continue;

        if (screen_y >= char_y - 2 && screen_y < char_y + CHAR_ROW_H) {
            return i;
        }
        char_y += CHAR_ROW_H;
    }
    return -1;
}

}  // namespace dlzr::presentation
