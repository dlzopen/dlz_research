// test_battle_calc.cpp — 战斗数值函数链运行时对拍（forwarder vs 独立 ref）
//
// 验证方法（铁律零）：对 12 个已语义化转发的战斗函数（含 2 个命中判定），
//   - 经 replica 转录 forwarder（calc_unit_power_2a82a 等）调用 battle_calc_* 语义实现；
//   - 与独立 ref_*（逐 Ghidra 指令流转录，读取同一 g_game_state / g_battlecalc_*）逐值比较。
// 随机化 g_game_state 与运行时表，交替覆盖 g_current_mode==0x96 与 !=0x96 两条分支，要求 0 分歧。
//
// 注意：运行时表（技能阈值 / 兵种×物品兼容 / 物品加成 / 地形网格 / 地形修正）静态 PE 无字节，
//   运行时由数据文件 fread 装载；此处随机化仅证明「控制流 + 索引 + 算术」1:1，不证明表内容。

#include <gtest/gtest.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "replica/replica_stubs.h"
#include "replica/battle_calc.hpp"

// ---------- 独立 reference：逐 Ghidra 指令流，读取同一全局 ----------
// 与 battle_calc.cpp 是两份独立转录，互相校验转录正确性。
static uint32_t ref_skill_level(uint32_t p1) {
    uint32_t local_8 = 0;
    while (1) {
        if (9 < local_8) return 10;
        if (p1 < (uint32_t)g_battlecalc_skill_thresholds[local_8]) break;
        local_8++;
    }
    return local_8;
}

static int32_t ref_item_attack_bonus(int32_t p1) {
    uint32_t uVar1;
    int local_14 = 0;
    for (uint32_t s = 0; s < 8; s++) {
        uint8_t item = g_game_state.people_array[p1].items[s];
        if (item != 0) {
            uVar1 = (uint32_t)(item - 1);
            if (0x50 < uVar1) {
                int cls = g_game_state.people_array[p1].class_type;
                if (g_battlecalc_class_flag_table[cls + (int)uVar1 * 96] != 0) {
                    int b = g_battlecalc_item_atk_bonus[uVar1];   // 逻辑每件 1 int（atk@+0），loader 负责 0x48 步长映射
                    if (local_14 < b) local_14 = b;
                }
            }
        }
    }
    return local_14;
}

static int32_t ref_item_defense_bonus(int32_t p1) {
    uint32_t uVar1;
    int local_10 = 0;
    for (uint32_t s = 0; s < 8; s++) {
        uint8_t item = g_game_state.people_array[p1].items[s];
        if (item != 0) {
            uVar1 = (uint32_t)(item - 1);
            if (0x50 < uVar1) {
                int cls = g_game_state.people_array[p1].class_type;
                if (g_battlecalc_class_flag_table[cls + (int)uVar1 * 96] != 0) {
                    int b = g_battlecalc_item_def_bonus[uVar1];   // 逻辑每件 1 int（def@+4），loader 负责 0x48 步长映射
                    if (local_10 < b) local_10 = b;
                }
            }
        }
    }
    return local_10;
}

static uint16_t ref_equip_defense(int32_t param_1, int32_t param_2) {
    int idx = param_2 * g_battlecalc_map_width + param_1;
    if (idx < 0 || idx >= (int)(sizeof(g_battlecalc_terrain_grid) / sizeof(g_battlecalc_terrain_grid[0])))
        return 0;
    return (uint16_t)(g_battlecalc_terrain_grid[idx] >> 12);
}

