// test_skill_grid_diff.cpp — clean ↔ oracle 跨层差分（FUN_004138ea 区域伤害/技能结算）
//
// 铁律零跨层闸门：把同一份 BattleGridState 同时喂给
//   · clean 层  dlzr::logic::{compute_skill_power, apply_cell_effect, settle_skill_grid}
//   · oracle 层 replica::{battle_calc_skill_power, battle_calc_apply_cell_effect,
//                         battle_calc_settle_skill_grid}（二进制逐指令转录）
// 断言全部可观测输出逐位一致。
//
// 范围（诚实保留）：
//   · 命中抽签共用二进制 Borland LCG（rand_impl，种子 g_game_state.g_pow2_1_448790）。
//     每个跨层对拍前复位同一种子，保证 clean 与 oracle 抽同一随机序列。
//   · 地形以同一百分比值分别注入两层（隔离"地形数据来源"差异），只验算术与守卫逻辑。
//   · case 9 仓库语义（FUN_0042b23a + FUN_0042920d + FUN_004290f8 全链）已逐指令 RE，
//     双端一致实现（take_item_to_depot），本测试锁死物品栏 / 战斗仓库 / attacker_index /
//     技能计数 / 贡献。

#include <gtest/gtest.h>
#include <random>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include "replica/replica_stubs.h"     // ::GameState g_game_state、::People/::Fighter
#include "replica/battle_sim.hpp"      // battle_calc_skill_power / _apply_cell_effect / _settle_skill_grid
#include "replica/battle_calc.hpp"     // g_battlecalc_skill_thresholds / _terrain_grid / _map_width
#include "logic/battle/battle_sim.hpp"  // dlzr::logic::BattleGridState + 镜像函数
#include "logic/combat/combat_formulas.hpp"    // get_terrain_defense_bonus / set_terrain_defense_bonus

namespace {

// 二进制真值技能阈值（battle_calc.cpp 默认值；运行时由数据文件装载）。
static void load_canonical_skill_thresholds() {
    static const uint32_t canonical[10] = {0, 1, 3, 6, 10, 15, 21, 28, 36, 45};
    for (int i = 0; i < 10; ++i) g_battlecalc_skill_thresholds[i] = canonical[i];
}

// 把 clean 层的一份 BattleGridState 逐字段镜像进 oracle 的 g_game_state + 模块级全局。
// 比的是"值"，不是内存布局（Tier B 允许 clean 层布局自由）。
static void mirror_grid(const dlzr::logic::BattleGridState& w) {
    for (int i = 0; i < 128; ++i) {
        const auto& u = w.units[i];
        auto& rp = g_game_state.people_array[i];
        rp.enemy = u.enemy;
        rp.intelligence = u.intelligence;
        rp.class_type = u.class_type;
        rp.level = u.level;
        rp.strength = u.strength;            // T3 攻击力/防御力
        rp.skill_gongji = u.skill_gongji;    // T3 攻击力/经验
        rp.experience = u.experience;        // T3-b 经验应用
        // 16 个技能计数器（base 0x34 = skill_buji）
        rp.skill_buji   = u.skill_use[0];
        rp.skill_jili   = u.skill_use[1];
        rp.skill_yiliao = u.skill_use[2];
        rp.skill_huogong= u.skill_use[3];
        rp.skill_gunmu  = u.skill_use[4];
        rp.skill_leishi = u.skill_use[5];
        rp.skill_weishe = u.skill_use[6];
        rp.skill_saorao = u.skill_use[7];
        rp.skill_pohuai = u.skill_use[8];
        rp.skill_lueduo = u.skill_use[9];   // 武器命中版命中率 += 此技能等级
        rp.skill_fanjian= u.skill_use[10];
        rp.skill_yibing = u.skill_use[11];
        rp.skill_duzhan = u.skill_use[12];
        rp.skill_ansha  = u.skill_use[13];
        rp.skill_shidu  = u.skill_use[14];
        rp.skill_kongxi = u.skill_use[15];
        for (int k = 0; k < 9; ++k) g_battlerec_0x7d0f18[i][k] = u.battlerec[k];

        auto& rf = g_game_state.fighter_array[i];
        rf.current_hp      = u.current_hp;
        rf.morale          = u.morale;
        rf.equip_integrity = u.equip_integrity;
        rf.defense_modifier= u.defense_modifier;
        rf.attacker_index  = u.attacker_index;
        rf.active_flag     = u.active_flag;
        rf.unit_state      = u.unit_state;
        for (int m = 0; m < 8; ++m) rp.items[m] = u.items[m];   // case 9 物品栏（People 侧，Fighter 无 items 字段）
        rf.pos_x = u.pos_x;                  // T3 防御地形查询
        rf.pos_y = u.pos_y;

        int cls = u.class_type;
        if (cls < 0) cls = 0;
        if (cls >= 96) cls = 95;
        g_game_state.classtype_array[cls].hp_base   = w.class_hp[cls][0];
        g_game_state.classtype_array[cls].hp_growth = w.class_hp[cls][1];
        g_game_state.classtype_array[cls].base_attack   = w.class_combat[cls][0];
        g_game_state.classtype_array[cls].attack_growth = w.class_combat[cls][1];
        g_game_state.classtype_array[cls].base_defense   = w.class_combat[cls][2];
        g_game_state.classtype_array[cls].defense_growth = w.class_combat[cls][3];
    }
    for (int c = 0; c < 130; ++c) {
        g_hp_dmg_buf_0x7d0488[c]  = w.heal_buffer[c];
        g_morale_dmg_buf_0x4565d0[c] = w.morale_buffer[c];
    }
    g_special_score_0x7d06d4 = w.special_score_a;
    g_special_score_0x7d06d8 = w.special_score_b;
    for (int k = 0; k < 16; ++k) {
        g_skill_param1_0x57ba50[k] = w.skill_param1[k];
        g_skill_param2_0x57ba54[k] = w.skill_param2[k];
        g_terrain_mod_a_0x475aa8[k]= w.terrain_mod_a[k];
        g_terrain_mod_b_0x475aac[k]= w.terrain_mod_b[k];
    }
    g_grid_rows        = w.grid_rows;
    g_grid_cols        = w.grid_cols;
    g_grid_origin_row  = w.grid_origin_row;
    g_grid_origin_col  = w.grid_origin_col;
    g_battle_double_grid = w.double_grid;
    g_map_width_0x57ba20 = w.map_width;
    g_game_state.g_current_mode = w.current_mode;   // T3 攻防 0x96 分支
    size_t n = w.unit_presence.size();
    if (n > 65536) n = 65536;
    for (size_t k = 0; k < n; ++k) {
        g_unit_presence_0x57bf10[k] = w.unit_presence[k];
        g_unit_index_0x57bf11[k]    = w.unit_index[k];
    }
    for (int k = 0; k < 256; ++k) g_warehouse_items_battle_0x7db3a8[k] = w.depot[k];  // case 9 仓库
    // T3-b 死亡/经验循环镜像字段
    g_game_state.g_aux_counter_b = w.lost_player;            // DAT_008331f8 我方阵亡
    g_game_state.g_aux_counter_a = w.lost_enemy;             // DAT_008331f0 敌方阵亡
    g_game_state.g_flow_phase     = w.battle_win_mode;        // DAT_007db578 胜负模式
    g_player_units_remaining      = w.player_units_remaining; // DAT_007d0f1c 剩余单位倒数
    for (int k = 0; k < 256; ++k) g_item_recovery[k] = w.item_recovery[k];  // DAT_007db3a6 物品回收
}

// oracle 端 16 技能计数器读取（镜像 battle_sim.cpp 的 skill_use_ref）。
static int32_t oracle_skill_use(int idx, int mode) {
    auto& p = g_game_state.people_array[idx];
    switch (mode) {
        case  0: return p.skill_buji;
        case  1: return p.skill_jili;
        case  2: return p.skill_yiliao;
        case  3: return p.skill_huogong;
        case  4: return p.skill_gunmu;
        case  5: return p.skill_leishi;
        case  6: return p.skill_weishe;
        case  7: return p.skill_saorao;
        case  8: return p.skill_pohuai;
        case  9: return p.skill_lueduo;
        case 10: return p.skill_fanjian;
        case 11: return p.skill_yibing;
        case 12: return p.skill_duzhan;
        case 13: return p.skill_ansha;
        case 14: return p.skill_shidu;
        case 15: return p.skill_kongxi;
    }
    return p.skill_buji;
}

// 单格效果：clean vs oracle 全部可观测输出比对。
static void assert_cell_outputs_match(const dlzr::logic::BattleGridState& w,
                                      uint32_t attacker, uint32_t skill_type,
                                      uint32_t occupant, uint32_t cell,
                                      uint32_t clean_ret, uint32_t oracle_ret,
                                      uint32_t clean_anim, uint32_t oracle_anim,
                                      int t) {
    auto& cu = w.units[occupant];
    auto& of = g_game_state.fighter_array[occupant];
    ASSERT_EQ(cu.current_hp,      of.current_hp)      << "t=" << t << " occ=" << occupant << " hp";
    ASSERT_EQ(cu.morale,          of.morale)          << "t=" << t << " occ=" << occupant << " morale";
    ASSERT_EQ(cu.equip_integrity, of.equip_integrity) << "t=" << t << " occ=" << occupant << " equip";
    ASSERT_EQ(cu.defense_modifier,of.defense_modifier)<< "t=" << t << " occ=" << occupant << " defmod";
    ASSERT_EQ(cu.attacker_index,  of.attacker_index)  << "t=" << t << " occ=" << occupant << " atkidx";
    ASSERT_EQ((int)cu.unit_state, (int)of.unit_state) << "t=" << t << " occ=" << occupant << " state";
    ASSERT_EQ(w.heal_buffer[cell],   g_hp_dmg_buf_0x7d0488[cell])   << "t=" << t << " cell=" << cell << " hpbuf";
    ASSERT_EQ(w.morale_buffer[cell], g_morale_dmg_buf_0x4565d0[cell])<< "t=" << t << " cell=" << cell << " mobuf";
    for (int k = 0; k < 9; ++k)
        ASSERT_EQ(w.units[occupant].battlerec[k], g_battlerec_0x7d0f18[occupant][k])
            << "t=" << t << " occ=" << occupant << " battlerec[" << k << "]";
    ASSERT_EQ(w.units[attacker].skill_use[skill_type], oracle_skill_use((int)attacker, (int)skill_type))
        << "t=" << t << " atk=" << attacker << " skill=" << skill_type << " skilluse";
    ASSERT_EQ(w.special_score_a, g_special_score_0x7d06d4) << "t=" << t << " sp_a";
    ASSERT_EQ(w.special_score_b, g_special_score_0x7d06d8) << "t=" << t << " sp_b";
    // case 9 物品栏 + 战斗仓库（仅 case 9 修改，但两 pass 始终相等，安全全比）
    for (int m = 0; m < 8; ++m) {
        ASSERT_EQ((int)w.units[attacker].items[m], (int)g_game_state.people_array[attacker].items[m])
            << "t=" << t << " atk=" << attacker << " item" << m;
        ASSERT_EQ((int)w.units[occupant].items[m], (int)g_game_state.people_array[occupant].items[m])
            << "t=" << t << " occ=" << occupant << " item" << m;
    }
    for (int k = 0; k < 156; ++k)
        ASSERT_EQ((int)w.depot[k], (int)g_warehouse_items_battle_0x7db3a8[k])
            << "t=" << t << " depot" << k;
    ASSERT_EQ(clean_ret,  oracle_ret)  << "t=" << t << " ret";
    ASSERT_EQ(clean_anim, oracle_anim) << "t=" << t << " anim";
}

}  // namespace

