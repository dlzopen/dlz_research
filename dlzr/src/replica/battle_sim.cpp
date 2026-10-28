// battle_sim.cpp — replica 层 FUN_004138ea 区域伤害/技能效果结算（逐指令转录）
//
// 铁律零证据：output/ghidra_all/FUN_004138ea.c（816 行）+ output/re_map/RE_FUN_004138ea.md。
// 本文件是二进制指令流的 1:1 语义转录；所有基元（伤害/夹取/最大HP/命中/技能等级）已在
// battle_calc.cpp 经 G-Func 验证，此处只组合它们，剔除 Tier B 渲染调用。
//
// 关键 RE 约定（RE doc §1）：
//   People[unit].enemy        @ offset 0x18  → 阵营（0=我方）
//   People[unit].skill_use[m]  @ offset 0x34 + m*4 → 16 个技能计数器（m=0..15，对应 skill_buji..skill_kongxi）
//   battlerec[unit].+0         @ 0x7d0f18[unit*0x21]        → 属性/智力类（uVar8/uVar9 预计算用）
//   battlerec[unit].+4         @ 0x7d0f1c[unit*0x21]        → 状态（case 0xa 递减）
//   battlerec[unit].+0xC       @ 0x7d0f24[unit*0x21]        → 状态2（case 0xc 比较 0x2d）
//   （注：0x21 = 单位记录 INT 步长，0x21*4=0x84=132B；0x7d0f18/0x7d0f1c/0x7d0f24 为同记录相邻 int 字段，
//    证据 FUN_00412030.c:23,25：&DAT_007d0f24[unit*0x21] 与 &DAT_007d0f28[unit*0x84] 同记录索引。）
//   Fighter[unit].current_hp  @ 0x96ab94[unit*0x10]  → +0x04（stride 0x40）
//   Fighter[unit].morale      @ 0x96ab98[unit*0x40]  → +0x08
//   Fighter[unit].equip_integrity @ 0x96ab9c[unit*0x40] → +0x0C
//   Fighter[unit].defense_modifier @ 0x96aba0[unit*0x40] → +0x10
//   Fighter[unit].attacker_index  @ 0x96abc8[unit*0x40] → +0x38
//   Fighter[unit].active_flag    @ 0x96abcc[unit*0x40] → +0x3C
//   Fighter[unit].unit_state     @ 0x96abcf[unit*0x40] → +0x3F

#include "battle_sim.hpp"
#include "battle_calc.hpp"        // 已验证基元
#include "replica_stubs.h"        // g_game_state（GameState）

// ---- 模块级缓冲/记录/计分全局定义（默认 0；运行时由数据文件/setup pass 装载）----
int32_t  g_hp_dmg_buf_0x7d0488[130] = {0};
int32_t  g_morale_dmg_buf_0x4565d0[130] = {0};
int32_t  g_battlerec_0x7d0f18[128][9] = {{0}};
int32_t  g_special_score_0x7d06d4 = 0;
int32_t  g_special_score_0x7d06d8 = 0;
int32_t  g_skill_param1_0x57ba50[16] = {0};
int32_t  g_skill_param2_0x57ba54[16] = {0};
int32_t  g_terrain_mod_a_0x475aa8[16] = {0};
int32_t  g_terrain_mod_b_0x475aac[16] = {0};

// 由 GAME.DAT 块#1（16×20B 记录，records80[16*5]）装载 FUN_004138ea 用的 mod_a/mod_b。
// 二进制：FUN_004138ea.c:262 读 *(int*)(0x475aa8 + t*0x14)=int0；:310/358 读 *(int*)(0x475aac + t*0x14)=int1。
void battle_calc_install_terrain_mod_ab(const int32_t* r) {
    for (int t = 0; t < 16; ++t) {
        g_terrain_mod_a_0x475aa8[t] = r[t * 5 + 0];
        g_terrain_mod_b_0x475aac[t] = r[t * 5 + 1];
    }
}
int32_t  g_grid_rows = 9;
int32_t  g_grid_cols = 9;
int32_t  g_grid_origin_row = 0;
int32_t  g_grid_origin_col = 0;
int32_t  g_battle_double_grid = 0;
int32_t  g_map_width_0x57ba20 = 64;
uint8_t  g_unit_presence_0x57bf10[65536] = {0};
uint8_t  g_unit_index_0x57bf11[65536] = {0};
int16_t  g_warehouse_items_battle_0x7db3a8[256] = {0};

// 16 个技能计数器 member 名（base offset 0x34 = skill_buji）。
// 用 switch 取引用，避开跨成员指针算术的 aliasing 隐患；语义 = 二进制 0x34 + m*4。
static int32_t& skill_use_ref(int32_t attacker, uint32_t mode) {
    auto& p = g_game_state.people_array[attacker];
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
    // mode 恒在 0..15（caller 保证），下方为编译完备性。
    return p.skill_buji;
}

// 英雄对决计分（RE doc §4：特定 attacker/occupant 索引对）。
static void apply_special_scores(uint32_t a, uint32_t o) {
    if (a == 0 && o == 7) g_special_score_0x7d06d4 += 2;
    if (a == 7 && o == 0) g_special_score_0x7d06d4 += 1;
    if (a == 8 && o == 7) g_special_score_0x7d06d8 += 2;
    if (a == 7 && o == 8) g_special_score_0x7d06d8 += 1;
}

