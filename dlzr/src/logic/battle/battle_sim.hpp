#pragma once

// battle_sim.hpp — clean 层：战斗区域伤害/技能效果结算（replica 镜像）
//
// 这是 dlzr::logic 对二进制 FUN_004138ea 的"干净"复刻。语义来源是 RE 文档
// output/re_map/RE_FUN_004138ea.md §4（16 分支效果表）+ Ghidra 指令流；本文件从语义出发
// 独立重写（非逐行抄 oracle），命名清晰，结构自由（Tier B），但可观测结果与 oracle 必须 1:1
// —— 由 tests/test_skill_grid_diff.cpp 的跨层差分闸门保证。
//
// 与 oracle 的关键区别：oracle 操作二进制保真 g_game_state；本层操作自包含的 BattleGridState
// （干净命名：heal_buffer / morale_buffer / battlerec / skill_use / 特殊计分 / 网格参数 /
// 技能威力表 / 地形修正表）。命中抽签共用二进制 Borland LCG（rand_impl）以保证与 oracle 抽同值。

#include <array>
#include <cstdint>
#include <vector>

namespace dlzr::logic {

// 战斗网格运行时状态（clean 命名；与 oracle 模块全局 1:1 功能，但独立表示）。
struct BattleGridState {
    // 每单位运行时战斗属性 + 关联名册字段（部署时拷贝；见 battle_state.hpp 的 decoupled 设计）。
    struct Unit {
        int32_t current_hp{0};        // 当前 HP（镜像 Fighter.current_hp）
        int32_t morale{0};            // 士气（镜像 Fighter.morale）
        int32_t equip_integrity{0};   // 装备完整度（Fighter.equip_integrity）
        int32_t defense_modifier{0};  // 防御修正（Fighter.defense_modifier）
        int32_t attacker_index{0};    // 最近攻击方（Fighter.attacker_index）
        uint8_t active_flag{0};       // 部署/激活（Fighter.active_flag，Tier-A 暂未被本函数改）
        uint8_t unit_state{0};        // 行动状态（Fighter.unit_state）
        int32_t enemy{0};             // 阵营 0=我方 / 1=敌方 / 2=特殊（镜像 People.enemy）
        int32_t intelligence{0};      // 智力（技能威力 + 命中）
        int32_t class_type{0};        // 兵种索引
        int32_t level{0};             // 等级
        int32_t experience{0};        // 级内经验 0..99（镜像 People.experience @ 0x28）—— T3-b 经验应用
        int32_t strength{0};          // 武力（镜像 People.strength @ 0x1c）—— T3 攻击力/防御力计算用
        int32_t skill_gongji{0};      // 攻击技能计数（镜像 People.skill_gongji @ 0x30）—— T3 攻击力/经验
        int32_t pos_x{0};             // 实时网格坐标X（镜像 Fighter.pos_x @ 0x20）—— T3 防御地形查询用
        int32_t pos_y{0};             // 实时网格坐标Y（镜像 Fighter.pos_y @ 0x24）
        std::array<int32_t, 16> skill_use{};   // 16 个技能使用计数（mode 0..15）
        std::array<int32_t, 9> battlerec{};     // 0x21 战斗记录：+0 属性 / +4 状态 / +0xC 状态2
        std::array<uint8_t, 8> items{};        // 8 格物品栏（镜像 People.items，case 9 用）
    };

    std::array<Unit, 128> units;

    // 持久化 per-cell 缓冲（尺寸 = 最大网格 13×10 = 130）。
    std::array<int32_t, 130> heal_buffer{};     // 镜像 DAT_007d0488（HP 伤害/修复缓冲）
    std::array<int32_t, 130> morale_buffer{};   // 镜像 DAT_004565d0（士气缓冲）

    // 英雄对决计分（特定 attacker/occupant 索引对）。
    int32_t special_score_a{0};          // 镜像 g_special_score_0x7d06d4
    int32_t special_score_b{0};          // 镜像 g_special_score_0x7d06d8

    // 网格 / 地图参数。
    int32_t grid_rows{9};
    int32_t grid_cols{9};
    int32_t grid_origin_row{0};
    int32_t grid_origin_col{0};
    int32_t map_width{64};
    int32_t double_grid{0};               // !=0 → 13×10 大网格

    // 技能威力参数表（stride 0x30，field +0 = param1，+4 = param2）。
    std::array<int32_t, 16> skill_param1{};
    std::array<int32_t, 16> skill_param2{};

    // 地形修正%（case 3/0xd/0xf 用 mod_a；cases 4/5 用 mod_b）。
    std::array<int32_t, 16> terrain_mod_a{};
    std::array<int32_t, 16> terrain_mod_b{};

    // 战斗仓库（镜像 g_warehouse_items_battle_0x7db3a8，short 数组按 item_idx 索引；case 9 用）。
    std::array<int16_t, 256> depot{};

    // 兵种 HP 基础/成长（max_hp = hp_base + (lv-1)*hp_growth）。
    std::array<std::array<int32_t, 2>, 96> class_hp{};   // [class][0]=hp_base, [1]=hp_growth

