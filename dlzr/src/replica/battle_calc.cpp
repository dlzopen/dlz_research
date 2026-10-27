// battle_calc.cpp — 战斗数值函数 RE 验证语义实现（extern "C"）
//
// 见 battle_calc.hpp 的证据说明。铁律零：每个函数逐一对照 Ghidra 指令流，
// 并经 output/verified/verify_battle_calc_full.c 对拍 0 分歧后落地。
//
// 数据来源：g_game_state（replica_stubs.h 的 GameState，其 People/Fighter/ClassType
// 字段偏移经结构化映射锁死，与二进制 1:1）。运行时表见 battle_calc.hpp。

#include "battle_calc.hpp"
#include "replica_stubs.h"   // g_game_state（GameState）

#include <cstring>   // memcpy（运行时装载真实数据表）

// ---- 表定义（默认值为最佳已知常量；运行时由数据文件覆盖）----
// 技能等级阈值（0x447280）：证据 FUN_00419ca4.c:17，10 个 uint 阈值，返回 0..10。
uint32_t g_battlecalc_skill_thresholds[10] = {0, 1, 3, 6, 10, 15, 21, 28, 36, 45};
uint8_t  g_battlecalc_class_flag_table[160 * 96] = {0};
int32_t  g_battlecalc_item_atk_bonus[160] = {0};
int32_t  g_battlecalc_item_def_bonus[160] = {0};
uint16_t g_battlecalc_terrain_grid[256 * 256] = {0};
int32_t  g_battlecalc_terrain_modifier[16] = {0};
int32_t  g_battlecalc_map_width = 64;
// 技能形状表（=二进制 DAT_007cdd00）：48 条目 × 196B(7×7 uint32) = 9408B。
// 真实内容运行时由 game.dat 块#11 装载（FUN_00424566.c:24）。
uint8_t  g_battlecalc_skill_shape_raw[9408] = {0};

// 由 GAME.DAT 块#1（16×20B 记录，records80[16*5]）抽取防御加成%（int2）到紧凑[16]。
// 二进制：FUN_0042a98e.c:21 读 *(int*)(0x475ab0 + t*0x14) = records80[t*5+2]。
void battle_calc_install_terrain_table(const int32_t* r) {
    for (int t = 0; t < 16; ++t) g_battlecalc_terrain_modifier[t] = r[t * 5 + 2];
}

// 由 MAP.DAT 解析出的瓦片网格装载进 replica 地形网格（=二进制 0x584390）。
// 证据 FUN_00416c70.c:24 fread 装载 width*2*height 字节 ushort 网格（行主序 t=y*width+x）；
// terrain_grid_get(0x0042a95b) 以 row*width+col 索引，与 load_map 行主序一致。
// 故 tile[t] 直写 grid[t]、g_battlecalc_map_width=width 即可让 oracle 读到真实地形。
void battle_calc_install_terrain_grid(const uint16_t* tiles, size_t count, int32_t width) {
    const size_t cap = sizeof(g_battlecalc_terrain_grid) / sizeof(g_battlecalc_terrain_grid[0]);
    const size_t n = (count < cap) ? count : cap;
    for (size_t i = 0; i < n; ++i) g_battlecalc_terrain_grid[i] = tiles[i];
    g_battlecalc_map_width = width;
}

// 由 game.dat 块#11（偏移 31820，9408B）装载技能形状表到 replica 全局（=二进制 DAT_007cdd00）。
// 与 battle_calc_skill_shape_at 的 table_base 入参同源；battle_calc_run_skill_pipeline 在
// 入参为 null 时回退到此全局，使真实数据在运行时可达。
void battle_calc_install_skill_shape_table(const uint8_t* raw_9408) {
    if (raw_9408) memcpy(g_battlecalc_skill_shape_raw, raw_9408, sizeof(g_battlecalc_skill_shape_raw));
}

// T3-b 死亡/经验循环镜像全局（FUN_0041307f.c:146-195）
int16_t  g_item_recovery[256] = {0};          // 镜像 DAT_007db3a6：物品回收计数（按 item_id 索引）
int32_t  g_player_units_remaining = 0;        // 镜像 DAT_007d0f1c：我方剩余单位倒数（地板 1）