static int32_t ref_attack_power(int32_t param_1) {
    int iVar1, iVar2, iVar3, iVar4, iVar5, iVar6;
    uint32_t local_8;
    int cls = g_game_state.people_array[param_1].class_type;
    iVar6 = g_game_state.classtype_array[cls].base_attack;
    iVar1 = g_game_state.people_array[param_1].strength;
    iVar2 = g_game_state.classtype_array[cls].attack_growth;
    iVar3 = g_game_state.people_array[param_1].level;
    iVar4 = g_game_state.people_array[param_1].strength;
    iVar5 = (int32_t)ref_skill_level((uint32_t)g_game_state.people_array[param_1].skill_gongji);
    local_8 = ((uint32_t)(iVar5 + 0x3b) *
               ((((uint32_t)(iVar6 * 3 * iVar1) / 100u +
                  (uint32_t)((iVar2 + -1) * (iVar3 + -1))) *
                 (uint32_t)(iVar4 + 500)) / 0x244u)) / 0x3cu;
    if (g_game_state.g_current_mode == 0x96) {
        local_8 = ((uint32_t)(g_game_state.fighter_array[param_1].morale + 100) *
                   ((local_8 * (uint32_t)g_game_state.fighter_array[param_1].equip_integrity) / 100u)) / 200u;
    }
    return (int32_t)local_8 + ref_item_attack_bonus(param_1);
}

static int32_t ref_defense(int32_t param_1) {
    int iVar1;
    uint32_t uVar2;
    int local_8;
    int cls = g_game_state.people_array[param_1].class_type;
    iVar1 = (int)ref_equip_defense(g_game_state.fighter_array[param_1].pos_x,
                                  g_game_state.fighter_array[param_1].pos_y);
    local_8 = g_game_state.classtype_array[cls].base_defense +
              (g_game_state.people_array[param_1].level + -1) *
              g_game_state.classtype_array[cls].defense_growth;
    if (g_game_state.g_current_mode == 0x96) {
        uVar2 = ((uint32_t)(g_game_state.fighter_array[param_1].morale + 100) *
                 ((uint32_t)(local_8 * g_game_state.fighter_array[param_1].defense_modifier) / 100u)) / 200u;
        // 铁律零：二进制 uVar2 为 uint，地形修正项按无符号相乘，故与 attack 对称保持无符号语义。
        local_8 = (int32_t)(uVar2 + (uVar2 * (uint32_t)g_battlecalc_terrain_modifier[iVar1]) / 100u);
    }
    return local_8 + ref_item_defense_bonus(param_1);
}

// 0x0042784B — 伤害结算核心（逐 Ghidra 指令流，独立转录）。
//   if ((defense < attack) && (incoming <= attack - defense)) incoming = attack - defense; return incoming;
static uint32_t ref_damage(uint32_t attack, uint32_t defense, uint32_t incoming) {
    if ((defense < attack) && (incoming <= attack - defense)) {
        incoming = attack - defense;
    }
    return incoming;
}

// 0x0042AA92 — 攻击经验（独立转录，与 battle_calc_attack_exp 交叉验证）。
//   diff = 防守方等级 − 攻击方等级；<-6→1,<-3→2,<-1→4,<0→6,==0→8,else diff*8+8；cap 0x20。
static int32_t ref_attack_exp(int32_t atk_idx, int32_t def_idx) {
    int32_t diff = g_game_state.people_array[def_idx].level - g_game_state.people_array[atk_idx].level;
    int32_t v;
    if (diff < -6) v = 1; else if (diff < -3) v = 2; else if (diff < -1) v = 4;
    else if (diff < 0) v = 6; else if (diff == 0) v = 8; else v = diff * 8 + 8;
    if (0x20 < v) v = 0x20;
    return v;
}

// 0x0042AB23 — 击杀经验（独立转录）。调用约定 param_1=防守方, param_2=攻击方；
//   diff = 防守方等级 − 攻击方等级；<-6→1,<-3→2,<-1→4,<0→8,==0→0xc,else diff*0x10；cap 0x50。
static int32_t ref_kill_exp(int32_t def_idx, int32_t atk_idx) {
    int32_t diff = g_game_state.people_array[def_idx].level - g_game_state.people_array[atk_idx].level;
    int32_t v;
    if (diff < -6) v = 1; else if (diff < -3) v = 2; else if (diff < -1) v = 4;
    else if (diff < 0) v = 8; else if (diff == 0) v = 0xc; else v = diff * 0x10;
    if (0x50 < v) v = 0x50;
    return v;
}

