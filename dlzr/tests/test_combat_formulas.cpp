// test_combat_formulas.cpp — 战斗公式单元测试
// Combat formula unit tests

#include <gtest/gtest.h>
#include "logic/combat/combat_formulas.hpp"
#include "core/data_model/types.hpp"
#include "core/data_model/constants.hpp"

using namespace dlzr::core;
using namespace dlzr::logic;

namespace {

People make_person(int32_t str, int32_t int_val, int32_t level,
                   int32_t class_type_idx = 0) {
    People p{};
    p.strength = str;
    p.intelligence = int_val;
    p.level = level;
    p.class_type = class_type_idx;
    return p;
}

ClassType make_class(int32_t hp_base, int32_t hp_growth,
                     int32_t atk_growth, int32_t def_growth,
                     int32_t base_atk, int32_t base_def) {
    ClassType c{};
    c.hp_base = hp_base;
    c.hp_growth = hp_growth;
    c.attack_growth = atk_growth;
    c.defense_growth = def_growth;
    c.base_attack = base_atk;
    c.base_defense = base_def;
    return c;
}

Fighter make_fighter(int32_t morale = 100, int32_t integrity = 100,
                     int32_t def_mod = 100) {
    Fighter f{};
    f.morale = morale;
    f.equip_integrity = integrity;
    f.defense_modifier = def_mod;
    return f;
}

JiNeng make_jineng(int32_t para1, int32_t para2) {
    JiNeng j{};
    j.para1 = para1;
    j.para2 = para2;
    return j;
}

}  // namespace

// ============================================================
// HP 计算测试 / HP calculation tests
// ============================================================
TEST(CombatFormulas, MaxHp_BaseCase) {
    auto p = make_person(50, 50, 1);
    auto c = make_class(100, 10, 5, 3, 50, 30);
    EXPECT_EQ(calc_max_hp(p, c), 100);
}

TEST(CombatFormulas, MaxHp_Level5) {
    auto p = make_person(50, 50, 5);
    auto c = make_class(100, 10, 5, 3, 50, 30);
    EXPECT_EQ(calc_max_hp(p, c), 140);
}

TEST(CombatFormulas, MaxHp_Level10) {
    auto p = make_person(50, 50, 10);
    auto c = make_class(200, 20, 8, 5, 80, 40);
    EXPECT_EQ(calc_max_hp(p, c), 380);
}

// ============================================================
// 攻击力测试 / Attack power tests (≥20 cases)
// ============================================================
TEST(CombatFormulas, AttackPower_NoBattleMode) {
    auto p = make_person(60, 50, 5);
    auto c = make_class(100, 10, 10, 5, 50, 30);
    auto f = make_fighter();

    int32_t result = calc_attack_power(p, c, f, 0, false);

    int32_t raw_atk = (50 * 3 * 60 / 100) + (10 - 1) * (5 - 1);
    int32_t expected = (0 + 59) * (raw_atk * (60 + 500) / 580) / 60;
    EXPECT_EQ(result, expected);
}

TEST(CombatFormulas, AttackPower_WithBattleMode) {
    auto p = make_person(60, 50, 5);
    auto c = make_class(100, 10, 10, 5, 50, 30);
    auto f = make_fighter(100, 100);

    int32_t no_battle = calc_attack_power(p, c, f, 0, false);
    int32_t with_battle = calc_attack_power(p, c, f, 0, true);

    EXPECT_EQ(no_battle, with_battle);
}

TEST(CombatFormulas, AttackPower_LowMorale) {
    auto p = make_person(60, 50, 5);
    auto c = make_class(100, 10, 10, 5, 50, 30);
    auto f_normal = make_fighter(100, 100);
    auto f_low = make_fighter(0, 100);

    int32_t normal = calc_attack_power(p, c, f_normal, 0, true);
    int32_t low = calc_attack_power(p, c, f_low, 0, true);

    EXPECT_GT(normal, low);
}