// ============================================================
// 0x00419CA4 — 技能等级分桶：返回首个 exp < threshold[i] 的 i，否则 10
// ============================================================
uint32_t battle_calc_skill_level(uint32_t skill_use_count) {
    uint32_t local_8 = 0;
    while (1) {
        if (9 < local_8) return 10;
        if (skill_use_count < (uint32_t)g_battlecalc_skill_thresholds[local_8]) break;
        local_8++;
    }
    return local_8;
}

// ============================================================
// 0x0042B042 — 物品攻击加成：遍历 8 槽，取兼容装备攻击加成的最大值
// ============================================================
int32_t battle_calc_item_attack_bonus(int32_t person_idx) {
    int best = 0;
    const People* p = &g_game_state.people_array[person_idx];
    for (int slot = 0; slot < 8; slot++) {
        uint8_t item = p->items[slot];
        if (item != 0) {
            uint32_t uVar1 = (uint32_t)(item - 1);
            if (0x50 < uVar1) {                                  // 仅装备：item_id > 0x51
                int cls = p->class_type;
                if (g_battlecalc_class_flag_table[cls + (int)uVar1 * 96] != 0) {
                    int b = g_battlecalc_item_atk_bonus[uVar1];  // int[idx*0x48] @ +0
                    if (best < b) best = b;
                }
            }
        }
    }
    return best;
}

// ============================================================
// 0x0042B0EA — 物品防御加成（与攻击对称，取防御表 +4）
// ============================================================
int32_t battle_calc_item_defense_bonus(int32_t person_idx) {
    int best = 0;
    const People* p = &g_game_state.people_array[person_idx];
    for (int slot = 0; slot < 8; slot++) {
        uint8_t item = p->items[slot];
        if (item != 0) {
            uint32_t uVar1 = (uint32_t)(item - 1);
            if (0x50 < uVar1) {
                int cls = p->class_type;
                if (g_battlecalc_class_flag_table[cls + (int)uVar1 * 96] != 0) {
                    int b = g_battlecalc_item_def_bonus[uVar1];  // int[idx*0x48] @ +4
                    if (best < b) best = b;
                }
            }
        }
    }
    return best;
}

// ============================================================
// 0x0042784b — 伤害结算核心
//   FUN_0041307f.c:64 调用 FUN_0042784b(uVar2, uVar3, 0)（uVar2=攻击 uVar3=防御）→ 基础伤害。
//   二进制（FUN_0042784b.c:10）：
//     if ((defense < attack) && (incoming <= attack - defense)) incoming = attack - defense;
//     return incoming;
//   即损伤 = max(既存伤害, 攻击−防御)，但守卫要求 defense<attack 才生效；
//   攻击<=防御时返回 incoming（基础伤害 incoming=0 → 0）。全 uint，绝不因负数环绕污染。
// ============================================================
uint32_t battle_calc_damage(uint32_t attack, uint32_t defense, uint32_t incoming) {
    if ((defense < attack) && (incoming <= attack - defense)) {
        incoming = attack - defense;
    }
    return incoming;
}

// ============================================================
// 0x0042A95B — 装备/地形防御值：地形网格 (row*width+col) 取 ushort >> 12
// ============================================================
uint16_t battle_calc_equip_defense(int32_t col, int32_t row) {
    int idx = row * g_battlecalc_map_width + col;
    if (idx < 0 || idx >= (int)(sizeof(g_battlecalc_terrain_grid) / sizeof(g_battlecalc_terrain_grid[0])))
        return 0;
    return (uint16_t)(g_battlecalc_terrain_grid[idx] >> 12);
}