// 诊断：包裹 apply_cell_effect，捕获 SEH（如整数除零）并打印触发参数。
static uint32_t dbg_clean_cell(dlzr::logic::BattleGridState& w, uint32_t a, uint32_t st, uint32_t o,
                               uint32_t cell, uint32_t tm, uint32_t u8, uint32_t u9, uint32_t* anim,
                               int t, uint32_t stype) {
    uint32_t r = 0, an = *anim;
    __try {
        r = dlzr::logic::apply_cell_effect(w, a, st, o, cell, tm, u8, u9, &an);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        fprintf(stderr, "CLEAN SEH t=%d skill_type=%u atk=%u occ=%u cell=%u tm=%d u8=%u u9=%u "
                        "hpbuf=%d mobuf=%d cura=%d same=%d ap=%d\n",
                t, stype, a, o, cell, (int)tm, u8, u9,
                w.heal_buffer[cell], w.morale_buffer[cell], w.units[o].current_hp,
                (int)w.units[a].enemy, (int)w.units[o].enemy);
        r = 0xFFFFFFFFu;
    }
    *anim = an;
    return r;
}
static uint32_t dbg_oracle_cell(uint32_t a, uint32_t st, uint32_t o, uint32_t cell, uint32_t tm,
                                uint32_t u8, uint32_t u9, uint32_t* anim, int t, uint32_t stype) {
    uint32_t r = 0, an = *anim;
    __try {
        r = battle_calc_apply_cell_effect(a, st, o, cell, tm, u8, u9, &an);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        fprintf(stderr, "ORACLE SEH t=%d skill_type=%u atk=%u occ=%u cell=%u tm=%d u8=%u u9=%u "
                        "hpbuf=%d mobuf=%d\n",
                t, stype, a, o, cell, (int)tm, u8, u9,
                g_hp_dmg_buf_0x7d0488[cell], g_morale_dmg_buf_0x4565d0[cell]);
        r = 0xFFFFFFFFu;
    }
    *anim = an;
    return r;
}

// ============================================================
// 技能威力标量：clean::compute_skill_power vs oracle::battle_calc_skill_power
// ============================================================
TEST(SkillGridDiff, SkillPower) {
    load_canonical_skill_thresholds();
    std::mt19937 rng(0x5F11E573u);
    std::uniform_int_distribution<int32_t> U(0, 300);
    std::uniform_int_distribution<int32_t> E(0, 2);     // enemy 阵营
    const int N = 5000;
    for (int t = 0; t < N; ++t) {
        dlzr::logic::BattleGridState w;
        for (int i = 0; i < 128; ++i) {
            w.units[i].enemy = E(rng);
            w.units[i].class_type = U(rng) % 96;
            w.units[i].level = 1 + (U(rng) % 99);
            w.units[i].intelligence = (int32_t)(U(rng) % 200) - 50;
            w.units[i].skill_use[U(rng) % 16] = U(rng) % 400;
            w.units[i].battlerec[0] = U(rng) % 200;
        }
        for (int k = 0; k < 16; ++k) {
            w.skill_param1[k] = U(rng) % 200;
            w.skill_param2[k] = U(rng) % 200;
        }
        mirror_grid(w);

        uint32_t a = (uint32_t)(U(rng) % 128);
        uint32_t st = (uint32_t)(U(rng) % 16);
        uint32_t cu8 = 0, cu9 = 0, ou8 = 0, ou9 = 0;
        dlzr::logic::compute_skill_power(w, a, st, &cu8, &cu9);
        battle_calc_skill_power(a, st, &ou8, &ou9);
        ASSERT_EQ(cu8, ou8) << "t=" << t << " u8";
        ASSERT_EQ(cu9, ou9) << "t=" << t << " u9";
    }
}

