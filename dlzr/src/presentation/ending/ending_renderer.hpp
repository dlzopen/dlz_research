#pragma once

#include "platform/platform.hpp"
#include "presentation/font/font_renderer.hpp"
#include "presentation/image/image_cache.hpp"

#include <cstdint>
#include <string>

namespace dlzr::presentation {

class EndingRenderer {
public:
    void init(platform::Platform& platform, FontRenderer& font,
              ImageCache& images);

    void start(int32_t ending_id, const std::string& ending_text);
    bool update(uint32_t delta_ms);
    void render();
    bool process_input(const platform::Event& event);
    bool is_finished() const { return finished_; }

private:
    platform::Platform* platform_{nullptr};
    FontRenderer* font_{nullptr};
    ImageCache* images_{nullptr};

    int32_t ending_id_{0};
    std::string ending_text_;
    uint32_t timer_{0};
    int32_t fade_alpha_{0};
    bool finished_{false};
    bool can_skip_{false};

    static constexpr int32_t FADE_IN_MS = 800;
    static constexpr int32_t SKIP_DELAY_MS = 1500;

    int32_t pick_image_index() const;
    const char* pick_title() const;
};

}  // namespace dlzr::presentation
