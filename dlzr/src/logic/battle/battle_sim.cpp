// battle_sim.cpp — clean 层：战斗区域伤害/技能效果结算（replica 镜像，语义重写）
//
// 铁律零：可观测结果必须与 replica::battle_calc_* 逐位一致（tests/test_skill_grid_diff.cpp 跨层闸门）。
// 语义来源：output/re_map/RE_FUN_004138ea.md §4（16 分支效果表）。本文件从语义独立重写，
// 命名清晰（heal_buffer / morale_buffer / battlerec / skill_use / 特殊计分），命中抽签共用 rand_impl。
//
// 16 分支效果语义（RE doc §4）：
//   0  同阵营 HP 修复（heal）            7  异阵营 防御修正 削减×7/10 下限20
//   1  同阵营 士气 提升                  8  异阵营 装备完整度 削减×7/10 下限50
//   2  同阵营 HP+士气 双提升             9  异阵营 消耗物品→仓库（武器命中）
//   3  异阵营 HP 伤害（地形a）           10 异阵营 战斗记录状态-1（下限1）+HP夹max_hp
//   4  异阵营 HP 伤害（地形b）           11 异阵营 清 unit_state=0
//   5  异阵营 HP 伤害（地形b）           12 同阵营 buff（equip/defense +0x14 cap100, unit_state=1）
//   6  异阵营 士气 伤害（无地形）        13 异阵营 HP 伤害（无地形；贡献固定8/2）
//                                        14 异阵营 HP+士气 双伤害（无地形；贡献固定8/2）
//                                        15 异阵营 HP 伤害（无地形；贡献按 ×8/×4 公式）

#include "logic/battle/battle_sim.hpp"

#include <cstdint>

#include "logic/combat/combat_formulas.hpp"   // clamp_stat_delta / calc_damage / get_skill_level
#include "replica/battle_calc.hpp"            // battle_calc_equip_defense（地形类型，与 oracle 同源）
#include "replica/replica_stubs.h"            // uint32_t rand_impl()（二进制共用 LCG）