    // 兵种 攻防 基础/成长（T3 攻击力/防御力计算用）：[class][0]=base_attack, [1]=atk_growth,
    //   [2]=base_defense, [3]=def_growth（镜像 ClassType @ 0x28/0x1c/0x2c/0x20）。
    std::array<std::array<int32_t, 4>, 96> class_combat{};

    // 战斗模式标志（镜像 g_game_state.g_current_mode；0x96 触发攻防的士气/装备/地形修正分支）。
    int32_t current_mode{0};

    // —— T3-b 死亡/经验循环镜像字段（FUN_0041307f.c:146-195）——
    int32_t lost_player{0};          // 镜像 DAT_008331f8（g_aux_counter_b）：我方单位阵亡计数
    int32_t lost_enemy{0};           // 镜像 DAT_008331f0（g_aux_counter_a）：敌方单位阵亡计数
    int32_t battle_win_mode{0};       // 镜像 DAT_007db578（g_flow_phase）：胜负模式（0/1/2）
    int32_t player_units_remaining{0};// 镜像 DAT_007d0f1c：我方剩余单位/回合倒数（地板 1）
    std::array<int16_t, 256> item_recovery{}; // 镜像 DAT_007db3a6：阵亡单位物品回收计数（按 item_id 索引）

    // 单位占驻位图（tile*2）：存在标志 + 占驻单位索引。
    std::vector<uint8_t> unit_presence;   // tile*2
    std::vector<uint8_t> unit_index;      // tile*2

    void resize_grid_buffers(int rows, int cols, int32_t mw) {
        grid_rows = rows; grid_cols = cols; map_width = mw;
        unit_presence.assign((size_t)mw * mw * 2, 0);
        unit_index.assign((size_t)mw * mw * 2, 0);
    }
};

// 预计算技能威力标量（镜像 battle_calc_skill_power）。
//   u8 = ((param1 * attr)/100) * (skill_level+7) >> 3 ;  u9 同式用 param2。
void compute_skill_power(const BattleGridState& w, uint32_t attacker, uint32_t skill_type,
                         uint32_t* u8, uint32_t* u9);

// 单格效果施加（镜像 battle_calc_apply_cell_effect，clean 语义重写）。
// 返回本格贡献；anim_dir 透传/更新（渲染方向，仅 Tier B）。terrain_mod 单格注入（cases 3/4/5）。
uint32_t apply_cell_effect(BattleGridState& w, uint32_t attacker, uint32_t skill_type,
                           uint32_t occupant, uint32_t cell, uint32_t terrain_mod,
                           uint32_t u8, uint32_t u9, uint32_t* anim_dir);

// case 9 子调用（镜像 battle_calc_take_item_to_depot）：从 donor 取首个非空物品→尝试装入
// recipient 空槽（满则收归 depot）→ 清 donor 槽。与 oracle 逐位一致（FUN_0042b23a 全链）。
void take_item_to_depot(BattleGridState& w, uint32_t recipient, uint32_t donor);

// 整网格结算（镜像 battle_calc_settle_skill_grid）。
// 返回封顶 0x10 的总贡献；清当前单位 unit_state。
uint32_t settle_skill_grid(BattleGridState& w, uint32_t attacker, uint32_t skill_type,
                           uint32_t current_unit);

// ============================================================
// 基础伤害写入 pass（镜像 replica::battle_calc_apply_cell_damage / FUN_0041307f.c:49-121）。
// 这是 3-pass 管线的中段：把已播种的 per-cell 伤害缓冲（heal_buffer）按攻击方攻击力/防守方
// 防御力折算为实际扣血（HP + 士气双扣），记录 attacker_index / unit_state=3，累计攻击经验
// （敌方单位 >>2），最终封顶 0x30 返回攻击经验小计（FUN_00429e92 应用交 T3-b）。
// 纯确定性算术，无 RNG（与 FUN_004138ea 的伤害前置闸门不同）。
uint32_t apply_base_damage(BattleGridState& w, uint32_t attacker);

// ============================================================
// T3-b — 死亡/经验循环（镜像 replica::battle_calc_apply_death_experience / FUN_0041307f.c:146-195）。
// 输入：本行动攻击经验小计 attack_exp_subtotal（来自 apply_base_damage，已封顶 0x30）。
// 流程（逐指令转录）：
//   ① 遍历整网格，凡 hp_buf[cell]!=0 且占驻单位 hp==0||士气==0 → active_flag=0（阵亡）；
//   ② 累加 FUN_0042ab23(阵亡者, 攻击者) 击杀经验到 local_24；
//   ③ FUN_0042afd2(阵亡者) 物品回收（敌方单位：items 槽非零 → item_recovery[item]++）；
//   ④ 我方阵亡 → lost_player++；敌方阵亡 → lost_enemy++；
//      我方阵亡且 1<=occ<0x20 且 (win_mode==2 || (win_mode==1 && lost_player<3)) →
//      player_units_remaining 地板 1 递减 + 主将(Fighter[0])HP 封顶 max_hp(0)；
//   ⑤ FUN_00429e92(攻击者, local_24) 应用总经验（改 experience/level）。
// 返回 local_24（= 封顶小计 + 击杀经验，可能 >0x30），与 oracle 逐位一致。
uint32_t apply_death_experience(BattleGridState& w, uint32_t attacker, uint32_t attack_exp_subtotal);

}  // namespace dlzr::logic
