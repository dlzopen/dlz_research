#pragma once

// battle_sim.hpp — replica 层：FUN_004138ea 区域伤害/技能效果结算编排
//
// 铁律零逐指令证据：output/ghidra_all/FUN_004138ea.c + output/re_map/RE_FUN_004138ea.md。
//
// 这是二进制真相的 1:1 转录层（replica）。它操作 g_game_state（People/Fighter/ClassType，
// 偏移与二进制 1:1）+ 以下模块级缓冲/战斗记录/特殊计分全局（与二进制内存 1:1），
// 复用已 G-Func 验证的 battle_calc_* 基元。Tier B 渲染/音效调用（FUN_00422769、
// FUN_0042728b、FUN_00405560 等）被刻意剔除——跨层差分只验可观测数值状态。
//
// 多 pass 模型（见 RE doc §1.4）：缓冲 DAT_007d0488/004565d0 由更早的 pass 播种
// （FUN_0041307f 写基础伤害 + 4 个 setup pass 清零），本函数按技能类型缩放施加。
// 单 pass 跨层差分（给定已播种缓冲）已足够锁死 16 分支组合逻辑（RE doc §7.5）。

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ---- 模块级缓冲 / 战斗记录（与二进制全局 1:1）----
// 持久化 per-cell 伤害/修复缓冲（尺寸 = 最大网格 13×10 = 130）。
extern int32_t  g_hp_dmg_buf_0x7d0488[130];      // DAT_007d0488  HP 伤害/修复缓冲
extern int32_t  g_morale_dmg_buf_0x4565d0[130];   // DAT_004565d0 士气缓冲
// 0x21 字节/单位 战斗记录数组（base 0x7d0f18，stride 0x21）；本数组每单位 9 个 int32（36B），
// 覆盖 +0（属性/智力类）、+4（状态）、+0xC（状态2）；+0x20 末字节未用。
extern int32_t  g_battlerec_0x7d0f18[128][9];
// 英雄对决计分（特定 attacker/occupant 索引对，见 RE doc §4 注释）。
extern int32_t  g_special_score_0x7d06d4;
extern int32_t  g_special_score_0x7d06d8;
// 技能威力参数表（stride 0x30，field +0 = param1，+4 = param2）。
extern int32_t  g_skill_param1_0x57ba50[16];
extern int32_t  g_skill_param2_0x57ba54[16];
// 地形修正%（cases 3/0xd/0xf 用 record +0 即 0x475aa8；cases 4/5 用 record +4 即 0x475aac）。
extern int32_t  g_terrain_mod_a_0x475aa8[16];
extern int32_t  g_terrain_mod_b_0x475aac[16];
// 由 GAME.DAT 块#1（16×20B 记录）装载：g_terrain_mod_a=t*5+0（mod_a），g_terrain_mod_b=t*5+1（mod_b）。
void battle_calc_install_terrain_mod_ab(const int32_t* records80);
// 网格与地图参数。
extern int32_t  g_grid_rows;                 // 网格行（local_10）：DAT_007d06b8==0→9，!=0→10
extern int32_t  g_grid_cols;                 // 网格列（local_24）：9 或 0xd(13)
extern int32_t  g_grid_origin_row;           // DAT_007d0c4c
extern int32_t  g_grid_origin_col;           // DAT_007d0c48
extern int32_t  g_battle_double_grid;        // DAT_007d06b8（!=0 → 13×10 大网格）
extern int32_t  g_map_width_0x57ba20;       // 地形网格宽（瓦片列步长）
// 单位占驻位图（tilemap_idx*2）：存在标志（0x57bf10）+ 占驻单位索引（0x57bf11）。
extern uint8_t  g_unit_presence_0x57bf10[65536];
extern uint8_t  g_unit_index_0x57bf11[65536];
// 战斗仓库（0x7db3a8，short 数组，按 item_idx 索引；case 9 把无法装入单位的物品收归此处）。
extern int16_t  g_warehouse_items_battle_0x7db3a8[256];

// ---- 语义实现（extern "C"，供 replica / clean 跨层差分调用）----

// 预计算技能威力标量（FUN_004138ea.c:64-71）：
//   u8 = ((param1 * attr)/100) * (skill_level+7) >> 3 ;  u9 同式用 param2。
//   attr = battlerec[attacker].+0（智力类）；skill_level = battle_calc_skill_level(skill_use[attacker][mode])。
void battle_calc_skill_power(uint32_t attacker_idx, uint32_t skill_type,
                             uint32_t* out_u8, uint32_t* out_u9);

// case 9 子调用（FUN_0042b23a + FUN_0042920d + FUN_004290f8 全链）：
// 从 donor 单位取首个非空物品 → 尝试装入 recipient 单位的空槽（满则收归战斗仓库）→ 清 donor 槽。
// recipient = 行动方（attacker），donor = 占驻方（occupant）。
void battle_calc_take_item_to_depot(uint32_t recipient_idx, uint32_t donor_idx);