namespace dlzr::logic {

// 英雄对决计分（与 oracle 同一组硬编码索引对）。
static void apply_special_scores(BattleGridState& w, uint32_t a, uint32_t o) {
    if (a == 0 && o == 7) w.special_score_a += 2;
    if (a == 7 && o == 0) w.special_score_a += 1;
    if (a == 8 && o == 7) w.special_score_b += 2;
    if (a == 7 && o == 8) w.special_score_b += 1;
}

// case 9 子调用（镜像 battle_calc_take_item_to_depot / FUN_0042b23a 全链）。
// 从 donor 取首个非空物品→尝试装入 recipient 空槽（满则收归 depot）→ 清 donor 槽。
// item_idx = item_id-1；item_idx<0x9c 且 recipient 有空槽→装入并压实→ret 0；无空槽→ret 1（仓库）；
// item_idx>=0x9c→ret 2（丢弃）。与 oracle 逐位一致。
static void take_item_to_depot(BattleGridState& w, uint32_t recipient, uint32_t donor) {
    auto& d = w.units[donor].items;
    int first = -1;
    for (int i = 0; i < 8; ++i) {
        if (d[i] != 0) { first = i; break; }
    }
    if (first < 0) return;
    int item_idx = (int)d[first] - 1;

    auto& r = w.units[recipient].items;
    int ret;
    if (item_idx < 0x9c) {
        int free_slot = -1;
        for (int i = 0; i < 8; ++i) {
            if (r[i] == 0) { free_slot = i; break; }
        }
        if (free_slot >= 0) {
            r[free_slot] = (uint8_t)(item_idx + 1);
            uint8_t tmp[8] = {0};
            int n = 0;
            for (int i = 0; i < 8; ++i) {
                if (r[i] != 0) tmp[n++] = r[i];
            }
            for (int i = 0; i < 8; ++i) r[i] = tmp[i];
            ret = 0;
        } else {
            ret = 1;
        }
    } else {
        ret = 2;
    }
    if (ret == 1) w.depot[item_idx] += 1;
    d[first] = 0;
}

// 基础命中判定（与 replica::battle_calc_hit_chance 同公式；共用 rand_impl 保证抽同值）。
static int32_t clean_hit(const BattleGridState::Unit& atk, const BattleGridState::Unit& def) {
    int32_t rate = (atk.intelligence - def.intelligence) * 2 + 0x1e;  // 30
    if (rate < 0) rate = 0;
    uint32_t roll = rand_impl() % 100;
    return (roll < (uint32_t)rate) ? 1 : 0;
}

// 武器命中判定（与 replica::battle_calc_hit_chance_weapon 同公式；skill_lv = skill_use[9]=lueduo）。
static int32_t clean_hit_weapon(const BattleGridState::Unit& atk, const BattleGridState::Unit& def,
                                int32_t skill_lv) {
    int32_t rate = (atk.intelligence - def.intelligence) * 2 + 0x1e;
    if (rate < 0) rate = 0;
    rate += skill_lv;
    uint32_t roll = rand_impl() % 100;
    return (roll < (uint32_t)rate) ? 1 : 0;
}

static int32_t max_hp_of(const BattleGridState& w, const BattleGridState::Unit& u) {
    int32_t cls = u.class_type;
    if (cls < 0 || cls >= 96) cls = 0;
    return w.class_hp[cls][0] + (u.level - 1) * w.class_hp[cls][1];
}

// ============================================================
// 预计算技能威力标量
// ============================================================
void compute_skill_power(const BattleGridState& w, uint32_t attacker, uint32_t skill_type,
                         uint32_t* u8, uint32_t* u9) {
    uint32_t attr = (uint32_t)w.units[attacker].battlerec[0];     // +0 属性/智力类
    uint32_t use  = (uint32_t)w.units[attacker].skill_use[skill_type];
    uint32_t lvl  = (uint32_t)get_skill_level((int32_t)use);
    uint32_t p1   = (uint32_t)w.skill_param1[skill_type];
    uint32_t p2   = (uint32_t)w.skill_param2[skill_type];
    *u8 = (((p1 * attr) / 100u) * (lvl + 7)) >> 3u;
    *u9 = (((p2 * attr) / 100u) * (lvl + 7)) >> 3u;
}

// ============================================================
// 单格效果施加
// ============================================================
uint32_t apply_cell_effect(BattleGridState& w, uint32_t attacker, uint32_t skill_type,
                           uint32_t occupant, uint32_t cell, uint32_t terrain_mod,
                           uint32_t u8, uint32_t u9, uint32_t* anim_dir) {
    uint32_t local_2c_cell = 0;
    uint32_t local_8 = (anim_dir ? *anim_dir : 0);
    bool attacker_player = (w.units[attacker].enemy == 0);
    auto& F = w.units[occupant];
    int32_t* hp_buf = &w.heal_buffer[cell];
    int32_t* mo_buf = &w.morale_buffer[cell];
    bool same = (w.units[attacker].enemy == w.units[occupant].enemy);

    switch (skill_type) {
    case 0: {  // 同阵营 HP 修复
        *mo_buf = 0;
        if (same) {
            uint32_t local_50 = ((uint32_t)u8 * (uint32_t)(*hp_buf)) / 100u;
            int32_t maxhp = max_hp_of(w, F);
            if ((uint32_t)(maxhp - F.current_hp) <= local_50) local_50 = (uint32_t)(maxhp - F.current_hp);
            F.current_hp = clamp_stat_delta(F.current_hp, (int32_t)local_50, (int32_t)maxhp);
            *hp_buf = local_50;
            if (local_50 != 0) {
                w.units[attacker].skill_use[skill_type] += 1;
                if (attacker_player) {
                    local_2c_cell = (((local_50 * 0xc) / (int32_t)u8) < 5) ? 4
                                                                         : (local_50 * 0xc) / (int32_t)u8;
                } else {
                    local_2c_cell = (((local_50 << 2) / (int32_t)u8) < 3) ? 2
                                                                          : (local_50 << 2) / (int32_t)u8;
                }
                local_8 = 0;
                apply_special_scores(w, attacker, occupant);
            }
        } else {
            *hp_buf = 0;
        }
        break;
    }
    case 1: {  // 同阵营 士气 提升
        *hp_buf = 0;
        if (same) {
            uint32_t local_5c = ((uint32_t)u9 * (uint32_t)(*mo_buf)) / 100u;
            if (100 - F.morale <= local_5c) local_5c = 100 - F.morale;
            F.morale = clamp_stat_delta(F.morale, (int32_t)local_5c, 100);
            *mo_buf = (int32_t)local_5c;
            if (local_5c != 0) {
                w.units[attacker].skill_use[skill_type] += 1;
                if (attacker_player) {
                    local_2c_cell = (((local_5c * 0xc) / u9) < 5u) ? 4u
                                                                     : (local_5c * 0xc) / u9;
                } else {
                    local_2c_cell = (((local_5c << 2) / u9) < 2u) ? 1u
                                                                   : (local_5c << 2) / u9;
                }
                local_8 = 0;
                apply_special_scores(w, attacker, occupant);
            }
        } else {
            *mo_buf = 0;
        }
        break;
    }
    case 2: {  // 同阵营 HP + 士气 双提升
        if (same) {
            uint32_t local_68 = ((uint32_t)u9 * (uint32_t)(*mo_buf)) / 100u;
            if (100 - F.morale <= local_68) local_68 = 100 - F.morale;
            *mo_buf = (int32_t)local_68;
            F.morale = clamp_stat_delta(F.morale, (int32_t)local_68, 100);
            uint32_t local_6c = ((uint32_t)u8 * (uint32_t)(*hp_buf)) / 100u;
            int32_t maxhp = max_hp_of(w, F);
            if ((uint32_t)(maxhp - F.current_hp) <= local_6c) local_6c = (uint32_t)(maxhp - F.current_hp);
            F.current_hp = clamp_stat_delta(F.current_hp, (int32_t)local_6c, (int32_t)maxhp);
            *hp_buf = local_6c;
            if (*hp_buf != 0 || *mo_buf != 0) {
                w.units[attacker].skill_use[skill_type] += 1;
                local_2c_cell = attacker_player ? 8 : 2;
                local_8 = 0;
                apply_special_scores(w, attacker, occupant);
            }
        } else {
            *hp_buf = 0;
            *mo_buf = 0;
        }
        break;
    }
    case 3: {  // 异阵营 HP 伤害（地形 a）
        *mo_buf = 0;
        if (same) {
            *hp_buf = 0;
        } else {
            if (clean_hit(w.units[attacker], w.units[occupant]) == 0) {
                *hp_buf = 0;
            } else {
                int32_t base = *hp_buf;
                uint32_t local_70 = ((u8 * (uint32_t)base) / 100u) *
                                             ((uint32_t)terrain_mod + 100u) / 100u;
                if (F.current_hp <= local_70) local_70 = F.current_hp;
                F.current_hp = calc_damage(F.current_hp, local_70, 0);
                F.attacker_index = (int32_t)attacker;
                *hp_buf = local_70;
                if (local_70 != 0) {
                    F.unit_state = 3;
                    w.units[attacker].skill_use[skill_type] += 1;
                    if (attacker_player) {
                        local_2c_cell = (((local_70 << 3) / (int32_t)u8) < 5) ? 4
                                                                             : (local_70 << 3) / (int32_t)u8;
                    } else {
                        local_2c_cell = (((local_70 << 2) / (int32_t)u8) < 3) ? 2
                                                                              : (local_70 << 2) / (int32_t)u8;
                    }
                    local_8 = 1;
                }
            }
        }
        break;
    }
    case 4: {  // 异阵营 HP 伤害（地形 b）
        *mo_buf = 0;
        if (same) {
            *hp_buf = 0;
            *mo_buf = 0;
        } else {
            if (clean_hit(w.units[attacker], w.units[occupant]) == 0) {
                *hp_buf = 0;
            } else {
                int32_t base = *hp_buf;
                uint32_t local_7c = ((u8 * (uint32_t)base) / 100u) *
                                             ((uint32_t)terrain_mod + 100u) / 100u;
                if (F.current_hp <= local_7c) local_7c = F.current_hp;
                F.current_hp = calc_damage(F.current_hp, local_7c, 0);
                F.attacker_index = (int32_t)attacker;
                *hp_buf = local_7c;
                if (local_7c != 0) {
                    F.unit_state = 3;
                    w.units[attacker].skill_use[skill_type] += 1;
                    if (attacker_player) {
                        local_2c_cell = (((local_7c << 3) / (int32_t)u8) < 5) ? 4
                                                                             : (local_7c << 3) / (int32_t)u8;
                    } else {
                        local_2c_cell = (((local_7c << 2) / (int32_t)u8) < 3) ? 2
                                                                              : (local_7c << 2) / (int32_t)u8;
                    }
                    local_8 = 1;
                }
            }
        }
        break;
    }
    case 5: {  // 异阵营 HP 伤害（地形 b，同 case 4）
        *mo_buf = 0;
        if (same) {
            *hp_buf = 0;
            *mo_buf = 0;
        } else {
            if (clean_hit(w.units[attacker], w.units[occupant]) == 0) {
                *hp_buf = 0;
            } else {
                int32_t base = *hp_buf;
                uint32_t local_88 = ((u8 * (uint32_t)base) / 100u) *
                                              ((uint32_t)terrain_mod + 100u) / 100u;
                if (F.current_hp <= local_88) local_88 = F.current_hp;
                F.current_hp = calc_damage(F.current_hp, local_88, 0);
                F.attacker_index = (int32_t)attacker;
                *hp_buf = local_88;
                if (local_88 != 0) {
                    F.unit_state = 3;
                    w.units[attacker].skill_use[skill_type] += 1;
                    if (attacker_player) {
                        local_2c_cell = (((local_88 << 3) / (int32_t)u8) < 5) ? 4
                                                                              : (local_88 << 3) / (int32_t)u8;
                    } else {
                        local_2c_cell = (((local_88 << 2) / (int32_t)u8) < 3) ? 2
                                                                               : (local_88 << 2) / (int32_t)u8;
                    }
                    local_8 = 1;
                }
            }
        }
        break;
    }
    case 6: {  // 异阵营 士气 伤害（无地形）
        *hp_buf = 0;
        if (same) {
            *mo_buf = 0;
        } else {
            if (clean_hit(w.units[attacker], w.units[occupant]) == 0) {
                *mo_buf = 0;
            } else {
                int32_t base = *mo_buf;
                uint32_t local_94 = ((uint32_t)u9 * (uint32_t)base) / 100u;
                if ((uint32_t)F.morale <= local_94) local_94 = F.morale;
                F.morale = calc_damage(F.morale, local_94, 0);
                F.attacker_index = (int32_t)attacker;
                *mo_buf = local_94;
                if (local_94 != 0) {
                    F.unit_state = 3;
                    w.units[attacker].skill_use[skill_type] += 1;
                    if (attacker_player) {
                        local_2c_cell = (((local_94 << 3) / (int32_t)u9) < 5) ? 4
                                                                              : (local_94 << 3) / (int32_t)u9;
                    } else {
                        local_2c_cell = (((local_94 << 2) / (int32_t)u9) < 3) ? 2
                                                                               : (local_94 << 2) / (int32_t)u9;
                    }
                    local_8 = 1;
                }
            }
        }
        break;
    }
    case 7: {  // 异阵营 防御修正 削减（×7/10，下限 20）
        *hp_buf = 0;
        *mo_buf = 0;
        if (!same && clean_hit(w.units[attacker], w.units[occupant]) != 0) {
            int32_t local_a0;
            if ((uint32_t)(F.defense_modifier * 7) / 10u < 0x15) local_a0 = 0x14;
            else local_a0 = (uint32_t)(F.defense_modifier * 7) / 10u;
            F.defense_modifier = local_a0;
            F.attacker_index = (int32_t)attacker;
            F.unit_state = 3;
            w.units[attacker].skill_use[skill_type] += 1;
            local_2c_cell = attacker_player ? 8 : 2;
            local_8 = 1;
        }
        break;
    }
    case 8: {  // 异阵营 装备完整度 削减（×7/10，下限 50）
        *hp_buf = 0;
        *mo_buf = 0;
        if (!same && clean_hit(w.units[attacker], w.units[occupant]) != 0) {
            int32_t local_a4;
            if ((uint32_t)(F.equip_integrity * 7) / 10u < 0x33) local_a4 = 0x32;
            else local_a4 = (uint32_t)(F.equip_integrity * 7) / 10u;
            F.equip_integrity = local_a4;
            F.attacker_index = (int32_t)attacker;
            F.unit_state = 3;
            w.units[attacker].skill_use[skill_type] += 1;
            local_2c_cell = attacker_player ? 8 : 2;
            local_8 = 1;
        }
        break;
    }
    case 9: {  // 异阵营 消耗物品→仓库（武器命中）
        *hp_buf = 0;
        *mo_buf = 0;
        int32_t skill_lv = get_skill_level(w.units[attacker].skill_use[9]);  // skill_lueduo = mode 9
        if (!same && clean_hit_weapon(w.units[attacker], w.units[occupant], skill_lv) != 0) {
            take_item_to_depot(w, attacker, occupant);   // FUN_0042b23a 全链
            F.attacker_index = (int32_t)attacker;
            w.units[attacker].skill_use[skill_type] += 1;
            local_2c_cell = attacker_player ? 8 : 2;
            // 二进制 case 9 不设置 local_8（保留先前值）—— 此处不修改。
        }
        break;
    }
    case 10: {  // 异阵营 战斗记录状态-1（下限1）+ HP 夹取至 max_hp
        *hp_buf = 0;
        *mo_buf = 0;
        if (!same && clean_hit(w.units[attacker], w.units[occupant]) != 0) {
            int32_t local_a8;
            if (F.battlerec[1] == 1) local_a8 = 1;          // +4
            else local_a8 = F.battlerec[1] + (-1);
            F.battlerec[1] = local_a8;
            int32_t maxhp = max_hp_of(w, F);
            int32_t local_ac = (maxhp < F.current_hp) ? maxhp : F.current_hp;
            F.current_hp = local_ac;
            F.attacker_index = (int32_t)attacker;
            w.units[attacker].skill_use[skill_type] += 1;
            local_2c_cell = attacker_player ? 8 : 2;
            // 二进制 case 10 不设置 local_8（保留先前值）。
        }
        break;
    }
    case 11: {  // 异阵营 清 unit_state=0
        *hp_buf = 0;
        *mo_buf = 0;
        if (!same && clean_hit(w.units[attacker], w.units[occupant]) != 0) {
            F.unit_state = 0;
            F.attacker_index = (int32_t)attacker;
            w.units[attacker].skill_use[skill_type] += 1;
            local_2c_cell = attacker_player ? 8 : 2;
            local_8 = 1;
        }
        break;
    }
    case 12: {  // 同阵营 buff（equip/defense +0x14 cap100，unit_state=1）
        *hp_buf = 0;
        *mo_buf = 0;
        if (same) {
            if (clean_hit(w.units[attacker], w.units[occupant]) != 0 && F.battlerec[3] != 0x2d) {
                F.unit_state = 1;
                int32_t local_b0;
                if ((100u - (uint32_t)F.equip_integrity) < 0x15u) local_b0 = (int32_t)(100u - (uint32_t)F.equip_integrity);
                else local_b0 = 0x14;
                F.equip_integrity = F.equip_integrity + local_b0;
                int32_t local_b4;
                if ((100u - (uint32_t)F.defense_modifier) < 0x15u) local_b4 = (int32_t)(100u - (uint32_t)F.defense_modifier);
                else local_b4 = 0x14;
                F.defense_modifier = F.defense_modifier + local_b4;
                F.attacker_index = (int32_t)attacker;
                w.units[attacker].skill_use[skill_type] += 1;
                local_2c_cell = attacker_player ? 8 : 2;
                local_8 = 1;
            }
        } else {
            *hp_buf = 0;
        }
        break;
    }
    case 13: {  // 异阵营 HP 伤害（无地形；贡献固定 8/2）
        *mo_buf = 0;
        if (same) {
            *hp_buf = 0;
        } else {
            if (clean_hit(w.units[attacker], w.units[occupant]) == 0) {
                *hp_buf = 0;
            } else {
                int32_t base = *hp_buf;
                int32_t local_b8 = (int32_t)((u8 * (uint32_t)base) / 100u);
                if ((uint32_t)F.current_hp <= (uint32_t)local_b8) local_b8 = F.current_hp;
                F.current_hp = calc_damage(F.current_hp, local_b8, 0);
                F.attacker_index = (int32_t)attacker;
                *hp_buf = local_b8;
                if (local_b8 != 0) {
                    F.unit_state = 3;
                    w.units[attacker].skill_use[skill_type] += 1;
                    local_2c_cell = attacker_player ? 8 : 2;
                    local_8 = 1;
                }
            }
        }
        break;
    }
    case 14: {  // 异阵营 HP + 士气 双伤害（无地形；贡献固定 8/2）
        if (same) {
            *hp_buf = 0;
            *mo_buf = 0;
        } else {
            if (clean_hit(w.units[attacker], w.units[occupant]) == 0) {
                *hp_buf = 0;
                *mo_buf = 0;
            } else {
                int32_t local_bc = (int32_t)((u8 * (uint32_t)(*hp_buf)) / 100u);
                if ((uint32_t)F.current_hp <= (uint32_t)local_bc) local_bc = F.current_hp;
                F.current_hp = calc_damage(F.current_hp, local_bc, 0);
                *hp_buf = local_bc;
                int32_t local_c0 = (int32_t)((u9 * (uint32_t)(*mo_buf)) / 100u);
                if ((uint32_t)F.morale <= (uint32_t)local_c0) local_c0 = F.morale;
                F.morale = calc_damage(F.morale, local_c0, 0);
                *mo_buf = local_c0;
                if (*hp_buf != 0 || *mo_buf != 0) {
                    F.unit_state = 3;
                    F.attacker_index = (int32_t)attacker;
                    w.units[attacker].skill_use[skill_type] += 1;
                    local_2c_cell = attacker_player ? 8 : 2;
                    local_8 = 1;
                }
            }
        }
        break;
    }
    case 15: {  // 异阵营 HP 伤害（无地形；贡献按 ×8/×4 公式）
        *mo_buf = 0;
        if (same) {
            *hp_buf = 0;
        } else {
            uint32_t local_c4 = ((uint32_t)u8 * (uint32_t)(*hp_buf)) / 100u;
            if ((uint32_t)F.current_hp <= local_c4) local_c4 = F.current_hp;
            F.current_hp = calc_damage(F.current_hp, local_c4, 0);
            F.attacker_index = (int32_t)attacker;
            *hp_buf = local_c4;
            if (local_c4 != 0) {
                F.unit_state = 3;
                w.units[attacker].skill_use[skill_type] += 1;
                if (attacker_player) {
                    local_2c_cell = (((local_c4 << 3) / (int32_t)u8) < 5) ? 4
                                                                          : (local_c4 << 3) / (int32_t)u8;
                } else {
                    local_2c_cell = (((local_c4 << 2) / (int32_t)u8) < 3) ? 2
                                                                           : (local_c4 << 2) / (int32_t)u8;
                }
                local_8 = 1;
            }
        }
        break;
    }
    default: {  // 清 HP 缓冲，local_8 = 1
        *hp_buf = 0;
        local_8 = 1;
        break;
    }
    }

    if (anim_dir) *anim_dir = local_8;
    return local_2c_cell;
}

// ============================================================
// 整网格结算
// ============================================================
uint32_t settle_skill_grid(BattleGridState& w, uint32_t attacker, uint32_t skill_type,
                           uint32_t current_unit) {
    uint32_t u8 = 0, u9 = 0;
    compute_skill_power(w, attacker, skill_type, &u8, &u9);

    if (w.double_grid == 0) { w.grid_rows = 9; w.grid_cols = 9; }
    else                     { w.grid_rows = 10; w.grid_cols = 13; }

    uint32_t local_2c = 0;
    uint32_t local_8 = 0;

    for (uint32_t r = 0; r < (uint32_t)w.grid_rows; ++r) {
        for (uint32_t c = 0; c < (uint32_t)w.grid_cols; ++c) {
            uint32_t tile = (uint32_t)((w.grid_origin_row + (int32_t)r) * w.map_width +
                                       (w.grid_origin_col + (int32_t)c));
            uint32_t cell = r * (uint32_t)w.grid_cols + c;
            if (w.unit_presence[tile * 2] == 0 || w.heal_buffer[cell] == 0) {
                w.heal_buffer[cell] = 0;
                w.morale_buffer[cell] = 0;
            } else {
                uint32_t occ = w.unit_index[tile * 2];
                // 地形修正：与 oracle 同源——battle_calc_equip_defense 返回地形类型（0..15），
                // 再按 skill_type 选 mod_a（cases 3/0xd/0xf）或 mod_b（cases 4/5）。
                uint32_t terrain_mod = 0;
                int32_t col = w.grid_origin_col + (int32_t)c;
                int32_t row = w.grid_origin_row + (int32_t)r;
                int32_t terrain_type = battle_calc_equip_defense(col, row);
                if (terrain_type < 0 || terrain_type > 15) terrain_type = 0;
                if (skill_type == 3 || skill_type == 13 || skill_type == 15)
                    terrain_mod = (uint32_t)w.terrain_mod_a[terrain_type];
                else if (skill_type == 4 || skill_type == 5)
                    terrain_mod = (uint32_t)w.terrain_mod_b[terrain_type];
                local_2c += apply_cell_effect(w, attacker, skill_type, occ, cell,
                                             terrain_mod, u8, u9, &local_8);
            }
        }
    }

    w.units[current_unit].unit_state = 0;
    if (local_2c >= 0x10) local_2c = 0x10;
    return local_2c;
}

// ============================================================
// T3 — 基础伤害写入 pass（镜像 FUN_0041307f.c:49-121 确定性 per-cell 伤害核心）
// ============================================================

// 攻击力（逐位匹配 replica::battle_calc_attack_power / 0x0042A82A，读 BattleGridState 字段）。
static int32_t clean_compute_attack_power(const BattleGridState& w, uint32_t idx) {
    const auto& u = w.units[idx];
    int cls = u.class_type;
    if (cls < 0 || cls >= 96) cls = 0;
    int32_t base_atk   = w.class_combat[cls][0];   // ClassType.base_attack  @ 0x28
    int32_t atk_growth = w.class_combat[cls][1];   // ClassType.attack_growth@ 0x1c
    int32_t strength   = u.strength;               // People.strength @ 0x1c
    int32_t level      = u.level;                  // People.level @ 0x24
    int32_t skill_use  = u.skill_gongji;           // People.skill_gongji @ 0x30
    uint32_t iVar5 = (uint32_t)get_skill_level(skill_use);
    // 逐位匹配：((iVar5+0x3b) * (((base_atk*3*strength)/100 + (atk_growth-1)*(level-1))
    //                         * (strength+500)/0x244) / 0x3c)
    uint32_t inner = (((uint32_t)(base_atk * 3 * strength)) / 100u +
                      (uint32_t)((atk_growth - 1) * (level - 1))) *
                     (uint32_t)(strength + 500);
    uint32_t local_8_u = ((iVar5 + 0x3bu) * (inner / 0x244u)) / 0x3cu;
    int32_t local_8 = (int32_t)local_8_u;
    if (w.current_mode == 0x96) {   // 二进制 mode==0x96 分支（士气×装备完整度修正）
        int32_t morale = u.morale;
        int32_t integ  = u.equip_integrity;
        local_8 = (int32_t)((uint32_t)(morale + 100) *
                            ((uint32_t)((uint32_t)local_8 * (uint32_t)integ) / 100u)) / 200u;
    }
    return local_8 + (int32_t)battle_calc_item_attack_bonus((int32_t)idx);

}

// 防御力（逐位匹配 replica::battle_calc_defense / 0x0042A98E，读 BattleGridState 字段）。
static int32_t clean_compute_defense(const BattleGridState& w, uint32_t idx) {
    const auto& u = w.units[idx];
    int cls = u.class_type;
    if (cls < 0 || cls >= 96) cls = 0;
    int32_t base_def   = w.class_combat[cls][2];   // ClassType.base_defense @ 0x2c
    int32_t def_growth = w.class_combat[cls][3];   // ClassType.defense_growth @ 0x20
    int32_t level      = u.level;
    int32_t local_8 = base_def + (level - 1) * def_growth;
    if (w.current_mode == 0x96) {   // mode==0x96：defense×def_modifier/100 ×(morale+100)/200 + 地形
        int32_t morale = u.morale;
        int32_t defmod = u.defense_modifier;
        uint32_t t = (uint32_t)(local_8 * defmod);
        uint32_t uVar2 = ((uint32_t)(morale + 100) * (t / 100u)) / 200u;
        int terrain_idx = battle_calc_equip_defense(u.pos_x, u.pos_y);  // 用实时坐标查地形
        local_8 = (int32_t)(uVar2 + (uVar2 * (uint32_t)g_battlecalc_terrain_modifier[terrain_idx]) / 100u);
    }
    return local_8 + (int32_t)battle_calc_item_defense_bonus((int32_t)idx);
}

// 整网格基础伤害写入（镜像 battle_calc_apply_cell_damage / FUN_0041307f.c:49-121）。
// 返回封顶 0x30 的攻击经验小计（FUN_00429e92 应用交 T3-b）。
uint32_t apply_base_damage(BattleGridState& w, uint32_t attacker) {
    uint32_t local_24 = 0;
    // line 35: uVar2 = FUN_0042a82a(attacker)（攻击力）
    uint32_t attack = (uint32_t)clean_compute_attack_power(w, attacker);
    // lines 36-47: 网格尺寸（DAT_007d06b8==0 → 9×9，否则 10×13）
    int rows, cols;
    if (w.double_grid == 0) { rows = 9;  cols = 9;  }
    else                    { rows = 10; cols = 13; }
    w.grid_rows = rows; w.grid_cols = cols;   // 与 oracle g_grid_rows/g_grid_cols 保持同步

    // lines 49-114: 双层循环（确定性 per-cell 伤害写入，无 RNG）
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            uint32_t tile = (uint32_t)((w.grid_origin_row + (int32_t)r) * w.map_width +
                                       (w.grid_origin_col + (int32_t)c));
            uint32_t cell = (uint32_t)(r * cols + c);
            // lines 52-56: 无占驻 或 无基础伤害 → 清零两缓冲
            if (w.heal_buffer[cell] == 0 || w.unit_presence[tile * 2] == 0) {
                w.heal_buffer[cell] = 0;
                w.morale_buffer[cell] = 0;
            }
            // lines 57-61: 同阵营（敌方攻击己方、己方攻击敌方都跳过）→ 清零两缓冲
            else if (w.units[attacker].enemy == w.units[w.unit_index[tile * 2]].enemy) {
                w.heal_buffer[cell] = 0;
                w.morale_buffer[cell] = 0;
            }
            else {
                uint32_t occ = (uint32_t)w.unit_index[tile * 2];   // 占驻单位索引
                // line 63: defense = FUN_0042a98e(occ)
                uint32_t defense = (uint32_t)clean_compute_defense(w, occ);
                // line 64: base_dmg = FUN_0042784b(attack, defense, 0) = max(0, attack-defense)
                uint32_t base_dmg = (uint32_t)calc_damage((int32_t)attack, (int32_t)defense, 0);
                // line 65-66: local_44 = (base_dmg * hp_buf[cell]) / 100
                uint32_t local_44 = (base_dmg * (uint32_t)w.heal_buffer[cell]) / 100u;
                // lines 67-69: if local_44 <= Fighter[occ].current_hp/0x14 → 取后者
                int32_t hp_int = w.units[occ].current_hp;
                uint32_t hp_div20 = (uint32_t)(hp_int / 0x14);
                if (local_44 <= hp_div20) local_44 = hp_div20;
                // lines 70-75: local_48 = (local_44<2)?1:local_44
                uint32_t local_48 = (local_44 < 2) ? 1 : local_44;
                // lines 76-81: local_4c = min(local_48, Fighter[occ].current_hp)
                uint32_t local_4c = (local_48 < (uint32_t)hp_int) ? local_48 : (uint32_t)hp_int;
                // line 85: Fighter[occ].current_hp = FUN_0042784b(current_hp, local_4c, 0)（扣血）
                w.units[occ].current_hp =
                    calc_damage(w.units[occ].current_hp, (int32_t)local_4c, 0);
                // line 87: Fighter[occ].attacker_index = attacker
                w.units[occ].attacker_index = (int32_t)attacker;
                // line 88: hp_buf[cell] = local_4c
                w.heal_buffer[cell] = (int32_t)local_4c;
                // lines 89-96: 士气伤害 = max(1, (local_4c*0x14)/max_hp)
                uint32_t maxhp = (uint32_t)max_hp_of(w, w.units[occ]);
                uint32_t local_50 = ((local_4c * 0x14) / maxhp < 2) ? 1 : (local_4c * 0x14) / maxhp;
                w.morale_buffer[cell] = (int32_t)local_50;
                // line 98-101: Fighter[occ].morale = FUN_0042784b(morale, local_50, 0)（扣士气）
                w.units[occ].morale = calc_damage(w.units[occ].morale, (int32_t)local_50, 0);
                // line 102: Fighter[occ].unit_state = 3
                w.units[occ].unit_state = 3;
                // line 103: People[attacker].skill_gongji++
                w.units[attacker].skill_gongji += 1;
                // lines 104-111: 攻击经验（敌方单位 >>2）
                uint32_t exp = (uint32_t)calc_attack_exp(w.units[attacker].level, w.units[occ].level);
                if (w.units[attacker].enemy != 0) exp >>= 2;
                local_24 += exp;
            }
        }
    }
    // lines 115-121: local_24 封顶 0x30(48)
    if (local_24 >= 0x30) local_24 = 0x30;
    return local_24;   // FUN_00429e92(attacker, local_24) 应用交 T3-b
}

