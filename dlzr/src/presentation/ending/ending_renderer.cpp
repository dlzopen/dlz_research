#include "presentation/ending/ending_renderer.hpp"

namespace dlzr::presentation {

void EndingRenderer::init(platform::Platform& platform, FontRenderer& font,
                           ImageCache& images) {
    platform_ = &platform;
    font_ = &font;
    images_ = &images;
}

void EndingRenderer::render(int32_t ending_id, const std::string& ending_text) {
    if (!platform_) return;

    // END00-05.IMG: 6 ending backgrounds
    // ending_id 591 = high score ending, 592 = low score ending
    int32_t img_index = (ending_id == 591) ? 0 : 1;
    char img_name[16];
    std::snprintf(img_name, sizeof(img_name), "END%02d.IMG", img_index);

    images_->draw_image(img_name, 0, 0);

    // Semi-transparent overlay for text readability
    platform_->fill_rect(32, 340, 576, 120, 0, 0, 0);

    // Ending title
    // 结局 / Ending
    const char* title = (ending_id == 591)
        ? "\xBD\xE1\xBE\xD6\xA3\xBA\xA1\xA1\xB9\xE2\xC3\xF7\xB4\xF3\xBD\xE1\xBE\xD6"  // 结局：光明大结局
        : "\xBD\xE1\xBE\xD6\xA3\xBA\xA1\xA1\xBC\xCC\xD0\xF8\xC8\xCE\xCE\xF1";          // 结局：继续任务
    font_->draw_text(48, 350, title, 255, 255, 200);

    // Ending text (wrapped)
    if (!ending_text.empty()) {
        font_->draw_text_wrapped(48, 380, 544, ending_text, 220, 220, 220);
    }

    // Hint
    // 按任意键返回 / Press any key to return
    font_->draw_text(200, 460, "\xB0\xB4\xC8\xCE\xD2\xBC\xFC\xB7\xB5\xBB\xD8", 160, 160, 160);
}

}  // namespace dlzr::presentation