// ============================================================
// 0x0042A82A — 攻击力
//   iVar6=base_attk(clsc*0xa0)  iVar1=strength  iVar2=atk_growth  iVar3=level
//   iVar5=threshold_bucket(skill_use)  iVar4=strength
//   local_8=(iVar5+0x3b)*(((iVar6*3*iVar1)/100+(iVar2-1)*(iVar3-1))*(iVar4+500)/0x244)/0x3c
//   if mode==0x96: local_8=((morale+100)*(local_8*equip_integrity/100))/200
//   + skill_table_max_40(person_idx)
// ============================================================
int32_t battle_calc_attack_power(int32_t person_idx) {
    const People* p = &g_game_state.people_array[person_idx];
    int cls = p->class_type;
    int32_t base_atk   = g_game_state.classtype_array[cls].base_attack;     // +0x14
    int32_t atk_growth = g_game_state.classtype_array[cls].attack_growth;   // +0x08
    int32_t strength   = p->strength;                                       // +0x1c
    int32_t level      = p->level;                                          // +0x24
    uint32_t skill_use = (uint32_t)p->skill_gongji;                         // +0x30
    uint32_t iVar5 = battle_calc_skill_level(skill_use);
    uint32_t local_8 =
        ((iVar5 + 0x3bu) *
         (((((uint32_t)(base_atk * 3 * strength)) / 100u +
            (uint32_t)((atk_growth - 1) * (level - 1))) *
           (uint32_t)(strength + 500)) / 0x244u)) / 0x3cu;
    if (g_game_state.g_current_mode == 0x96) {
        int morale = g_game_state.fighter_array[person_idx].morale;            // +0x08
        int integ  = g_game_state.fighter_array[person_idx].equip_integrity;  // +0x0c
        local_8 = ((uint32_t)(morale + 100) *
                   (((uint32_t)local_8 * (uint32_t)integ) / 100u)) / 200u;
    }
    return (int32_t)local_8 + battle_calc_item_attack_bonus(person_idx);
}

// ============================================================
// 0x0042A98E — 防御力
//   iVar1=terrain_grid_get(pos_x,pos_y)
//   local_8=base_def(clsc*0xa0)+(level-1)*def_growth(clsc*0xa0)
//   if mode==0x96:
//     uVar2=((morale+100)*((local_8*def_modifier)/100))/200
//     local_8=uVar2+(uVar2*terrain_mod[iVar1*0x14])/100
//   + skill_table_max_44(person_idx)
// ============================================================
int32_t battle_calc_defense(int32_t person_idx) {
    const People* p = &g_game_state.people_array[person_idx];
    int cls = p->class_type;
    int32_t base_def   = g_game_state.classtype_array[cls].base_defense;     // +0x18
    int32_t def_growth = g_game_state.classtype_array[cls].defense_growth;  // +0x0c
    int32_t level      = p->level;                                          // +0x24
    int terrain_idx = battle_calc_equip_defense(
        g_game_state.fighter_array[person_idx].pos_x,                  // +0x20
        g_game_state.fighter_array[person_idx].pos_y);                 // +0x24
    int32_t local_8 = base_def + (level - 1) * def_growth;
    if (g_game_state.g_current_mode == 0x96) {
        int morale  = g_game_state.fighter_array[person_idx].morale;          // +0x08
        int defmod  = g_game_state.fighter_array[person_idx].defense_modifier; // +0x10
        uint32_t t = (uint32_t)(local_8 * defmod);
        uint32_t uVar2 = ((uint32_t)(morale + 100) * (t / 100u)) / 200u;
        // 铁律零：二进制里 uVar2 为 uint，地形修正项 (uVar2 * int_terrain) 按无符号相乘再 /100，
        // 故此处保持无符号语义，与 Ghidra 指令流逐位一致（地形修正为负时与有符号分道扬镳）。
        local_8 = (int32_t)(uVar2 + (uVar2 * (uint32_t)g_battlecalc_terrain_modifier[terrain_idx]) / 100u);
    }
    return local_8 + battle_calc_item_defense_bonus(person_idx);
}