// ============================================================
// T3-b — 死亡/经验循环（镜像 battle_calc_apply_death_experience / FUN_0041307f.c:146-195）
// 铁律零逐指令证据：output/ghidra_all/FUN_0041307f.c（lines 146-195）。
// attack_exp_subtotal 来自 apply_base_damage（已封顶 0x30）；本函数在其上累加击杀经验并应用总经验。
// 注意：DAT_007d0f1c 在二进制里与 "level 表(stride 0x21) 首元素" 共用基址，真实语义（剩余单位倒数 vs
// 主将 level）待 RE 深核；此处以 g_player_units_remaining 通用 32 位全局保真双端一致，跨层闸门仅证
// clean≡oracle，不证真实二进制保真（已显式标注）。
// ============================================================
uint32_t apply_death_experience(BattleGridState& w, uint32_t attacker, uint32_t attack_exp_subtotal) {
    uint32_t local_24 = attack_exp_subtotal;

    int rows = w.grid_rows, cols = w.grid_cols;
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            uint32_t tile = (uint32_t)((w.grid_origin_row + (int32_t)r) * w.map_width +
                                       (w.grid_origin_col + (int32_t)c));
            uint32_t cell = (uint32_t)(r * cols + c);
            // line 148: hp_buf[cell] != 0（死亡循环只处理本行动造成过伤害的格）
            if (w.heal_buffer[cell] != 0) {
                uint32_t occ = (uint32_t)w.unit_index[tile * 2];
                // lines 148-152: 占驻单位 hp==0 || 士气==0 → 阵亡
                if (w.units[occ].current_hp == 0 || w.units[occ].morale == 0) {
                    // line 153: Fighter[occ].active_flag = 0
                    w.units[occ].active_flag = 0;
                    // line 154: 击杀经验 FUN_0042ab23(阵亡者=occ, 攻击者)
                    uint32_t kexp = (uint32_t)calc_kill_exp(w.units[attacker].level,
                                                            w.units[occ].level);
                    local_24 += kexp;
                    // line 156: FUN_0042afd2(occ) 物品回收（敌方单位 items→item_recovery++）
                    if (w.units[occ].enemy != 0) {
                        for (int s = 0; s < 8; ++s) {
                            int item = (int)w.units[occ].items[s];
                            if (item != 0) w.item_recovery[item] += 1;
                        }
                    }
                    // lines 157-181: 阵亡计数 + 主将 HP 封顶 / 剩余单位倒数
                    if (w.units[occ].enemy == 0) {
                        w.lost_player += 1;                       // DAT_008331f8++
                        if (occ >= 1 && occ < 0x20 &&
                            (w.battle_win_mode == 2 ||
                             (w.battle_win_mode == 1 && w.lost_player < 3))) {
                            int32_t prev = w.player_units_remaining;
                            w.player_units_remaining = (prev == 1) ? 1 : (prev - 1);
                            // line 168-175: 主将(Fighter[0])HP 封顶 max_hp(0)
                            uint32_t maxhp0 = (uint32_t)max_hp_of(w, w.units[0]);
                            int32_t hp0 = w.units[0].current_hp;
                            w.units[0].current_hp = (hp0 < (int32_t)maxhp0) ? hp0 : (int32_t)maxhp0;
                        }
                    } else {
                        w.lost_enemy += 1;                        // DAT_008331f0++
                    }
                    // line 182: FUN_0041e4e0 渲染（Tier-A 跳过）
                }
            }
        }
    }
    // line 195: FUN_00429e92(攻击者, local_24) 应用总经验（改 experience/level）
    int32_t total = w.units[attacker].experience + (int32_t)local_24;
    w.units[attacker].experience = total % 100;
    int32_t nl = w.units[attacker].level + total / 100;
    if (nl >= 100) nl = 99;
    w.units[attacker].level = nl;
    return local_24;
}

}  // namespace dlzr::logic
