#pragma once

// 地雷战 (Landmine War, 1998) — 全局游戏数据聚合
// Aggregated game data loaded from original binary files

#include "core/data_model/types.hpp"
#include "core/data_model/constants.hpp"

#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace dlzr::core {

// ============================================================
// 从原始 DAT 文件加载的全局数据
// Global data loaded from original DAT files at runtime
// ============================================================
struct GameData {
    std::array<People, RUNTIME_PEOPLE_MAX> people;  // 128 槽运行时 people 数组（原作存档块16/17 = 128×132B；[0..39]=战役名册，[40..127]=本关名册）
    std::array<ClassType, MAX_CLASS_TYPES> class_types;
    std::array<JiNeng, MAX_SKILLS> skills;
    std::array<Town, MAX_TOWNS> towns;
    std::array<WuPin, MAX_WUPIN> items;
    std::array<DiLei, MAX_DILEI> mines;
    std::array<Material, 10> materials;
    DiLeiRecipe mine_recipe;

    // 兵种×物品装备兼容性矩阵 / Class × item equipment compatibility matrix
    // RE: battle_config_3 @ 0x95EB88, layout = WuPinValid (160 rows × 96 bytes)
    // GAME.DAT 末尾 15360 字节 / last 15360 bytes of GAME.DAT
    // wupin_compat[item_idx][class_type] != 0 表示该兵种可使用此物品
    std::array<std::array<uint8_t, MAX_CLASS_TYPES>, MAX_WUPIN> wupin_compat{};

    // 兵种地形移动消耗矩阵 / Class × terrain movement cost (BingZhongYiDong)
    // RE: GAME.DAT +0x3F20, 96 兵种 × 16 地形 × uint8 = 1536 字节（二进制实测）。
    // 值 = 该兵种通过该地形的移动消耗；99 = 不可通行。
    // 证据：0x3F20 处 class[0] = [1×14, 99, 99]、class[1] 地形 3/4 = 2（Python 抽取 DLZFULL/GAME.DAT）。
    std::array<std::array<uint8_t, 16>, MAX_CLASS_TYPES> class_move_cost{};

    // 地形修正表 / Terrain modifiers (DiXingModifier)
    // RE: GAME.DAT +0x00A0, FUN_00424566.c:15 装载 16×0x14=320B；
    // FUN_0042a98e.c:21 / FUN_004138ea.c:262/310 按 20B(=5×int32) 记录索引：
    //   int0=mod_a(case3/4/5) int1=mod_b(case6) int2=防御加成%(case def) int3/int4 保留。
    // ⚠️ 旧实现读 40B/地形是读穿进相邻块的 bug；以 20B/记录（5 int）为准，二进制字节实测钉死。
    std::array<std::array<int32_t, 5>, 16> terrain_modifiers{};

    // FIGHT.DAT: 每关数据按需加载，不全部驻留内存
    // Fight data loaded on demand per battle, not all resident

    // 运行时状态（对应原作 28 存档数据块中的全局变量）
    // Runtime state (maps to globals in the 28 save data blocks)
    int32_t current_fight_index{0};   // fight_index @ 0x7D06A0
    int32_t current_town{0};          // town @ 0x7D0484
    // 剧情进度标记 / Story progress flags
    // 原作地址 / Original: DAT_0095EAF8, 14 × int32_t, Block 12 (56 bytes)
    // 索引→地址映射 / Index→address mapping:
    //   [0] +0x00 DAT_0095eaf8   [1] +0x04 DAT_0095eafc   [2] +0x08 DAT_0095eb00
    //   [3] +0x0c DAT_0095eb04   [4] +0x10 DAT_0095eb08   [5] +0x14 DAT_0095eb0c
    //   [6] +0x18 DAT_0095eb10   [7] +0x1c DAT_0095eb14   [8] +0x20 DAT_0095eb18
    //   [9] +0x24 DAT_0095eb1c  [10] +0x28 DAT_0095eb20  [11] +0x2c DAT_0095eb24
    //  [12] +0x30 DAT_0095eb28  [13] +0x34 DAT_0095eb2c
    // 使用场景 / Used by:
    //   [2] DAT_0095eb00: Level 36 position trigger (FUN_00409C99)
    //   [5] DAT_0095eb0c: Level 2 completion flag (FUN_00407002)
    //   [6] DAT_0095eb10: Level 3 map feature (FUN_00407049)
    //   [7] DAT_0095eb14: Level 1 map feature (FUN_00406F96)
    //  [10] DAT_0095eb20: Level 6 map feature (FUN_0040714D)
    //  [11] DAT_0095eb24: Level 8 completion flag (FUN_00407224)
    //  [12] DAT_0095eb28: Level 7 story flag (FUN_004071CA)
    std::array<int32_t, 14> story_flags{};
    std::array<uint16_t, 160> warehouse{};  // 仓库物品数量 / Warehouse item quantities