// case 9 子调用 FUN_0042b23a（消耗占驻单位增益物品→战斗仓库 0x7db3a8）。
// 铁律零逐指令证据：FUN_0042b23a.c（31 行）+ FUN_0042920d.c（27 行）+ FUN_004290f8.c（24 行）。
//   1) FUN_0042b23a(recipient=param_1, donor=param_2)：扫 donor.items[0..7] 首个非空物品，
//      item_idx = item_id-1；调 FUN_0042920d(recipient, item_idx)；若返回 1（recipient 满）
//      → 仓库计数[item_idx]++；最后清 donor.items[首槽]=0。
//   2) FUN_0042920d(recipient, item_idx)：item_idx<0x9c 时找 recipient.items 首个空槽填入
//      item_id，再 FUN_004290f8(recipient) 压实 → 返回 0；无空槽 → 返回 1（→ 仓库）；
//      item_idx>=0x9c → 返回 2（物品丢弃，不进仓库）。
//   3) FUN_004290f8(recipient)：把 recipient.items 非零项压实到前端（8 字节），memcpy 回写。
// 注意：FUN_0043de50(dest,src,8) 在此即 8 字节内存拷贝（memcpy），直接以内联压实替代。
void battle_calc_take_item_to_depot(uint32_t recipient_idx, uint32_t donor_idx) {
    auto& donor = g_game_state.people_array[donor_idx];
    int first = -1;
    for (int i = 0; i < 8; ++i) {
        if (donor.items[i] != 0) { first = i; break; }
    }
    if (first < 0) return;                          // donor 无物品
    int item_idx = (int)donor.items[first] - 1;

    auto& recip = g_game_state.people_array[recipient_idx];
    int ret;
    if (item_idx < 0x9c) {
        int free_slot = -1;
        for (int i = 0; i < 8; ++i) {
            if (recip.items[i] == 0) { free_slot = i; break; }
        }
        if (free_slot >= 0) {
            recip.items[free_slot] = (uint8_t)(item_idx + 1);   // 装入 recipient 空槽
            // FUN_004290f8(recipient)：非零项压实到前端
            uint8_t tmp[8] = {0};
            int n = 0;
            for (int i = 0; i < 8; ++i) {
                if (recip.items[i] != 0) tmp[n++] = recip.items[i];
            }
            for (int i = 0; i < 8; ++i) recip.items[i] = tmp[i];
            ret = 0;
        } else {
            ret = 1;                                   // recipient 满 → 收归仓库
        }
    } else {
        ret = 2;                                       // item_idx>=0x9c → 丢弃
    }
    if (ret == 1) {
        g_warehouse_items_battle_0x7db3a8[item_idx] += 1;
    }
    donor.items[first] = 0;                            // 清 donor 原槽（无论走哪条路径）
}

// ============================================================
// 预计算技能威力标量（FUN_004138ea.c:64-71）
// ============================================================
void battle_calc_skill_power(uint32_t attacker_idx, uint32_t skill_type,
                             uint32_t* out_u8, uint32_t* out_u9) {
    uint32_t attr = (uint32_t)g_battlerec_0x7d0f18[attacker_idx][0];  // +0 属性/智力类
    uint32_t use  = (uint32_t)skill_use_ref((int32_t)attacker_idx, skill_type);
    uint32_t lvl  = battle_calc_skill_level(use);                     // bucket
    uint32_t p1   = (uint32_t)g_skill_param1_0x57ba50[skill_type];
    uint32_t p2   = (uint32_t)g_skill_param2_0x57ba54[skill_type];
    *out_u8 = (((p1 * attr) / 100u) * (lvl + 7)) >> 3u;   // ((param1*attr/100)*(lvl+7))>>3
    *out_u9 = (((p2 * attr) / 100u) * (lvl + 7)) >> 3u;
}