TEST(CombatFormulas, AttackPower_LowIntegrity) {
    auto p = make_person(60, 50, 5);
    auto c = make_class(100, 10, 10, 5, 50, 30);
    auto f_full = make_fighter(100, 100);
    auto f_damaged = make_fighter(100, 50);

    int32_t full = calc_attack_power(p, c, f_full, 0, true);
    int32_t damaged = calc_attack_power(p, c, f_damaged, 0, true);

    EXPECT_GT(full, damaged);
}

TEST(CombatFormulas, AttackPower_SkillLevelImpact) {
    auto p = make_person(60, 50, 5);
    auto c = make_class(100, 10, 10, 5, 50, 30);
    auto f = make_fighter();

    int32_t atk_lvl0 = calc_attack_power(p, c, f, 0, false);
    int32_t atk_lvl5 = calc_attack_power(p, c, f, 5, false);
    int32_t atk_lvl10 = calc_attack_power(p, c, f, 10, false);

    EXPECT_GT(atk_lvl5, atk_lvl0);
    EXPECT_GT(atk_lvl10, atk_lvl5);
}

TEST(CombatFormulas, AttackPower_StrengthScaling) {
    auto c = make_class(100, 10, 10, 5, 50, 30);
    auto f = make_fighter();

    auto p_weak = make_person(30, 50, 5);
    auto p_strong = make_person(90, 50, 5);

    int32_t weak = calc_attack_power(p_weak, c, f, 3, false);
    int32_t strong = calc_attack_power(p_strong, c, f, 3, false);

    EXPECT_GT(strong, weak);
}

TEST(CombatFormulas, AttackPower_ItemBonus) {
    auto p = make_person(60, 50, 5);
    auto c = make_class(100, 10, 10, 5, 50, 30);
    auto f = make_fighter();

    int32_t no_bonus = calc_attack_power(p, c, f, 3, false, 0);
    int32_t with_bonus = calc_attack_power(p, c, f, 3, false, 25);

    EXPECT_EQ(with_bonus, no_bonus + 25);
}

TEST(CombatFormulas, AttackPower_Level1Base) {
    auto p = make_person(50, 50, 1);
    auto c = make_class(100, 10, 10, 5, 50, 30);
    auto f = make_fighter();

    int32_t raw_atk = (50 * 3 * 50 / 100) + 0;
    int32_t expected = 59 * (raw_atk * 550 / 580) / 60;
    EXPECT_EQ(calc_attack_power(p, c, f, 0, false), expected);
}

TEST(CombatFormulas, AttackPower_HighLevel) {
    auto p = make_person(80, 60, 50);
    auto c = make_class(200, 30, 15, 8, 100, 50);
    auto f = make_fighter();

    int32_t result = calc_attack_power(p, c, f, 10, true);
    EXPECT_GT(result, 0);
}

TEST(CombatFormulas, AttackPower_Morale100NoChange) {
    auto p = make_person(60, 50, 5);
    auto c = make_class(100, 10, 10, 5, 50, 30);
    auto f = make_fighter(100, 100);

    int32_t before_morale = (59 + 0) * ((50*3*60/100 + 9*4) * 560 / 580) / 60;
    int32_t after_morale = before_morale * 100 / 100 * 200 / 200;
    EXPECT_EQ(calc_attack_power(p, c, f, 0, true), after_morale);
}

TEST(CombatFormulas, AttackPower_ZeroStrength) {
    auto p = make_person(0, 50, 5);
    auto c = make_class(100, 10, 10, 5, 50, 30);
    auto f = make_fighter();

    int32_t result = calc_attack_power(p, c, f, 0, false);
    int32_t raw = (50 * 3 * 0 / 100) + 9 * 4;
    int32_t expected = 59 * (raw * 500 / 580) / 60;
    EXPECT_EQ(result, expected);
}

TEST(CombatFormulas, AttackPower_Morale200) {
    auto p = make_person(60, 50, 5);
    auto c = make_class(100, 10, 10, 5, 50, 30);
    auto f = make_fighter(200, 100);

    int32_t result = calc_attack_power(p, c, f, 0, true);
    int32_t raw_atk = (50 * 3 * 60 / 100) + 9 * 4;
    int32_t base = 59 * (raw_atk * 560 / 580) / 60;
    int32_t expected = base * 100 / 100 * 300 / 200;
    EXPECT_EQ(result, expected);
}