// ============================================================
// 0x0042AA92 — 攻击经验（每次命中获得）
//   FUN_0041307f.c:104-111 调用 FUN_0042aa92(attacker, defender) 并累加；
//   若防守方为"敌方单位"则结果 >>2（/4，调用方行为，不在本函数内）。
//   二进制（FUN_0042aa92.c:13-35）：diff = 防守方等级 − 攻击方等级；
//     <-6→1, <-3→2, <-1→4, <0→6, ==0→8, else diff*8+8；上限 0x20(32)。
//   本函数取 (attacker_idx, defender_idx)，diff = level[defender] − level[attacker]。
// ============================================================
int32_t battle_calc_attack_exp(int32_t attacker_idx, int32_t defender_idx) {
    int32_t diff = g_game_state.people_array[defender_idx].level
                 - g_game_state.people_array[attacker_idx].level;
    int32_t v;
    if (diff < -6)      v = 1;
    else if (diff < -3) v = 2;
    else if (diff < -1) v = 4;
    else if (diff < 0)  v = 6;
    else if (diff == 0) v = 8;
    else                v = diff * 8 + 8;
    if (0x20 < v) v = 0x20;
    return v;
}

// ============================================================
// 0x0042AB23 — 击杀经验
//   调用约定（FUN_0041307f.c:154）：FUN_0042ab23(defender, attacker)；
//   二进制（FUN_0042ab23.c:12-34）：diff = 防守方等级 − 攻击方等级（与攻击经验同号）；
//     <-6→1, <-3→2, <-1→4, <0→8, ==0→0xc(12), else diff*0x10(16)；上限 0x50(80)。
//   本函数取 (defender_idx, attacker_idx) 以匹配二进制调用约定，
//   即 battle_calc_kill_exp(defender_idx, attacker_idx)。
// ============================================================
int32_t battle_calc_kill_exp(int32_t defender_idx, int32_t attacker_idx) {
    int32_t diff = g_game_state.people_array[defender_idx].level
                 - g_game_state.people_array[attacker_idx].level;
    int32_t v;
    if (diff < -6)      v = 1;
    else if (diff < -3) v = 2;
    else if (diff < -1) v = 4;
    else if (diff < 0)  v = 8;
    else if (diff == 0) v = 0xc;
    else                v = diff * 0x10;
    if (0x50 < v) v = 0x50;
    return v;
}

// ============================================================
// 0x00429E92 — 累加经验 / 升级
//   二进制（FUN_00429e92.c:18-40）：total = exp_mod + gained;
//     leveled_up = (total/100 != 0);
//     new_exp_mod = total % 100;
//     new_level = min(level + total/100, 99)；
//   表现层升级动画（FUN_0043c954 等）属 Tier B，由 clean 负责，此处只收口数值核心。
//   返回 1 表示发生升级，否则 0。读写 g_game_state.people_array[unit_idx]。
// ============================================================
int32_t battle_calc_gain_exp(int32_t unit_idx, int32_t gained) {
    int32_t total = g_game_state.people_array[unit_idx].experience + gained;
    int32_t leveled_up = (total / 100 != 0) ? 1 : 0;
    g_game_state.people_array[unit_idx].experience = total % 100;
    int32_t nl = g_game_state.people_array[unit_idx].level + total / 100;
    if (nl >= 100) nl = 99;
    g_game_state.people_array[unit_idx].level = nl;
    return leveled_up;
}

// ============================================================
// 0x0042ABB0 — 命中判定（基础版，无武器加成）
//   二进制（FUN_0042abb0.c:13-18）：
//     local_8 = (people[atk].intelligence - people[def].intelligence) * 2 + 0x1e;  // 0x1e=30
//     if (local_8 < 0) local_8 = 0;
//     return rand_impl() % 100 < local_8;
//   注意：DAT_007d0f18[unit*0x21] = people[unit].intelligence（0x21*4=0x84=People stride；
//   007d0f18 权威映射 = intelligence@0x20）。索引空间 = people 索引（调用方 FUN_004138ea 传入）。
// ============================================================
int32_t battle_calc_hit_chance(int32_t attacker_idx, int32_t defender_idx) {
    int32_t ai = g_game_state.people_array[attacker_idx].intelligence;   // 007d0f18[atk*0x21]
    int32_t di = g_game_state.people_array[defender_idx].intelligence;   // 007d0f18[def*0x21]
    int32_t rate = (ai - di) * 2 + 0x1e;
    if (rate < 0) rate = 0;
    uint32_t roll = rand_impl() % 100;
    return (roll < (uint32_t)rate) ? 1 : 0;
}