    // Block 2: 难度 / Difficulty (0=Easy, 1=Normal, 2=Hard) @ 0x7DB578
    int32_t difficulty{1};

    // Block 7: 隐藏关卡积分 / Hidden level score @ 0x6223A8
    int32_t hidden_level_score{0};

    // Block 9: 战斗结果标量 / Battle result scalar @ 0x7D069C
    int32_t battle_result_scalar{0};

    // Block 11: 城镇状态数组 / Town state array [19] @ 0x7CDCB0
    std::array<uint32_t, 19> town_states{};

    // Block 13: 职业状态复合块 / Class state composite block (380 bytes)
    // Layout: [0-11] header, [12] ending_flag_0, [16] ending_flag_7,
    //         [20] ally_mode, [24] battle_phase, [28] ending_flag_third, [32-379] class_states[87]
    int32_t ending_flag_protagonist{0};   // @ offset 0x0C in block (DAT_007d06d4)
    int32_t ending_flag_xiaofei{0};       // @ offset 0x10 in block (DAT_007d06d8)
    int32_t ally_mode_flag{0};            // @ offset 0x14 in block
    int32_t battle_phase_2{0};            // @ offset 0x18 in block
    int32_t ending_flag_third{0};         // @ offset 0x24 in block (DAT_007d06ec)
    std::array<uint32_t, 87> class_states{};  // @ offset 0x20 in block (skips 0x24)

    // Block 20: 角色状态数组 / Character state array [40] @ 0x95EB30
    std::array<uint16_t, 40> character_states{};

    // Block 25: 战斗计数器 / Combat counters [3] @ 0x8331F0
    // [0]=survival_count, [1]=unknown, [2]=kill_count
    std::array<uint32_t, 3> combat_counters{};

    // Block 27: 游戏帧计数 / Game tick counter @ 0x584388
    uint32_t game_tick{0};

    // 战斗缓存 / Cached battle data (only valid during/after battle)
    core::FightIntro cached_fight_intro{};  // Block 4 @ 0x7D01C8
    std::array<core::Fighter, 128> fighter_array{};   // Block 14 @ 0x96AB90
    std::array<core::Fighter, 128> fighter_mirror{};   // Block 15 @ 0x7D93A8
    std::array<uint16_t, 160> battle_warehouse{};      // Block 19 @ 0x7DB3A8
    std::array<uint16_t, 16384> map_grid{};             // Block 22 @ 0x962790
    int32_t map_width{128};                             // Block 5 @ 0x962788
    int32_t grid_camera_col{0};                         // Block 23 @ 0x7D0C48
    int32_t grid_camera_row{0};                         // Block 23 @ 0x7D0C4C
    std::array<uint32_t, 7> box_state{};                // Block 24 @ 0x7D06A8

    // 进度/资源计数器 / Progression & resource counters (DAT_0057bdXX)
    int32_t progression_counter{0};   // DAT_0057bd68: game progression
    int32_t resource_counter{0};      // DAT_0057bd8a: resource increment

    // 战略标记 / Strategic markers
    int32_t state_marker{0};          // DAT_007d081c: state/phase marker for hidden level branching
    int32_t battle_counter{0};        // DAT_0057bdc0: battles completed counter
    int32_t turn_counter{0};          // DAT_00475aa4: turn/round counter for strategic events

    // 场景标记数组 / Scenario flags (DAT_007cdcb0..DAT_007cdcf4)
    // 索引 = (地址 - 0x7cdcb0) / 4, 共19个条目
    // Index = (address - 0x7cdcb0) / 4, 19 entries
    // Used by Table 4 strategic callbacks to track progression
    std::array<int32_t, 19> scenario_flags{};