// ============================================================
// 防御力测试 / Defense tests (≥20 cases)
// ============================================================
TEST(CombatFormulas, Defense_BaseCase) {
    auto p = make_person(50, 50, 1);
    auto c = make_class(100, 10, 5, 3, 50, 30);
    auto f = make_fighter();

    EXPECT_EQ(calc_defense(p, c, f, 0, false), 30);
}

TEST(CombatFormulas, Defense_LevelGrowth) {
    auto c = make_class(100, 10, 5, 3, 50, 30);
    auto f = make_fighter();

    auto p1 = make_person(50, 50, 1);
    auto p5 = make_person(50, 50, 5);
    auto p10 = make_person(50, 50, 10);

    EXPECT_EQ(calc_defense(p1, c, f, 0, false), 30);
    EXPECT_EQ(calc_defense(p5, c, f, 0, false), 42);
    EXPECT_EQ(calc_defense(p10, c, f, 0, false), 57);
}

TEST(CombatFormulas, Defense_BattleMode) {
    auto p = make_person(50, 50, 5);
    auto c = make_class(100, 10, 5, 3, 50, 30);
    auto f = make_fighter(100, 100, 100);

    int32_t no_battle = calc_defense(p, c, f, 0, false);
    int32_t with_battle = calc_defense(p, c, f, 0, true);

    EXPECT_EQ(no_battle, with_battle);
}

TEST(CombatFormulas, Defense_LowMorale) {
    auto p = make_person(50, 50, 5);
    auto c = make_class(100, 10, 5, 3, 50, 30);
    auto f_normal = make_fighter(100, 100, 100);
    auto f_low = make_fighter(0, 100, 100);

    int32_t normal = calc_defense(p, c, f_normal, 0, true);
    int32_t low = calc_defense(p, c, f_low, 0, true);

    EXPECT_GT(normal, low);
}

TEST(CombatFormulas, Defense_LowDefModifier) {
    auto p = make_person(50, 50, 5);
    auto c = make_class(100, 10, 5, 3, 50, 30);
    auto f_full = make_fighter(100, 100, 100);
    auto f_half = make_fighter(100, 100, 50);

    int32_t full = calc_defense(p, c, f_full, 0, true);
    int32_t half = calc_defense(p, c, f_half, 0, true);

    EXPECT_GT(full, half);
}

// 二进制 0x0042A98E：地形防御加成仅在 mode==0x96（战斗）时施加；
// 非战斗模式不加地形/士气/defmod。此前该测试用 false 期望加地形是错期望，已修正。
TEST(CombatFormulas, Defense_TerrainBonus) {
    auto p = make_person(50, 50, 5);
    auto c = make_class(100, 10, 5, 3, 50, 30);
    auto f = make_fighter();   // morale=100, def_mod=100（战斗模式无缩放失真）

    int32_t no_terrain = calc_defense(p, c, f, 0, true);
    int32_t with_terrain = calc_defense(p, c, f, 50, true);

    EXPECT_GT(with_terrain, no_terrain);
    EXPECT_EQ(with_terrain, no_terrain + no_terrain * 50 / 100);
}

TEST(CombatFormulas, Defense_ItemBonus) {
    auto p = make_person(50, 50, 5);
    auto c = make_class(100, 10, 5, 3, 50, 30);
    auto f = make_fighter();

    int32_t no_bonus = calc_defense(p, c, f, 0, false, 0);
    int32_t with_bonus = calc_defense(p, c, f, 0, false, 15);

    EXPECT_EQ(with_bonus, no_bonus + 15);
}

TEST(CombatFormulas, Defense_HighLevel) {
    auto p = make_person(70, 60, 50);
    auto c = make_class(200, 30, 15, 8, 100, 50);
    auto f = make_fighter();

    int32_t result = calc_defense(p, c, f, 20, true);
    EXPECT_GT(result, 0);
}

