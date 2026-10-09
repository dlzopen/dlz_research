#pragma once

// promotion_renderer.hpp — 转职界面渲染器
// Class promotion screen renderer

#include "logic/game_state_manager.hpp"
#include "logic/promotion/promotion_system.hpp"
#include "platform/platform.hpp"
#include "presentation/font/font_renderer.hpp"

#include <cstdint>

namespace dlzr::presentation {

class PromotionRenderer {
public:
    void init(platform::Platform& platform, FontRenderer& font);

    void render(const logic::GameStateManager& gsm,
                const logic::PromotionSystem& promotion,
                int32_t selected_char);

    int32_t char_at_position(int32_t mx, int32_t my,
                              const core::GameData& game_data) const;

private:
    platform::Platform* platform_{nullptr};
    FontRenderer* font_{nullptr};

    static constexpr int32_t CHAR_W = 280;
    static constexpr int32_t CHAR_H = 30;
    static constexpr int32_t GRID_X = 40;
    static constexpr int32_t GRID_Y = 80;
    static constexpr int32_t COLS = 2;
};

}  // namespace dlzr::presentation
