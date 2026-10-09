#include "presentation/title/title_renderer.hpp"

namespace dlzr::presentation {

void TitleRenderer::init(platform::Platform& platform, FontRenderer& font,
                          ImageCache& images) {
    platform_ = &platform;
    font_ = &font;
    images_ = &images;
}

void TitleRenderer::render(int32_t selected_option, int32_t difficulty_level) {
    if (!platform_) return;

    images_->draw_image("DLZ00.IMG", 0, 0);

    // 难度标签 / Difficulty label
    const char* diff_labels[] = {
        "\xBC\xF2\xB5\xA5",  // 简单 (Easy)
        "\xC6\xD5\xCD\xA8",  // 普通 (Normal)
        "\xC0\xA7\xC4\xD1",  // 困难 (Hard)
    };
    const char* diff_label = (difficulty_level >= 0 && difficulty_level <= 2)
        ? diff_labels[difficulty_level] : diff_labels[1];

    char diff_buf[64];
    std::snprintf(diff_buf, sizeof(diff_buf),
                  "\xC4\xD1\xB6\xC8\xA3\xBA%s", diff_label);  // 难度：XXX

    const char* labels[] = {
        "\xD0\xC2\xD3\xCE\xCF\xB7",       // 新游戏
        "\xB6\xC1\xC8\xA1\xB4\xE6\xB5\xB5", // 读取存档
        diff_buf,
        "\xCD\xCB\xB3\xF6",                   // 退出
    };

    for (int i = 0; i < OPTION_COUNT; ++i) {
        int32_t y = MENU_Y + i * MENU_SPACING;
        uint8_t r = (i == selected_option) ? 255 : 200;
        uint8_t g = (i == selected_option) ? 255 : 200;
        uint8_t b = (i == selected_option) ? 0 : 200;

        int32_t text_w, text_h;
        font_->measure_text(labels[i], text_w, text_h);
        int32_t text_x = MENU_X - text_w / 2;

        font_->draw_text(text_x, y, labels[i], r, g, b);
    }
}

}  // namespace dlzr::presentation