// ============================================================
// 单格效果施加（FUN_004138ea 网格循环体 lines 88-704）
// ============================================================
uint32_t battle_calc_apply_cell_effect(uint32_t attacker_idx, uint32_t skill_type,
                                       uint32_t occupant_idx, uint32_t cell,
                                       uint32_t terrain_mod, uint32_t u_var8, uint32_t u_var9,
                                       uint32_t* out_anim_dir) {
    uint32_t local_2c_cell = 0;
    uint32_t local_8 = (out_anim_dir ? *out_anim_dir : 0);
    bool attacker_player = (g_game_state.people_array[attacker_idx].enemy == 0);
    auto& F = g_game_state.fighter_array[occupant_idx];
    int32_t* hp_buf = &g_hp_dmg_buf_0x7d0488[cell];
    int32_t* mo_buf = &g_morale_dmg_buf_0x4565d0[cell];
    int32_t same = (g_game_state.people_array[attacker_idx].enemy ==
                    g_game_state.people_array[occupant_idx].enemy);

    switch (skill_type) {
    // ---- case 0：同阵营 HP 修复（heal）----
    case 0: {
        *mo_buf = 0;
        if (same) {
            uint32_t local_50 = ((uint32_t)u_var8 * (uint32_t)(*hp_buf)) / 100u;
            int32_t maxhp = battle_calc_max_hp((int32_t)occupant_idx);
            if ((uint32_t)(maxhp - F.current_hp) <= local_50) local_50 = (uint32_t)(maxhp - F.current_hp);
            F.current_hp = battle_calc_clamp_resource(F.current_hp, (int32_t)local_50, (uint32_t)maxhp);
            *hp_buf = local_50;
            if (local_50 != 0) {
                skill_use_ref((int32_t)attacker_idx, skill_type) += 1;
                if (attacker_player) {
                    local_2c_cell = (((local_50 * 0xc) / (int32_t)u_var8) < 5) ? 4
                                                                           : (local_50 * 0xc) / (int32_t)u_var8;
                } else {
                    local_2c_cell = (((local_50 << 2) / (int32_t)u_var8) < 3) ? 2
                                                                             : (local_50 << 2) / (int32_t)u_var8;
                }
                local_8 = 0;
                apply_special_scores(attacker_idx, occupant_idx);
            }
        } else {
            *hp_buf = 0;
        }
        break;
    }
    // ---- case 1：同阵营 士气 提升 ----
    case 1: {
        *hp_buf = 0;
        if (same) {
            uint32_t local_5c = ((uint32_t)u_var9 * (uint32_t)(*mo_buf)) / 100u;
            if (100 - F.morale <= local_5c) local_5c = 100 - F.morale;
            F.morale = battle_calc_clamp_resource(F.morale, (int32_t)local_5c, 100u);
            *mo_buf = (int32_t)local_5c;
            if (local_5c != 0) {
                skill_use_ref((int32_t)attacker_idx, skill_type) += 1;
                if (attacker_player) {
                    local_2c_cell = (((local_5c * 0xc) / u_var9) < 5u) ? 4u
                                                                       : (local_5c * 0xc) / u_var9;
                } else {
                    local_2c_cell = (((local_5c << 2) / u_var9) < 2u) ? 1u
                                                                      : (local_5c << 2) / u_var9;
                }
                local_8 = 0;
                apply_special_scores(attacker_idx, occupant_idx);
            }
        } else {
            *mo_buf = 0;
        }
        break;
    }
    // ---- case 2：同阵营 HP + 士气 双提升 ----
    case 2: {
        if (same) {
            uint32_t local_68 = ((uint32_t)u_var9 * (uint32_t)(*mo_buf)) / 100u;
            if (100 - F.morale <= local_68) local_68 = 100 - F.morale;
            *mo_buf = (int32_t)local_68;
            F.morale = battle_calc_clamp_resource(F.morale, (int32_t)local_68, 100u);
            uint32_t local_6c = ((uint32_t)u_var8 * (uint32_t)(*hp_buf)) / 100u;
            int32_t maxhp = battle_calc_max_hp((int32_t)occupant_idx);
            if ((uint32_t)(maxhp - F.current_hp) <= local_6c) local_6c = (uint32_t)(maxhp - F.current_hp);
            F.current_hp = battle_calc_clamp_resource(F.current_hp, (int32_t)local_6c, (uint32_t)maxhp);
            *hp_buf = local_6c;
            if (*hp_buf != 0 || *mo_buf != 0) {
                skill_use_ref((int32_t)attacker_idx, skill_type) += 1;
                local_2c_cell = attacker_player ? 8 : 2;
                local_8 = 0;
                apply_special_scores(attacker_idx, occupant_idx);
            }
        } else {
            *hp_buf = 0;
            *mo_buf = 0;
        }
        break;
    }
    // ---- case 3：异阵营 HP 伤害（地形修正 a）----
    case 3: {
        *mo_buf = 0;
        if (same) {
            *hp_buf = 0;
        } else {
            if (battle_calc_hit_chance((int32_t)attacker_idx, (int32_t)occupant_idx) == 0) {
                *hp_buf = 0;
            } else {
                int32_t base = *hp_buf;
                uint32_t local_70 = ((u_var8 * (uint32_t)base) / 100u) *
                                             ((uint32_t)terrain_mod + 100u) / 100u;
                if (F.current_hp <= local_70) local_70 = F.current_hp;
                F.current_hp = (int32_t)battle_calc_damage((uint32_t)F.current_hp,
                                                           (uint32_t)local_70, 0u);
                F.attacker_index = (int32_t)attacker_idx;
                *hp_buf = local_70;
                if (local_70 != 0) {
                    F.unit_state = 3;
                    skill_use_ref((int32_t)attacker_idx, skill_type) += 1;
                    if (attacker_player) {
                        local_2c_cell = (((local_70 << 3) / (int32_t)u_var8) < 5) ? 4
                                                                               : (local_70 << 3) / (int32_t)u_var8;
                    } else {
                        local_2c_cell = (((local_70 << 2) / (int32_t)u_var8) < 3) ? 2
                                                                                 : (local_70 << 2) / (int32_t)u_var8;
                    }
                    local_8 = 1;
                }
            }
        }
        break;
    }
    // ---- case 4：异阵营 HP 伤害（地形修正 b）----
    case 4: {
        *mo_buf = 0;
        if (same) {
            *hp_buf = 0;
            *mo_buf = 0;
        } else {
            if (battle_calc_hit_chance((int32_t)attacker_idx, (int32_t)occupant_idx) == 0) {
                *hp_buf = 0;
            } else {
                int32_t base = *hp_buf;
                uint32_t local_7c = ((u_var8 * (uint32_t)base) / 100u) *
                                             ((uint32_t)terrain_mod + 100u) / 100u;
                if (F.current_hp <= local_7c) local_7c = F.current_hp;
                F.current_hp = (int32_t)battle_calc_damage((uint32_t)F.current_hp,
                                                           (uint32_t)local_7c, 0u);
                F.attacker_index = (int32_t)attacker_idx;
                *hp_buf = local_7c;
                if (local_7c != 0) {
                    F.unit_state = 3;
                    skill_use_ref((int32_t)attacker_idx, skill_type) += 1;
                    if (attacker_player) {
                        local_2c_cell = (((local_7c << 3) / (int32_t)u_var8) < 5) ? 4
                                                                               : (local_7c << 3) / (int32_t)u_var8;
                    } else {
                        local_2c_cell = (((local_7c << 2) / (int32_t)u_var8) < 3) ? 2
                                                                                 : (local_7c << 2) / (int32_t)u_var8;
                    }
                    local_8 = 1;
                }
            }
        }
        break;
    }
    // ---- case 5：异阵营 HP 伤害（地形修正 b，同 case 4）----
    case 5: {
        *mo_buf = 0;
        if (same) {
            *hp_buf = 0;
            *mo_buf = 0;
        } else {
            if (battle_calc_hit_chance((int32_t)attacker_idx, (int32_t)occupant_idx) == 0) {
                *hp_buf = 0;
            } else {
                int32_t base = *hp_buf;
                uint32_t local_88 = ((u_var8 * (uint32_t)base) / 100u) *
                                              ((uint32_t)terrain_mod + 100u) / 100u;
                if (F.current_hp <= local_88) local_88 = F.current_hp;
                F.current_hp = (int32_t)battle_calc_damage((uint32_t)F.current_hp,
                                                           (uint32_t)local_88, 0u);
                F.attacker_index = (int32_t)attacker_idx;
                *hp_buf = local_88;
                if (local_88 != 0) {
                    F.unit_state = 3;
                    skill_use_ref((int32_t)attacker_idx, skill_type) += 1;
                    if (attacker_player) {
                        local_2c_cell = (((local_88 << 3) / (int32_t)u_var8) < 5) ? 4
                                                                                : (local_88 << 3) / (int32_t)u_var8;
                    } else {
                        local_2c_cell = (((local_88 << 2) / (int32_t)u_var8) < 3) ? 2
                                                                                  : (local_88 << 2) / (int32_t)u_var8;
                    }
                    local_8 = 1;
                }
            }
        }
        break;
    }
    // ---- case 6：异阵营 士气 伤害（无地形）----
    case 6: {
        *hp_buf = 0;
        if (same) {
            *mo_buf = 0;
        } else {
            if (battle_calc_hit_chance((int32_t)attacker_idx, (int32_t)occupant_idx) == 0) {
                *mo_buf = 0;
            } else {
                int32_t base = *mo_buf;
                uint32_t local_94 = ((uint32_t)u_var9 * (uint32_t)base) / 100u;
                if ((uint32_t)F.morale <= local_94) local_94 = F.morale;
                F.morale = (int32_t)battle_calc_damage((uint32_t)F.morale,
                                                       (uint32_t)local_94, 0u);
                F.attacker_index = (int32_t)attacker_idx;
                *mo_buf = local_94;
                if (local_94 != 0) {
                    F.unit_state = 3;
                    skill_use_ref((int32_t)attacker_idx, skill_type) += 1;
                    if (attacker_player) {
                        local_2c_cell = (((local_94 << 3) / (int32_t)u_var9) < 5) ? 4
                                                                                : (local_94 << 3) / (int32_t)u_var9;
                    } else {
                        local_2c_cell = (((local_94 << 2) / (int32_t)u_var9) < 3) ? 2
                                                                                  : (local_94 << 2) / (int32_t)u_var9;
                    }
                    local_8 = 1;
                }
            }
        }
        break;
    }
    // ---- case 7：异阵营 防御修正 削减（×7/10，下限 20）----
    case 7: {
        *hp_buf = 0;
        *mo_buf = 0;
        if (!same && battle_calc_hit_chance((int32_t)attacker_idx, (int32_t)occupant_idx) != 0) {
            int32_t local_a0;
            if ((uint32_t)(F.defense_modifier * 7) / 10u < 0x15) local_a0 = 0x14;
            else local_a0 = (uint32_t)(F.defense_modifier * 7) / 10u;
            F.defense_modifier = local_a0;
            F.attacker_index = (int32_t)attacker_idx;
            F.unit_state = 3;
            skill_use_ref((int32_t)attacker_idx, skill_type) += 1;
            local_2c_cell = attacker_player ? 8 : 2;
            local_8 = 1;
        }
        break;
    }
    // ---- case 8：异阵营 装备完整度 削减（×7/10，下限 50）----
    case 8: {
        *hp_buf = 0;
        *mo_buf = 0;
        if (!same && battle_calc_hit_chance((int32_t)attacker_idx, (int32_t)occupant_idx) != 0) {
            int32_t local_a4;
            if ((uint32_t)(F.equip_integrity * 7) / 10u < 0x33) local_a4 = 0x32;
            else local_a4 = (uint32_t)(F.equip_integrity * 7) / 10u;
            F.equip_integrity = local_a4;
            F.attacker_index = (int32_t)attacker_idx;
            F.unit_state = 3;
            skill_use_ref((int32_t)attacker_idx, skill_type) += 1;
            local_2c_cell = attacker_player ? 8 : 2;
            local_8 = 1;
        }
        break;
    }
    // ---- case 9：异阵营 消耗物品→仓库（武器命中）----
    case 9: {
        *hp_buf = 0;
        *mo_buf = 0;
        if (!same && battle_calc_hit_chance_weapon((int32_t)attacker_idx,
                                                   (int32_t)occupant_idx) != 0) {
            battle_calc_take_item_to_depot(attacker_idx, occupant_idx);   // FUN_0042b23a 全链
            F.attacker_index = (int32_t)attacker_idx;
            skill_use_ref((int32_t)attacker_idx, skill_type) += 1;
            local_2c_cell = attacker_player ? 8 : 2;
            // 注意：二进制 case 9 不设置 local_8（保留先前值），此处不修改 local_8。
        }
        break;
    }
    // ---- case 10 (0xa)：异阵营 战斗记录状态 -1（下限 1）+ HP 夹取至 max_hp ----
    case 10: {
        *hp_buf = 0;
        *mo_buf = 0;
        if (!same && battle_calc_hit_chance((int32_t)attacker_idx, (int32_t)occupant_idx) != 0) {
            int32_t local_a8;
            if (g_battlerec_0x7d0f18[occupant_idx][1] == 1) local_a8 = 1;          // +4
            else local_a8 = g_battlerec_0x7d0f18[occupant_idx][1] + (-1);
            g_battlerec_0x7d0f18[occupant_idx][1] = local_a8;
            int32_t maxhp = battle_calc_max_hp((int32_t)occupant_idx);
            int32_t local_ac = (maxhp < F.current_hp) ? maxhp : F.current_hp;
            F.current_hp = local_ac;
            F.attacker_index = (int32_t)attacker_idx;
            skill_use_ref((int32_t)attacker_idx, skill_type) += 1;
            local_2c_cell = attacker_player ? 8 : 2;
            // 注意：二进制 case 10 不设置 local_8（保留先前值）。
        }
        break;
    }
    // ---- case 11 (0xb)：异阵营 清 unit_state=0 ----
    case 11: {
        *hp_buf = 0;
        *mo_buf = 0;
        if (!same && battle_calc_hit_chance((int32_t)attacker_idx, (int32_t)occupant_idx) != 0) {
            F.unit_state = 0;
            F.attacker_index = (int32_t)attacker_idx;
            skill_use_ref((int32_t)attacker_idx, skill_type) += 1;
            local_2c_cell = attacker_player ? 8 : 2;
            local_8 = 1;
        }
        break;
    }
    // ---- case 12 (0xc)：同阵营 buff（equip +0x14 cap100，defense +0x14 cap100，unit_state=1）----
    case 12: {
        *hp_buf = 0;
        *mo_buf = 0;
        if (same) {
            if (battle_calc_hit_chance((int32_t)attacker_idx, (int32_t)occupant_idx) != 0 &&
                g_battlerec_0x7d0f18[occupant_idx][3] != 0x2d) {                    // +0xC
                F.unit_state = 1;
                int32_t local_b0;
                if ((100u - (uint32_t)F.equip_integrity) < 0x15u) local_b0 = (int32_t)(100u - (uint32_t)F.equip_integrity);
                else local_b0 = 0x14;
                F.equip_integrity = F.equip_integrity + local_b0;                   // cap 100
                int32_t local_b4;
                if ((100u - (uint32_t)F.defense_modifier) < 0x15u) local_b4 = (int32_t)(100u - (uint32_t)F.defense_modifier);
                else local_b4 = 0x14;
                F.defense_modifier = F.defense_modifier + local_b4;                  // cap 100
                F.attacker_index = (int32_t)attacker_idx;
                skill_use_ref((int32_t)attacker_idx, skill_type) += 1;
                local_2c_cell = attacker_player ? 8 : 2;
                local_8 = 1;
            }
        } else {
            *hp_buf = 0;
        }
        break;
    }
    // ---- case 13 (0xd)：异阵营 HP 伤害（无地形；贡献固定 8/2）----
    case 13: {
        *mo_buf = 0;
        if (same) {
            *hp_buf = 0;
        } else {
            if (battle_calc_hit_chance((int32_t)attacker_idx, (int32_t)occupant_idx) == 0) {
                *hp_buf = 0;
            } else {
                int32_t base = *hp_buf;
                int32_t local_b8 = (int32_t)((u_var8 * (uint32_t)base) / 100u);     // 无地形
                if ((uint32_t)F.current_hp <= (uint32_t)local_b8) local_b8 = F.current_hp;
                F.current_hp = (int32_t)battle_calc_damage((uint32_t)F.current_hp,
                                                           (uint32_t)local_b8, 0u);
                F.attacker_index = (int32_t)attacker_idx;
                *hp_buf = local_b8;
                if (local_b8 != 0) {
                    F.unit_state = 3;
                    skill_use_ref((int32_t)attacker_idx, skill_type) += 1;
                    local_2c_cell = attacker_player ? 8 : 2;
                    local_8 = 1;
                }
            }
        }
        break;
    }
    // ---- case 14 (0xe)：异阵营 HP + 士气 双伤害（无地形；贡献固定 8/2）----
    case 14: {
        if (same) {
            *hp_buf = 0;
            *mo_buf = 0;
        } else {
            if (battle_calc_hit_chance((int32_t)attacker_idx, (int32_t)occupant_idx) == 0) {
                *hp_buf = 0;
                *mo_buf = 0;
            } else {
                int32_t local_bc = (int32_t)((u_var8 * (uint32_t)(*hp_buf)) / 100u); // HP，无地形
                if ((uint32_t)F.current_hp <= (uint32_t)local_bc) local_bc = F.current_hp;
                F.current_hp = (int32_t)battle_calc_damage((uint32_t)F.current_hp,
                                                           (uint32_t)local_bc, 0u);
                *hp_buf = local_bc;
                int32_t local_c0 = (int32_t)((u_var9 * (uint32_t)(*mo_buf)) / 100u); // 士气，无地形
                if ((uint32_t)F.morale <= (uint32_t)local_c0) local_c0 = F.morale;
                F.morale = (int32_t)battle_calc_damage((uint32_t)F.morale,
                                                        (uint32_t)local_c0, 0u);
                *mo_buf = local_c0;
                if (*hp_buf != 0 || *mo_buf != 0) {
                    F.unit_state = 3;
                    F.attacker_index = (int32_t)attacker_idx;
                    skill_use_ref((int32_t)attacker_idx, skill_type) += 1;
                    local_2c_cell = attacker_player ? 8 : 2;
                    local_8 = 1;
                }
            }
        }
        break;
    }
    // ---- case 15 (0xf)：异阵营 HP 伤害（无地形；贡献固定 8/2）----
    case 15: {
        *mo_buf = 0;
        if (same) {
            *hp_buf = 0;
        } else {
            uint32_t local_c4 = ((uint32_t)u_var8 * (uint32_t)(*hp_buf)) / 100u;     // 无地形
            if ((uint32_t)F.current_hp <= (uint32_t)local_c4) local_c4 = F.current_hp;
            F.current_hp = (int32_t)battle_calc_damage((uint32_t)F.current_hp,
                                                       (uint32_t)local_c4, 0u);
            F.attacker_index = (int32_t)attacker_idx;
            *hp_buf = local_c4;
            if (local_c4 != 0) {
                F.unit_state = 3;
                skill_use_ref((int32_t)attacker_idx, skill_type) += 1;
                if (attacker_player) {
                    local_2c_cell = (((local_c4 << 3) / (int32_t)u_var8) < 5) ? 4
                                                                             : (local_c4 << 3) / (int32_t)u_var8;
                } else {
                    local_2c_cell = (((local_c4 << 2) / (int32_t)u_var8) < 3) ? 2
                                                                              : (local_c4 << 2) / (int32_t)u_var8;
                }
                local_8 = 1;
            }
        }
        break;
    }
    // ---- default：清 HP 缓冲，local_8 = 1 ----
    default: {
        *hp_buf = 0;
        local_8 = 1;
        break;
    }
    }

    if (out_anim_dir) *out_anim_dir = local_8;
    return local_2c_cell;
}

