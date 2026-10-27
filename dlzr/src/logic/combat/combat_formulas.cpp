// combat_formulas.cpp — 战斗公式实现
// Combat formula implementations
//
// 所有公式严格匹配原作 DLZ.exe 反编译结果。
// 整数除法使用 C++ 截断除法（向零取整），与 x86 idiv 行为一致。

#include "logic/combat/combat_formulas.hpp"

#include <algorithm>

// 二进制全工程共用的 Borland LCG 随机数（FUN_0043e310）：clean 层直接复用，
// 以保证命中抽签与 replica oracle、原版二进制共用同一条随机流。
#include "replica/replica_stubs.h"   // uint32_t rand_impl();

namespace dlzr::logic {

// 地形防御加成表 / Terrain defense bonus table
// RE: terrain_defense_table @ 0x00475AB0, FUN_0042a98e.c:21 读 *(int*)(0x475ab0 + idx*0x14)
//   = 每地形 20B 记录的 int2 字段（防御加成%）。值运行时由 GAME.DAT 块#1 装载（见 set_terrain_defense_bonus），
//   不再用手写魔法数组（RE 违规）。二进制实测钉死：terr[0].def=10, terr[9].def=15, terr[11].def=40。
int32_t g_terrain_defense_bonus[16] = {0};

void set_terrain_defense_bonus(const int32_t* src16) {
    for (int i = 0; i < 16; ++i) g_terrain_defense_bonus[i] = src16[i];
}

int32_t get_terrain_defense_bonus(int32_t terrain_type) {
    if (terrain_type < 0 || terrain_type >= 16) {
        return 0;
    }
    return g_terrain_defense_bonus[terrain_type];
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

    // Layer 2+3: 战斗修正 + 地形加成（仅战斗模式，与 0x0042A98E 一致）
    // 二进制：mode==0x96 时 uVar2=(morale+100)*(local_8*defmod/100)/200;
    //         local_8 = uVar2 + uVar2*terrain_mod/100；非战斗模式不加地形/士气/defmod。
    if (in_battle_mode) {
        defense = defense * fighter.defense_modifier / 100;
        defense = defense * (fighter.morale + core::MORALE_BASE)
                / core::MORALE_DIVISOR;
        defense += defense * terrain_bonus_percent / 100;
    }

    // Layer 4: 物品加成 / Item bonus
    defense += item_defense_bonus;

    return defense;
}

int32_t get_skill_level(int32_t skill_use_count) {
    // RE: 0x00419CA4 (FUN_00419ca4.c:17) — 二进制算法：
    //   返回首个不满足的桶号 skill_use < threshold[i]，否则 10。
    //   与 replica::battle_calc_skill_level 逐位一致（已用 Ghidra 核验）。
    //   二进制以 uint 读 skill_use：负输入经 uint 环绕成大数 → 返回 10（调用方恒传 >=0 的计数，此分支仅边界保真）。
    const auto& T = core::SKILL_LEVEL_THRESHOLDS;
    const uint32_t s = static_cast<uint32_t>(skill_use_count);
    for (int32_t i = 0; i < core::SKILL_LEVEL_COUNT; ++i) {
        if (s < static_cast<uint32_t>(T[i])) {
            return i;
        }
    }
    return core::SKILL_LEVEL_COUNT;  // 10
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

int32_t calc_attack_exp(int32_t attacker_level, int32_t defender_level) {
    // RE: 0x0042AA92 — diff = 防守方等级 − 攻击方等级。
    int32_t diff = defender_level - attacker_level;
    int32_t v;
    if (diff < -6)      v = 1;
    else if (diff < -3) v = 2;
    else if (diff < -1) v = 4;
    else if (diff < 0)  v = 6;
    else if (diff == 0) v = 8;
    else                v = diff * 8 + 8;
    if (v > 0x20) v = 0x20;
    return v;
}

int32_t calc_kill_exp(int32_t killer_level, int32_t victim_level) {
    // RE: 0x0042AB23 — diff = 受害者等级 − 击杀者等级（与攻击经验同号）。
    int32_t diff = victim_level - killer_level;
    int32_t v;
    if (diff < -6)      v = 1;
    else if (diff < -3) v = 2;
    else if (diff < -1) v = 4;
    else if (diff < 0)  v = 8;
    else if (diff == 0) v = 0xc;
    else                v = diff * 0x10;
    if (v > 0x50) v = 0x50;
    return v;
}

bool apply_experience_gain(core::People& person, int32_t gained) {
    // RE: 0x00429E92 — total=exp_mod+gained; exp_mod=total%100;
    //   level=min(level+total/100, 99)；total/100!=0 触发升级。
    int32_t total = person.experience + gained;
    bool leveled_up = (total / 100 != 0);
    person.experience = total % 100;
    int32_t nl = person.level + total / 100;
    if (nl >= 100) nl = 99;
    person.level = nl;
    return leveled_up;
}

int32_t calc_damage(int32_t attack_power, int32_t defense, int32_t incoming_damage) {
    // RE: 0x0042784B (FUN_0042784b.c:10) — 二进制精确语义：
    //   if ((defense < attack) && (incoming <= attack - defense)) incoming = attack - defense; return incoming;
    //   即损伤 = max(既存伤害, 攻击−防御)，且仅当 攻击>防御 生效；
    //   攻击<=防御时返回 incoming（基础伤害 incoming=0 → 0，非 1）。
    //   全 uint 语义：防御>=攻击时守卫拦截，绝不把 incoming 改写成负数环绕的大数。
    uint32_t a = static_cast<uint32_t>(attack_power);
    uint32_t d = static_cast<uint32_t>(defense);
    uint32_t inc = static_cast<uint32_t>(incoming_damage);
    if ((d < a) && (inc <= a - d)) {
        inc = a - d;
    }
    return static_cast<int32_t>(inc);
}

int32_t get_item_attack_bonus(const core::People& person, const core::GameData& gd) {
    int32_t best = 0;
    int32_t cls = person.class_type;
    for (int slot = 0; slot < 8; ++slot) {
        uint8_t item_id = person.items[slot];
        if (item_id == 0) continue;              // 空栏位跳过
        uint32_t idx = static_cast<uint32_t>(item_id) - 1;  // uVar1 = item_id - 1
        if (idx <= 0x50) continue;               // 仅装备：item_idx > 0x50 (ID > 0x51)
        if (idx >= static_cast<uint32_t>(core::MAX_WUPIN)) continue;
        if (cls < 0 || cls >= core::MAX_CLASS_TYPES) continue;
        if (gd.wupin_compat[idx][cls] == 0) continue;  // 兵种不兼容此装备
        int32_t bonus = gd.items[idx].para3;      // WuPin.para3 = 攻击加成
        if (best < bonus) best = bonus;
    }
    return best;
}

int32_t get_item_defense_bonus(const core::People& person, const core::GameData& gd) {
    int32_t best = 0;
    int32_t cls = person.class_type;
    for (int slot = 0; slot < 8; ++slot) {
        uint8_t item_id = person.items[slot];
        if (item_id == 0) continue;
        uint32_t idx = static_cast<uint32_t>(item_id) - 1;
        if (idx <= 0x50) continue;
        if (idx >= static_cast<uint32_t>(core::MAX_WUPIN)) continue;
        if (cls < 0 || cls >= core::MAX_CLASS_TYPES) continue;
        if (gd.wupin_compat[idx][cls] == 0) continue;
        int32_t bonus = gd.items[idx].para4;      // WuPin.para4 = 防御加成
        if (best < bonus) best = bonus;
    }
    return best;
}

int32_t calc_hit_chance(const core::People& attacker, const core::People& defender) {
    // RE: 0x0042ABB0 (FUN_0042abb0.c:13-18) — 命中率由智力差决定（非 class diff）。
    //   命中率 = clamp((攻智 − 守智)×2 + 30, 0, ∞)；rand_impl()%100 < 命中率 → 命中。
    int32_t rate = (attacker.intelligence - defender.intelligence) * 2 + 0x1e;  // 0x1e = 30
    if (rate < 0) rate = 0;
    uint32_t roll = rand_impl() % 100;
    return (roll < static_cast<uint32_t>(rate)) ? 1 : 0;
}

int32_t calc_hit_chance_weapon(const core::People& attacker, const core::People& defender,
                               int32_t skill_level) {
    // RE: 0x0042AC0B (FUN_0042ac0b.c:14-20) — 同基础版，命中率额外 += 技能等级（已分桶）。
    //   武器/技能版：local_8 = (攻智−守智)×2 + 30（clamp 0）+ bucket(skill_lueduo)。
    int32_t rate = (attacker.intelligence - defender.intelligence) * 2 + 0x1e;  // 30
    if (rate < 0) rate = 0;
    rate += skill_level;
    uint32_t roll = rand_impl() % 100;
    return (roll < static_cast<uint32_t>(rate)) ? 1 : 0;
}

int32_t clamp_stat_delta(int32_t base, int32_t delta, int32_t cap) {
    // RE: 0x00427830 (FUN_00427830.c:10) — 仅封顶：min(cap, base+delta)，无下界夹取。
    //   用无符号比较，使 base+delta 负数环绕成大数时不穿透（调用方保证 delta>=0）。
    if (static_cast<uint32_t>(base + delta) <= static_cast<uint32_t>(cap)) {
        return base + delta;
    }
    return cap;
}

int32_t calc_level_bonus(const std::vector<core::People>& people,
                         const std::vector<core::Fighter>& fighters,
                         int32_t fight_index,
                         int32_t flow_phase) {
    // RE: 0x0042A199 (FUN_0042a199.c) — 等级奖励核心。
    //   仅当 fight_index<0x24 时遍历单位；否则直接 0（与 oracle 行为一致）。
    if (fight_index >= 0x24) return 0;
    int32_t sum = 0, count = 0;
    const int n = static_cast<int>(people.size() < fighters.size() ? people.size() : fighters.size());
    const int limit = (n < 0x20) ? n : 0x20;   // 与 oracle 的 0x20 上限一致
    for (int i = 0; i < limit; ++i) {
        if (people[i].enemy == 0 && fighters[i].active_flag != 0) {
            sum += people[i].level;
            count += 1;
        }
    }
    if (count == 0) return 0;
    int32_t bonus = sum / count - (fight_index * 2 + 1);
    if (bonus < 2) return 0;
    if (flow_phase == 0) return 0;
    if (flow_phase == 1) return bonus / 2;
    return (bonus * 4) / 5;
}

}  // namespace dlzr::logic
