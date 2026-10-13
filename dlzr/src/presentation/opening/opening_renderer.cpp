#include "presentation/opening/opening_renderer.hpp"

#include <algorithm>

namespace dlzr::presentation {

void OpeningRenderer::init(platform::Platform& platform, FontRenderer& font, ImageCache& images) {
    platform_ = &platform;
    font_ = &font;
    images_ = &images;
}

void OpeningRenderer::start(const logic::GameStateManager& gsm) {
    scenes_.clear();
    current_scene_ = 0;
    scene_timer_ = 0;
    playing_ = true;
    finished_ = false;
    fade_alpha_ = 0;
    fading_in_ = true;
    fading_out_ = false;
    requested_bg_ = -1;

    build_sequence(gsm);
}

void OpeningRenderer::build_sequence(const logic::GameStateManager& gsm) {
    scenes_.push_back({"\xBF\xB9\xC8\xD5\xD6\xAE\xB5\xD8\xC0\xD7\xD5\xBD",
                       0, 3000, 800, 600, -1, true, 180});

    scenes_.push_back({"\xD2\xBB\xBE\xC5\xCB\xC4\xC4\xC4\xEA\xA3\xAC\xC8\xD5\xBF\xDB\xC7\xC0\xB8\xBA\xBB\xAA\xB1\xB1\xA1\xa3"
                       "\xD6\xD0\xB9\xFA\xB9\xb2\xb2\xB5\xB3\xC1\xEC\xB5\xBC\xCF\xC2\xA3\xAC"
                       "\xBF\xB9\xC8\xD5\xCE\xe4\xd7\xb0\xb6\xd3\xD4\xDA\xBB\xAA\xB1\xB1\xB5\xD8\xC7\xF8\xD5\xB9\xBF\xAA\xA3\xAC"
                       "\xB7\xA2\xB6\xAF\xC8\xBA\xD6\xDA\xA3\xAC\xBF\xAA\xD5\xB9\xB5\xD8\xC0\xD7\xD5\xBD\xA1\xa3",
                       2, 6000, 600, 400, -1, true, 160});

    scenes_.push_back({"\xC4\xE3\xBD\xAB\xD6\xB8\xBB\xD3\xBF\xB9\xC8\xD5\xCE\xe4\xd7\xb0\xb6\xd3\xA3\xAC"
                       "\xD4\xCB\xD3\xC3\xB5\xD8\xC0\xD7\xD3\xEB\xC8\xD5\xBE\xFC\xD6\xDC\xD0\xFD\xA3\xAC"
                       "\xB1\xA3\xCE\xC0\xCF\xE7\xC0\xEF\xB5\xC4\xB8\xB8\xC0\xCF\xCF\xE7\xC7\xD7\xA1\xa3",
                       3, 5000, 600, 400, -1, true, 200});

    if (gsm.game_data().current_fight_index == 0) {
        scenes_.push_back({"\xB5\xDA\xD2\xBB\xB9\xD8\xA3\xBA\xB1\xA3\xCE\xC0\xD7\xAF\xD4\xB0"
                           "\xA1\xa3\xCF\xFB\xC3\xF0\xC8\xEB\xC7\xD6\xB5\xC4\xC8\xD5\xBE\xFC\xA3\xA1",
                           1, 4000, 500, 400, -1, true, 220});
    } else {
        scenes_.push_back({"\xBC\xCC\xD0\xF8\xD5\xBD\xB6\xB7\xA3\xA1"
                           "\xC4\xE3\xD2\xD1\xBE\xAD\xCD\xEA\xB3\xC9\xC1\xCB\xB9\xD8\xBF\xA8\xA1\xa3",
                           1, 3000, 500, 400, -1, true, 220});
    }

    scenes_.push_back({"\xB0\xB4\xC8\xCE\xD2\xE2\xBC\xFC\xBF\xAA\xCA\xBC\xD3\xCE\xCF\xB7"
                       "\xA1\xa3",
                       0, 0, 300, 200, -1, true, 400});
}

int32_t OpeningRenderer::compute_fade_alpha() const {
    if (current_scene_ >= scenes_.size()) return 0;
    const auto& scene = scenes_[current_scene_];

    if (fading_in_) {
        if (scene.fade_in_ms <= 0) return 255;
        float t = static_cast<float>(scene_timer_) / static_cast<float>(scene.fade_in_ms);
        if (t > 1.0f) t = 1.0f;
        return static_cast<int32_t>(255.0f * t);
    }
    if (fading_out_) {
        int32_t remaining = scene.duration_ms - static_cast<int32_t>(scene_timer_);
        if (remaining < 0) remaining = 0;
        if (scene.fade_out_ms <= 0) return (remaining > 0) ? 255 : 0;
        float t = static_cast<float>(remaining) / static_cast<float>(scene.fade_out_ms);
        if (t > 1.0f) t = 1.0f;
        return static_cast<int32_t>(255.0f * t);
    }
    return 255;
}

bool OpeningRenderer::update(uint32_t delta_ms) {
    if (!playing_ || finished_) return true;
    if (current_scene_ >= scenes_.size()) {
        finished_ = true;
        playing_ = false;
        return true;
    }

    scene_timer_ += delta_ms;
    const auto& scene = scenes_[current_scene_];

    if (fading_in_ && scene.fade_in_ms > 0 &&
        scene_timer_ >= static_cast<uint32_t>(scene.fade_in_ms)) {
        fading_in_ = false;
    }

    if (scene.duration_ms > 0 && !fading_in_) {
        int32_t fade_start = scene.duration_ms - scene.fade_out_ms;
        if (fade_start < 0) fade_start = 0;
        if (static_cast<int32_t>(scene_timer_) >= scene.duration_ms) {
            advance_scene();
        } else if (static_cast<int32_t>(scene_timer_) >= fade_start && !fading_out_) {
            fading_out_ = true;
        }
    }

    return finished_;
}

void OpeningRenderer::render() {
    if (!playing_ || finished_ || current_scene_ >= scenes_.size()) return;

    const auto& scene = scenes_[current_scene_];
    int32_t alpha = compute_fade_alpha();

    if (scene.background_img >= 0) {
        char img_name[16];
        if (scene.background_img == 0) {
            std::snprintf(img_name, sizeof(img_name), "DLZ00.IMG");
        } else {
            std::snprintf(img_name, sizeof(img_name), "BG0%d.IMG", scene.background_img);
        }
        images_->draw_image(img_name, 0, 0);
    } else {
        platform_->fill_rect(0, 0, 640, 480, 0, 0, 0);
    }

    if (alpha < 255) {
        int32_t overlay = 255 - alpha;
        platform_->fill_rect(0, 0, 640, 480,
                             static_cast<uint8_t>(overlay),
                             static_cast<uint8_t>(overlay),
                             static_cast<uint8_t>(overlay));
    }

    if (!scene.text.empty() && alpha > 30) {
        uint8_t cr = static_cast<uint8_t>(std::min(255, 255 * alpha / 255));
        uint8_t cg = static_cast<uint8_t>(std::min(255, 255 * alpha / 255));
        uint8_t cb = static_cast<uint8_t>(std::min(255, 255 * alpha / 255));

        if (scene.text_shadow) {
            font_->draw_text(42, scene.text_y + 2, scene.text, 0, 0, 0);
        }
        font_->draw_text(40, scene.text_y, scene.text, cr, cg, cb);
    }

    if (current_scene_ == scenes_.size() - 1 && scenes_.back().duration_ms == 0) {
        uint8_t ha = static_cast<uint8_t>(180 * alpha / 255);
        font_->draw_text(200, 440, "\xa3\xa8\xb0\xb4\xc8\xce\xd2\xe2\xbc\xfc\xbc\xcc\xd0\xf8\xa3\xa9",
                         ha, ha, ha);
    }
}

bool OpeningRenderer::process_input(const platform::Event& event) {
    if (!playing_ || finished_) return false;

    if (event.type == platform::Event::Type::KeyDown ||
        (event.type == platform::Event::Type::MouseDown && event.mouse_button == 1)) {
        advance_scene();
        return true;
    }

    return false;
}

void OpeningRenderer::advance_scene() {
    ++current_scene_;
    scene_timer_ = 0;
    fading_in_ = true;
    fading_out_ = false;

    if (current_scene_ >= scenes_.size()) {
        finished_ = true;
        playing_ = false;
    }
}

}  // namespace dlzr::presentation