// ============================================================
// 单格效果：clean::apply_cell_effect vs oracle::battle_calc_apply_cell_effect
// 覆盖 16 个 skill_type × 随机占驻/buffer/战斗记录/技能计数/特殊计分；RNG 种子复位。
// ============================================================
TEST(SkillGridDiff, SkillEffectCell) {
    load_canonical_skill_thresholds();
    std::mt19937 rng(0xCE11E57u);
    std::uniform_int_distribution<int32_t> U(0, 400);
    std::uniform_int_distribution<int32_t> E(0, 2);
    std::uniform_int_distribution<int32_t> Sgn(-200, 200);
    const int N = 20000;
    const uint32_t SEED = 0x1A2B3C4Du;
    for (int t = 0; t < N; ++t) {
        dlzr::logic::BattleGridState w;
        for (int i = 0; i < 128; ++i) {
            auto& u = w.units[i];
            u.enemy = E(rng);
            u.class_type = U(rng) % 96;
            u.level = 1 + (U(rng) % 99);
            u.intelligence = (int32_t)(U(rng) % 200) - 50;
            u.current_hp = Sgn(rng);
            u.morale = Sgn(rng);
            u.equip_integrity = Sgn(rng);
            u.defense_modifier = Sgn(rng);
            u.attacker_index = (int32_t)(U(rng) % 128);
            u.active_flag = (uint8_t)(U(rng) % 2);
            u.unit_state = (uint8_t)(U(rng) % 4);
            for (int m = 0; m < 8; ++m) u.items[m] = (uint8_t)(U(rng) % 82);
            for (int m = 0; m < 16; ++m) u.skill_use[m] = U(rng) % 400;
            u.battlerec[0] = U(rng) % 200;     // 属性/智力类（uVar8/uVar9 预计算用）
            u.battlerec[1] = 1 + (U(rng) % 0x40);   // +4 状态（case 0xa 递减下限1）
            u.battlerec[3] = (U(rng) % 2) ? 0x2d : 0; // +0xC 状态2（case 0xc 比较 0x2d）
        }
        for (int c = 0; c < 130; ++c) {
            w.heal_buffer[c]   = Sgn(rng);   // 可负（case 守卫依赖符号语义）
            w.morale_buffer[c] = Sgn(rng);
        }
        w.special_score_a = Sgn(rng);
        w.special_score_b = Sgn(rng);
        for (int k = 0; k < 16; ++k) {
            w.skill_param1[k] = U(rng) % 200;
            w.skill_param2[k] = U(rng) % 200;
            w.terrain_mod_a[k]= (int32_t)(U(rng) % 200) - 50;
            w.terrain_mod_b[k]= (int32_t)(U(rng) % 200) - 50;
        }
        // class_hp（max_hp 用）
        for (int c = 0; c < 96; ++c) {
            w.class_hp[c][0] = Sgn(rng);
            w.class_hp[c][1] = U(rng) % 20;
        }

        uint32_t attacker = (uint32_t)(U(rng) % 128);
        uint32_t occupant = (uint32_t)(U(rng) % 128);
        uint32_t skill_type = (uint32_t)(U(rng) % 16);
        uint32_t cell = (uint32_t)(U(rng) % 130);
        uint32_t terrain_mod = (uint32_t)((int32_t)(U(rng) % 200) - 50);

        mirror_grid(w);

        // 预计算技能威力（两者同源，附断言）
        uint32_t cu8 = 0, cu9 = 0, ou8 = 0, ou9 = 0;
        dlzr::logic::compute_skill_power(w, attacker, skill_type, &cu8, &cu9);
        battle_calc_skill_power(attacker, skill_type, &ou8, &ou9);
        ASSERT_EQ(cu8, ou8) << "t=" << t << " u8";
        ASSERT_EQ(cu9, ou9) << "t=" << t << " u9";

        uint32_t clean_anim = 0, oracle_anim = 0;
        // 种子复位 → clean/oracle 抽同一随机序列
        g_game_state.g_pow2_1_448790 = (int32_t)SEED;
        uint32_t clean_ret = dbg_clean_cell(w, attacker, skill_type, occupant, cell, terrain_mod,
                                           cu8, cu9, &clean_anim, t, skill_type);
        g_game_state.g_pow2_1_448790 = (int32_t)SEED;
        uint32_t oracle_ret = dbg_oracle_cell(attacker, skill_type, occupant, cell, terrain_mod,
                                              cu8, cu9, &oracle_anim, t, skill_type);
        ASSERT_NE(clean_ret, 0xFFFFFFFFu) << "clean SEH at t=" << t;
        ASSERT_NE(oracle_ret, 0xFFFFFFFFu) << "oracle SEH at t=" << t;

        assert_cell_outputs_match(w, attacker, skill_type, occupant, cell,
                                  clean_ret, oracle_ret, clean_anim, oracle_anim, t);
    }
}

