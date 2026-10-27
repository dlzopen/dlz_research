#pragma once

// battle_animation_controller.hpp — 战斗动画序列控制器
// Battle animation sequence controller
//
// 管理多个并发动画：移动插值、攻击序列、伤害数字、死亡效果。
// Manages concurrent animations: movement interpolation, attack sequences,
// damage numbers, death effects.

#include "logic/battle/battle_state.hpp"
#include "platform/platform.hpp"
#include "presentation/font/font_renderer.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace dlzr::presentation {

enum class BattleAnimType {
    NONE,
    MOVE,           // 移动插值 / Movement interpolation
    ATTACK,         // 攻击动作 / Attack action (attacker lunge)
    IMPACT,         // 命中特效 / Impact flash at target
    DAMAGE_NUMBER,  // 伤害数字弹出 / Damage number popup
    DEATH,          // 死亡效果 / Death fade-out
    SKILL_CAST,     // 技能施放 / Skill cast glow
    HEAL,           // 治疗光效 / Heal glow
};

struct BattleAnimEntry {
    BattleAnimType type{BattleAnimType::NONE};
    int32_t entity_idx{-1};
    int32_t target_idx{-1};

    // 位置 / Position
    float start_x{0}, start_y{0};
    float end_x{0}, end_y{0};
    float current_x{0}, current_y{0};

    // 时间 / Timing
    int32_t duration_ms{0};
    int32_t elapsed_ms{0};

    // 数值 / Value (for damage numbers)
    int32_t value{0};

    // 颜色 / Color (for damage numbers and effects)
    uint8_t color_r{255}, color_g{255}, color_b{0};

    bool active{false};
};

class BattleAnimationController {
public:
    static constexpr int32_t MAX_CONCURRENT_ANIMS = 32;
    static constexpr int32_t MOVE_DURATION_MS = 300;
    static constexpr int32_t ATTACK_DURATION_MS = 200;
    static constexpr int32_t IMPACT_DURATION_MS = 250;
    static constexpr int32_t DAMAGE_NUMBER_DURATION_MS = 800;
    static constexpr int32_t DEATH_DURATION_MS = 500;
    static constexpr int32_t SKILL_CAST_DURATION_MS = 400;
    static constexpr int32_t HEAL_DURATION_MS = 400;

    void init(int32_t grid_origin_x, int32_t grid_origin_y, int32_t cell_size);

    // 队列动画 / Queue animations
    void queue_move(int32_t entity_idx, int32_t from_x, int32_t from_y,
                    int32_t to_x, int32_t to_y);
    void queue_attack(int32_t attacker_idx, int32_t attacker_x, int32_t attacker_y,
                      int32_t target_idx, int32_t target_x, int32_t target_y);
    void queue_impact(int32_t target_x, int32_t target_y);
    void queue_damage_number(int32_t target_x, int32_t target_y, int32_t damage,
                             bool is_heal = false);
    void queue_death(int32_t entity_idx, int32_t grid_x, int32_t grid_y);
    void queue_skill_cast(int32_t caster_idx, int32_t grid_x, int32_t grid_y);
    void queue_skill_cast(int32_t caster_idx, int32_t grid_x, int32_t grid_y,
                          int32_t skill_type);

    // 每帧更新 / Per-frame update
    void update(int32_t delta_ms);

    // 获取移动偏移（战斗渲染器用来偏移精灵位置）
    // Get movement offset (battle renderer uses this to offset sprite position)
    bool get_move_offset(int32_t entity_idx, float& offset_x, float& offset_y) const;

    // 获取死亡透明度（0-255，255=完全可见）
    // Get death alpha (0-255, 255=fully visible)
    int32_t get_death_alpha(int32_t entity_idx) const;

    // 渲染动画效果 / Render animation effects
    void render(platform::Platform& plat, FontRenderer& font) const;

    // 是否有活跃动画 / Whether any animation is active
    bool has_active_animations() const;

    // 等待动画完成（阻塞） / Wait for all animations to complete
    bool is_busy() const;

private:
    std::array<BattleAnimEntry, MAX_CONCURRENT_ANIMS> entries_{};

    int32_t grid_origin_x_{0};
    int32_t grid_origin_y_{0};
    int32_t cell_size_{40};

    BattleAnimEntry* find_free_entry();
    float grid_to_screen_x(int32_t grid_x) const;
    float grid_to_screen_y(int32_t grid_y) const;

    static float ease_out_quad(float t);
    static float ease_in_out_sine(float t);
};

}  // namespace dlzr::presentation
