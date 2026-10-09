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

TEST(CombatFormulas, Defense_TerrainBonus) {
    auto p = make_person(50, 50, 5);
    auto c = make_class(100, 10, 5, 3, 50, 30);
    auto f = make_fighter();

    int32_t no_terrain = calc_defense(p, c, f, 0, false);
    int32_t with_terrain = calc_defense(p, c, f, 50, false);

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
    int32_t max_terrain = calc_defense(p, c, f, 100, false);

    EXPECT_EQ(max_terrain, base * 2);
}

// ============================================================
// 技能等级测试 / Skill level lookup tests
// ============================================================
TEST(CombatFormulas, SkillLevel_Zero) {
    EXPECT_EQ(get_skill_level(0), 0);
}

TEST(CombatFormulas, SkillLevel_ExactThresholds) {
    EXPECT_EQ(get_skill_level(0), 0);
    EXPECT_EQ(get_skill_level(1), 1);
    EXPECT_EQ(get_skill_level(3), 2);
    EXPECT_EQ(get_skill_level(6), 3);
    EXPECT_EQ(get_skill_level(10), 4);
    EXPECT_EQ(get_skill_level(15), 5);
    EXPECT_EQ(get_skill_level(21), 6);
    EXPECT_EQ(get_skill_level(28), 7);
    EXPECT_EQ(get_skill_level(36), 8);
    EXPECT_EQ(get_skill_level(45), 9);
    EXPECT_EQ(get_skill_level(55), 10);
}

TEST(CombatFormulas, SkillLevel_BetweenThresholds) {
    EXPECT_EQ(get_skill_level(2), 1);
    EXPECT_EQ(get_skill_level(5), 2);
    EXPECT_EQ(get_skill_level(9), 3);
    EXPECT_EQ(get_skill_level(14), 4);
    EXPECT_EQ(get_skill_level(20), 5);
    EXPECT_EQ(get_skill_level(27), 6);
    EXPECT_EQ(get_skill_level(35), 7);
    EXPECT_EQ(get_skill_level(44), 8);
    EXPECT_EQ(get_skill_level(54), 9);
}

TEST(CombatFormulas, SkillLevel_AboveMax) {
    EXPECT_EQ(get_skill_level(100), 10);
    EXPECT_EQ(get_skill_level(9999), 10);
}

TEST(CombatFormulas, SkillLevel_NegativeClamp) {
    EXPECT_EQ(get_skill_level(-1), 0);
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
// 经验值测试 / Experience calculation tests
// ============================================================
TEST(CombatFormulas, Experience_SameLevel) {
    EXPECT_EQ(calc_experience(5, 5, 100), 100);
}

TEST(CombatFormulas, Experience_LowerKiller) {
    EXPECT_EQ(calc_experience(3, 8, 100), 150);
}

TEST(CombatFormulas, Experience_MuchLowerKiller) {
    EXPECT_EQ(calc_experience(1, 10, 100), 200);
}

TEST(CombatFormulas, Experience_HigherKiller) {
    EXPECT_EQ(calc_experience(8, 5, 100), 100);   // diff=3, <5 → 1.0x
    EXPECT_EQ(calc_experience(12, 5, 100), 75);   // diff=7, <10 → 0.75x
}

TEST(CombatFormulas, Experience_MuchHigherKiller) {
    EXPECT_EQ(calc_experience(20, 5, 100), 50);
}

TEST(CombatFormulas, Experience_LevelDiffBoundary) {
    EXPECT_EQ(calc_experience(5, 0, 100), 75);     // diff=5, <10 → 0.75x
    EXPECT_EQ(calc_experience(5, 1, 100), 100);    // diff=4, <5 → 1.0x
    EXPECT_EQ(calc_experience(5, 10, 100), 150);   // diff=-5, <0 → 1.5x
    EXPECT_EQ(calc_experience(5, 15, 100), 200);   // diff=-10, <-5 → 2.0x
}

TEST(CombatFormulas, Experience_ZeroBase) {
    EXPECT_EQ(calc_experience(5, 5, 0), 0);
}

TEST(CombatFormulas, Experience_IntegerDivision) {
    int32_t result = calc_experience(5, 5, 7);
    EXPECT_EQ(result, 7);
}

// ============================================================
// 伤害结算测试 / Damage resolution tests
// ============================================================
TEST(CombatFormulas, Damage_NormalCase) {
    EXPECT_EQ(calc_damage(100, 30), 70);
}

TEST(CombatFormulas, Damage_MinimumOne) {
    EXPECT_EQ(calc_damage(10, 100), 1);
}

TEST(CombatFormulas, Damage_EqualPower) {
    EXPECT_EQ(calc_damage(50, 50), 1);
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
