// save_load_renderer.cpp — 存档界面渲染实现

#include "presentation/strategic/save_load_renderer.hpp"

#include <cstdio>

namespace dlzr::presentation {

void SaveLoadRenderer::init(platform::Platform& platform, FontRenderer& font,
                             const app::SaveLoader& save_loader) {
    platform_ = &platform;
    font_ = &font;
    save_loader_ = &save_loader;
}

void SaveLoadRenderer::render(SaveLoadMode mode, int32_t selected_slot) {
    platform_->clear(30, 30, 50);

    uint8_t title_r = (mode == SaveLoadMode::SAVE) ? 80 : 50;
    uint8_t title_g = 60;
    uint8_t title_b = (mode == SaveLoadMode::SAVE) ? 50 : 80;
    platform_->fill_rect(0, 0, 640, 40, title_r, title_g, title_b);

    const char* title_text = (mode == SaveLoadMode::SAVE)
        ? "\xB1\xA3\xB4\xE6"       // 保存 (Save)
        : "\xB6\xC1\xC8\xA1";     // 读取 (Load)
    font_->draw_text(280, 10, title_text, 255, 255, 255);

    for (int32_t i = 0; i < core::SAVE_SLOT_COUNT; ++i) {
        int32_t col = i / 5;
        int32_t row = i % 5;
        int32_t x = START_X + col * (SLOT_WIDTH + 20);
        int32_t y = START_Y + row * (SLOT_HEIGHT + SLOT_SPACING);

        bool exists = save_loader_->slot_exists(i);
        bool selected = (i == selected_slot);

        uint8_t r = selected ? 100 : 60;
        uint8_t g = selected ? 100 : 60;
        uint8_t b = selected ? 120 : 80;

        platform_->fill_rect(x, y, SLOT_WIDTH, SLOT_HEIGHT, r, g, b);
        platform_->draw_rect(x, y, SLOT_WIDTH, SLOT_HEIGHT,
                             selected ? 255 : 150,
                             selected ? 255 : 150,
                             selected ? 255 : 150);

        char slot_label[32];
        std::snprintf(slot_label, sizeof(slot_label), "%s %d",
                       exists ? "[*]" : "[ ]", i + 1);
        font_->draw_text(x + 8, y + 10, slot_label, 220, 220, 220);
    }

    platform_->fill_rect(0, 440, 640, 40, 40, 40, 60);
    font_->draw_text(10, 450,
                     "\xD3\xD2\xBC\xFC\xC8\xA1\xCF\xFB / \xCA\xF3\xB1\xEA\xD1\xA1\xD4\xF1",
                     200, 200, 200);
}

void SaveLoadRenderer::render_slot_preview(int32_t slot, const app::SaveInfo& info) {
    if (!info.exists || slot < 0) return;

    int32_t px = START_X;
    int32_t py = START_Y + 5 * (SLOT_HEIGHT + SLOT_SPACING) + 10;

    platform_->fill_rect(px, py, 560, 80, 40, 40, 70);
    platform_->draw_rect(px, py, 560, 80, 120, 120, 180);

    char fight_buf[64];
    std::snprintf(fight_buf, sizeof(fight_buf),
                  "\xB9\xD8\xBF\xA8: %d/47",  // 关卡: N/47
                  info.fight_index + 1);
    font_->draw_text(px + 10, py + 8, fight_buf, 200, 200, 255);

    char town_buf[64];
    std::snprintf(town_buf, sizeof(town_buf),
                  "\xB3\xC7\xD5\xF2: %d",  // 城镇: N
                  info.current_town + 1);
    font_->draw_text(px + 10, py + 32, town_buf, 200, 255, 200);

    char gold_buf[64];
    std::snprintf(gold_buf, sizeof(gold_buf),
                  "\xBD\xF0\xC7\xAE: %d",  // 金钱: N
                  info.gold);
    font_->draw_text(px + 200, py + 8, gold_buf, 255, 255, 100);
}

void SaveLoadRenderer::render_confirm_dialog(SaveLoadMode mode, int32_t slot) {
    platform_->fill_rect(160, 180, 320, 120, 20, 20, 40);
    platform_->draw_rect(160, 180, 320, 120, 200, 200, 100);

    const char* prompt = (mode == SaveLoadMode::SAVE)
        ? "\xC8\xB7\xC8\xCF\xB1\xA3\xB4\xE6\xB5\xBD\xB4\xE6\xB5\xB5 %d?"  // 确认保存到存档 N?
        : "\xC8\xB7\xC8\xCF\xB6\xC1\xC8\xA1\xB4\xE6\xB5\xB5 %d?";          // 确认读取存档 N?
    char msg[128];
    std::snprintf(msg, sizeof(msg), prompt, slot + 1);
    font_->draw_text(180, 200, msg, 255, 255, 200);

    // [Y] 确认 / [N] 取消
    font_->draw_text(190, 240, "[Y] \xC8\xB7\xC8\xCF", 100, 255, 100);   // 确认
    font_->draw_text(340, 240, "[N] \xC8\xA1\xCF\xFB", 255, 100, 100);   // 取消
}

int32_t SaveLoadRenderer::slot_at_position(int32_t screen_x, int32_t screen_y) const {
    for (int32_t i = 0; i < core::SAVE_SLOT_COUNT; ++i) {
        int32_t col = i / 5;
        int32_t row = i % 5;
        int32_t x = START_X + col * (SLOT_WIDTH + 20);
        int32_t y = START_Y + row * (SLOT_HEIGHT + SLOT_SPACING);

        if (screen_x >= x && screen_x < x + SLOT_WIDTH &&
            screen_y >= y && screen_y < y + SLOT_HEIGHT) {
            return i;
        }
    }
    return -1;
}

}  // namespace dlzr::presentation