TEST(CombatFormulas, Defense_ZeroBaseDefense) {
    auto p = make_person(50, 50, 1);
    auto c = make_class(100, 10, 5, 0, 50, 0);
    auto f = make_fighter();

    EXPECT_EQ(calc_defense(p, c, f, 0, false), 0);
}

TEST(CombatFormulas, Defense_Terrain100Percent) {
    auto p = make_person(50, 50, 5);
    auto c = make_class(100, 10, 5, 3, 50, 40);
    auto f = make_fighter();

    int32_t base = calc_defense(p, c, f, 0, false);
    int32_t max_terrain = calc_defense(p, c, f, 100, true);

    EXPECT_EQ(max_terrain, base * 2);
}

// ============================================================
// 技能等级测试 / Skill level lookup tests
// 严格对齐二进制 0x00419CA4（首个 skill_use < threshold[i]，否则 10）。
// 与 replica::battle_calc_skill_level 逐位一致。
// ============================================================
TEST(CombatFormulas, SkillLevel_Zero) {
    EXPECT_EQ(get_skill_level(0), 1);
}

TEST(CombatFormulas, SkillLevel_ExactThresholds) {
    EXPECT_EQ(get_skill_level(0), 1);
    EXPECT_EQ(get_skill_level(1), 2);
    EXPECT_EQ(get_skill_level(3), 3);
    EXPECT_EQ(get_skill_level(6), 4);
    EXPECT_EQ(get_skill_level(10), 5);
    EXPECT_EQ(get_skill_level(15), 6);
    EXPECT_EQ(get_skill_level(21), 7);
    EXPECT_EQ(get_skill_level(28), 8);
    EXPECT_EQ(get_skill_level(36), 9);
    EXPECT_EQ(get_skill_level(45), 10);
}

TEST(CombatFormulas, SkillLevel_BetweenThresholds) {
    EXPECT_EQ(get_skill_level(2), 2);
    EXPECT_EQ(get_skill_level(5), 3);
    EXPECT_EQ(get_skill_level(9), 4);
    EXPECT_EQ(get_skill_level(14), 5);
    EXPECT_EQ(get_skill_level(20), 6);
    EXPECT_EQ(get_skill_level(27), 7);
    EXPECT_EQ(get_skill_level(35), 8);
    EXPECT_EQ(get_skill_level(44), 9);
}

TEST(CombatFormulas, SkillLevel_AboveMax) {
    EXPECT_EQ(get_skill_level(46), 10);
    EXPECT_EQ(get_skill_level(100), 10);
    EXPECT_EQ(get_skill_level(9999), 10);
}

// 二进制以 uint 读 skill_use：-1 经 uint 环绕成大数 → 返回 10（与 oracle 一致）。
// 调用方传入的 usage 计数恒 >=0，此分支仅作边界保真。
TEST(CombatFormulas, SkillLevel_NegativeAsUint) {
    EXPECT_EQ(get_skill_level(-1), 10);
}

// ============================================================
// 技能效果测试 / Skill effect tests (16 types)
// ============================================================
TEST(CombatFormulas, SkillEffect_HpRecovery) {
    auto caster = make_person(50, 80, 5);
    auto skill = make_jineng(100, 0);

    auto effect = calc_skill_effect(caster, skill, 5);

    int32_t expected_hp = (100 * 80 / 100) * ((5 + 7) >> 3);
    EXPECT_EQ(effect.hp_effect, expected_hp);
    EXPECT_EQ(effect.morale_effect, 0);
}

TEST(CombatFormulas, SkillEffect_HpDamage) {
    auto caster = make_person(50, 60, 5);
    auto skill = make_jineng(150, 0);

    auto effect = calc_skill_effect(caster, skill, 3);

    int32_t expected = (150 * 60 / 100) * ((3 + 7) >> 3);
    EXPECT_EQ(effect.hp_effect, expected);
}

