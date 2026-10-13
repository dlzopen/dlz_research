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

    // Full-screen title background (640×480)
    // Contains: cave texture, soldier portrait, and 5 menu options pre-rendered
    images_->draw_image("MINE01.IMG", 0, 0);
}

}  // namespace dlzr::presentation
