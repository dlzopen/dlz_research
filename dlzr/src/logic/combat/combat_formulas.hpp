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
#include "core/data_model/game_data.hpp"

#include <cstdint>
#include <vector>

namespace dlzr::logic {

// ============================================================
// 地形防御加成查询 / Terrain defense bonus lookup
//
// RE: terrain_defense_table @ 0x00475AB0
// Returns defense bonus percentage for given terrain type
// ============================================================
int32_t get_terrain_defense_bonus(int32_t terrain_type);

// 运行时装载的地形防御加成表（16 项，每项 = 每地形防御加成% = GAME.DAT 块#1 记录 int2）。
// 由 DatLoader::load_game_data 经 set_terrain_defense_bonus 装载，替代原先的手写魔法数组（RE 违规）。
// 二进制实测钉死：terr[0]=(40,-20,10,0,0)→def=10；terr[9]=(20,0,15,20,10)→def=15；terr[11]→def=40。
extern int32_t g_terrain_defense_bonus[16];
void set_terrain_defense_bonus(const int32_t* src16);

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
// 经验值计算 / Experience calculation  (RE: 0x0042AA92 / 0x0042AB23 / 0x00429E92)
//
// 二进制无 base_exp 概念：经验纯由"等级差"分桶决定。两套函数同号
// （diff = 防守方/受害者等级 − 攻击方/击杀者等级），但分桶与上限不同：
//   · 攻击经验 calc_attack_exp(attacker, defender)：<-6→1,<-3→2,<-1→4,<0→6,==0→8,
//     else diff*8+8；上限 32。调用方在"攻击敌方"时对结果 >>2（/4），此修正不在本函数内。
//   · 击杀经验 calc_kill_exp(killer, victim)：<-6→1,<-3→2,<-1→4,<0→8,==0→12,
//     else diff*16；上限 80。
//   · 累加/升级 apply_experience_gain(person, gained)：total=exp_mod+gained;
//     exp_mod=total%100; level=min(level+total/100,99)；返回是否升级。
//   旧 calc_experience(killer,victim,base_exp) 用 base_exp*mult/4 是手写猜测，已删除。
// ============================================================
int32_t calc_attack_exp(int32_t attacker_level, int32_t defender_level);
int32_t calc_kill_exp(int32_t killer_level, int32_t victim_level);

// 累加经验/升级（RE: 0x00429E92）。修改 person.experience(=exp_mod, 0..99) 与 person.level。
// 返回 true 表示发生升级。注意：二进制把经验存为"当前级内经验"而非累计总值。
bool apply_experience_gain(core::People& person, int32_t gained);

// ============================================================
// 伤害结算 / Damage resolution (0x0042784B)
//
// RE: FUN_0042784b.c:10 — 二进制精确语义：
//   损伤 = max(既存伤害, 攻击−防御)，且仅当 攻击 > 防御 时生效；
//   攻击 <= 防御 时返回 incoming（基础伤害 incoming=0 → 0，非 1）。
//   全 uint 语义：防御>=攻击时绝不把 incoming 改造成负数环绕的大数。
// ============================================================
int32_t calc_damage(int32_t attack_power, int32_t defense, int32_t incoming_damage = 0);

// ============================================================
// 装备攻防加成查询 / Equipped item attack/defense bonus lookup
// RE: 0x0042B042 (get_item_attack_bonus) / 0x0042B0EA (get_item_defense_bonus)
//
// 扫描人物 8 个物品栏位，找出与当前兵种兼容且加成最高的装备。
// 仅物品 ID > 0x51（即 item_idx = ID-1 > 0x50）的才算装备；
// 再通过 battle_config_3 兼容性矩阵（wupin_compat[item_idx][class_type] != 0）过滤。
// 攻击加成 = WuPin[item_idx].para3，防御加成 = WuPin[item_idx].para4
// （运行时 goods 表基址 0x578D18 = WuPin 数组，攻防字段分别在 +0x28/+0x2C）。
// ============================================================
int32_t get_item_attack_bonus(const core::People& person, const core::GameData& gd);
int32_t get_item_defense_bonus(const core::People& person, const core::GameData& gd);

// ============================================================
// 命中判定 / Hit chance (RE: 0x0042ABB0 / 0x0042AC0B)
//
// 命中机制是伤害结算前的独立"攻击是否命中"闸门（FUN_004138ea 在伤害应用之前调用）：
//   · 基础版 0x0042ABB0：命中率 = clamp((攻方智力 − 守方智力)×2 + 30, 0, ∞)；
//                          rand_impl()%100 < 命中率 → 命中(1)，否则 0。
//   · 武器/技能版 0x0042AC0B：命中率额外 += 技能等级（攻方.skill_lueduo 经阈值分桶）。
//   rand_impl 为二进制全工程共用的 Borland LCG（种子 g_game_state.g_pow2_1_448790），
//   clean 层直接复用，保证两条随机流完全一致（跨层差分用种子复位验证）。
//   skill_level 由调用方以 get_skill_level(attacker.skill_lueduo) 预分桶传入（与 0x0042AC0B 一致）。
// ============================================================
int32_t calc_hit_chance(const core::People& attacker, const core::People& defender);
int32_t calc_hit_chance_weapon(const core::People& attacker, const core::People& defender,
                               int32_t skill_level);

// ============================================================
// 资源增减夹取 / Clamp resource delta (RE: 0x00427830)
//
// 二进制 FUN_00427830.c:10：if ((uint)(base+delta) <= cap) result = base+delta; else cap。
// 即「仅封顶」的 max(cap, base+delta) 取小，无下界夹取（下界靠调用方约束 delta>=0）。
// 用于 HP 修复（封顶 max_hp）、士气/装备完整度（封顶 100）等资源增量。
// ============================================================
int32_t clamp_stat_delta(int32_t base, int32_t delta, int32_t cap);

// 等级奖励：遍历已部署(active_flag!=0)且未装备(enemy==0)的单位，求 level 平均，
// bonus = avg - (fight_index*2+1)；<2→0；按 flow_phase 缩放(0→0,1→/2,else*4/5)。
// 仅当 fight_index<0x24 时计算，否则直接 0。RE: 0x0042A199。
// clean↔oracle 跨层差分见 CrossLayerDiff::LevelBonus（oracle=replica::calc_level_bonus_a199）。
int32_t calc_level_bonus(const std::vector<core::People>& people,
                         const std::vector<core::Fighter>& fighters,
                         int32_t fight_index,
                         int32_t flow_phase);

}  // namespace dlzr::logic
