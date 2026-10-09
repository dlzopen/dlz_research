#pragma once

// opening_renderer.hpp — 开场动画渲染器
// Opening sequence renderer
//
// 播放游戏开场序列（原作用 MPEG 过场，此处用文本/图片序列替代）。
// Plays game opening sequence (original used MPEG cutscenes, this uses text/image sequence).

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
    std::string text;           // GBK encoded text
    int32_t background_img;     // Image ID (-1 = no image)
    int32_t duration_ms;        // Auto-advance time (0 = wait for input)
};

class OpeningRenderer {
public:
    void init(platform::Platform& platform, FontRenderer& font, ImageCache& images);

    // 开始播放开场序列 / Start opening sequence
    void start(const logic::GameStateManager& gsm);

    // 更新状态 / Update state
    // 返回 true 表示开场结束 / Returns true when opening is finished
    bool update(uint32_t delta_ms);

    // 渲染当前场景 / Render current scene
    void render();

    // 处理输入（跳过/推进）/ Handle input (skip/advance)
    bool process_input(const platform::Event& event);

    // 是否正在播放 / Is playing
    bool is_playing() const { return playing_; }

private:
    platform::Platform* platform_{nullptr};
    FontRenderer* font_{nullptr};
    ImageCache* images_{nullptr};

    std::vector<OpeningScene> scenes_;
    size_t current_scene_{0};
    uint32_t scene_timer_{0};
    bool playing_{false};
    bool finished_{false};

    void build_sequence(const logic::GameStateManager& gsm);
    void advance_scene();
};

}  // namespace dlzr::presentation