// ============================================================
// 整网格结算：clean::settle_skill_grid vs oracle::battle_calc_settle_skill_grid
// 随机整网格占驻 + 地形网格播种；RNG 种子复位。验证 16 个 skill_type × 双网格尺寸。
// ============================================================
TEST(SkillGridDiff, SkillEffectGrid) {
    load_canonical_skill_thresholds();
    std::mt19937 rng(0x9E3779B9u);
    std::uniform_int_distribution<int32_t> U(0, 400);
    std::uniform_int_distribution<int32_t> E(0, 2);
    std::uniform_int_distribution<int32_t> Sgn(-200, 200);
    std::uniform_int_distribution<int32_t> T(0, 15);   // 地形类型 0..15
    const int N = 4000;
    const uint32_t SEED = 0x7E57C0DEu;
    for (int t = 0; t < N; ++t) {
        dlzr::logic::BattleGridState w;
        int mw = 64;
        int origin = (int)(U(rng) % 4);             // 0..3，保证 tile 落点不越界
        bool big = (U(rng) % 2) == 0;
        w.resize_grid_buffers(13, 13, mw);          // 仅用于尺寸化 presence 向量
        w.map_width = mw;
        w.grid_origin_row = origin;
        w.grid_origin_col = origin;
        w.double_grid = big ? 1 : 0;

        for (int i = 0; i < 128; ++i) {
            auto& u = w.units[i];
            u.enemy = E(rng);
            u.class_type = U(rng) % 96;
            u.level = 1 + (U(rng) % 99);
            u.intelligence = (int32_t)(U(rng) % 200) - 50;
            u.current_hp = Sgn(rng);
            u.morale = Sgn(rng);
            u.equip_integrity = Sgn(rng);
            u.defense_modifier = Sgn(rng);
            u.attacker_index = (int32_t)(U(rng) % 128);
            u.active_flag = (uint8_t)(U(rng) % 2);
            u.unit_state = (uint8_t)(U(rng) % 4);
            for (int m = 0; m < 8; ++m) u.items[m] = (uint8_t)(U(rng) % 82);
            for (int m = 0; m < 16; ++m) u.skill_use[m] = U(rng) % 400;
            u.battlerec[0] = U(rng) % 200;
            u.battlerec[1] = 1 + (U(rng) % 0x40);
            u.battlerec[3] = (U(rng) % 2) ? 0x2d : 0;
        }
        for (int c = 0; c < 130; ++c) {
            w.heal_buffer[c]   = Sgn(rng);
            w.morale_buffer[c] = Sgn(rng);
        }
        w.special_score_a = Sgn(rng);
        w.special_score_b = Sgn(rng);
        for (int k = 0; k < 16; ++k) {
            w.skill_param1[k] = U(rng) % 200;
            w.skill_param2[k] = U(rng) % 200;
            w.terrain_mod_a[k]= (int32_t)(U(rng) % 200) - 50;
            w.terrain_mod_b[k]= (int32_t)(U(rng) % 200) - 50;
        }
        for (int c = 0; c < 96; ++c) {
            w.class_hp[c][0] = Sgn(rng);
            w.class_hp[c][1] = U(rng) % 20;
        }

        // 随机占驻：在网格区域每格以 50% 概率布置单位；attacker/current_unit 固定为某单位。
        int rows = big ? 10 : 9;
        int cols = big ? 13 : 9;
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                uint32_t tile = (uint32_t)((origin + r) * mw + (origin + c));
                uint32_t cell = (uint32_t)(r * cols + c);
                if ((U(rng) % 2) == 0) {
                    w.unit_presence[tile * 2] = 1;
                    w.unit_index[tile * 2] = (uint8_t)(U(rng) % 128);
                    w.heal_buffer[cell] = Sgn(rng);   // 非零 → 不被"空格清零"跳过
                    if (w.heal_buffer[cell] == 0) w.heal_buffer[cell] = 1;
                }
            }
        }

        // 地形网格播种（隔离地形数据来源，两层读同一全局 → 同地形类型 → 同 mod 表）
        g_battlecalc_map_width = mw;
        for (int r = 0; r <= 15; ++r)
            for (int c = 0; c <= 15; ++c)
                g_battlecalc_terrain_grid[r * mw + c] = (uint16_t)((T(rng) << 12));

        uint32_t attacker = (uint32_t)(U(rng) % 128);
        uint32_t skill_type = (uint32_t)(U(rng) % 16);
        uint32_t current_unit = (uint32_t)(U(rng) % 128);

        mirror_grid(w);

        // 种子复位 → 整网格随机序列一致
        g_game_state.g_pow2_1_448790 = (int32_t)SEED;
        uint32_t clean_ret = dlzr::logic::settle_skill_grid(w, attacker, skill_type, current_unit);
        g_game_state.g_pow2_1_448790 = (int32_t)SEED;
        uint32_t oracle_ret = battle_calc_settle_skill_grid(attacker, skill_type, current_unit);

        // 逐单位 / 逐缓冲 / 特殊计分 / 返回值 比对
        for (int i = 0; i < 128; ++i) {
            auto& cu = w.units[i];
            auto& of = g_game_state.fighter_array[i];
            ASSERT_EQ(cu.current_hp,      of.current_hp)      << "t=" << t << " u" << i << " hp";
            ASSERT_EQ(cu.morale,          of.morale)          << "t=" << t << " u" << i << " morale";
            ASSERT_EQ(cu.equip_integrity, of.equip_integrity) << "t=" << t << " u" << i << " equip";
            ASSERT_EQ(cu.defense_modifier,of.defense_modifier)<< "t=" << t << " u" << i << " defmod";
            ASSERT_EQ(cu.attacker_index,  of.attacker_index)  << "t=" << t << " u" << i << " atkidx";
            ASSERT_EQ((int)cu.unit_state, (int)of.unit_state) << "t=" << t << " u" << i << " state";
            for (int m = 0; m < 16; ++m)
                ASSERT_EQ(cu.skill_use[m], oracle_skill_use(i, m))
                    << "t=" << t << " u" << i << " skill" << m;
            for (int k = 0; k < 9; ++k)
                ASSERT_EQ(cu.battlerec[k], g_battlerec_0x7d0f18[i][k])
                    << "t=" << t << " u" << i << " battlerec" << k;
            for (int m = 0; m < 8; ++m)
                ASSERT_EQ((int)cu.items[m], (int)g_game_state.people_array[i].items[m])
                    << "t=" << t << " u" << i << " item" << m;
        }
        for (int c = 0; c < 130; ++c) {
            ASSERT_EQ(w.heal_buffer[c],   g_hp_dmg_buf_0x7d0488[c])   << "t=" << t << " hpbuf" << c;
            ASSERT_EQ(w.morale_buffer[c], g_morale_dmg_buf_0x4565d0[c])<< "t=" << t << " mobuf" << c;
        }
        for (int k = 0; k < 156; ++k)
            ASSERT_EQ((int)w.depot[k], (int)g_warehouse_items_battle_0x7db3a8[k])
                << "t=" << t << " depot" << k;
        ASSERT_EQ(w.special_score_a, g_special_score_0x7d06d4) << "t=" << t << " sp_a";
        ASSERT_EQ(w.special_score_b, g_special_score_0x7d06d8) << "t=" << t << " sp_b";
        ASSERT_EQ(clean_ret, oracle_ret) << "t=" << t << " ret";
    }
}

// ============================================================
// T3 — 基础伤害写入 pass：clean::apply_base_damage vs oracle::battle_calc_apply_cell_damage
// 随机整网格占驻 + 兵种攻防表 + 双网格尺寸 + current_mode(0/0x96) 分支；
// 断言 per-occupant Fighter 字段、per-cell 缓冲、attacker.skill_gongji、返回值 逐位一致。
// 纯确定性（无 RNG）；current_hp 允许负值以锁死 uint 环绕语义。
// ============================================================
TEST(SkillGridDiff, CellBaseDamage) {
    load_canonical_skill_thresholds();
    std::mt19937 rng(0x1337C0DEu);
    std::uniform_int_distribution<int32_t> U(0, 400);
    std::uniform_int_distribution<int32_t> E(0, 2);
    std::uniform_int_distribution<int32_t> Sgn(-200, 200);
    std::uniform_int_distribution<int32_t> T(0, 15);   // 地形类型 0..15
    const int N = 10000;
    for (int t = 0; t < N; ++t) {
        dlzr::logic::BattleGridState w;
        int mw = 64;
        int origin = (int)(U(rng) % 4);
        bool big = (U(rng) % 2) == 0;
        w.resize_grid_buffers(13, 13, mw);
        w.map_width = mw;
        w.grid_origin_row = origin;
        w.grid_origin_col = origin;
        w.double_grid = big ? 1 : 0;

        for (int i = 0; i < 128; ++i) {
            auto& u = w.units[i];
            u.enemy = E(rng);
            u.class_type = U(rng) % 96;
            u.level = 1 + (U(rng) % 99);
            u.intelligence = (int32_t)(U(rng) % 200) - 50;
            u.current_hp = Sgn(rng);      // 允许负值以锁死 uint 环绕语义
            u.morale = Sgn(rng);
            u.equip_integrity = Sgn(rng);
            u.defense_modifier = Sgn(rng);
            u.attacker_index = (int32_t)(U(rng) % 128);
            u.active_flag = (uint8_t)(U(rng) % 2);
            u.unit_state = (uint8_t)(U(rng) % 4);
            u.strength = 1 + (U(rng) % 200);     // 正武力（真实域）
            u.skill_gongji = U(rng) % 400;
            u.pos_x = T(rng);                     // 0..15 → 落入已播种地形区
            u.pos_y = T(rng);
            for (int m = 0; m < 8; ++m) u.items[m] = (uint8_t)(U(rng) % 82);
            for (int m = 0; m < 16; ++m) u.skill_use[m] = U(rng) % 400;
            u.battlerec[0] = U(rng) % 200;
        }
        for (int c = 0; c < 130; ++c) {
            w.heal_buffer[c]   = Sgn(rng);
            w.morale_buffer[c] = Sgn(rng);
        }
        // class_hp：hp_base 严格为正（避免 T3 士气伤害 maxhp 除零 SIGFPE），growth 任意。
        for (int c = 0; c < 96; ++c) {
            w.class_hp[c][0] = 1 + (U(rng) % 200);
            w.class_hp[c][1] = U(rng) % 20;
            w.class_combat[c][0] = Sgn(rng);   // base_attack
            w.class_combat[c][1] = U(rng) % 20;  // atk_growth
            w.class_combat[c][2] = Sgn(rng);   // base_defense
            w.class_combat[c][3] = U(rng) % 20; // def_growth
        }
        w.current_mode = (U(rng) % 2) ? 0x96 : 0;   // 交替触发攻防 0x96 分支

        // 随机占驻（与 SkillEffectGrid 同构）
        int rows = big ? 10 : 9;
        int cols = big ? 13 : 9;
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                uint32_t tile = (uint32_t)((origin + r) * mw + (origin + c));
                uint32_t cell = (uint32_t)(r * cols + c);
                if ((U(rng) % 2) == 0) {
                    w.unit_presence[tile * 2] = 1;
                    w.unit_index[tile * 2] = (uint8_t)(U(rng) % 128);
                    w.heal_buffer[cell] = Sgn(rng);
                    if (w.heal_buffer[cell] == 0) w.heal_buffer[cell] = 1;  // 非零 → 不跳过
                }
            }
        }

        // 地形网格 + 地形修正表播种（隔离数据源差异，两层读同一全局）
        g_battlecalc_map_width = mw;
        for (int r = 0; r <= 15; ++r)
            for (int c = 0; c <= 15; ++c)
                g_battlecalc_terrain_grid[r * mw + c] = (uint16_t)((T(rng) << 12));
        for (int k = 0; k < 16; ++k)
            g_battlecalc_terrain_modifier[k] = (int32_t)(U(rng) % 200) - 50;

        uint32_t attacker = (uint32_t)(U(rng) % 128);

        mirror_grid(w);

        uint32_t clean_ret = dlzr::logic::apply_base_damage(w, attacker);
        uint32_t oracle_ret = battle_calc_apply_cell_damage(attacker);

        // per-occupant Fighter 字段（T3 触及：current_hp/morale/attacker_index/unit_state）
        for (int i = 0; i < 128; ++i) {
            auto& cu = w.units[i];
            auto& of = g_game_state.fighter_array[i];
            ASSERT_EQ(cu.current_hp,      of.current_hp)      << "t=" << t << " u" << i << " hp";
            ASSERT_EQ(cu.morale,          of.morale)          << "t=" << t << " u" << i << " morale";
            ASSERT_EQ(cu.attacker_index,  of.attacker_index)  << "t=" << t << " u" << i << " atkidx";
            ASSERT_EQ((int)cu.unit_state, (int)of.unit_state) << "t=" << t << " u" << i << " state";
        }
        // per-cell 缓冲（含清空路径）
        for (int c = 0; c < 130; ++c) {
            ASSERT_EQ(w.heal_buffer[c],   g_hp_dmg_buf_0x7d0488[c])   << "t=" << t << " hpbuf" << c;
            ASSERT_EQ(w.morale_buffer[c], g_morale_dmg_buf_0x4565d0[c])<< "t=" << t << " mobuf" << c;
        }
        // attacker.skill_gongji（T3 每次命中 +1）
        ASSERT_EQ(w.units[attacker].skill_gongji, g_game_state.people_array[attacker].skill_gongji)
            << "t=" << t << " atk=" << attacker << " skillgongji";
        // 返回值（封顶 0x30 的攻击经验小计）
        ASSERT_EQ(clean_ret, oracle_ret) << "t=" << t << " ret";
    }
}