// ============================================================
// 0x0042AC0B — 命中判定（武器/技能加成版）
//   二进制（FUN_0042ac0b.c:14-20）：
//     local_8 = (people[atk].intelligence - people[def].intelligence) * 2 + 0x1e;  // 同基础版
//     if (local_8 < 0) local_8 = 0;
//     local_8 += threshold_bucket(people[atk].skill_lueduo);   // 007d0f50[atk*0x84]=offset 0x58
//     return rand_impl() % 100 < local_8;
//   注意：DAT_007d0f50[atk*0x84] = people[atk] @ (007d0f50-007d0ef8)=0x58 = skill_lueduo（掠夺）。
//   阈值分桶即 battle_calc_skill_level（FUN_00419ca4）。
// ============================================================
int32_t battle_calc_hit_chance_weapon(int32_t attacker_idx, int32_t defender_idx) {
    int32_t ai = g_game_state.people_array[attacker_idx].intelligence;   // 007d0f18[atk*0x21]
    int32_t di = g_game_state.people_array[defender_idx].intelligence;   // 007d0f18[def*0x21]
    int32_t rate = (ai - di) * 2 + 0x1e;
    if (rate < 0) rate = 0;
    uint32_t skill_use = (uint32_t)g_game_state.people_array[attacker_idx].skill_lueduo; // 007d0f50[atk*0x84]
    rate += (int32_t)battle_calc_skill_level(skill_use);
    uint32_t roll = rand_impl() % 100;
    return (roll < (uint32_t)rate) ? 1 : 0;
}

// ============================================================
// 0x00427830 — 资源增减夹取：min(cap, base+delta)（无符号加法溢出则取 cap）
//   二进制（FUN_00427830.c:10）：if ((uint)(base+delta) <= cap) result = base+delta; else cap。
//   用于 HP 修复（case 0：current_hp + 修复量，封顶 max_hp）、士气/装备完整度（封顶 100）等资源增减。
// ============================================================
int32_t battle_calc_clamp_resource(int32_t base, int32_t delta, uint32_t cap) {
    if ((uint32_t)(base + delta) <= cap) return base + delta;
    return (int32_t)cap;
}

// ============================================================
// 0x00429e31 — 单位最大 HP：class.hp_base + (level-1)*class.hp_growth
//   二进制（FUN_00429e31.c:10-12）：
//     *(int*)(0x0096cbac + class_type*0xa0)        // ClassType[class].hp_base (+0x00)
//   + (people[idx].level - 1)
//     * *(int*)(0x0096cbb0 + class_type*0xa0);      // ClassType[class].hp_growth (+0x04)
//   与 clean::calc_max_hp 逐位一致（跨层差分验证）。
//   注：renames.csv 旧名 anim_frame_offset 系望文生义误命名（实为 max HP），已正名。
// ============================================================
int32_t battle_calc_max_hp(int32_t unit_idx) {
    int cls = g_game_state.people_array[unit_idx].class_type;            // 007d0f24[idx*0x21]
    int32_t lv  = g_game_state.people_array[unit_idx].level;             // 007d0f1c[idx*0x21]
    int32_t hp_base   = g_game_state.classtype_array[cls].hp_base;       // 0096cbac[cls*0xa0]
    int32_t hp_growth = g_game_state.classtype_array[cls].hp_growth;     // 0096cbb0[cls*0xa0]
    return hp_base + (lv - 1) * hp_growth;
}

// ============================================================
// 0x00427a71 — N×N 矩阵旋转/拷贝原语
//   二进制（FUN_00427a71.c:13-54）逐索引复刻。mode 含义见 battle_calc.hpp。
//   r,c ∈ [0,n)。被 FUN_00412030 按朝向旋转 7×7 技能形状表（DAT_007cdd00）后叠加到战场。
//   注：原文档误标"7×7 技能影响播种"，铁律零纠正为"形状朝向旋转"。
// ============================================================
void battle_calc_matrix_rotate_27a71(const uint32_t* src, uint32_t* dst, int n, int mode) {
    if (mode == 0 || mode == (int)0xffffffff) {
        for (int i = 0; i < n * n; i++) dst[i] = src[i];
        return;
    }
    if (mode == 1) {  // 180°
        for (int r = 0; r < n; r++)
            for (int c = 0; c < n; c++)
                dst[(n - 1 - r) * n + (n - 1 - c)] = src[r * n + c];
        return;
    }
    if (mode == 2) {  // 90° CCW
        for (int r = 0; r < n; r++)
            for (int c = 0; c < n; c++)
                dst[(n - 1 - c) * n + r] = src[r * n + c];
        return;
    }
    if (mode == 3) {  // 90° CW
        for (int r = 0; r < n; r++)
            for (int c = 0; c < n; c++)
                dst[c * n + (n - 1 - r)] = src[r * n + c];
        return;
    }
    // 未知 mode：与原版一致，不写 dst（原版返回 0xffffffff）。
}

