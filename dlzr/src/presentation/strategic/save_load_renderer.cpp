// save_load_renderer.cpp — 存档界面渲染实现

#include "presentation/strategic/save_load_renderer.hpp"

namespace dlzr::presentation {

static constexpr int32_t SLOT_WIDTH = 280;
static constexpr int32_t SLOT_HEIGHT = 36;
static constexpr int32_t SLOT_SPACING = 4;
static constexpr int32_t START_X = 180;
static constexpr int32_t START_Y = 60;

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
        font_->draw_text(x + 20, y + 10, slot_label, 220, 220, 220);
    }

    platform_->fill_rect(0, 440, 640, 40, 40, 40, 60);
    font_->draw_text(10, 450,
                     "\xD3\xD2\xBC\xFC\xC8\xA1\xCF\xFB / \xCA\xF3\xB1\xEA\xD1\xA1\xD4\xF1",
                     200, 200, 200);
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
