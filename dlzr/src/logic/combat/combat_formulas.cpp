// combat_formulas.cpp — 战斗公式实现
// Combat formula implementations
//
// 所有公式严格匹配原作 DLZ.exe 反编译结果。
// 整数除法使用 C++ 截断除法（向零取整），与 x86 idiv 行为一致。

#include "logic/combat/combat_formulas.hpp"

#include <algorithm>

namespace dlzr::logic {

// 地形防御加成表 / Terrain defense bonus table
// RE: terrain_defense_table @ 0x00475AB0, 20-byte stride, first field = bonus %
// Simplified for remake: index = terrain type, value = defense bonus percentage
static constexpr std::array<int32_t, 16> TERRAIN_DEFENSE_BONUS = {
    0,   // 0: 平地 / Plain
    10,  // 1: 草地 / Grass
    20,  // 2: 树林 / Forest
    30,  // 3: 山地 / Mountain
    15,  // 4: 丘陵 / Hill
    25,  // 5: 沼泽 / Swamp
    0,   // 6: 河流 / River (impassable)
    0,   // 7: 未知 / Unknown
    0,   // 8: 未知 / Unknown
    0,   // 9: 未知 / Unknown
    0,   // 10: 未知 / Unknown
    0,   // 11: 未知 / Unknown
    0,   // 12: 未知 / Unknown
    0,   // 13: 未知 / Unknown
    0,   // 14: 未知 / Unknown
    0,   // 15: 障碍 / Obstacle (99 mapped here)
};

int32_t get_terrain_defense_bonus(int32_t terrain_type) {
    if (terrain_type < 0 || terrain_type >= static_cast<int32_t>(TERRAIN_DEFENSE_BONUS.size())) {
        return 0;
    }
    return TERRAIN_DEFENSE_BONUS[terrain_type];
}

int32_t calc_max_hp(const core::People& person,
                    const core::ClassType& class_type) {
    return class_type.hp_base
         + (person.level - 1) * class_type.hp_growth;
}

int32_t calc_attack_power(const core::People& person,
                          const core::ClassType& class_type,
                          const core::Fighter& fighter,
                          int32_t skill_level,
                          bool in_battle_mode,
                          int32_t item_attack_bonus) {
    // Layer 1: 基础攻击 / Base attack
    int32_t raw_atk = (class_type.base_attack * 3 * person.strength / 100)
                    + (class_type.attack_growth - 1) * (person.level - 1);

    // Layer 2: 武力×技能缩放 / Strength × skill scaling
    int32_t attack = (skill_level + core::SKILL_LEVEL_BASE)
                   * (raw_atk * (person.strength + core::STRENGTH_OFFSET)
                      / core::ATTACK_SCALE_DIV)
                   / core::ATTACK_DIVISOR;

    // Layer 3: 战斗模式修正 / Battle mode modifier
    if (in_battle_mode) {
        attack = attack * fighter.equip_integrity / core::INTEGRITY_DIVISOR;
        attack = attack * (fighter.morale + core::MORALE_BASE)
               / core::MORALE_DIVISOR;
    }

    // Layer 4: 物品加成 / Item bonus
    attack += item_attack_bonus;

    return attack;
}

int32_t calc_defense(const core::People& person,
                     const core::ClassType& class_type,
                     const core::Fighter& fighter,
                     int32_t terrain_bonus_percent,
                     bool in_battle_mode,
                     int32_t item_defense_bonus) {
    // Layer 1: 基础防御 / Base defense
    int32_t defense = class_type.base_defense
                    + (person.level - 1) * class_type.defense_growth;

    // Layer 2: 战斗修正 / Battle modifier
    if (in_battle_mode) {
        defense = defense * fighter.defense_modifier / 100;
        defense = defense * (fighter.morale + core::MORALE_BASE)
                / core::MORALE_DIVISOR;
    }

    // Layer 3: 地形加成 / Terrain bonus
    defense += defense * terrain_bonus_percent / 100;

    // Layer 4: 物品加成 / Item bonus
    defense += item_defense_bonus;

    return defense;
}

int32_t get_skill_level(int32_t skill_use_count) {
    for (int32_t level = core::SKILL_LEVEL_COUNT - 1; level >= 0; --level) {
        if (skill_use_count >= core::SKILL_LEVEL_THRESHOLDS[level]) {
            return level;
        }
    }
    return 0;
}

SkillEffect calc_skill_effect(const core::People& caster,
                              const core::JiNeng& skill,
                              int32_t skill_level) {
    SkillEffect effect;

    int32_t factor = (skill_level + 7) >> 3;

    effect.hp_effect = (skill.para1 * caster.intelligence / 100) * factor;
    effect.morale_effect = (skill.para2 * caster.intelligence / 100) * factor;

    return effect;
}

int32_t calc_experience(int32_t killer_level,
                        int32_t victim_level,
                        int32_t base_exp) {
    int32_t level_diff = killer_level - victim_level;
    int32_t multiplier_x4;

    if (level_diff < -5) {
        multiplier_x4 = 8;   // 2.0 × 4
    } else if (level_diff < 0) {
        multiplier_x4 = 6;   // 1.5 × 4
    } else if (level_diff < 5) {
        multiplier_x4 = 4;   // 1.0 × 4
    } else if (level_diff < 10) {
        multiplier_x4 = 3;   // 0.75 × 4
    } else {
        multiplier_x4 = 2;   // 0.5 × 4
    }

    return base_exp * multiplier_x4 / 4;
}

int32_t calc_damage(int32_t attack_power, int32_t defense) {
    int32_t damage = attack_power - defense;
    return std::max(damage, 1);
}

}  // namespace dlzr::logic