// 0x00429E92 — 累加经验/升级（独立转录数值核心）。
static int32_t ref_apply_exp(int32_t level, int32_t exp_mod, int32_t gained,
                             int32_t* out_level, int32_t* out_exp) {
    int32_t total = exp_mod + gained;
    int32_t lu = (total / 100 != 0) ? 1 : 0;
    *out_exp = total % 100;
    int32_t nl = level + total / 100;
    if (nl >= 100) nl = 99;
    *out_level = nl;
    return lu;
}

// 0x0042ABB0 — 命中判定（基础版，独立转录）。
//   local_8 = (people[atk].intelligence - people[def].intelligence)*2 + 0x1e; if(<0)=0;
//   return rand_impl()%100 < local_8.
static int32_t ref_hit_chance(int32_t atk_idx, int32_t def_idx) {
    int32_t ai = g_game_state.people_array[atk_idx].intelligence;
    int32_t di = g_game_state.people_array[def_idx].intelligence;
    int32_t rate = (ai - di) * 2 + 0x1e;
    if (rate < 0) rate = 0;
    uint32_t roll = rand_impl() % 100;
    return (roll < (uint32_t)rate) ? 1 : 0;
}

// 0x0042AC0B — 命中判定（武器/技能版，独立转录）。
//   local_8 = (people[atk].intelligence - people[def].intelligence)*2 + 0x1e; if(<0)=0;
//   local_8 += threshold_bucket(people[atk].skill_lueduo); return rand_impl()%100 < local_8.
static int32_t ref_hit_chance_weapon(int32_t atk_idx, int32_t def_idx) {
    int32_t ai = g_game_state.people_array[atk_idx].intelligence;
    int32_t di = g_game_state.people_array[def_idx].intelligence;
    int32_t rate = (ai - di) * 2 + 0x1e;
    if (rate < 0) rate = 0;
    uint32_t su = (uint32_t)g_game_state.people_array[atk_idx].skill_lueduo;
    rate += (int32_t)battle_calc_skill_level(su);
    uint32_t roll = rand_impl() % 100;
    return (roll < (uint32_t)rate) ? 1 : 0;
}

// ---------- 随机化辅助 ----------
static int s_ri(int lo, int hi) { return lo + rand() % (hi - lo + 1); }

static void setup_random_state(int p, bool battle_mode) {
    g_game_state.g_current_mode = battle_mode ? 0x96 : 0x10;
    People& pe = g_game_state.people_array[p];
    pe.class_type = s_ri(0, 95);
    pe.strength = s_ri(1, 200);
    pe.level = s_ri(1, 60);
    pe.intelligence = s_ri(-200, 200);   // 命中率函数依赖智力差
    pe.skill_gongji = s_ri(0, 400);
    pe.skill_lueduo = s_ri(0, 400);      // 武器命中率版本依赖该技能等级
    for (int s = 0; s < 8; s++) {
        int r = rand() % 3;
        if (r == 0) pe.items[s] = 0;                                   // 空槽
        else if (r == 1) pe.items[s] = (uint8_t)s_ri(1, 81);           // 非装备（id<=81，不计加成）
        else pe.items[s] = (uint8_t)s_ri(82, 160);                     // 装备（id∈[82,160]，计加成）
    }
    Fighter& fi = g_game_state.fighter_array[p];
    fi.morale = s_ri(-50, 200);
    fi.equip_integrity = s_ri(-50, 200);
    fi.defense_modifier = s_ri(-50, 200);
    fi.pos_x = s_ri(0, 15);   // 保证地形网格索引 >>12 ∈ [0,15]
    fi.pos_y = s_ri(0, 15);
    ClassType& ct = g_game_state.classtype_array[pe.class_type];
    ct.base_attack = s_ri(1, 200);
    ct.attack_growth = s_ri(1, 50);
    ct.base_defense = s_ri(1, 200);
    ct.defense_growth = s_ri(1, 50);
}