// ============================================================
// T3-b — 死亡/经验循环：clean::apply_death_experience vs oracle::battle_calc_apply_death_experience
// 随机整网格占驻 + 兵种攻防表 + 双网格尺寸 + current_mode(0/0x96) + 阵亡计数/胜负模式/剩余单位倒数随机化；
// 断言 per-occupant active_flag / experience / level、主将 HP 封顶、阵亡计数、物品回收数组、返回值 逐位一致。
// 约 30% 单位低血/低士气以稳定触发阵亡分支；纯确定性（无 RNG）。
// ============================================================
TEST(SkillGridDiff, CellDeathExp) {
    load_canonical_skill_thresholds();
    std::mt19937 rng(0xDEADBEEFu);
    std::uniform_int_distribution<int32_t> U(0, 400);
    std::uniform_int_distribution<int32_t> E(0, 2);
    std::uniform_int_distribution<int32_t> Sgn(-200, 200);
    std::uniform_int_distribution<int32_t> T(0, 15);
    const int N = 10000;
    for (int t = 0; t < N; ++t) {
        dlzr::logic::BattleGridState w;
        int mw = 64;
        int origin = (int)(U(rng) % 4);
        bool big = (U(rng) % 2) == 0;
        w.resize_grid_buffers(13, 13, mw);
        w.map_width = mw;
        w.grid_origin_row = origin;
        w.grid_origin_col = origin;
        w.double_grid = big ? 1 : 0;

        for (int i = 0; i < 128; ++i) {
            auto& u = w.units[i];
            u.enemy = E(rng);
            u.class_type = U(rng) % 96;
            u.level = 1 + (U(rng) % 99);
            u.intelligence = (int32_t)(U(rng) % 200) - 50;
            u.experience = (int32_t)(U(rng) % 100);   // 级内经验 0..99
            u.current_hp = Sgn(rng);
            u.morale = Sgn(rng);
            u.equip_integrity = Sgn(rng);
            u.defense_modifier = Sgn(rng);
            u.attacker_index = (int32_t)(U(rng) % 128);
            u.active_flag = (uint8_t)(U(rng) % 2);
            u.unit_state = (uint8_t)(U(rng) % 4);
            u.strength = 1 + (U(rng) % 200);
            u.skill_gongji = U(rng) % 400;
            u.pos_x = T(rng);
            u.pos_y = T(rng);
            for (int m = 0; m < 8; ++m) u.items[m] = (uint8_t)(U(rng) % 82);
            for (int m = 0; m < 16; ++m) u.skill_use[m] = U(rng) % 400;
            u.battlerec[0] = U(rng) % 200;
            // 约 30% 单位低血/低士气 → 稳定触发阵亡分支
            if ((U(rng) % 10) < 3) { u.current_hp = 1 + (U(rng) % 4); u.morale = 1 + (U(rng) % 4); }
        }
        for (int c = 0; c < 130; ++c) {
            w.heal_buffer[c]   = Sgn(rng);
            w.morale_buffer[c] = Sgn(rng);
        }
        for (int c = 0; c < 96; ++c) {
            w.class_hp[c][0] = 1 + (U(rng) % 200);
            w.class_hp[c][1] = U(rng) % 20;
            w.class_combat[c][0] = Sgn(rng);
            w.class_combat[c][1] = U(rng) % 20;
            w.class_combat[c][2] = Sgn(rng);
            w.class_combat[c][3] = U(rng) % 20;
        }
        w.current_mode = (U(rng) % 2) ? 0x96 : 0;
        // T3-b 输入：阵亡计数 / 胜负模式 / 剩余单位倒数（随机化覆盖各分支）
        w.lost_player = (int32_t)(U(rng) % 6);
        w.lost_enemy = (int32_t)(U(rng) % 6);
        w.battle_win_mode = (int32_t)(U(rng) % 3);     // 0/1/2
        w.player_units_remaining = 1 + (int32_t)(U(rng) % 10);

        int rows = big ? 10 : 9;
        int cols = big ? 13 : 9;
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                uint32_t tile = (uint32_t)((origin + r) * mw + (origin + c));
                uint32_t cell = (uint32_t)(r * cols + c);
                if ((U(rng) % 2) == 0) {
                    w.unit_presence[tile * 2] = 1;
                    w.unit_index[tile * 2] = (uint8_t)(U(rng) % 128);
                    w.heal_buffer[cell] = Sgn(rng);
                    if (w.heal_buffer[cell] == 0) w.heal_buffer[cell] = 1;  // 非零 → 不跳过
                }
            }
        }

        g_battlecalc_map_width = mw;
        for (int r = 0; r <= 15; ++r)
            for (int c = 0; c <= 15; ++c)
                g_battlecalc_terrain_grid[r * mw + c] = (uint16_t)((T(rng) << 12));
        for (int k = 0; k < 16; ++k)
            g_battlecalc_terrain_modifier[k] = (int32_t)(U(rng) % 200) - 50;

        uint32_t attacker = (uint32_t)(U(rng) % 128);

        mirror_grid(w);

        // pass-1：基础伤害写入（返回攻击经验小计，已封顶 0x30）
        uint32_t clean_sub = dlzr::logic::apply_base_damage(w, attacker);
        uint32_t oracle_sub = battle_calc_apply_cell_damage(attacker);
        ASSERT_EQ(clean_sub, oracle_sub) << "t=" << t << " subtotal";

        // T3-b：死亡/经验循环（返回总经验 = 小计 + 击杀经验）
        uint32_t clean_total = dlzr::logic::apply_death_experience(w, attacker, clean_sub);
        uint32_t oracle_total = battle_calc_apply_death_experience(attacker, oracle_sub);
        ASSERT_EQ(clean_total, oracle_total) << "t=" << t << " total";

        // 逐单位：active_flag / experience / level（经验仅改攻击者，全比安全）
        for (int i = 0; i < 128; ++i) {
            auto& cu = w.units[i];
            auto& of = g_game_state.fighter_array[i];
            ASSERT_EQ((int)cu.active_flag, (int)of.active_flag) << "t=" << t << " u" << i << " active";
            ASSERT_EQ(cu.experience, g_game_state.people_array[i].experience) << "t=" << t << " u" << i << " exp";
            ASSERT_EQ(cu.level, g_game_state.people_array[i].level) << "t=" << t << " u" << i << " lvl";
        }
        // 主将 HP 封顶（Fighter[0].current_hp）
        ASSERT_EQ(w.units[0].current_hp, g_game_state.fighter_array[0].current_hp) << "t=" << t << " leader_hp";
        // 阵亡计数
        ASSERT_EQ(w.lost_player, g_game_state.g_aux_counter_b) << "t=" << t << " lost_player";
        ASSERT_EQ(w.lost_enemy, g_game_state.g_aux_counter_a) << "t=" << t << " lost_enemy";
        // 胜负模式 / 剩余单位倒数（后者仅特定支路被改，全比安全）
        ASSERT_EQ(w.battle_win_mode, g_game_state.g_flow_phase) << "t=" << t << " win_mode";
        ASSERT_EQ(w.player_units_remaining, g_player_units_remaining) << "t=" << t << " player_units";
        // 物品回收数组
        for (int k = 0; k < 256; ++k)
            ASSERT_EQ((int)w.item_recovery[k], (int)g_item_recovery[k]) << "t=" << t << " irec" << k;
    }
}