// ============================================================
// 整网格结算（FUN_004138ea.c:62-715）
// ============================================================
uint32_t battle_calc_settle_skill_grid(uint32_t attacker_idx, uint32_t skill_type,
                                       uint32_t current_unit) {
    uint32_t u8 = 0, u9 = 0;
    battle_calc_skill_power(attacker_idx, skill_type, &u8, &u9);

    // 网格尺寸（line 72-83）
    if (g_battle_double_grid == 0) { g_grid_rows = 9; g_grid_cols = 9; }
    else                           { g_grid_rows = 10; g_grid_cols = 13; }

    uint32_t local_2c = 0;
    uint32_t local_8 = 0;   // 渲染方向（仅 Tier B，确定性；保留跨格持久语义）

    for (uint32_t r = 0; r < (uint32_t)g_grid_rows; ++r) {
        for (uint32_t c = 0; c < (uint32_t)g_grid_cols; ++c) {
            uint32_t tile = (uint32_t)((g_grid_origin_row + (int32_t)r) * g_map_width_0x57ba20 +
                                       (g_grid_origin_col + (int32_t)c));
            uint32_t cell = r * (uint32_t)g_grid_cols + c;
            if (g_unit_presence_0x57bf10[tile * 2] == 0 || g_hp_dmg_buf_0x7d0488[cell] == 0) {
                g_hp_dmg_buf_0x7d0488[cell] = 0;
                g_morale_dmg_buf_0x4565d0[cell] = 0;
            } else {
                uint32_t occ = g_unit_index_0x57bf11[tile * 2];
                // 解析地形修正（按 case）
                uint32_t terrain_mod = 0;
                uint16_t t_idx = battle_calc_equip_defense((int32_t)(g_grid_origin_col + (int32_t)c),
                                                           (int32_t)(g_grid_origin_row + (int32_t)r));
                if (skill_type == 3 || skill_type == 13 || skill_type == 15)
                    terrain_mod = (uint32_t)g_terrain_mod_a_0x475aa8[t_idx];
                else if (skill_type == 4 || skill_type == 5)
                    terrain_mod = (uint32_t)g_terrain_mod_b_0x475aac[t_idx];
                local_2c += battle_calc_apply_cell_effect(attacker_idx, skill_type, occ, cell,
                                                          terrain_mod, u8, u9, &local_8);
            }
        }
    }

    // line 708：清当前单位 unit_state
    g_game_state.fighter_array[current_unit].unit_state = 0;

    // line 709-715：local_2c 封顶 0x10
    if (local_2c >= 0x10) local_2c = 0x10;
    return local_2c;
}