// 防泄漏：保存/恢复技能阈值（唯一被跨测试用例依赖的全局）。
// 本测试随机化全局表仅作内部 forwarder==ref 对拍；测试结束后必须恢复原值，
// 否则会污染后续 CrossLayerDiff.* 等依赖真值阈值的用例。
static uint32_t s_saved_skill_thresholds[10];
static void save_skill_thresholds() {
    for (int i = 0; i < 10; i++) s_saved_skill_thresholds[i] = g_battlecalc_skill_thresholds[i];
}
static void restore_skill_thresholds() {
    for (int i = 0; i < 10; i++) g_battlecalc_skill_thresholds[i] = s_saved_skill_thresholds[i];
}

static void setup_random_tables() {
    for (int i = 0; i < 10; i++)
        g_battlecalc_skill_thresholds[i] = (uint32_t)s_ri(1, 400);
    for (size_t i = 0; i < sizeof(g_battlecalc_class_flag_table); i++)
        g_battlecalc_class_flag_table[i] = (uint8_t)(rand() % 2 ? rand() % 200 : 0);
    for (int i = 0; i < 160; i++) {
        g_battlecalc_item_atk_bonus[i] = s_ri(-500, 500);
        g_battlecalc_item_def_bonus[i] = s_ri(-500, 500);
    }
    g_battlecalc_map_width = 64;
    for (int i = 0; i < 16; i++)
        g_battlecalc_terrain_modifier[i] = s_ri(-100, 100);
    // 地形网格：填充使 >>12 ∈ [0,15]（地形类型 0..15），覆盖地形修正索引范围
    for (size_t i = 0; i < sizeof(g_battlecalc_terrain_grid) / sizeof(g_battlecalc_terrain_grid[0]); i++) {
        uint16_t t = (uint16_t)s_ri(0, 15);
        g_battlecalc_terrain_grid[i] = (uint16_t)(t << 12);
    }
}