// ============================================================
// §9.3② — 3-pass 管线集成（setup → FUN_0041307f → FUN_004138ea），真实 game.dat 技能形状表播种
//
// 铁律零集成闸门：用真实技能形状表（game.dat 顺序块 #11，铁律零实证见 test_battle_setup.cpp）
// 作 setup pass 播种，跑 T3（apply_cell_damage / FUN_0041307f 基础伤害写入）→ 结算
// （settle_skill_grid / FUN_004138ea 按技能类型缩放），断言 clean 层与 oracle 层 end-to-end 逐位一致；
// 并断言 HP 缓冲确由真实形状播种（中心格 == 形状中心值）。
//
// 限伤害型技能（3/4/5/6/0xd/0xe/0xf）：T3→结算 是二进制文档化的通用伤害管线
// （RE doc §1.4：FUN_0041307f 写基础伤害 → FUN_004138ea 读缓冲 ×uVar8/uVar9 缩放施加）。
// heal/buff/status 的编排（是否调 T3）属 Tier-B 细节，不在本伤害路径集成闸门范围内。
//
// 注：FUN_00412030 的旋转/叠加原语（battle_setup_skill_overlay_12030 / battle_calc_skill_shape_at）
// 已在 test_battle_setup.cpp 单独逐指令对拍；此处复用其输出（真实形状）作缓冲播种。
// ============================================================
namespace {

// 文件内 static（匿名命名空间，内部链接），避免与其他 test TU 的 kGameDataDir 多定义冲突。
const char* kGameDataDir = "C:/seesaw/source/dlz_research/DLZFULL";

// 复用真实技能形状表装载（与 test_battle_setup.cpp 同源，文件内 static 避免多定义）。
static size_t pipeline_shape_offset() {
    const uint32_t pre[][2] = {
        {1, 0xa0}, {0x14, 0x10}, {0xa0, 0x60}, {1, 0x600}, {0x48, 0xa0},
        {0x1c, 10}, {1, 100}, {0x30, 10}, {0x30, 0x10}, {1, 0x510},
    };
    size_t off = 0;
    for (auto& p : pre) off += (size_t)p[0] * p[1];
    return off;  // = 31820 = 0x7C4C
}
static bool pipeline_load_shape(std::vector<uint8_t>& out) {
    std::string path = std::string(kGameDataDir) + "/game.dat";
    std::ifstream f(path, std::ios::binary);
    if (!f) { f.open(std::string(kGameDataDir) + "/GAME.DAT", std::ios::binary); if (!f) return false; }
    const size_t off = pipeline_shape_offset();
    const size_t nbytes = 4u * 0x930u;  // 9408
    f.seekg((std::streamoff)off);
    out.resize(nbytes);
    f.read((char*)out.data(), (std::streamsize)nbytes);
    return (size_t)f.gcount() == nbytes;
}

}  // namespace

