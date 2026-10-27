// test_combat_formula_anchors.cpp — 战斗数值公式的 RE 实证具体值锚点
//
// 目的：calc_attack_exp / calc_kill_exp / calc_max_hp / calc_damage 是纯函数，
// 已由 replica（按 Ghidra 逐指令转录）与 clean（语义化重实现）交叉层差分验证「两实现一致」。
// 但交叉层差分只能捕获「两实现分歧」，无法捕获「两实现同错」（例如某次提交把经验公式符号
// 翻转——两边一起翻，差分仍绿）。本测试用 RE 函数 FUN_0042aa92 / FUN_0042ab23 / FUN_00429e31 /
// FUN_0042784b 的**确切开销值**独立断言，钉死公式语义不被静默篡改。
//
// RE 坐标（铁律零）：
//   FUN_0042aa92.c — 攻击经验：diff=守−攻；<-6→1,<-3→2,<-1→4,<0→6,==0→8,else diff*8+8；封顶 0x20。
//   FUN_0042ab23.c — 击杀经验：diff=受害者−击杀者；<-6→1,<-3→2,<-1→4,<0→8,==0→0xc,else diff*0x10；封顶 0x50。
//                     调用方 FUN_0041307f.c:154 = FUN_0042ab23(受害者, 击杀者) ⇒ diff=受害者−击杀者（已核对）。
//   FUN_0042784b.c — 伤害：max(既存, 攻−防) 仅当 攻>防；否则不变（基础伤害=0→0）。

#include <gtest/gtest.h>

#include "core/data_model/types.hpp"
#include "logic/combat/combat_formulas.hpp"

using namespace dlzr::core;
using namespace dlzr::logic;

// ===== 攻击经验 calc_attack_exp(attacker_level, defender_level) =====
TEST(CombatFormula, AttackExpMatchesRE_0042aa92) {
    // diff = defender - attacker
    EXPECT_EQ(calc_attack_exp(1, 5), 0x20) << "diff=4 → 4*8+8=40，但封顶 0x20=32";
    EXPECT_EQ(calc_attack_exp(5, 1), 2)    << "diff=-4 → < -3 → 2";
    EXPECT_EQ(calc_attack_exp(3, 1), 4)    << "diff=-2 → < -1 → 4";
    EXPECT_EQ(calc_attack_exp(5, 5), 8)    << "diff=0 → 8";
    EXPECT_EQ(calc_attack_exp(1, 20), 0x20) << "diff=19 → 19*8+8=160，封顶 0x20=32";
}

// ===== 击杀经验 calc_kill_exp(killer_level, victim_level) =====
TEST(CombatFormula, KillExpMatchesRE_0042ab23) {
    // diff = victim - killer （已核对 FUN_0041307f.c:154 调用方传 (受害者, 击杀者)）
    EXPECT_EQ(calc_kill_exp(1, 5), 64)    << "diff=4 → 4*0x10=64";
    EXPECT_EQ(calc_kill_exp(5, 1), 2)     << "diff=-4 → < -3 → 2";
    EXPECT_EQ(calc_kill_exp(3, 1), 4)     << "diff=-2 → < -1 → 4";
    EXPECT_EQ(calc_kill_exp(5, 5), 0xc)   << "diff=0 → 0xc=12";
    EXPECT_EQ(calc_kill_exp(1, 20), 0x50) << "diff=19 → 19*16=304，封顶 0x50=80";
}

// ===== 最大 HP calc_max_hp(person, class_type) =====
TEST(CombatFormula, MaxHpMatchesRE_00429e31) {
    // RE：max_hp = class.hp_base + (level-1) * class.hp_growth
    ClassType pistol;
    pistol.hp_base = 80;
    pistol.hp_growth = 12;
    People p{};
    p.level = 1;
    EXPECT_EQ(calc_max_hp(p, pistol), 80)  << "L1 手枪队 → 80";
    p.level = 10;
    EXPECT_EQ(calc_max_hp(p, pistol), 188) << "L10 手枪队 → 80+9*12=188";

    ClassType god;
    god.hp_base = 1000;
    god.hp_growth = 100;
    People p2{};
    p2.level = 1;
    EXPECT_EQ(calc_max_hp(p2, god), 1000) << "L1 神仙队 → 1000 (RE: ClassType[0].hp_base=1000)";
}

// ===== 伤害结算 calc_damage(attack, defense, incoming) =====
TEST(CombatFormula, DamageMatchesRE_0042784b) {
    // 攻>防：损伤=max(既存, 攻−防)
    EXPECT_EQ(calc_damage(100, 50, 0), 50)  << "攻100防50 → 100-50=50";
    EXPECT_EQ(calc_damage(100, 50, 30), 50) << "既存30 < 50 → 取 50";
    EXPECT_EQ(calc_damage(100, 50, 80), 80) << "既存80 > 50 → 保留 80";
    // 攻<=防：守卫拦截，返回既存（基础伤害0→0，非 1）
    EXPECT_EQ(calc_damage(50, 100, 0), 0)  << "攻50<=防100 → 0（非负数环绕）";
    EXPECT_EQ(calc_damage(50, 100, 40), 40) << "攻<=防 仅保既存 40";
}