// ============================================================
// T3 — 基础伤害写入 pass（FUN_0041307f.c:49-121 逐指令转录）
// 铁律零逐指令证据：output/ghidra_all/FUN_0041307f.c（lines 49-121）。
// 本函数只覆盖确定性 per-cell 伤害核心（无 RNG）；死亡/经验循环（lines 146-189）与
// FUN_00429e92(attacker, local_24) 经验应用属 T3-b，不在此处。
// ============================================================
uint32_t battle_calc_apply_cell_damage(uint32_t attacker_idx) {
    uint32_t local_24 = 0;
    // line 35: uVar2 = FUN_0042a82a(attacker)（攻击力）
    uint32_t attack = (uint32_t)battle_calc_attack_power((int32_t)attacker_idx);
    // lines 36-47: 网格尺寸（DAT_007d06b8==0 → 9×9，否则 10×13）
    int rows, cols;
    if (g_battle_double_grid == 0) { rows = 9;  cols = 9;  }
    else                          { rows = 10; cols = 13; }

    // lines 49-114: 双层循环（确定性 per-cell 伤害写入）
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            int tile = (g_grid_origin_row + r) * g_map_width_0x57ba20 + (g_grid_origin_col + c);
            int cell = r * cols + c;
            // lines 52-56: 无占驻 或 无基础伤害 → 清零两缓冲
            if (g_hp_dmg_buf_0x7d0488[cell] == 0 || g_unit_presence_0x57bf10[tile * 2] == 0) {
                g_hp_dmg_buf_0x7d0488[cell] = 0;
                g_morale_dmg_buf_0x4565d0[cell] = 0;
            }
            // lines 57-61: 同阵营（enemy 相等）→ 清零两缓冲
            else if (g_game_state.people_array[(uint32_t)g_unit_index_0x57bf11[tile * 2]].enemy ==
                     g_game_state.people_array[attacker_idx].enemy) {
                g_hp_dmg_buf_0x7d0488[cell] = 0;
                g_morale_dmg_buf_0x4565d0[cell] = 0;
            }
            else {
                uint32_t occ = (uint32_t)g_unit_index_0x57bf11[tile * 2];   // 占驻单位索引
                // line 63: defense = FUN_0042a98e(occ)
                uint32_t defense = (uint32_t)battle_calc_defense((int32_t)occ);
                // line 64: base_dmg = FUN_0042784b(attack, defense, 0) = max(0, attack-defense)
                uint32_t base_dmg = battle_calc_damage(attack, defense, 0);
                // line 65-66: local_44 = (base_dmg * hp_buf[cell]) / 100
                uint32_t local_44 = (base_dmg * (uint32_t)g_hp_dmg_buf_0x7d0488[cell]) / 100u;
                // lines 67-69: if local_44 <= Fighter[occ].current_hp/0x14 → 取后者
                uint32_t hp_cur = (uint32_t)g_game_state.fighter_array[occ].current_hp;
                uint32_t hp_div20 = (uint32_t)((int32_t)g_game_state.fighter_array[occ].current_hp / 0x14);
                if (local_44 <= hp_div20) local_44 = hp_div20;
                // lines 70-75: local_48 = (local_44<2)?1:local_44; local_4c = min(local_48, current_hp)
                uint32_t local_48 = (local_44 < 2) ? 1 : local_44;
                uint32_t local_4c = (local_48 < hp_cur) ? local_48 : hp_cur;
                // line 85: Fighter[occ].current_hp = FUN_0042784b(current_hp, local_4c, 0)（扣血）
                g_game_state.fighter_array[occ].current_hp =
                    (int32_t)battle_calc_damage(hp_cur, local_4c, 0);
                // line 87: Fighter[occ].attacker_index = attacker
                g_game_state.fighter_array[occ].attacker_index = (int32_t)attacker_idx;
                // line 88: hp_buf[cell] = local_4c
                g_hp_dmg_buf_0x7d0488[cell] = (int32_t)local_4c;
                // lines 89-96: 士气伤害 = max(1, (local_4c*0x14)/max_hp)
                uint32_t maxhp = (uint32_t)battle_calc_max_hp((int32_t)occ);
                uint32_t local_50 = ((local_4c * 0x14) / maxhp < 2) ? 1 : (local_4c * 0x14) / maxhp;
                g_morale_dmg_buf_0x4565d0[cell] = (int32_t)local_50;
                // lines 98-101: Fighter[occ].morale = FUN_0042784b(morale, local_50, 0)（扣士气）
                g_game_state.fighter_array[occ].morale =
                    (int32_t)battle_calc_damage((uint32_t)g_game_state.fighter_array[occ].morale,
                                                local_50, 0);
                // line 102: Fighter[occ].unit_state = 3（DAT_0096abcf[occ*0x40]）
                g_game_state.fighter_array[occ].unit_state = 3;
                // line 103: People[attacker].skill_gongji++
                g_game_state.people_array[attacker_idx].skill_gongji += 1;
                // lines 104-111: 攻击经验（敌方单位 >>2）
                uint32_t exp = (uint32_t)battle_calc_attack_exp((int32_t)attacker_idx, (int32_t)occ);
                if (g_game_state.people_array[attacker_idx].enemy != 0) exp >>= 2;
                local_24 += exp;
            }
        }
    }
    // lines 115-121: local_24 封顶 0x30(48)
    if (local_24 >= 0x30) local_24 = 0x30;
    return local_24;   // FUN_00429e92(attacker, local_24) 应用交 T3-b
}

