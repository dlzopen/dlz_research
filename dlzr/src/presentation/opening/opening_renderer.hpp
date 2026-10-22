#pragma once

// opening_renderer.hpp — 开场幻灯片渲染器
// Opening slideshow renderer
//
// RE 证据（output/ghidra_all/FUN_0042e0f1.c + FUN_0042dc9e.c）：
//   - 确定后进幻灯片：先画 NPIC25.IMG（居中），然后模态消息循环；
//   - WM_TIMER(0x113) 每 55ms 一跳（FUN_004249a2: SetTimer(...,1,0x37)），
//     计数器 DAT_007d0cf4 达到各阈值时 FUN_0042dc9e 换下一张图；
//   - WM_LBUTTONDOWN(0x201) 立即结束整个幻灯片 → 进战斗；
//   - 全程无任何文字叠加。

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
    std::string image;        // 本屏 IMG 文件名（DLZFULL 目录）
    int32_t duration_ms{0};   // 0 = 等待鼠标左键（末屏）
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

    int32_t requested_bg_{-1};

    void build_sequence(const logic::GameStateManager& gsm);
    void advance_scene();
};

}  // namespace dlzr::presentation