TEST(SkillGridDiff, ThreePassPipelineRealSkillShape) {
    std::vector<uint8_t> shape_table;
    ASSERT_TRUE(pipeline_load_shape(shape_table)) << "game.dat skill shape table load failed";
    ASSERT_EQ(shape_table.size(), (size_t)(4 * 0x930));

    load_canonical_skill_thresholds();
    std::mt19937 rng(0x3C0DE5u);
    std::uniform_int_distribution<int32_t> U(0, 400);
    std::uniform_int_distribution<int32_t> E(0, 2);
    std::uniform_int_distribution<int32_t> Sgn(-200, 200);
    std::uniform_int_distribution<int32_t> T(0, 15);
    std::uniform_int_distribution<int32_t> DmgSkill(0, 6);   // → 3,4,5,6,13,14,15
    const int N = 3000;
    const uint32_t SEED = 0x5EEDC0DEu;
    static const uint32_t dmg[7] = {3, 4, 5, 6, 13, 14, 15};

    for (int t = 0; t < N; ++t) {
        dlzr::logic::BattleGridState w;
        int mw = 64;
        int origin = (int)(U(rng) % 4);
        bool big = (U(rng) % 2) == 0;
        w.resize_grid_buffers(13, 13, mw);
        w.map_width = mw;
        w.grid_origin_row = origin;
        w.grid_origin_col = origin;
        w.double_grid = big ? 1 : 0;

        for (int i = 0; i < 128; ++i) {
            auto& u = w.units[i];
            u.enemy = E(rng);
            u.class_type = U(rng) % 96;
            u.level = 1 + (U(rng) % 99);
            u.intelligence = (int32_t)(U(rng) % 200) - 50;
            u.current_hp = Sgn(rng);
            u.morale = Sgn(rng);
            u.equip_integrity = Sgn(rng);
            u.defense_modifier = Sgn(rng);
            u.attacker_index = (int32_t)(U(rng) % 128);
            u.active_flag = (uint8_t)(U(rng) % 2);
            u.unit_state = (uint8_t)(U(rng) % 4);
            u.strength = 1 + (U(rng) % 200);
            u.skill_gongji = U(rng) % 400;
            u.pos_x = T(rng);
            u.pos_y = T(rng);
            for (int m = 0; m < 8; ++m) u.items[m] = (uint8_t)(U(rng) % 82);
            for (int m = 0; m < 16; ++m) u.skill_use[m] = U(rng) % 400;
            u.battlerec[0] = U(rng) % 200;
            u.battlerec[1] = 1 + (U(rng) % 0x40);
            u.battlerec[3] = (U(rng) % 2) ? 0x2d : 0;
        }
        for (int c = 0; c < 130; ++c) { w.heal_buffer[c] = 0; w.morale_buffer[c] = 0; }
        w.special_score_a = Sgn(rng);
        w.special_score_b = Sgn(rng);
        for (int k = 0; k < 16; ++k) {
            w.skill_param1[k] = U(rng) % 200;
            w.skill_param2[k] = U(rng) % 200;
            w.terrain_mod_a[k]= (int32_t)(U(rng) % 200) - 50;
            w.terrain_mod_b[k]= (int32_t)(U(rng) % 200) - 50;
        }
        for (int c = 0; c < 96; ++c) {
            w.class_hp[c][0] = 1 + (U(rng) % 200);
            w.class_hp[c][1] = U(rng) % 20;
            w.class_combat[c][0] = Sgn(rng);
            w.class_combat[c][1] = U(rng) % 20;
            w.class_combat[c][2] = Sgn(rng);
            w.class_combat[c][3] = U(rng) % 20;
        }
        w.current_mode = (U(rng) % 2) ? 0x96 : 0;

        int rows = big ? 10 : 9;
        int cols = big ? 13 : 9;

        // ---- SETUP PASS：真实技能形状表播种（FUN_00412030 等价）----
        uint32_t shape_index = (uint32_t)(U(rng) % 48);
        uint32_t facing = (uint32_t)(U(rng) % 4);
        uint32_t shape[49];
        ASSERT_EQ(battle_calc_skill_shape_at(shape_table.data(), (int32_t)shape_index, (int)facing, shape), 0)
            << "t=" << t;
        int anchor_r = (int)(U(rng) % rows);
        int anchor_c = (int)(U(rng) % cols);
        for (int sr = 0; sr < 7; ++sr) {
            for (int sc = 0; sc < 7; ++sc) {
                int bx = anchor_c - 3 + sc;
                int by = anchor_r - 3 + sr;
                if (bx >= 0 && bx < cols && by >= 0 && by < rows) {
                    int cell = by * cols + bx;
                    int tile = (origin + by) * mw + (origin + bx);
                    uint32_t v = shape[sr * 7 + sc];
                    w.heal_buffer[cell] = (int32_t)v;
                    w.morale_buffer[cell] = (int32_t)v;  // setup pass 把 HP 种子拷入士气缓冲（FUN_0043de50）
                    if (v != 0 && (U(rng) % 2) == 0) {
                        w.unit_presence[tile * 2] = 1;
                        w.unit_index[tile * 2] = (uint8_t)(U(rng) % 128);
                    }
                }
            }
        }
        // 锚点中心格 == 形状中心值（shape[3*7+3]=shape[24]），证明缓冲确由真实形状播种。
        int anchor_cell = anchor_r * cols + anchor_c;
        ASSERT_EQ(w.heal_buffer[anchor_cell], (int32_t)shape[24]) << "t=" << t << " seed anchor hp";
        ASSERT_EQ(w.morale_buffer[anchor_cell], (int32_t)shape[24]) << "t=" << t << " seed anchor mo";

        uint32_t attacker = (uint32_t)(U(rng) % 128);
        uint32_t skill_type = dmg[(int)DmgSkill(rng)];
        uint32_t current_unit = attacker;

        mirror_grid(w);
        // 镜像后 oracle 缓冲也应等于真实形状种子。
        ASSERT_EQ(g_hp_dmg_buf_0x7d0488[anchor_cell], (int32_t)shape[24]) << "t=" << t << " seed anchor hp(oracle)";
        ASSERT_EQ(g_morale_dmg_buf_0x4565d0[anchor_cell], (int32_t)shape[24]) << "t=" << t << " seed anchor mo(oracle)";

        // ---- 管线：T3（FUN_0041307f）基础伤害写入 → 结算（FUN_004138ea）缩放施加 ----
        g_game_state.g_pow2_1_448790 = (int32_t)SEED;
        uint32_t clean_t3 = dlzr::logic::apply_base_damage(w, attacker);
        uint32_t clean_ret = dlzr::logic::settle_skill_grid(w, attacker, skill_type, current_unit);

        g_game_state.g_pow2_1_448790 = (int32_t)SEED;
        uint32_t oracle_t3 = battle_calc_apply_cell_damage(attacker);
        uint32_t oracle_ret = battle_calc_settle_skill_grid(attacker, skill_type, current_unit);

        ASSERT_EQ(clean_t3, oracle_t3) << "t=" << t << " t3_ret";
        ASSERT_EQ(clean_ret, oracle_ret) << "t=" << t << " settle_ret";

        // ---- 逐位可观测输出比对（clean vs oracle）----
        for (int i = 0; i < 128; ++i) {
            auto& cu = w.units[i];
            auto& of = g_game_state.fighter_array[i];
            ASSERT_EQ(cu.current_hp,      of.current_hp)      << "t=" << t << " u" << i << " hp";
            ASSERT_EQ(cu.morale,          of.morale)          << "t=" << t << " u" << i << " morale";
            ASSERT_EQ(cu.equip_integrity, of.equip_integrity) << "t=" << t << " u" << i << " equip";
            ASSERT_EQ(cu.defense_modifier,of.defense_modifier)<< "t=" << t << " u" << i << " defmod";
            ASSERT_EQ(cu.attacker_index,  of.attacker_index)  << "t=" << t << " u" << i << " atkidx";
            ASSERT_EQ((int)cu.unit_state, (int)of.unit_state) << "t=" << t << " u" << i << " state";
            ASSERT_EQ(cu.skill_gongji, g_game_state.people_array[i].skill_gongji) << "t=" << t << " u" << i << " skillgongji";
            for (int m = 0; m < 16; ++m)
                ASSERT_EQ(cu.skill_use[m], oracle_skill_use(i, m)) << "t=" << t << " u" << i << " skill" << m;
            for (int k = 0; k < 9; ++k)
                ASSERT_EQ(cu.battlerec[k], g_battlerec_0x7d0f18[i][k]) << "t=" << t << " u" << i << " battlerec" << k;
            for (int m = 0; m < 8; ++m)
                ASSERT_EQ((int)cu.items[m], (int)g_game_state.people_array[i].items[m]) << "t=" << t << " u" << i << " item" << m;
        }
        for (int c = 0; c < 130; ++c) {
            ASSERT_EQ(w.heal_buffer[c],   g_hp_dmg_buf_0x7d0488[c])   << "t=" << t << " hpbuf" << c;
            ASSERT_EQ(w.morale_buffer[c], g_morale_dmg_buf_0x4565d0[c])<< "t=" << t << " mobuf" << c;
        }
        ASSERT_EQ(w.special_score_a, g_special_score_0x7d06d4) << "t=" << t << " sp_a";
        ASSERT_EQ(w.special_score_b, g_special_score_0x7d06d8) << "t=" << t << " sp_b";
    }
}

// ============================================================
// 地形修正表（GAME.DAT 块#1，off 0xA0，320B=16×20B 记录）真实数据差分闸门
//
// 铁律零：FUN_00424566.c:15 装载 16×0x14B 到 0x475aa8；FUN_0042a98e.c:21 读
// *(int*)(0x475ab0 + t*0x14)（=每记录 int2=防御加成%）；FUN_004138ea.c:262/310 读
// int0/int1（mod_a/mod_b）。装载后三层视图（replica 防御加成 / oracle mod_a,mod_b /
// clean g_terrain_defense_bonus）必须都 == GAME.DAT 字节实测值。
// ============================================================
TEST(SkillGridDiff, TerrainModifierRealData) {
    std::string path = std::string(kGameDataDir) + "/game.dat";
    std::ifstream f(path, std::ios::binary);
    ASSERT_TRUE(f) << path;
    f.seekg(0xA0);
    int32_t rec[80];
    f.read((char*)rec, 320);
    ASSERT_EQ((size_t)f.gcount(), 320u);

    battle_calc_install_terrain_table(rec);
    battle_calc_install_terrain_mod_ab(rec);
    dlzr::logic::set_terrain_defense_bonus(g_battlecalc_terrain_modifier);

    // 二进制字节实测钉死的关键记录（20B/记录=5 int）
    const int32_t expect0[5]  = {40, -20, 10, 0, 0};    // terr[0]  def=10
    const int32_t expect9[5]  = {20, 0, 15, 20, 10};    // terr[9]  def=15
    const int32_t expect11[5] = {20, 0, 40, 10, 5};     // terr[11] def=40

    for (int t = 0; t < 16; ++t) {
        // 防御加成%（int2）== 二进制 *(int*)(0x475ab0 + t*0x14)
        ASSERT_EQ(g_battlecalc_terrain_modifier[t], rec[t * 5 + 2]) << "def t" << t;
        ASSERT_EQ((int)dlzr::logic::get_terrain_defense_bonus(t), rec[t * 5 + 2]) << "clean def t" << t;
        // mod_a / mod_b（FUN_004138ea case 3/4/5 / case 6）
        ASSERT_EQ(g_terrain_mod_a_0x475aa8[t], rec[t * 5 + 0]) << "moda t" << t;
        ASSERT_EQ(g_terrain_mod_b_0x475aac[t], rec[t * 5 + 1]) << "modb t" << t;
    }
    ASSERT_EQ(g_battlecalc_terrain_modifier[0],  expect0[2]);
    ASSERT_EQ(g_terrain_mod_a_0x475aa8[0],  expect0[0]);
    ASSERT_EQ(g_terrain_mod_b_0x475aac[0],  expect0[1]);
    ASSERT_EQ(g_battlecalc_terrain_modifier[9],  expect9[2]);
    ASSERT_EQ(g_battlecalc_terrain_modifier[11], expect11[2]);
}

