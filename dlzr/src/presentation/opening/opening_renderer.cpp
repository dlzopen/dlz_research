#include "presentation/opening/opening_renderer.hpp"

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

    build_sequence(gsm);
}

void OpeningRenderer::build_sequence(const logic::GameStateManager& gsm) {
    // 开场序列（GBK 编码）/ Opening sequence (GBK encoded)
    // 原作中这些是 MPEG 过场动画，此处用文本替代
    // In original these were MPEG cutscenes, here we use text instead

    // Scene 1: Title card
    scenes_.push_back({"\xBF\xB9\xC8\xD5\xD6\xAE\xB5\xD8\xC0\xD7\xD5\xBD",  // 抗日之地雷战
                       -1, 3000});

    // Scene 2: Story intro
    scenes_.push_back({"\xD2\xBB\xBE\xC5\xCB\xC4\xC4\xC4\xEA\xA3\xAC\xC8\xD5\xBF\xDB\xC7\xC0\xB8\xBA\xBB\xAA\xB1\xB1\xA1\xa3"
                       "\xD6\xD0\xB9\xFA\xB9\xb2\xb2\xB5\xB3\xC1\xEC\xB5\xBC\xCF\xC2\xA3\xAC"
                       "\xBF\xB9\xC8\xD5\xCE\xe4\xd7\xb0\xb6\xd3\xD4\xDA\xBB\xAA\xB1\xB1\xB5\xD8\xC7\xF8\xD5\xB9\xBF\xAA\xA3\xAC"
                       "\xB7\xA2\xB6\xAF\xC8\xBA\xD6\xDA\xA3\xAC\xBF\xAA\xD5\xB9\xB5\xD8\xC0\xD7\xD5\xBD\xA1\xa3",
                       // 一九四四年，日寇穷凶极恶。中国共产党领导下，
                       // 抗日武装在华北地区展开，发动群众，开展地雷战。
                       -1, 5000});

    // Scene 3: Mission briefing
    scenes_.push_back({"\xC4\xE3\xBD\xAB\xD6\xB8\xBB\xD3\xBF\xB9\xC8\xD5\xCE\xe4\xd7\xb0\xb6\xd3\xA3\xAC"
                       "\xD4\xCB\xD3\xC3\xB5\xD8\xC0\xD7\xD3\xEB\xC8\xD5\xBE\xFC\xD6\xDC\xD0\xFD\xA3\xAC"
                       "\xB1\xA3\xCE\xC0\xCF\xE7\xC0\xEF\xB5\xC4\xB8\xB8\xC0\xCF\xCF\xE7\xC7\xD7\xA1\xa3",
                       // 你将指挥抗日武装队伍，运用地雷与日军周旋，
                       // 保卫乡里的父老乡亲。
                       -1, 5000});

    // Scene 4: Check if this is a new game or has progress
    if (gsm.game_data().current_fight_index == 0) {
        scenes_.push_back({"\xB5\xDA\xD2\xBB\xB9\xD8\xA3\xBA\xB1\xA3\xCE\xC0\xD7\xAF\xD4\xB0"
                           "\xA1\xa3\xCF\xFB\xC3\xF0\xC8\xEB\xC7\xD6\xB5\xC4\xC8\xD5\xBE\xFC\xA3\xA1",
                           // 第一关：保卫庄园。消灭入侵的日军！
                           -1, 4000});
    } else {
        scenes_.push_back({"\xBC\xCC\xD0\xF8\xD5\xBD\xB6\xB7\xA3\xA1"
                           "\xC4\xE3\xD2\xD1\xBE\xAD\xCD\xEA\xB3\xC9\xC1\xCB" "10" "\xB8\xF6\xB9\xD8\xBF\xA8\xA1\xa3",
                           // 继续战斗！你已经完成了10个关卡。
                           -1, 3000});
    }

    // Final scene: "Press any key to start"
    scenes_.push_back({"\xB0\xB4\xC8\xCE\xD2\xE2\xBC\xFC\xBF\xAA\xCA\xBC\xD3\xCE\xCF\xB7"
                       "\xA1\xa3",
                       // 按任意键开始游戏。
                       -1, 0});  // 0 = wait for input
}

bool OpeningRenderer::update(uint32_t delta_ms) {
    if (!playing_ || finished_) {
        return true;
    }

    if (current_scene_ >= scenes_.size()) {
        finished_ = true;
        playing_ = false;
        return true;
    }

    scene_timer_ += delta_ms;
    const auto& scene = scenes_[current_scene_];

    // Auto-advance if duration > 0 and timer exceeded
    if (scene.duration_ms > 0 && scene_timer_ >= static_cast<uint32_t>(scene.duration_ms)) {
        advance_scene();
    }

    return finished_;
}

void OpeningRenderer::render() {
    if (!playing_ || finished_ || current_scene_ >= scenes_.size()) {
        return;
    }

    const auto& scene = scenes_[current_scene_];

    // Dark background (opening sequence uses text only, no images)
    platform_->fill_rect(0, 0, 640, 480, 0, 0, 0);

    // Render text centered
    if (!scene.text.empty()) {
        int32_t text_y = 200;
        int32_t text_x = 40;
        font_->draw_text(text_x, text_y, scene.text, 255, 255, 255);
    }

    // Show "press any key" hint on final scene
    if (current_scene_ == scenes_.size() - 1 && scenes_.back().duration_ms == 0) {
        std::string hint = "\xa3\xa8\xb0\xb4\xc8\xce\xd2\xe2\xbc\xfc\xbc\xcc\xd0\xf8\xa3\xa9";
        // （按任意键继续）
        font_->draw_text(200, 400, hint, 180, 180, 180);
    }
}

bool OpeningRenderer::process_input(const platform::Event& event) {
    if (!playing_ || finished_) {
        return false;
    }

    // Any key or mouse click advances
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

    if (current_scene_ >= scenes_.size()) {
        finished_ = true;
        playing_ = false;
    }
}

}  // namespace dlzr::presentation
