#pragma once

// battle_result_renderer.hpp — 战斗结果画面渲染器
// Battle result screen (victory/defeat)

#include "logic/battle/battle_engine.hpp"
#include "platform/platform.hpp"
#include "presentation/font/font_renderer.hpp"

#include <cstdint>

namespace dlzr::presentation {

class BattleResultRenderer {
public:
    void init(platform::Platform& platform, FontRenderer& font);

    void render(bool is_victory, int32_t score,
                const logic::BattleEngine::ScoreBreakdown& breakdown);

private:
    platform::Platform* platform_{nullptr};
    FontRenderer* font_{nullptr};
};

}  // namespace dlzr::presentation
