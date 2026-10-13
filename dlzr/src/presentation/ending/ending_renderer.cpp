#include "presentation/ending/ending_renderer.hpp"

#include <algorithm>
#include <cstdio>

namespace dlzr::presentation {

void EndingRenderer::init(platform::Platform& platform, FontRenderer& font,
                           ImageCache& images) {
    platform_ = &platform;
    font_ = &font;
    images_ = &images;
}

void EndingRenderer::start(int32_t ending_id, const std::string& ending_text) {
    ending_id_ = ending_id;
    ending_text_ = ending_text;
    timer_ = 0;
    fade_alpha_ = 0;
    finished_ = false;
    can_skip_ = false;
}

int32_t EndingRenderer::pick_image_index() const {
    // ending_id 0-5 from select_ending() maps to ending images
    // 0=完美结局, 1=柳忻路线, 2=普通路线, 3=萧飞路线, 4=失败路线, 5=隐藏结局
    if (ending_id_ >= 0 && ending_id_ <= 5) {
        return ending_id_;
    }
    return 5;  // default to hidden ending image
}

const char* EndingRenderer::pick_title() const {
    switch (ending_id_) {
    case 0:  // 完美结局 / Perfect ending
        return "\xBD\xE1\xBE\xD6\xA3\xBA\xA1\xA1\xB9\xE2\xC3\xF7\xB4\xF3\xBD\xE1\xBE\xD6";
    case 1:  // 继续任务 / Continue mission
        return "\xBD\xE1\xBE\xD6\xA3\xBA\xA1\xA1\xBC\xCC\xD0\xF8\xC8\xCE\xCE\xF1";
    case 2:  // 全军覆灭 / Total annihilation
        return "\xBD\xE1\xBE\xD6\xA3\xBA\xA1\xA1\xC8\xAB\xBE\xFC\xB8\xB2\xC3\xF0";
    case 3:  // 潜伏安息 / Quiet rest
        return "\xBD\xE1\xBE\xD6\xA3\xBA\xA1\xA1\xC7\xB1\xCF\xAF\xB0\xB2";
    case 4:  // 英雄不屈 / Hero unyielding
        return "\xBD\xE1\xBE\xD6\xA3\xBA\xA1\xA1\xD3\xA2\xD0\xDB\xB2\xBB\xC3\xE5";
    case 5:  // 隐藏结局 / Hidden ending
        return "\xBD\xE1\xBE\xD6\xA3\xBA\xA1\xA1\xD3\xA2\xD0\xDB\xB2\xBB\xC3\xE5";
    default:
        return "\xBD\xE1\xBE\xD6";
    }
}

bool EndingRenderer::update(uint32_t delta_ms) {
    if (finished_) return true;

    timer_ += delta_ms;

    if (timer_ < static_cast<uint32_t>(FADE_IN_MS)) {
        float t = static_cast<float>(timer_) / static_cast<float>(FADE_IN_MS);
        fade_alpha_ = static_cast<int32_t>(255.0f * t);
    } else {
        fade_alpha_ = 255;
    }

    if (timer_ >= static_cast<uint32_t>(SKIP_DELAY_MS)) {
        can_skip_ = true;
    }

    return false;
}

void EndingRenderer::render() {
    if (!platform_) return;

    int32_t img_idx = pick_image_index();
    char img_name[16];
    std::snprintf(img_name, sizeof(img_name), "END%02d.IMG", img_idx);
    images_->draw_image(img_name, 0, 0);

    platform_->fill_rect(32, 330, 576, 130, 0, 0, 0);
    platform_->draw_rect(32, 330, 576, 130, 80, 80, 120);

    uint8_t ta = static_cast<uint8_t>(std::max(0, fade_alpha_));
    const char* title = pick_title();
    font_->draw_text(48, 340, title, ta, ta, static_cast<uint8_t>(200 * ta / 255));

    if (!ending_text_.empty() && fade_alpha_ > 50) {
        uint8_t text_a = static_cast<uint8_t>(std::min(220, 220 * fade_alpha_ / 255));
        font_->draw_text_wrapped(48, 370, 544, ending_text_, text_a, text_a, text_a);
    }

    if (can_skip_) {
        uint8_t ha = static_cast<uint8_t>(160 * fade_alpha_ / 255);
        font_->draw_text(200, 460, "\xB0\xB4\xC8\xCE\xD2\xBC\xFC\xB7\xB5\xBB\xD8",
                         ha, ha, ha);
    }
}

bool EndingRenderer::process_input(const platform::Event& event) {
    if (finished_ || !can_skip_) return false;

    if (event.type == platform::Event::Type::KeyDown ||
        (event.type == platform::Event::Type::MouseDown && event.mouse_button == 1)) {
        finished_ = true;
        return true;
    }

    return false;
}

}  // namespace dlzr::presentation