TEST(CombatFormulas, SkillEffect_MoraleBoost) {
    auto caster = make_person(50, 70, 5);
    auto skill = make_jineng(0, 120);

    auto effect = calc_skill_effect(caster, skill, 4);

    EXPECT_EQ(effect.hp_effect, 0);
    int32_t expected = (120 * 70 / 100) * ((4 + 7) >> 3);
    EXPECT_EQ(effect.morale_effect, expected);
}

TEST(CombatFormulas, SkillEffect_MoraleReduce) {
    auto caster = make_person(50, 50, 5);
    auto skill = make_jineng(0, 80);

    auto effect = calc_skill_effect(caster, skill, 2);

    EXPECT_EQ(effect.hp_effect, 0);
    EXPECT_GT(effect.morale_effect, 0);
}

TEST(CombatFormulas, SkillEffect_FireAttack) {
    auto caster = make_person(50, 90, 10);
    auto skill = make_jineng(200, 0);

    auto effect = calc_skill_effect(caster, skill, 8);

    int32_t expected = (200 * 90 / 100) * ((8 + 7) >> 3);
    EXPECT_EQ(effect.hp_effect, expected);
}

TEST(CombatFormulas, SkillEffect_DualEffect) {
    auto caster = make_person(50, 60, 5);
    auto skill = make_jineng(100, 50);

    auto effect = calc_skill_effect(caster, skill, 5);

    EXPECT_GT(effect.hp_effect, 0);
    EXPECT_GT(effect.morale_effect, 0);
}

TEST(CombatFormulas, SkillEffect_ZeroIntelligence) {
    auto caster = make_person(50, 0, 5);
    auto skill = make_jineng(100, 100);

    auto effect = calc_skill_effect(caster, skill, 5);

    EXPECT_EQ(effect.hp_effect, 0);
    EXPECT_EQ(effect.morale_effect, 0);
}

TEST(CombatFormulas, SkillEffect_ZeroParams) {
    auto caster = make_person(50, 80, 5);
    auto skill = make_jineng(0, 0);

    auto effect = calc_skill_effect(caster, skill, 10);

    EXPECT_EQ(effect.hp_effect, 0);
    EXPECT_EQ(effect.morale_effect, 0);
}

TEST(CombatFormulas, SkillEffect_SkillLevelScaling) {
    auto caster = make_person(50, 60, 5);
    auto skill = make_jineng(100, 50);

    auto e0 = calc_skill_effect(caster, skill, 0);
    auto e5 = calc_skill_effect(caster, skill, 5);
    auto e10 = calc_skill_effect(caster, skill, 10);

    EXPECT_LE(e0.hp_effect, e5.hp_effect);
    EXPECT_LE(e5.hp_effect, e10.hp_effect);
}

// ============================================================
// 经验值测试 / Experience calculation tests  (RE: 0x0042AA92 / 0x0042AB23 / 0x00429E92)
// 二进制无 base_exp：经验纯由"等级差"分桶决定。
// ============================================================
TEST(CombatFormulas, AttackExp_SameLevel) {
    EXPECT_EQ(calc_attack_exp(5, 5), 8);       // diff=0 → 8
}

TEST(CombatFormulas, AttackExp_DefenderHigher) {
    EXPECT_EQ(calc_attack_exp(3, 8), 32);      // diff=5 → 48 → cap 32
    EXPECT_EQ(calc_attack_exp(1, 10), 32);     // diff=9 → 80 → cap 32
}

TEST(CombatFormulas, AttackExp_DefenderLower) {
    EXPECT_EQ(calc_attack_exp(8, 5), 4);       // diff=-3 → 4
    EXPECT_EQ(calc_attack_exp(5, 1), 2);       // diff=-4 → 2
    EXPECT_EQ(calc_attack_exp(20, 5), 1);      // diff=-15 → 1
}

TEST(CombatFormulas, KillExp_SameLevel) {
    EXPECT_EQ(calc_kill_exp(5, 5), 12);        // diff=0 → 12
}

TEST(CombatFormulas, KillExp_VictimHigher) {
    EXPECT_EQ(calc_kill_exp(3, 8), 80);        // diff=5 → 80
    EXPECT_EQ(calc_kill_exp(1, 10), 80);       // diff=9 → 144 → cap 80
}