TEST(BattleCalc, ForwarderMatchesReference) {
    srand(20261024u);
    const int N = 100000;
    save_skill_thresholds();   // 防泄漏：测试结束前恢复真值阈值
    setup_random_tables();   // 表内容仅随机化一次（运算量）；分支与索引由每人的状态覆盖
    int mismatches = 0;

    for (int t = 0; t < N; t++) {
        int p = s_ri(0, 127);
        bool bm = (t % 2 == 0);   // 交替覆盖 mode==0x96 / !=0x96
        setup_random_state(p, bm);

        int sa = calc_unit_power_2a82a(p);
        int la = ref_attack_power(p);
        if (sa != la) { if (mismatches < 8) printf("ATK mismatch p=%d mode=%d: %d vs %d\n", p, g_game_state.g_current_mode, sa, la); mismatches++; }

        int sd = calc_unit_defense_2a98e(p);
        int ld = ref_defense(p);
        if (sd != ld) { if (mismatches < 8) printf("DEF mismatch p=%d mode=%d: %d vs %d\n", p, g_game_state.g_current_mode, sd, ld); mismatches++; }

        uint16_t se = terrain_grid_get_2a95b(g_game_state.fighter_array[p].pos_x, g_game_state.fighter_array[p].pos_y);
        uint16_t le = ref_equip_defense(g_game_state.fighter_array[p].pos_x, g_game_state.fighter_array[p].pos_y);
        if (se != le) { if (mismatches < 8) printf("EQUIP mismatch: %d vs %d\n", (int)se, (int)le); mismatches++; }

        uint32_t sk = threshold_bucket_19ca4((uint32_t)g_game_state.people_array[p].skill_gongji);
        uint32_t lk = ref_skill_level((uint32_t)g_game_state.people_array[p].skill_gongji);
        if (sk != lk) { if (mismatches < 8) printf("SKILL mismatch: %u vs %u\n", sk, lk); mismatches++; }

        int sia = skill_table_max_40(p);
        int lia = ref_item_attack_bonus(p);
        if (sia != lia) { if (mismatches < 8) printf("ITEM-ATK mismatch: %d vs %d\n", sia, lia); mismatches++; }

        int sid = skill_table_max_44(p);
        int lid = ref_item_defense_bonus(p);
        if (sid != lid) { if (mismatches < 8) printf("ITEM-DEF mismatch: %d vs %d\n", sid, lid); mismatches++; }

        // 伤害结算：forwarder（battle_damage_resolve_2784b）vs 独立 ref（ref_damage）
        uint32_t da = (uint32_t)s_ri(0, 2000);
        uint32_t dd = (uint32_t)s_ri(0, 2000);
        uint32_t di = (uint32_t)s_ri(0, 2000);
        uint32_t sdmg = battle_damage_resolve_2784b(da, dd, di);
        uint32_t ldmg = ref_damage(da, dd, di);
        if (sdmg != ldmg) { if (mismatches < 8) printf("DMG mismatch: %u vs %u (a=%u d=%u i=%u)\n", sdmg, ldmg, da, dd, di); mismatches++; }

        // 经验/升级公式：forwarder vs 独立 ref（攻击经验 / 击杀经验 / 升级累加）
        int a_idx = p;
        int d_idx = (p + 1) % 128;
        g_game_state.people_array[a_idx].level = s_ri(1, 99);
        g_game_state.people_array[d_idx].level = s_ri(1, 99);
        int sea = battle_exp_from_attack_aa92(a_idx, d_idx);
        int lea = ref_attack_exp(a_idx, d_idx);
        if (sea != lea) { if (mismatches < 8) printf("ATK-EXP mismatch: %d vs %d\n", sea, lea); mismatches++; }
        int sek = battle_exp_from_kill_ab23(d_idx, a_idx);  // 二进制约定 param_1=防守方, param_2=攻击方
        int lek = ref_kill_exp(d_idx, a_idx);
        if (sek != lek) { if (mismatches < 8) printf("KILL-EXP mismatch: %d vs %d\n", sek, lek); mismatches++; }

        int q = (p + 2) % 128;
        g_game_state.people_array[q].level = s_ri(1, 99);
        g_game_state.people_array[q].experience = s_ri(0, 99);
        int before_lv = g_game_state.people_array[q].level;
        int before_ex = g_game_state.people_array[q].experience;
        int gained = s_ri(0, 500);
        int slu = unit_gain_exp_9e92(q, gained);
        int llu = ref_apply_exp(before_lv, before_ex, gained, &before_lv, &before_ex);
        if (slu != llu ||
            g_game_state.people_array[q].level != before_lv ||
            g_game_state.people_array[q].experience != before_ex) {
            if (mismatches < 8) printf("GAIN-EXP mismatch: lu %d vs %d, lv %d vs %d, ex %d vs %d\n",
                slu, llu, g_game_state.people_array[q].level, before_lv,
                g_game_state.people_array[q].experience, before_ex);
            mismatches++;
        }

        // 命中判定：forwarder（hit_chance_check_abb0 / hit_chance_check_weapon_ac0b）vs 独立 ref。
        // 关键：两函数各自内部调用 rand_impl() 各消耗一个随机抽签；为使 forwarder 与 ref
        // 抽「同一个」随机值（从而比的是公式而非抽签），比较前把 RNG 种子复位到同一初值。
        int h_atk = p;
        int h_def = (p + 3) % 128;
        setup_random_state(h_def, bm);   // 守方也随机化（intelligence/skill_lueduo 已由 setup 覆盖）
        uint32_t seed0 = (uint32_t)g_game_state.g_pow2_1_448790;
        g_game_state.g_pow2_1_448790 = (int32_t)seed0;
        int sh = hit_chance_check_abb0(h_atk, h_def);
        g_game_state.g_pow2_1_448790 = (int32_t)seed0;   // 复位 → ref 抽同一随机值
        int lh = ref_hit_chance(h_atk, h_def);
        if (sh != lh) { if (mismatches < 8) printf("HIT mismatch: %d vs %d (atk=%d def=%d)\n", sh, lh, h_atk, h_def); mismatches++; }
        g_game_state.g_pow2_1_448790 = (int32_t)seed0;   // 复位 → weapon forwarder 抽同一值
        int sw = hit_chance_check_weapon_ac0b(h_atk, h_def);
        g_game_state.g_pow2_1_448790 = (int32_t)seed0;   // 复位 → weapon ref 抽同一值
        int lw = ref_hit_chance_weapon(h_atk, h_def);
        if (sw != lw) { if (mismatches < 8) printf("HIT-WPN mismatch: %d vs %d\n", sw, lw); mismatches++; }
    }

    EXPECT_EQ(mismatches, 0) << "battle_calc forwarder 与独立 ref 对拍存在分歧";
    if (mismatches == 0)
        printf("[BattleCalc] %d trials: ALL 12 FUNCTIONS 1:1 VERIFIED (0 mismatches)\n", N);

    restore_skill_thresholds();   // 防泄漏：恢复真值阈值，避免污染后续用例
}