// ============================================================
// §9.3② — 三 pass 管线「运行时入口」闸门：battle_calc_run_skill_pipeline
// 直接在 g_game_state + 模块全局上施加一次技能效果（setup 用真实 game.dat 形状表播种 →
// T3 → 结算），验证其与 clean 层（apply_base_damage + settle_skill_grid）逐位一致。
// 这是「3-pass 管线接入 GameState 运行时」的落地点（此前仅由镜像+直接 oracle 调用验证）。
// ============================================================
TEST(SkillGridDiff, RuntimePipelineAppliesToGameState) {
    std::vector<uint8_t> shape_table;
    ASSERT_TRUE(pipeline_load_shape(shape_table)) << "game.dat skill shape table load failed";
    load_canonical_skill_thresholds();

    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<int32_t> U(0, 400);
    std::uniform_int_distribution<int32_t> E(0, 2);
    std::uniform_int_distribution<int32_t> Sgn(-200, 200);
    std::uniform_int_distribution<int32_t> T(0, 15);

    dlzr::logic::BattleGridState w;
    int mw = 64, origin = 1;
    int rows = 9, cols = 9;
    w.resize_grid_buffers(13, 13, mw);
    w.map_width = mw;
    w.grid_origin_row = origin;
    w.grid_origin_col = origin;
    w.double_grid = 0;

    for (int i = 0; i < 128; ++i) {
        auto& u = w.units[i];
        u.enemy = E(rng);
        u.class_type = U(rng) % 96;
        u.level = 1 + (U(rng) % 99);
        u.intelligence = (int32_t)(U(rng) % 200) - 50;
        u.current_hp = Sgn(rng);
        u.morale = Sgn(rng);
        u.equip_integrity = Sgn(rng);
        u.defense_modifier = Sgn(rng);
        u.attacker_index = (int32_t)(U(rng) % 128);
        u.active_flag = (uint8_t)(U(rng) % 2);
        u.unit_state = (uint8_t)(U(rng) % 4);
        u.strength = 1 + (U(rng) % 200);
        u.skill_gongji = U(rng) % 400;
        u.pos_x = T(rng);
        u.pos_y = T(rng);
        for (int m = 0; m < 8; ++m) u.items[m] = (uint8_t)(U(rng) % 82);
        for (int m = 0; m < 16; ++m) u.skill_use[m] = U(rng) % 400;
        u.battlerec[0] = U(rng) % 200;
        u.battlerec[1] = 1 + (U(rng) % 0x40);
        u.battlerec[3] = (U(rng) % 2) ? 0x2d : 0;
    }
    for (int c = 0; c < 130; ++c) { w.heal_buffer[c] = 0; w.morale_buffer[c] = 0; }
    w.special_score_a = Sgn(rng);
    w.special_score_b = Sgn(rng);
    for (int k = 0; k < 16; ++k) {
        w.skill_param1[k] = U(rng) % 200;
        w.skill_param2[k] = U(rng) % 200;
        w.terrain_mod_a[k] = (int32_t)(U(rng) % 200) - 50;
        w.terrain_mod_b[k] = (int32_t)(U(rng) % 200) - 50;
    }
    for (int c = 0; c < 96; ++c) {
        w.class_hp[c][0] = 1 + (U(rng) % 200);
        w.class_hp[c][1] = U(rng) % 20;
        w.class_combat[c][0] = Sgn(rng);
        w.class_combat[c][1] = U(rng) % 20;
        w.class_combat[c][2] = Sgn(rng);
        w.class_combat[c][3] = U(rng) % 20;
    }
    w.current_mode = (U(rng) % 2) ? 0x96 : 0;

    // SETUP 播种：真实技能形状表（setup pass 等价）
    uint32_t shape_index = (uint32_t)(U(rng) % 48);
    uint32_t facing = (uint32_t)(U(rng) % 4);
    uint32_t shape[49];
    ASSERT_EQ(battle_calc_skill_shape_at(shape_table.data(), (int32_t)shape_index, (int)facing, shape), 0);
    int anchor_r = (int)(U(rng) % rows);
    int anchor_c = (int)(U(rng) % cols);
    for (int sr = 0; sr < 7; ++sr) {
        for (int sc = 0; sc < 7; ++sc) {
            int bx = anchor_c - 3 + sc;
            int by = anchor_r - 3 + sr;
            if (bx >= 0 && bx < cols && by >= 0 && by < rows) {
                int cell = by * cols + bx;
                int tile = (origin + by) * mw + (origin + bx);
                uint32_t v = shape[sr * 7 + sc];
                w.heal_buffer[cell] = (int32_t)v;
                w.morale_buffer[cell] = (int32_t)v;
                if (v != 0) {
                    w.unit_presence[tile * 2] = 1;
                    w.unit_index[tile * 2] = (uint8_t)(U(rng) % 128);
                }
            }
        }
    }

    uint32_t attacker = (uint32_t)(U(rng) % 128);
    uint32_t skill_type = 3;   // 伤害型技能（通用伤害管线）
    uint32_t current_unit = attacker;

    mirror_grid(w);

    // ---- 运行时入口：在 g_game_state 上施加技能（setup→T3→结算）----
    g_game_state.g_pow2_1_448790 = 0;   // 复位 RNG（与 clean 侧共享同一随机序列闸门）
    int32_t ret = battle_calc_run_skill_pipeline(attacker, skill_type, shape_index,
                                                anchor_r, anchor_c, (int)facing,
                                                cols, rows, mw, origin, shape_table.data());
    (void)ret;

    // ---- 参照：clean 层直接施加 ----
    g_game_state.g_pow2_1_448790 = 0;   // 与 clean 侧共享同一（空）RNG 序列闸门
    dlzr::logic::apply_base_damage(w, attacker);
    dlzr::logic::settle_skill_grid(w, attacker, skill_type, current_unit);

    // ---- 逐位比对（oracle g_game_state vs clean w）----
    for (int i = 0; i < 128; ++i) {
        ASSERT_EQ(w.units[i].current_hp,      g_game_state.fighter_array[i].current_hp)      << "u" << i << " hp";
        ASSERT_EQ(w.units[i].morale,          g_game_state.fighter_array[i].morale)          << "u" << i << " mo";
        ASSERT_EQ(w.units[i].attacker_index,  g_game_state.fighter_array[i].attacker_index)  << "u" << i << " atk";
        ASSERT_EQ((int)w.units[i].unit_state, (int)g_game_state.fighter_array[i].unit_state) << "u" << i << " st";
        ASSERT_EQ(w.units[i].equip_integrity, g_game_state.fighter_array[i].equip_integrity)<< "u" << i << " eq";
        ASSERT_EQ(w.units[i].defense_modifier,g_game_state.fighter_array[i].defense_modifier)<< "u" << i << " dm";
    }
    for (int c = 0; c < 130; ++c) {
        ASSERT_EQ(w.heal_buffer[c],   g_hp_dmg_buf_0x7d0488[c])   << "hpbuf" << c;
        ASSERT_EQ(w.morale_buffer[c], g_morale_dmg_buf_0x4565d0[c])<< "mobuf" << c;
    }
    ASSERT_EQ(w.special_score_a, g_special_score_0x7d06d4);
    ASSERT_EQ(w.special_score_b, g_special_score_0x7d06d8);
}