// 单格效果施加（FUN_004138ea 网格循环体 lines 88-704）。
// 读取 g_game_state Fighter[occupant]/People[attacker,occupant] + 模块缓冲/战斗记录/特殊计分，
// 按 skill_type 的 16 分支施加效果并写回。返回本格对 local_2c 的贡献；
// 通过 out_anim_dir 透传/更新 local_8（渲染方向，仅 Tier B 表现用，但确定性可比对）。
// terrain_mod：cases 3/4/5 已解析的地形修正%（单格注入，隔离地形表数据源差异）。
uint32_t battle_calc_apply_cell_effect(uint32_t attacker_idx, uint32_t skill_type,
                                       uint32_t occupant_idx, uint32_t cell,
                                       uint32_t terrain_mod, uint32_t u_var8, uint32_t u_var9,
                                       uint32_t* out_anim_dir);

// 整网格结算（FUN_004138ea.c:62-715）。
// 遍历 g_grid_rows×g_grid_cols：无占驻/无基础伤害格清零缓冲；其余调 apply_cell_effect
// （按 skill_type 解析地形修正%）。末尾 (line 708) 清当前单位 unit_state；local_2c 封顶 0x10。
// 返回封顶后的 local_2c（调用方喂 battle_calc_gain_exp 作本次行动总贡献）。
uint32_t battle_calc_settle_skill_grid(uint32_t attacker_idx, uint32_t skill_type,
                                       uint32_t current_unit);

// T3 — 基础伤害写入 pass（FUN_0041307f.c:49-121 逐指令转录）。
// 确定性 per-cell 伤害核心：把已播种的 hp 伤害缓冲（DAT_007d0488）按攻击方攻击力
// （FUN_0042a82a）/ 防守方防御力（FUN_0042a98e）折算为实际扣血（HP + 士气双扣），
// 记录 Fighter.attacker_index / unit_state=3，累计攻击经验（敌方单位 >>2），封顶 0x30 返回
// 攻击经验小计（FUN_00429e92 应用交 T3-b）。无 RNG；操作 g_game_state + 模块缓冲。
uint32_t battle_calc_apply_cell_damage(uint32_t attacker_idx);

// §9.3② — 三 pass 管线运行时入口（setup → FUN_0041307f → FUN_004138ea）。
// 直接在 g_game_state + 模块全局（跨 pass 缓冲）上施加一次技能效果，是游戏主循环会调用的入口。
//   SETUP：用真实技能形状表（game.dat 顺序块#11，铁律零实证）按 facing 旋转成 7×7，
//          以 (anchor_r,anchor_c) 为锚写入 hp/morale 伤害缓冲 + 标记占驻（FUN_00412030 等价）。
//   T3   ：battle_calc_apply_cell_damage(attacker_idx) 写基础伤害。
//   结算 ：battle_calc_settle_skill_grid(attacker_idx, skill_type, current_unit) 按技能类型缩放施加。
// 入参 cols/rows = 当前网格尺寸，mw = 地图宽（unit_presence 索引用），origin = 网格原点行列。
// 返回：结算 pass 的返回值（沿用 battle_calc_settle_skill_grid 语义）。
int32_t battle_calc_run_skill_pipeline(uint32_t attacker_idx, uint32_t skill_type,
                                      uint32_t shape_index, int anchor_r, int anchor_c,
                                      int facing, int cols, int rows, int mw, int origin,
                                      const uint8_t* skill_shape_table);

// T3-b — 死亡/经验循环（FUN_0041307f.c:146-195 逐指令转录）。
// 输入：本行动攻击经验小计 attack_exp_subtotal（来自 battle_calc_apply_cell_damage，已封顶 0x30）。
// 遍历整网格：凡 DAT_007d0488[cell]!=0 且占驻单位 hp==0||士气==0 → Fighter.active_flag=0（阵亡）；
// 累加 FUN_0042ab23(阵亡者, 攻击者) 击杀经验；FUN_0042afd2(阵亡者) 物品回收；我方阵亡→
// g_aux_counter_b(DAT_008331f8)++、敌方阵亡→g_aux_counter_a(DAT_008331f0)++；我方阵亡且 1<=occ<0x20
// 且 (g_flow_phase==2 || (g_flow_phase==1 && g_aux_counter_b<3)) → g_player_units_remaining 地板 1 递减
// + 主将(Fighter[0])HP 封顶 max_hp(0)；末尾 FUN_00429e92(攻击者, local_24) 应用总经验。
// 返回 local_24（= 封顶小计 + 击杀经验，可能 >0x30）。
uint32_t battle_calc_apply_death_experience(uint32_t attacker_idx, uint32_t attack_exp_subtotal);

#ifdef __cplusplus
}
#endif