// ============================================================
// §9.3② — 三 pass 管线运行时入口（在 g_game_state 上施加一次技能效果）
// 铁律零：SETUP 用 battle_calc_skill_shape_at（FUN_00427a71 旋转核，已逐指令核对）播种真实形状；
//         T3 / 结算 直接复用已验证 oracle，不另写分流逻辑。
// ============================================================
int32_t battle_calc_run_skill_pipeline(uint32_t attacker_idx, uint32_t skill_type,
                                      uint32_t shape_index, int anchor_r, int anchor_c,
                                      int facing, int cols, int rows, int mw, int origin,
                                      const uint8_t* skill_shape_table) {
    // 真实数据接线：调用方未传表时回退到已安装的 replica 全局（=二进制 DAT_007cdd00），
    // 使 battle_calc_install_skill_shape_table 装载的形状表在运行时可达（residual #1）。
    const uint8_t* table = skill_shape_table ? skill_shape_table : g_battlecalc_skill_shape_raw;
    if (skill_shape_table == nullptr) {
        bool installed = false;
        for (size_t i = 0; i < sizeof(g_battlecalc_skill_shape_raw); ++i)
            if (g_battlecalc_skill_shape_raw[i] != 0) { installed = true; break; }
        if (!installed) return -1;   // 全局未安装真实数据时不静默读全零形状表
    }

    // ---- SETUP PASS：真实技能形状表播种跨 pass 缓冲（FUN_00412030 等价）----
    uint32_t shape[49];
    if (battle_calc_skill_shape_at(table, (int32_t)shape_index, facing, shape) != 0)
        return -1;
    for (int sr = 0; sr < 7; sr++) {
        for (int sc = 0; sc < 7; sc++) {
            int bx = anchor_c - 3 + sc;
            int by = anchor_r - 3 + sr;
            if (bx >= 0 && bx < cols && by >= 0 && by < rows) {
                int cell = by * cols + bx;
                int tile = (origin + by) * mw + (origin + bx);
                uint32_t v = shape[sr * 7 + sc];
                g_hp_dmg_buf_0x7d0488[cell] = (int32_t)v;
                g_morale_dmg_buf_0x4565d0[cell] = (int32_t)v;   // FUN_0043de50：HP 种子拷入士气缓冲
                if (v != 0) g_unit_presence_0x57bf10[tile * 2] = 1;
            }
        }
    }

    // ---- T3（FUN_0041307f）→ 结算（FUN_004138ea）----
    battle_calc_apply_cell_damage(attacker_idx);
    return (int32_t)battle_calc_settle_skill_grid(attacker_idx, skill_type, attacker_idx);
}

