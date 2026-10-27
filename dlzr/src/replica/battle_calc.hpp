#pragma once

// 战斗数值函数（RE 逐指令验证，铁律零证据）
//
// 源（Ghidra 指令流，权威）:
//   output/ghidra_all/FUN_0042a82a.c  攻击力
//   output/ghidra_all/FUN_0042a98e.c  防御力
//   output/ghidra_all/FUN_0042a95b.c  装备/地形防御值
//   output/ghidra_all/FUN_00419ca4.c  技能等级分桶
//   output/ghidra_all/FUN_0042b042.c  物品攻击加成
//   output/ghidra_all/FUN_0042b0ea.c  物品防御加成
// 交叉验证: output/verified/verify_battle_calc_full.c（语义 vs 逐指令 0 分歧）
//
// 这 6 个函数由 scripts/gen_replica_stubs.py 从 0x0042A82A 等转录桩转发而来。
// 原版转录因「表全局被声明成标量 + &gs+i*4 指针算术 + 空参列表丢弃实参」而非功能；
// 此处为正确语义实现。
//
// 表内容运行时由数据文件 fread 装载，静态 PE 无字节；此处仅按二进制 stride/计数
// 声明容量，内容不可静态验证（已用随机化对拍证明控制流 + 索引 1:1）。

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ---- 运行时表（容量按二进制；内容运行时装载）----
extern uint32_t g_battlecalc_skill_thresholds[10];          // 0x447280 uint[10] 技能等级阈值
extern uint8_t  g_battlecalc_class_flag_table[160 * 96];    // 0x95eb88 兵种×物品兼容 [idx=item-1][class]
extern int32_t  g_battlecalc_item_atk_bonus[160];           // 0x578d40 每物品 stride 0x48 取 +0
extern int32_t  g_battlecalc_item_def_bonus[160];           // 0x578d44 每物品 stride 0x48 取 +4
extern uint16_t g_battlecalc_terrain_grid[256 * 256];       // 0x584390 地形网格 ushort，(row*width+col) 索引，>>12 得地形类型
extern int32_t  g_battlecalc_terrain_modifier[16];          // 0x475ab0 地形防御加成%（紧凑[16]）：每地形 20B 记录的 int2 字段
                                                            // FUN_0042a98e.c:21 读 *(int*)(0x475ab0 + idx*0x14) 等价；由 battle_calc_install_terrain_table 装载。
extern int32_t  g_battlecalc_map_width;                     // 0x57ba20 地图列数/行步长（地形网格宽）
extern uint8_t  g_battlecalc_skill_shape_raw[9408];         // 0x7cdd00 技能形状表（48×196B）运行时装载（game.dat 块#11）；battle_calc_run_skill_pipeline 回退源

// 由 GAME.DAT 块#1（320B=16×20B 记录）装载地形修正：本数组仅承载防御加成%（每记录 int2）。
// 紧凑[16]：g_battlecalc_terrain_modifier[t] = records80[t*5+2]；与 FUN_0042a98e.c:21 等价。
void battle_calc_install_terrain_table(const int32_t* records80);

// 由 MAP.DAT 解析出的瓦片网格（行主序 tile[t]，t=y*width+x）装载进 replica 地形网格（=二进制 0x584390）。
// oracle battle_calc_equip_defense(0x0042a95b) 读 grid[row*width+col]，与 load_map 行主序一致。
void battle_calc_install_terrain_grid(const uint16_t* tiles, size_t count, int32_t width);

// 由 game.dat 块#11（偏移 31820，9408B）装载技能形状表到 replica 全局（=二进制 DAT_007cdd00）。
void battle_calc_install_skill_shape_table(const uint8_t* raw_9408);

// ---- 语义实现（extern "C"，供 replica 转录体调用）----
int32_t  battle_calc_attack_power(int32_t person_idx);
int32_t  battle_calc_defense(int32_t person_idx);
uint16_t battle_calc_equip_defense(int32_t col, int32_t row);
uint32_t battle_calc_skill_level(uint32_t skill_use_count);
int32_t  battle_calc_item_attack_bonus(int32_t person_idx);
int32_t  battle_calc_item_defense_bonus(int32_t person_idx);

