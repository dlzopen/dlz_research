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

    // Background (640×480)
    images_->draw_image("DLZ00.IMG", 0, 0);

    // Title logo "地雷战" (40×360), centered at top
    int32_t logo_x = (640 - 40) / 2;
    int32_t logo_y = 20;
    images_->draw_image("DLZ00.TGA", logo_x, logo_y);

    // Decorative frame (404×127), centered
    int32_t frame_x = (640 - 404) / 2;
    int32_t frame_y = 380;
    images_->draw_image("FRAME00.TGA", frame_x, frame_y);

    // Menu panel (88×120), centered below frame
    int32_t menu_x = (640 - 88) / 2;
    int32_t menu_y = 510;
    images_->draw_image("MENU.TGA", menu_x, menu_y);

    // Difficulty label
    const char* diff_labels[] = {
        "\xBC\xF2\xB5\xA5",  // 简单
        "\xC6\xD5\xCD\xA8",  // 普通
        "\xC0\xA7\xC4\xD1",  // 困难
    };
    const char* diff_label = (difficulty_level >= 0 && difficulty_level <= 2)
        ? diff_labels[difficulty_level] : diff_labels[1];

    char diff_buf[64];
    std::snprintf(diff_buf, sizeof(diff_buf),
                  "\xC4\xD1\xB6\xC8\xA3\xBA%s", diff_label);

    const char* labels[] = {
        "\xD0\xC2\xD3\xCE\xCF\xB7",
        "\xB6\xC1\xC8\xA1\xB4\xE6\xB5\xB5",
        diff_buf,
        "\xCD\xCB\xB3\xF6",
    };

    // Draw menu items with selection circle
    for (int i = 0; i < OPTION_COUNT; ++i) {
        int32_t y = 520 + i * 25;
        int32_t text_w, text_h;
        font_->measure_text(labels[i], text_w, text_h);
        int32_t text_x = 320 - text_w / 2;

        // Selection circle (48×48)
        if (i == selected_option) {
            int32_t circle_x = text_x - 55;
            int32_t circle_y = y - 8;
            images_->draw_image("CIRCLE.TGA", circle_x, circle_y);
        }

        uint8_t r = (i == selected_option) ? 255 : 200;
        uint8_t g = (i == selected_option) ? 255 : 200;
        uint8_t b = (i == selected_option) ? 0 : 200;

        font_->draw_text(text_x, y, labels[i], r, g, b);
    }
}

}  // namespace dlzr::presentation