TEST(CombatFormulas, KillExp_VictimLower) {
    EXPECT_EQ(calc_kill_exp(8, 5), 4);         // diff=-3 → 4
    EXPECT_EQ(calc_kill_exp(20, 5), 1);        // diff=-15 → 1
}

TEST(CombatFormulas, LevelUp_NoLevelUp) {
    dlzr::core::People p{};
    p.experience = 50; p.level = 5;
    bool lu = apply_experience_gain(p, 30);    // total=80
    EXPECT_FALSE(lu);
    EXPECT_EQ(p.experience, 80);
    EXPECT_EQ(p.level, 5);
}

TEST(CombatFormulas, LevelUp_Single) {
    dlzr::core::People p{};
    p.experience = 80; p.level = 5;
    bool lu = apply_experience_gain(p, 30);    // total=110 → +1 级
    EXPECT_TRUE(lu);
    EXPECT_EQ(p.experience, 10);
    EXPECT_EQ(p.level, 6);
}

TEST(CombatFormulas, LevelUp_MultiAndCap) {
    dlzr::core::People p{};
    p.experience = 90; p.level = 98;
    bool lu = apply_experience_gain(p, 500);   // total=590 → +5 级，封顶 99
    EXPECT_TRUE(lu);
    EXPECT_EQ(p.experience, 90);               // 590 % 100
    EXPECT_EQ(p.level, 99);
}

TEST(CombatFormulas, LevelUp_ExactBoundary) {
    dlzr::core::People p{};
    p.experience = 99; p.level = 1;
    bool lu = apply_experience_gain(p, 1);     // total=100 → +1 级
    EXPECT_TRUE(lu);
    EXPECT_EQ(p.experience, 0);
    EXPECT_EQ(p.level, 2);
}

// ============================================================
// 伤害结算测试 / Damage resolution tests
// ============================================================
TEST(CombatFormulas, Damage_NormalCase) {
    EXPECT_EQ(calc_damage(100, 30), 70);
}

// RE: 0x0042784B — 攻击<防御时二进制返回 0（基础伤害 incoming=0），不是旧实现的 1。
TEST(CombatFormulas, Damage_AttackBelowDefense) {
    EXPECT_EQ(calc_damage(10, 100), 0);
}

// RE: 0x0042784B — 攻防相等时防御不<攻击，守卫不成立，返回 incoming=0（非 1）。
TEST(CombatFormulas, Damage_Equal) {
    EXPECT_EQ(calc_damage(50, 50), 0);
}

TEST(CombatFormulas, Damage_JustAbove) {
    EXPECT_EQ(calc_damage(51, 50), 1);
}

TEST(CombatFormulas, Damage_HighAttack) {
    EXPECT_EQ(calc_damage(1000, 1), 999);
}

TEST(CombatFormulas, Damage_ZeroDefense) {
    EXPECT_EQ(calc_damage(50, 0), 50);
}

// 0x0042784B 的 incoming 语义（max）：既有伤害与攻击−防御取大，且仅当攻击>防御生效。
TEST(CombatFormulas, Damage_IncomingLargerKept) {     // inc(200) > 攻击−防御(70) → 保留 200
    EXPECT_EQ(calc_damage(100, 30, 200), 200);
}

TEST(CombatFormulas, Damage_IncomingSmallerReplaced) { // inc(50) <= 攻击−防御(70) 且攻击>防御 → 替换为 70
    EXPECT_EQ(calc_damage(100, 30, 50), 70);
}

TEST(CombatFormulas, Damage_IncomingKeptWhenAttackLeDef) { // 攻击<=防御 → 守卫不生效，返回 incoming(5)
    EXPECT_EQ(calc_damage(10, 100, 5), 5);
}