// ============================================================
// T3-b — 死亡/经验循环（FUN_0041307f.c:146-195 逐指令转录）
// 铁律零证据：output/ghidra_all/FUN_0041307f.c（lines 146-195）。
// 注：g_player_units_remaining(DAT_007d0f1c) 真实语义（剩余单位倒数 vs 主将 level）待 RE 深核，
// 此处作通用 32 位计数器，与 clean 层一致；跨层闸门仅证 oracle≡clean。
// ============================================================
uint32_t battle_calc_apply_death_experience(uint32_t attacker_idx, uint32_t attack_exp_subtotal) {
    uint32_t local_24 = attack_exp_subtotal;

    // 与二进制一致：死亡循环用 DAT_007d06b8(=g_battle_double_grid) 派生的网格尺寸（非 g_grid_rows，
    // 后者在跨层桥接中会被 resize_grid_buffers 的占位值污染）。
    int rows, cols;
    if (g_battle_double_grid == 0) { rows = 9;  cols = 9;  }
    else                          { rows = 10; cols = 13; }
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            int tile = (g_grid_origin_row + r) * g_map_width_0x57ba20 + (g_grid_origin_col + c);
            int cell = r * cols + c;
            // line 148: hp_buf[cell] != 0
            if (g_hp_dmg_buf_0x7d0488[cell] != 0) {
                uint32_t occ = (uint32_t)g_unit_index_0x57bf11[tile * 2];
                // lines 148-152: 占驻单位 hp==0 || 士气==0 → 阵亡
                if (g_game_state.fighter_array[occ].current_hp == 0 ||
                    g_game_state.fighter_array[occ].morale == 0) {
                    // line 153: Fighter[occ].active_flag = 0
                    g_game_state.fighter_array[occ].active_flag = 0;
                    // line 154: 击杀经验 FUN_0042ab23(阵亡者=occ, 攻击者)
                    uint32_t kexp = (uint32_t)battle_calc_kill_exp((int32_t)occ, (int32_t)attacker_idx);
                    local_24 += kexp;
                    // line 156: FUN_0042afd2(occ) 物品回收（已实现，写 g_item_recovery）
                    count_unit_skills((int32_t)occ);
                    // lines 157-181: 阵亡计数 + 主将 HP 封顶 / 剩余单位倒数
                    if (g_game_state.people_array[occ].enemy == 0) {
                        g_game_state.g_aux_counter_b += 1;             // DAT_008331f8++
                        if (((occ != 0) && (occ < 0x20)) &&
                            (g_game_state.g_flow_phase == 2 ||
                             ((g_game_state.g_flow_phase == 1) &&
                              (g_game_state.g_aux_counter_b < 3)))) {
                            int32_t prev = g_player_units_remaining;
                            g_player_units_remaining = (prev == 1) ? 1 : (prev - 1);
                            // line 168-175: 主将(Fighter[0])HP 封顶 max_hp(0)
                            uint32_t maxhp0 = (uint32_t)battle_calc_max_hp(0);
                            int32_t hp0 = g_game_state.fighter_array[0].current_hp;
                            g_game_state.fighter_array[0].current_hp =
                                (hp0 < (int32_t)maxhp0) ? hp0 : (int32_t)maxhp0;
                        }
                    } else {
                        g_game_state.g_aux_counter_a += 1;             // DAT_008331f0++
                    }
                    // line 182: FUN_0041e4e0 渲染（Tier-A 跳过）
                }
            }
        }
    }
    // line 195: FUN_00429e92(攻击者, local_24) 应用总经验
    battle_calc_gain_exp((int32_t)attacker_idx, (int32_t)local_24);
    return local_24;
}