// ============================================================
// FUN_00427a71 — 7×7 技能影响形状旋转/变换核（setup pass 播种源，铁律零逐指令核对）
// 顺序源矩阵 src[i]=i（i=0..n*n-1），验证每个 mode 的精确置换（与 FUN_00427a71.c 一一对应）：
//   mode 0 / 0xffffffff → 拷贝
//   mode 1 → 180° ：(r,c)→(n-1-r, n-1-c)
//   mode 2 → 90°CW：(r,c)→(n-1-c, r)
//   mode 3 → 90°CCW：(r,c)→(c, n-1-r)
// ============================================================
TEST(BattleCalc, MatrixRotate27a71Permutation) {
    const int n = 7;
    std::vector<uint32_t> src((size_t)n * n);
    for (int i = 0; i < n * n; i++) src[i] = (uint32_t)i;
    std::vector<uint32_t> dst((size_t)n * n);

    battle_calc_matrix_rotate_27a71(src.data(), dst.data(), n, 0);
    ASSERT_EQ(dst, src) << "mode 0 (copy) 失败";
    battle_calc_matrix_rotate_27a71(src.data(), dst.data(), n, (int)0xffffffff);
    ASSERT_EQ(dst, src) << "mode 0xffffffff (copy) 失败";

    battle_calc_matrix_rotate_27a71(src.data(), dst.data(), n, 1);
    for (int r = 0; r < n; r++) for (int c = 0; c < n; c++)
        ASSERT_EQ(dst[(n - 1 - r) * n + (n - 1 - c)], src[r * n + c]) << "mode 1 (180) r" << r << " c" << c;

    battle_calc_matrix_rotate_27a71(src.data(), dst.data(), n, 2);
    for (int r = 0; r < n; r++) for (int c = 0; c < n; c++)
        ASSERT_EQ(dst[(n - 1 - c) * n + r], src[r * n + c]) << "mode 2 (90CW) r" << r << " c" << c;

    battle_calc_matrix_rotate_27a71(src.data(), dst.data(), n, 3);
    for (int r = 0; r < n; r++) for (int c = 0; c < n; c++)
        ASSERT_EQ(dst[c * n + (n - 1 - r)], src[r * n + c]) << "mode 3 (90CCW) r" << r << " c" << c;
}