// 0x0042784b — 伤害结算核心：损伤 = max(既存伤害, 攻击−防御)，且仅当攻击>防御时生效。
//   二进制：if ((defense < attack) && (incoming <= attack - defense)) incoming = attack - defense; return incoming;
//   基础伤害（调用方传 incoming=0，如 FUN_0041307f.c:64）= max(0, 攻击−防御)；攻击<=防御时返回 0（非 1）。
//   全 uint 语义；defense>=attack 时绝不把 incoming 改写成负数环绕的大数（守卫拦截）。
uint32_t battle_calc_damage(uint32_t attack, uint32_t defense, uint32_t incoming);

// 0x0042AA92 — 攻击经验（每次命中）：diff = 防守方等级 − 攻击方等级；
//   <-6→1, <-3→2, <-1→4, <0→6, ==0→8, else diff*8+8；上限 0x20(32)。
//   取 (attacker_idx, defender_idx)；调用方在"攻击敌方"时对结果 >>2（/4），此修正不在本函数内。
int32_t  battle_calc_attack_exp(int32_t attacker_idx, int32_t defender_idx);
// 0x0042AB23 — 击杀经验：diff = 防守方等级 − 攻击方等级（与攻击经验同号）；
//   <-6→1, <-3→2, <-1→4, <0→8, ==0→0xc(12), else diff*0x10(16)；上限 0x50(80)。
//   取 (defender_idx, attacker_idx) 以匹配二进制调用约定（FUN_0042ab23(defender, attacker)）。
int32_t  battle_calc_kill_exp(int32_t defender_idx, int32_t attacker_idx);
// 0x00429E92 — 累加经验/升级：total=exp_mod+gained; new_exp_mod=total%100;
//   new_level=min(level+total/100, 99)；total/100!=0 触发升级（表现层属 Tier B）。返回 1=升级。
int32_t  battle_calc_gain_exp(int32_t unit_idx, int32_t gained);

// 0x0042ABB0 — 命中判定（基础版）：命中率 = clamp((攻方智力−守方智力)*2+30, 0, ∞)；
//   rand_impl()%100 < 命中率 → 命中(返回1)。证据 FUN_0042abb0.c:13-18。
//   索引空间 = people 索引（DAT_007d0f18[unit*0x21]=people[unit].intelligence）。
int32_t  battle_calc_hit_chance(int32_t attacker_idx, int32_t defender_idx);
// 0x0042AC0B — 命中判定（武器/技能版）：同基础版，但命中率 += 技能等级(攻方.skill_lueduo@0x58)。
//   证据 FUN_0042ac0b.c:14-20（threshold_bucket = battle_calc_skill_level）。
int32_t  battle_calc_hit_chance_weapon(int32_t attacker_idx, int32_t defender_idx);

// 0x00427830 — 资源增减夹取：min(cap, base+delta)（无符号加法溢出则取 cap）。证据 FUN_00427830.c:10。
int32_t  battle_calc_clamp_resource(int32_t base, int32_t delta, uint32_t cap);

// 0x00429E31 — 单位最大 HP：class.hp_base + (level-1)*class.hp_growth。证据 FUN_00429e31.c:10-12。
int32_t  battle_calc_max_hp(int32_t unit_idx);

// 0x00427a71 — N×N 矩阵旋转/拷贝原语（被 setup pass 用于按朝向旋转 7×7 技能形状表）。
//   二进制 FUN_00427a71.c:13-54 逐索引复刻：
//     mode 0 / 0xffffffff : 拷贝（memcpy n*n*4）
//     mode 1 : dst[(n-1-r)*n+(n-1-c)] = src[r*n+c]          (180°)
//     mode 2 : dst[(n-1-c)*n+r]         = src[r*n+c]          (90° CCW)
//     mode 3 : dst[c*n+(n-1-r)]         = src[r*n+c]          (90° CW)
//   r,c ∈ [0,n)。注：文档曾误标为"7×7 技能影响播种"，实为形状朝向旋转，已纠正。
void battle_calc_matrix_rotate_27a71(const uint32_t* src, uint32_t* dst, int n, int mode);

