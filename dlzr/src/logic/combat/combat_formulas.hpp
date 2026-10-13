#pragma once

// combat_formulas.hpp — 战斗公式实现
// Combat formula implementations
//
// 所有公式严格匹配原作 DLZ.exe 反编译结果。
// All formulas strictly match decompiled DLZ.exe binary.
// 整数除法使用截断除法（向零取整），与 x86 idiv 一致。
// Integer division uses truncation toward zero, matching x86 idiv.

#include "core/data_model/types.hpp"
#include "core/data_model/constants.hpp"

#include <cstdint>

namespace dlzr::logic {

// ============================================================
// 地形防御加成查询 / Terrain defense bonus lookup
//
// RE: terrain_defense_table @ 0x00475AB0
// Returns defense bonus percentage for given terrain type
// ============================================================
int32_t get_terrain_defense_bonus(int32_t terrain_type);

// ============================================================
// HP 计算 / HP calculation
// max_hp = class_hp_base + (level - 1) × class_hp_growth
// ============================================================
int32_t calc_max_hp(const core::People& person,
                    const core::ClassType& class_type);

// ============================================================
// 攻击力计算 / Attack power calculation (0x0042A82A)
//
// Layer 1: raw_atk = (class_base_atk × 3 × strength / 100)
//                    + (attack_growth - 1) × (level - 1)
// Layer 2: attack = (skill_level + 59) × (raw_atk × (strength + 500) / 580) / 60
// Layer 3: (battle mode only)
//          attack = attack × equip_integrity / 100
//          attack = attack × (morale + 100) / 200
// Layer 4: attack += item_attack_bonus
// ============================================================
int32_t calc_attack_power(const core::People& person,
                          const core::ClassType& class_type,
                          const core::Fighter& fighter,
                          int32_t skill_level,
                          bool in_battle_mode,
                          int32_t item_attack_bonus = 0);

// ============================================================
// 防御力计算 / Defense calculation (0x0042A98E)
//
// Layer 1: defense = class_base_defense + (level - 1) × defense_growth
// Layer 2: (battle mode only)
//          defense = defense × defense_modifier / 100
//          defense = defense × (morale + 100) / 200
// Layer 3: defense += defense × terrain_bonus_percent / 100
// Layer 4: defense += item_defense_bonus
// ============================================================
int32_t calc_defense(const core::People& person,
                     const core::ClassType& class_type,
                     const core::Fighter& fighter,
                     int32_t terrain_bonus_percent,
                     bool in_battle_mode,
                     int32_t item_defense_bonus = 0);

// ============================================================
// 技能等级查询 / Skill level lookup
// 查表: {0,1,3,6,10,15,21,28,36,45,55} → level 0-10
// ============================================================
int32_t get_skill_level(int32_t skill_use_count);

// ============================================================
// 技能效果计算 / Skill effect calculation (0x0042B2E0)
//
// hp_effect     = ((param1 × intelligence) / 100) × (skill_level + 7) >> 3
// morale_effect = ((param2 × intelligence) / 100) × (skill_level + 7) >> 3
// ============================================================
struct SkillEffect {
    int32_t hp_effect{0};
    int32_t morale_effect{0};
};

SkillEffect calc_skill_effect(const core::People& caster,
                              const core::JiNeng& skill,
                              int32_t skill_level);

// ============================================================
// 经验值计算 / Experience calculation
//
// level_diff = killer_level - victim_level
// multiplier: <-5: 2.0, <0: 1.5, <5: 1.0, <10: 0.75, else: 0.5
// final_exp = base_exp × multiplier (整数运算)
// ============================================================
int32_t calc_experience(int32_t killer_level,
                        int32_t victim_level,
                        int32_t base_exp);

// ============================================================
// 伤害结算 / Damage resolution
//
// damage = attack_power - defense
// min damage = 1
// ============================================================
int32_t calc_damage(int32_t attack_power, int32_t defense);

}  // namespace dlzr::logic