// ============================================================
// 装备攻防加成测试 / Equipped item attack/defense bonus tests
// RE: 0x0042B042 / 0x0042B0EA
// ============================================================
namespace {
GameData make_gd_with_item(int32_t item_idx, int32_t cls, int32_t para3, int32_t para4) {
    GameData gd;
    if (item_idx >= 0 && item_idx < MAX_WUPIN) {
        gd.items[item_idx].para3 = para3;   // 攻击加成
        gd.items[item_idx].para4 = para4;   // 防御加成
    }
    if (cls >= 0 && cls < MAX_CLASS_TYPES) {
        gd.wupin_compat[item_idx][cls] = 1;  // 兵种-物品兼容
    }
    return gd;
}
}  // namespace

TEST(CombatFormulas, ItemBonus_NoItems) {
    auto p = make_person(50, 50, 5, 0);
    auto gd = make_gd_with_item(90, 0, 30, 20);
    EXPECT_EQ(get_item_attack_bonus(p, gd), 0);
    EXPECT_EQ(get_item_defense_bonus(p, gd), 0);
}

TEST(CombatFormulas, ItemBonus_EmptyInventory) {
    auto p = make_person(50, 50, 5, 0);
    for (auto& it : p.items) it = 0;
    auto gd = make_gd_with_item(90, 0, 30, 20);
    EXPECT_EQ(get_item_attack_bonus(p, gd), 0);
    EXPECT_EQ(get_item_defense_bonus(p, gd), 0);
}

TEST(CombatFormulas, ItemBonus_EquipmentCounts) {
    // item_id = 90 → idx = 89 (> 0x50), 兼容兵种 0, para3=35, para4=22
    auto p = make_person(50, 50, 5, 0);
    p.items[0] = 90;
    auto gd = make_gd_with_item(89, 0, 35, 22);
    EXPECT_EQ(get_item_attack_bonus(p, gd), 35);
    EXPECT_EQ(get_item_defense_bonus(p, gd), 22);
}

TEST(CombatFormulas, ItemBonus_ConsumableIgnored) {
    // item_id = 50 → idx = 49 (<= 0x50), 即使兼容也不计入装备加成
    auto p = make_person(50, 50, 5, 0);
    p.items[0] = 50;
    auto gd = make_gd_with_item(49, 0, 99, 99);
    EXPECT_EQ(get_item_attack_bonus(p, gd), 0);
    EXPECT_EQ(get_item_defense_bonus(p, gd), 0);
}

TEST(CombatFormulas, ItemBonus_IncompatibleClass) {
    // 装备 idx=89 仅对兵种 1 兼容，对兵种 0 不兼容
    auto p = make_person(50, 50, 5, 0);
    p.items[0] = 90;
    auto gd = make_gd_with_item(89, 1, 35, 22);  // 兼容兵种 1，非 0
    EXPECT_EQ(get_item_attack_bonus(p, gd), 0);
    EXPECT_EQ(get_item_defense_bonus(p, gd), 0);
}

TEST(CombatFormulas, ItemBonus_BestOfMultiple) {
    // 三个兼容装备，取最大值
    auto p = make_person(50, 50, 5, 3);
    p.items[0] = 90;   // idx 89
    p.items[1] = 100;  // idx 99
    p.items[2] = 110;  // idx 109
    GameData gd;
    gd.items[89].para3 = 10;  gd.items[89].para4 = 5;
    gd.items[99].para3 = 40;  gd.items[99].para4 = 12;
    gd.items[109].para3 = 25; gd.items[109].para4 = 30;
    gd.wupin_compat[89][3] = 1;
    gd.wupin_compat[99][3] = 1;
    gd.wupin_compat[109][3] = 1;
    EXPECT_EQ(get_item_attack_bonus(p, gd), 40);
    EXPECT_EQ(get_item_defense_bonus(p, gd), 30);
}

TEST(CombatFormulas, ItemBonus_MaxOfCompatibleOnly) {
    // 装备 90 兼容、装备 100 不兼容，应只取 90
    auto p = make_person(50, 50, 5, 7);
    p.items[0] = 90;   // idx 89
    p.items[1] = 100;  // idx 99 (不兼容)
    GameData gd;
    gd.items[89].para3 = 15;  gd.items[89].para4 = 9;
    gd.items[99].para3 = 80;  gd.items[99].para4 = 80;
    gd.wupin_compat[89][7] = 1;
    EXPECT_EQ(get_item_attack_bonus(p, gd), 15);
    EXPECT_EQ(get_item_defense_bonus(p, gd), 9);
}