// ============================================================
// 0x00412030 — 技能形状叠加（setup pass 逻辑核心，FUN_00412030.c:41-50 复刻）
//   shape7x7[49] 经 FUN_00427a71(.,.,7,facing) 旋转进局部 7×7，再以 (anchor_x,anchor_y)
//   为锚写入 buf（buf_w×buf_h），仅当 (anchor_x-3+col)∈[0,buf_w) 且 (anchor_y-3+row)∈[0,buf_h)
//   时写入（越界裁剪）。
// ============================================================
void battle_setup_skill_overlay_12030(const uint32_t shape7x7[49], int anchor_x, int anchor_y,
                                      int facing, uint32_t* buf, int buf_w, int buf_h) {
    uint32_t rot[49];
    battle_calc_matrix_rotate_27a71(shape7x7, rot, 7, facing);
    for (int row = 0; row < 7; row++) {
        for (int col = 0; col < 7; col++) {
            int bx = anchor_x - 3 + col;
            int by = anchor_y - 3 + row;
            if (bx >= 0 && bx < buf_w && by >= 0 && by < buf_h) {
                buf[(size_t)by * buf_w + bx] = rot[row * 7 + col];
            }
        }
    }
}

// ============================================================
// 0x0043de50 — 手写 memcpy（被 FUN_00412030 用于 HP 缓冲→morale 缓冲 0x208 字节拷贝）
// ============================================================
void battle_setup_memcpy_3de50(void* dst, const void* src, size_t size) {
    const uint8_t* s = (const uint8_t*)src;
    uint8_t* d = (uint8_t*)dst;
    for (size_t i = 0; i < size; i++) d[i] = s[i];
}

// ============================================================
// 0x00412030.c:23-26 — 技能形状表 DAT_007cdd00 索引 + 读取
// ============================================================
int32_t battle_calc_skill_shape_index(int32_t class_field_shifted, int32_t ivar2_level_scaled) {
    // local_14 = class_field << 2; index = (local_14 - 1 + (iVar2 + 2U) / 3)
    return (class_field_shifted - 1) + (ivar2_level_scaled + 2) / 3;
}

int battle_calc_skill_shape_at(const uint8_t* table_base, int32_t index, int facing, uint32_t out[49]) {
    if (!table_base || index < 0) return -1;
    const uint8_t* entry = table_base + (size_t)index * 196;   // 每项 7*7*4 = 196 字节
    uint32_t raw[49];
    for (int i = 0; i < 49; i++) {
        uint32_t v = 0;
        for (int b = 0; b < 4; b++) v |= (uint32_t)entry[i * 4 + b] << (8 * b);  // 小端读取（x86）
        raw[i] = v;
    }
    battle_calc_matrix_rotate_27a71(raw, out, 7, facing);
    return 0;
}

// §9.3② 第三 pass（FUN_004138ea 结算数值核心）已由 battle_sim.cpp 的权威 oracle
// （battle_calc_apply_cell_effect / battle_calc_settle_skill_grid）1:1 转录。
// 此处原 battle_calc_skill_effect 等 4 个分流函数与权威 oracle 语义相反
// （同/异阵营门控颠倒、经验公式偏离），违反铁律零"单一权威 oracle 每函数"约束，
// 已于 2026-10-25 清理移除。技能形状读取（battle_calc_skill_shape_index /
// battle_calc_skill_shape_at）保留，供 setup pass 与测试使用。
