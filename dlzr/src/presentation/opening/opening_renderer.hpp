#pragma once

// opening_renderer.hpp — 开场动画渲染器
// Opening sequence renderer with fade transitions and background images

#include "core/data_model/types.hpp"
#include "logic/game_state_manager.hpp"
#include "platform/platform.hpp"
#include "presentation/font/font_renderer.hpp"
#include "presentation/image/image_cache.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace dlzr::presentation {

struct OpeningScene {
    std::string text;
    int32_t background_img{-1};
    int32_t duration_ms{0};
    int32_t fade_in_ms{500};
    int32_t fade_out_ms{500};
    int32_t bgm_track{-1};
    bool text_shadow{true};
    int32_t text_y{200};
};

class OpeningRenderer {
public:
    void init(platform::Platform& platform, FontRenderer& font, ImageCache& images);

    void start(const logic::GameStateManager& gsm);
    bool update(uint32_t delta_ms);
    void render();
    bool process_input(const platform::Event& event);

    bool is_playing() const { return playing_; }
    int32_t requested_bg() const { return requested_bg_; }

private:
    platform::Platform* platform_{nullptr};
    FontRenderer* font_{nullptr};
    ImageCache* images_{nullptr};

    std::vector<OpeningScene> scenes_;
    size_t current_scene_{0};
    uint32_t scene_timer_{0};
    bool playing_{false};
    bool finished_{false};

    int32_t fade_alpha_{0};
    bool fading_in_{false};
    bool fading_out_{false};
    int32_t requested_bg_{-1};

    void build_sequence(const logic::GameStateManager& gsm);
    void advance_scene();
    int32_t compute_fade_alpha() const;
};

}  // namespace dlzr::presentation