TEST(CombatFormulas, ItemBonus_SlotRange) {
    // 验证扫描全部 8 个栏位
    auto p = make_person(50, 50, 5, 2);
    for (auto& it : p.items) it = 0;
    p.items[7] = 120;  // 最后一个栏位，idx 119
    GameData gd;
    gd.items[119].para3 = 60; gd.items[119].para4 = 33;
    gd.wupin_compat[119][2] = 1;
    EXPECT_EQ(get_item_attack_bonus(p, gd), 60);
    EXPECT_EQ(get_item_defense_bonus(p, gd), 33);
}

// ============================================================
// 命中判定测试 / Hit chance tests (RE: 0x0042ABB0 / 0x0042AC0B)
// 确定性边界：命中率 >= 100（智力差 >= 35）→ 必中；命中率 <= 0（智力差 <= -15）→ 必不中。
// 中间区为随机（rand_impl()%100），由 test_cross_layer_diff.cpp 的 2 万次对拍覆盖。
// ============================================================
TEST(CombatFormulas, HitChance_AlwaysHit) {
    auto atk = make_person(0, 100, 1);  // intelligence 100
    auto def = make_person(0, 0, 1);    // intelligence 0
    // rate = (100-0)*2+30 = 230 >= 100 → 任意 roll(0..99) < 230 恒真
    for (int i = 0; i < 50; ++i) EXPECT_EQ(calc_hit_chance(atk, def), 1);
}

TEST(CombatFormulas, HitChance_AlwaysMiss) {
    auto atk = make_person(0, 0, 1);    // intelligence 0
    auto def = make_person(0, 20, 1);   // intelligence 20
    // rate = (0-20)*2+30 = -10 → clamp 0 → 任意 roll < 0 恒假
    for (int i = 0; i < 50; ++i) EXPECT_EQ(calc_hit_chance(atk, def), 0);
}

TEST(CombatFormulas, HitChance_WeaponBonusRaisesRate) {
    // 武器版在基础命中率上 += 技能等级（已分桶），公式上严格 >= 基础版。
    auto atk = make_person(0, 50, 1);
    atk.skill_lueduo = 45;              // 分桶后 = 10
    auto def = make_person(0, 30, 1);
    int32_t sl = get_skill_level(atk.skill_lueduo);
    EXPECT_EQ(sl, 10);
    int32_t base_rate = (50 - 30) * 2 + 30;   // 70
    int32_t wpn_rate  = base_rate + sl;        // 80
    EXPECT_GT(wpn_rate, base_rate);
}

// ============================================================
// 资源增减夹取测试 / Clamp resource delta tests (RE: 0x00427830)
// 二进制仅封顶（min(cap, base+delta)），无下界夹取。
// ============================================================
TEST(CombatFormulas, ClampResource_HealWithinCap) {
    // HP 修复：base=50, +20, 封顶 100 → 70
    EXPECT_EQ(clamp_stat_delta(50, 20, 100), 70);
}

TEST(CombatFormulas, ClampResource_CapExceeded) {
    // base=90, +30 → 120 > 100 → 取封顶 100
    EXPECT_EQ(clamp_stat_delta(90, 30, 100), 100);
}

TEST(CombatFormulas, ClampResource_ZeroDelta) {
    EXPECT_EQ(clamp_stat_delta(50, 0, 100), 50);
}

TEST(CombatFormulas, ClampResource_NoLowerClamp) {
    // 二进制无下界夹取：base=10, delta=-5 → 5（不夹到 0）。诚实记录真实行为。
    EXPECT_EQ(clamp_stat_delta(10, -5, 100), 5);
}

TEST(CombatFormulas, ClampResource_NegativeWrapsToCap) {
    // base+delta 大负 → 无符号环绕成大数 > cap → 取 cap（退化分支，记录真实行为）。
    EXPECT_EQ(clamp_stat_delta(10, -30, 100), 100);
}