    // 原始全局写入 / Raw global writes (RE addresses without named fields)
    // 地址→值映射，用于尚未分配语义字段的原作全局变量
    // Address→value map for original globals without dedicated fields
    std::map<int32_t, int32_t> raw_globals;

    // 关卡事件阶段 / Level event phases (per-level state machine variables)
    // DAT_007d06c8..DAT_007d0820: phase trackers for each level's callback
    std::array<int32_t, 47> level_phases{};

    // 锻造系统运行时 / Forging system runtime
    std::array<int32_t, 10> material_inventory{};   // 材料库存 / Material stock
    std::array<std::array<int16_t, 10>, 4> crafted_stock{};  // 4页×10物品 / 4 tabs × 10 items

    // 主角运行态 / Protagonist runtime state (set on CHARACTER_SELECT confirm)
    // 玩家在角色选择界面修改的主角名（对应对话令牌 GG 的替换值）
    // Player-edited protagonist name (replacement for dialogue token GG)
    std::string protagonist_name{"\xD6\xF7\xBD\xC7"};  // 默认"主角" (GBK)
    // 对话令牌 TT 的替换值（战斗/地名，对应原版 DAT_007d01c8；运行时由场景装载，
    // 非 PE 静态全局）。原版 FUN_0043cc96.c:71-82 把它逐字包 "AA" 着红(0xb7)。
    // 确切运行时值（平阳战役名等）需游戏流程 RE 落定；默认空→TT 退化为剔除。
    std::string battle_name{};
    // 主角头像索引 / Protagonist portrait index (FACE.DAT, hero slot 103+idx)
    int32_t protagonist_face{-1};
    // 主角兵种索引 / Protagonist class type, derived from formation selection {1,4,8,11}
    // 原版确认路径(FUN_0042e2d6)按阵型把兵种索引写入 People[0].class_type
    int32_t protagonist_class_type{1};
    // 主角武力 / Protagonist strength (from 武力 slider, 原版 DAT_007d0f14)
    int32_t protagonist_strength{80};
    // 主角智力 / Protagonist intelligence = 160 - 武力 (原版 DAT_007d0f18 = 0xa0 - 武力)
    int32_t protagonist_intelligence{80};
    // 主角等级 / Protagonist level（战役起始等级；完整升级系统待实现，先用此占位）。
    int32_t protagonist_level{10};

    // 战役招募名册（事件 FSM 写回；BattleEngine::init 消费为同伴部署）。
    // 见 types.hpp RecruitedCompanion / CampaignRoster 注释。
    core::CampaignRoster campaign_roster{};
};

// ============================================================
// 战斗场景数据（从 FIGHT.DAT 单关加载）
// Battle scene data (loaded per-fight from FIGHT.DAT)
// 磁盘记录 20508 字节 = Intro(700) + Fighter[128](8192) + People[88](11616)
// ⚠️ 但**运行时** people 数组是 128 条，不是 88 条 —— 这是此前"敌人全是孙悟空"
//    的根因：把磁盘 88 条当成 people[0..87]，于是槽位40 取到 people[40]=孙悟空。
// ============================================================
struct FightData {
    FightIntro intro;
    std::array<FightFighter, 128> fighters;

    // 运行时布局 128 条 × 132B = 16896B。
    // 证据（三重一致）：
    //   · FUN_00427c94.c:36 / FUN_0041b53c.c:27：整体搬运 people 数组 0x4200=
    //     16896B = 0x80(128) × 0x84(132)（存档读写亦按 128 条）。
    //   · FUN_004292fe.c:18：FIGHT.DAT 装载把磁盘 88 条写到
    //     DAT_007d2398 = people 基址(0x7d0ef8) + 0x14A0 = +40×0x84
    //     ⇒ 运行时 people[40+k] = 磁盘 people[k]。
    // 故 [0..39] = 战役槽（主角 = people[0]），[40..127] = 本关名册。
    std::array<FightPeople, 128> people;
};
static_assert(sizeof(FightData) == 700 + 128 * 64 + 128 * 132,
              "FightData 运行时布局: Intro(700) + Fighter[128](8192) + People[128](16896)");

}  // namespace dlzr::core