// 0x00412030 — 技能形状叠加（setup pass 逻辑核心，FUN_00412030.c:41-50 复刻）。
//   shape7x7[49] 经 FUN_00427a71(.,.,7,facing) 旋转进局部 7×7，再以 (anchor_x,anchor_y)
//   为锚写入 buf（buf_w×buf_h），仅当 (anchor_x-3+col)∈[0,buf_w) 且 (anchor_y-3+row)∈[0,buf_h)
//   时写入（越界裁剪）。真实 7×7 形状表内容 = DAT_007cdd00（按技能索引偏移），属数据抽取子任务。
void battle_setup_skill_overlay_12030(const uint32_t shape7x7[49], int anchor_x, int anchor_y,
                                      int facing, uint32_t* buf, int buf_w, int buf_h);

// 0x0043de50 — 手写 memcpy(dst, src, size)（被 FUN_00412030 用于 HP 缓冲→morale 缓冲 0x208 字节拷贝）。
void battle_setup_memcpy_3de50(void* dst, const void* src, size_t size);

// T3-b 死亡/经验循环镜像全局（FUN_0041307f.c:146-195）：
//   g_item_recovery  — 镜像 DAT_007db3a6：阵亡单位物品回收计数（按 item_id 索引的 int16 数组）。
//   g_player_units_remaining — 镜像 DAT_007d0f1c：我方剩余单位/回合倒数（地板 1）；
//      注：0x21 = 单位记录 INT 步长（0x21*4=0x84=132B=People stride）；0x7d0f18/0x7d0f1c/0x7d0f24
//      为同一单位记录的相邻 int 字段（证据 FUN_00412030.c:23,25：&DAT_007d0f24[unit*0x21] 与
//      &DAT_007d0f28[unit*0x84] 同记录索引）。"共用基址"实为相邻字段，非独立 level 表；此处按通用计数器用。
extern int16_t  g_item_recovery[256];
extern int32_t  g_player_units_remaining;

// ============================================================
// 0x00412030.c:23-26 — 技能形状表 DAT_007cdd00：每项 7×7 uint32 = 0xc4(196) 字节。
//   索引 = (local_14 - 1 + (iVar2+2)/3)，local_14 = class_field<<2，iVar2 = 技能等级分桶(FUN_00419ca4)。
//   facing = param_4，经 FUN_00427a71 旋转（mode 0/0xff=拷贝,1=180,2=90CCW,3=90CW）。
//   真实数据由 PE 映射从 DLZ.exe 抽取（见 test_battle_setup.cpp 的 va_to_offset 读取）。
// ============================================================
int32_t battle_calc_skill_shape_index(int32_t class_field_shifted, int32_t ivar2_level_scaled);
// 从 DAT_007cdd00 基址 + index 读取 196 字节(7×7 uint32) 并按 facing 旋转进 out[49]。返回 0=成功, -1=越界/非法。
int battle_calc_skill_shape_at(const uint8_t* table_base, int32_t index, int facing, uint32_t out[49]);

// §9.3② 第三 pass（FUN_004138ea 结算数值核心）的权威 oracle 在 battle_sim.hpp
// （battle_calc_apply_cell_effect / battle_calc_settle_skill_grid），本文件不再另设分流声明。
// 原 battle_calc_skill_attack_power / battle_calc_skill_exp_cap / battle_calc_skill_effect /
// battle_settle_apply_to_game_state 因语义与权威 oracle 相反，已于 2026-10-25 清理移除。

#ifdef __cplusplus
}
#endif
