#pragma once

// level_event_manager.hpp — 关卡事件管理器
// Level event manager — per-level callback system
//
// 重现原作4个回调表的行为：
//   Table 1 (state 51): 战前事件 / Pre-battle events
//   Table 2 (state 58): 波次管理 / Wave management
//   Table 3 (mode 96):  战后渲染 / Post-battle rendering
//   Table 4 (mode 150): 战略地图推进 / Strategic map advance
//
// 基于逆向工程分析：181个唯一函数，428个对话调用点，11个命令ID
// Based on RE analysis: 181 unique functions, 428 dialogue call sites, 11 command IDs

#include "core/data_model/game_data.hpp"
#include "core/data_model/constants.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace dlzr::logic {

// ============================================================
// 关卡事件动作类型 / Level event action types
// ============================================================
enum class LevelEventAction {
    NONE,
    SHOW_DIALOGUE,        // 显示对话 / Display dialogue
    SHOW_CHOICE,          // 显示选项 / Show choice dialog
    APPLY_FEATURE_FLAG,   // 应用特性标记 / Set map feature flag
    APPLY_COUNTER,        // 应用计数器 / Increment counter
    START_BATTLE,         // 开始战斗 / Initialize battle
    SPAWN_REINFORCEMENT,  // 生成增援 / Spawn reinforcements
    CHECK_VICTORY,        // 检查胜利 / Check victory condition
    COMPLETE,             // 完成关卡 / Complete level
    GRID_SCAN,            // 网格扫描 / Scan grid for enemies
    POSITION_TRIGGER,     // 位置触发 / Position-based trigger
    WAYPOINT_ACTIVATE,    // 激活路径点 / Activate waypoint
    HP_MODIFY,            // HP修改 / Modify HP
    STAT_BOOST,           // 属性提升 / Boost stats
    ADD_GOLD,             // 增加金钱 / Add gold
    SCORE_CHECK,          // 分数检查 / Check performance score
    LEVEL_TRANSITION,     // 关卡转换 / Transition to level
    PLAY_ANIMATION,       // 播放动画 / Play animation
    AUTO_COMPLETE,        // 自动完成 / Auto-complete after N turns
};

// ============================================================
// 战前事件结果 / Pre-battle event result (Table 1, state 51)
// ============================================================
struct PreBattleEvents {
    std::vector<int32_t> dialogue_entries;  // TXT.DAT entry indices
    bool has_choice{false};
    std::string choice_prompt;
    std::vector<std::string> choice_options;  // 内联选项标签 / Inline option labels (from DAT_00446xxx)
    int32_t accept_option{0};
    int32_t battle_id{0x33};      // 战斗ID / Battle ID (default 51)
    int32_t map_id{0x37};         // 地图ID / Map ID (default 55)
    int32_t fighter_count{4};     // 战斗员数量 / Fighter count
};

// ============================================================
// 战后事件结果 / Post-battle event result (Table 3)
// ============================================================
struct PostBattleEvents {
    // 剧情标记 / Story flag (DAT_0095ebXX → story_flags[index])
    int32_t set_story_flag{-1};       // story_flags index, -1 = none
    int32_t story_flag_value{0};

    // 计数器 / Counters
    int32_t add_progression{0};       // DAT_0057bd68
    int32_t add_resource{0};         // DAT_0057bd8a

    // 结局标记 / Ending flags (Block 13 composite)
    bool set_ending_protagonist{false};  // DAT_007d06d4 → ending_flag_protagonist
    int32_t ending_protagonist_value{1};
    bool set_ending_xiaofei{false};      // DAT_007d06d8 → ending_flag_xiaofei
    int32_t ending_xiaofei_value{1};
};

// ============================================================
// 波次事件结果 / Wave event result (Table 2, state 58)
// ============================================================
struct WaveEvent {
    // 增援生成 / Reinforcement spawning
    std::vector<int32_t> spawn_entity_ids;      // FUN_00429cc8 参数

    // 网格扫描 / Grid scan
    bool do_grid_scan{false};
    int32_t scan_x_min{0}, scan_x_max{0};
    int32_t scan_y_min{0}, scan_y_max{0};

    // 位置触发 / Position trigger
    bool check_position{false};
    int32_t trigger_x{0}, trigger_y{0};

    // 路径点 / Waypoint
    bool activate_waypoint{false};
    int32_t waypoint_index{-1};

    // HP修改 / HP modification
    bool modify_hp{false};
    int32_t hp_entity_min{0}, hp_entity_max{0};
    int32_t hp_subtract{0};                     // HP减少量

    // 胜利检查 / Victory check
    bool check_victory{false};
    int32_t victory_dialogue{-1};  // TXT.DAT entry index, -1 = none

    // 对话 / Dialogue
    std::vector<int32_t> dialogues;  // TXT.DAT entry indices
    bool has_choice{false};
    std::vector<std::string> choice_options;  // 内联选项标签 / Inline option labels

    // 回合触发 / Turn triggers
    int32_t trigger_turn{-1};                   // -1 = no turn trigger

    // 自动完成 / Auto-complete
    bool auto_complete{false};
    int32_t auto_complete_turn{30};

    // 战斗配置 / Battle config (for multi-phase)
    bool start_battle{false};
    int32_t battle_id{0x3a};
    int32_t map_id{0x37};

    // 标记设置 / Flag setting (story_flags index)
    int32_t set_flag{-1};  // story_flags index, -1 = none
    int32_t flag_value{1};

    // 状态推进 / State advance
    bool advance_state{false};

    // 金币/分数奖励 / Gold/score bonus (DAT_007d0484)
    int32_t add_gold{0};
};

// ============================================================
// 战略地图事件结果 / Strategic map event result (Table 4, mode 150)
// ============================================================
struct StrategicEvent {
    // 清理和重置 / Cleanup and reset
    bool do_cleanup{false};
    bool reset_level{false};
    int32_t reset_to_level{0};

    // 战斗ID设置 / Battle ID setup
    int32_t set_battle_id{0x96};                // 150
    int32_t set_map_id{0x32};                   // 50

    // 模式设置 / Mode setup (DAT_00584388)
    int32_t set_mode{0x2a};                     // 默认42 / Default 42

    // 状态标记 / State marker (DAT_007d081c)
    bool set_state_marker{false};
    int32_t state_marker_value{0};

    // 计数器递增 / Counter increment (DAT_0057bdc0)
    bool increment_counter{false};

    // 场景标记 / Scenario flag (sequential globals _DAT_007cdcb0-_DAT_007cdcf8)
    // 索引 = (地址 - 0x7cdcb0) / 4, 值通常为1
    // Index = (address - 0x7cdcb0) / 4, value typically 1
    bool set_scenario_flag{false};
    int32_t scenario_flag_index{0};
    int32_t scenario_flag_value{1};

    // 剧情标记写入 / Story flag write (DAT_0095ebXX → story_flags[index])
    bool set_story_flag{false};
    int32_t story_flag_index{-1};
    int32_t story_flag_value{1};

    // 自定义全局写入 / Custom global writes (non-scenario globals)
    // 地址→值对，按RE中的实际地址 / address→value pairs per RE addresses
    std::vector<std::pair<int32_t, int32_t>> custom_global_writes;

    // 分数检查 / Score check
    bool check_score{false};
    int32_t score_threshold{2800};
    int32_t low_score_level{0};                 // 低分关卡 / Low score level
    int32_t high_score_level{0};                // 高分关卡 / High score level

    // 关卡转换 / Level transition
    bool do_transition{false};
    int32_t target_level{-1};
    int32_t target_mode{0};

    // 回合事件 / Turn-based events (for indices 49-63)
    std::vector<int32_t> turn_spawns;           // turn -> entity spawns
    std::vector<int32_t> turn_dialogues;        // TXT.DAT entry indices
    int32_t auto_complete_turn{30};

    // 增援生成 / Reinforcement (for mid-battle events)
    std::vector<int32_t> spawn_entities;

    // 对话 / Dialogue
    std::vector<int32_t> dialogues;  // TXT.DAT entry indices

    // 选项结果 / Choice result
    bool has_choice{false};
    int32_t choice_options_count{0};

    // 跳过finalize_battle / Skip finalize_battle (index 14 uses update_battle_mode_96)
    bool skip_finalize{false};

    // 回合计数器条件 / Turn counter condition (DAT_00475aa4)
    // check_turn_counter=true时检查回合数
    // 默认比较方式为大于(>)，turn_check_equals=true时为等于(==)
    bool check_turn_counter{false};
    int32_t turn_threshold{-1};
    bool turn_check_equals{false};           // true: == threshold, false: > threshold

    // 回合条件满足时将state_marker设为-1 / Set state_marker to -1 when turn condition met
    bool turn_set_minus_one{false};
};

// ============================================================
// 关卡选项处理结果 / Level choice processing result
// ============================================================
struct LevelChoiceResult {
    bool proceed_to_battle{false};
    int32_t battle_id{0x33};
    int32_t map_id{0x37};
    int32_t fighter_count{4};
    bool set_feature_flag{false};
    int32_t feature_flag_index{-1};
    int32_t feature_flag_value{1};
    int32_t add_gold{0};
    int32_t stat_boost_amount{0};
    int32_t stat_boost_count{0};
    bool reset_counters{false};
    int32_t target_level{-1};                   // 关卡跳转 / Level redirect

    // 对话链效果 / Dialogue chain effects
    bool set_story_flag{false};                 // 设置剧情标志 / Set story flag
    int32_t story_flag_index{-1};               // story_flags数组索引 / story_flags array index
    int32_t story_flag_value{1};                // 标志值 / Flag value

    bool trigger_dialogue{false};               // 触发后续对话 / Trigger follow-up dialogue
    int32_t dialogue_group{-1};                 // 对话组索引 / Dialogue group index

    bool advance_level_phase{false};            // 推进关卡阶段 / Advance level phase
    int32_t phase_increment{1};                 // 阶段增量 / Phase increment
};

// ============================================================
// 关卡事件管理器类 / Level event manager class
// ============================================================
class LevelEventManager {
public:
    LevelEventManager() = default;

    // 获取战前事件序列 / Get pre-battle event sequence for a fight
    PreBattleEvents get_pre_battle_events(
        int32_t fight_index,
        const core::GameData& game_data) const;

    // 获取波次事件 / Get wave management event for a fight
    WaveEvent get_wave_event(
        int32_t fight_index,
        const core::GameData& game_data) const;

    // 获取战后事件序列 / Get post-battle event sequence for a fight
    PostBattleEvents get_post_battle_events(
        int32_t fight_index,
        bool victory,
        const core::GameData& game_data) const;

    // 获取战略地图事件 / Get strategic map advance event
    StrategicEvent get_strategic_event(
        int32_t fight_index,
        const core::GameData& game_data) const;

    // 应用战后事件到游戏数据 / Apply post-battle effects to game data
    void apply_post_battle(
        const PostBattleEvents& events,
        core::GameData& game_data) const;

    // 应用波次事件效果 / Apply wave event effects
    void apply_wave_effects(
        const WaveEvent& event,
        core::GameData& game_data) const;

    // 应用战略事件效果 / Apply strategic event effects
    void apply_strategic_effects(
        const StrategicEvent& event,
        core::GameData& game_data) const;

    // 处理关卡选择结果 / Process choice selection for a level
    LevelChoiceResult process_choice(
        int32_t fight_index,
        int32_t option_index,
        core::GameData& game_data) const;

private:
    // ============================================================
    // 战前事件实现 / Pre-battle implementations (Table 1)
    // FUN_00446470 表，按关卡索引 / Table indexed by level
    // ============================================================
    PreBattleEvents level_0_pre_battle(const core::GameData&) const;
    PreBattleEvents level_1_pre_battle(const core::GameData&) const;
    PreBattleEvents level_2_pre_battle(const core::GameData&) const;
    PreBattleEvents level_3_pre_battle(const core::GameData&) const;
    PreBattleEvents level_4_pre_battle(const core::GameData&) const;
    PreBattleEvents level_5_pre_battle(const core::GameData&) const;
    PreBattleEvents level_6_pre_battle(const core::GameData&) const;
    PreBattleEvents level_7_pre_battle(const core::GameData&) const;
    PreBattleEvents level_8_pre_battle(const core::GameData&) const;
    PreBattleEvents level_9_pre_battle(const core::GameData&) const;
    PreBattleEvents level_10_pre_battle(const core::GameData&) const;
    PreBattleEvents level_11_pre_battle(const core::GameData&) const;
    PreBattleEvents level_12_pre_battle(const core::GameData&) const;
    PreBattleEvents level_13_pre_battle(const core::GameData&) const;
    PreBattleEvents level_14_pre_battle(const core::GameData&) const;
    PreBattleEvents level_15_pre_battle(const core::GameData&) const;
    PreBattleEvents level_16_pre_battle(const core::GameData&) const;
    PreBattleEvents level_17_pre_battle(const core::GameData&) const;
    PreBattleEvents level_18_19_pre_battle(const core::GameData&) const;
    PreBattleEvents level_20_pre_battle(const core::GameData&) const;
    PreBattleEvents level_21_pre_battle(const core::GameData&) const;
    PreBattleEvents level_22_pre_battle(const core::GameData&) const;
    PreBattleEvents level_23_pre_battle(const core::GameData&) const;
    PreBattleEvents level_24_pre_battle(const core::GameData&) const;
    PreBattleEvents level_25_pre_battle(const core::GameData&) const;
    PreBattleEvents level_26_pre_battle(const core::GameData&) const;
    PreBattleEvents level_27_pre_battle(const core::GameData&) const;
    PreBattleEvents level_28_pre_battle(const core::GameData&) const;
    PreBattleEvents level_29_pre_battle(const core::GameData&) const;
    PreBattleEvents level_30_pre_battle(const core::GameData&) const;
    PreBattleEvents level_31_pre_battle(const core::GameData&) const;
    PreBattleEvents level_32_pre_battle(const core::GameData&) const;
    PreBattleEvents level_33_pre_battle(const core::GameData&) const;
    PreBattleEvents level_34_pre_battle(const core::GameData&) const;
    PreBattleEvents level_35_pre_battle(const core::GameData&) const;
    PreBattleEvents level_36_pre_battle(const core::GameData&) const;
    PreBattleEvents level_37_pre_battle(const core::GameData&) const;
    PreBattleEvents level_38_pre_battle(const core::GameData&) const;
    PreBattleEvents level_39_pre_battle(const core::GameData&) const;
    PreBattleEvents level_40_pre_battle(const core::GameData&) const;
    PreBattleEvents level_41_pre_battle(const core::GameData&) const;
    PreBattleEvents level_42_pre_battle(const core::GameData&) const;
    PreBattleEvents level_43_pre_battle(const core::GameData&) const;
    PreBattleEvents level_44_pre_battle(const core::GameData&) const;
    PreBattleEvents level_45_pre_battle(const core::GameData&) const;
    PreBattleEvents level_46_pre_battle(const core::GameData&) const;

    // ============================================================
    // 波次事件实现 / Wave management implementations (Table 2)
    // FUN_004464C0 表，按关卡索引 / Table indexed by level
    // ============================================================
    WaveEvent level_0_wave(const core::GameData&) const;
    WaveEvent level_1_wave(const core::GameData&) const;
    WaveEvent level_2_wave(const core::GameData&) const;
    WaveEvent level_3_wave(const core::GameData&) const;
    WaveEvent level_4_wave(const core::GameData&) const;
    WaveEvent level_5_wave(const core::GameData&) const;
    WaveEvent level_6_wave(const core::GameData&) const;
    WaveEvent level_7_wave(const core::GameData&) const;
    WaveEvent level_8_wave(const core::GameData&) const;
    WaveEvent level_9_wave(const core::GameData&) const;
    WaveEvent level_10_wave(const core::GameData&) const;
    WaveEvent level_11_wave(const core::GameData&) const;
    WaveEvent level_12_wave(const core::GameData&) const;
    WaveEvent level_13_wave(const core::GameData&) const;
    WaveEvent level_14_wave(const core::GameData&) const;
    WaveEvent level_15_wave(const core::GameData&) const;
    WaveEvent level_16_wave(const core::GameData&) const;
    WaveEvent level_17_wave(const core::GameData&) const;
    WaveEvent level_18_19_wave(const core::GameData&) const;
    WaveEvent level_20_wave(const core::GameData&) const;
    WaveEvent level_21_wave(const core::GameData&) const;
    WaveEvent level_22_wave(const core::GameData&) const;
    WaveEvent level_23_wave(const core::GameData&) const;
    WaveEvent level_24_wave(const core::GameData&) const;
    WaveEvent level_25_wave(const core::GameData&) const;
    WaveEvent level_26_wave(const core::GameData&) const;
    WaveEvent level_27_wave(const core::GameData&) const;
    WaveEvent level_28_wave(const core::GameData&) const;
    WaveEvent level_29_wave(const core::GameData&) const;
    WaveEvent level_30_wave(const core::GameData&) const;
    WaveEvent level_31_wave(const core::GameData&) const;
    WaveEvent level_32_wave(const core::GameData&) const;
    WaveEvent level_33_wave(const core::GameData&) const;
    WaveEvent level_34_wave(const core::GameData&) const;
    WaveEvent level_35_wave(const core::GameData&) const;
    WaveEvent level_36_wave(const core::GameData&) const;
    WaveEvent level_37_wave(const core::GameData&) const;
    WaveEvent level_38_wave(const core::GameData&) const;
    WaveEvent level_39_wave(const core::GameData&) const;
    WaveEvent level_40_wave(const core::GameData&) const;
    WaveEvent level_41_wave(const core::GameData&) const;
    WaveEvent level_42_wave(const core::GameData&) const;
    WaveEvent level_43_wave(const core::GameData&) const;
    WaveEvent level_44_wave(const core::GameData&) const;
    WaveEvent level_45_wave(const core::GameData&) const;
    WaveEvent level_46_wave(const core::GameData&) const;

    // ============================================================
    // 战略地图事件实现 / Strategic map implementations (Table 4)
    // FUN_0044668C 表，按战斗索引索引 / Table indexed by fight_index
    // ============================================================
    StrategicEvent strategic_index_0(const core::GameData&) const;
    StrategicEvent strategic_index_1(const core::GameData&) const;
    StrategicEvent strategic_index_2(const core::GameData&) const;
    StrategicEvent strategic_index_3(const core::GameData&) const;
    StrategicEvent strategic_index_4(const core::GameData&) const;
    StrategicEvent strategic_index_5(const core::GameData&) const;
    StrategicEvent strategic_index_6(const core::GameData&) const;
    StrategicEvent strategic_index_7(const core::GameData&) const;
    StrategicEvent strategic_index_8(const core::GameData&) const;
    StrategicEvent strategic_index_9(const core::GameData&) const;
    StrategicEvent strategic_index_10(const core::GameData&) const;
    StrategicEvent strategic_index_11(const core::GameData&) const;
    StrategicEvent strategic_index_12(const core::GameData&) const;
    StrategicEvent strategic_index_13(const core::GameData&) const;
    StrategicEvent strategic_index_14(const core::GameData&) const;
    StrategicEvent strategic_index_15(const core::GameData&) const;
    StrategicEvent strategic_index_16(const core::GameData&) const;
    StrategicEvent strategic_index_17(const core::GameData&) const;
    StrategicEvent strategic_index_18(const core::GameData&) const;
    StrategicEvent strategic_index_19(const core::GameData&) const;
    StrategicEvent strategic_index_20(const core::GameData&) const;
    StrategicEvent strategic_index_21(const core::GameData&) const;
    StrategicEvent strategic_index_22(const core::GameData&) const;
    StrategicEvent strategic_index_23(const core::GameData&) const;
    StrategicEvent strategic_index_24(const core::GameData&) const;
    StrategicEvent strategic_index_25(const core::GameData&) const;
    StrategicEvent strategic_index_26(const core::GameData&) const;
    StrategicEvent strategic_index_27(const core::GameData&) const;
    StrategicEvent strategic_index_28(const core::GameData&) const;
    StrategicEvent strategic_index_29(const core::GameData&) const;
    StrategicEvent strategic_index_30(const core::GameData&) const;
    StrategicEvent strategic_index_31(const core::GameData&) const;
    StrategicEvent strategic_index_32(const core::GameData&) const;
    StrategicEvent strategic_index_33(const core::GameData&) const;
    StrategicEvent strategic_index_34(const core::GameData&) const;
    StrategicEvent strategic_index_35(const core::GameData&) const;
    StrategicEvent strategic_index_36(const core::GameData&) const;
    StrategicEvent strategic_index_37(const core::GameData&) const;
    StrategicEvent strategic_index_38_40(int32_t index, const core::GameData&) const;
    StrategicEvent strategic_index_41(const core::GameData&) const;
    StrategicEvent strategic_index_42(const core::GameData&) const;
    StrategicEvent strategic_index_43(const core::GameData&) const;
    StrategicEvent strategic_index_44(const core::GameData&) const;
    StrategicEvent strategic_index_45_47(int32_t index, const core::GameData&) const;
    StrategicEvent strategic_index_46(const core::GameData&) const;
    StrategicEvent strategic_index_49(const core::GameData&) const;
    StrategicEvent strategic_index_50(const core::GameData&) const;
    StrategicEvent strategic_index_51(const core::GameData&) const;
    StrategicEvent strategic_index_52(const core::GameData&) const;
    StrategicEvent strategic_index_53(const core::GameData&) const;
    StrategicEvent strategic_index_54(const core::GameData&) const;
    StrategicEvent strategic_index_55(const core::GameData&) const;
    StrategicEvent strategic_index_56(const core::GameData&) const;
    StrategicEvent strategic_index_57(const core::GameData&) const;
    StrategicEvent strategic_index_58(const core::GameData&) const;
    StrategicEvent strategic_index_59(const core::GameData&) const;
    StrategicEvent strategic_index_60(const core::GameData&) const;
    StrategicEvent strategic_index_61(const core::GameData&) const;
    StrategicEvent strategic_index_62(const core::GameData&) const;
    StrategicEvent strategic_index_63(const core::GameData&) const;

    // ============================================================
    // 通用辅助函数 / Common helper functions
    // ============================================================
    static PreBattleEvents make_dialogue_only(const std::vector<int32_t>& dialogue_indices);
    static PreBattleEvents make_dialogue_choice_battle(
        const std::vector<int32_t>& dialogue_indices,
        const std::string& choice_prompt,
        const std::vector<std::string>& option_labels,
        int32_t battle_id, int32_t map_id, int32_t fighter_count = 4);
    static WaveEvent make_standard_victory_check(int32_t victory_dialogue_idx);
};

}  // namespace dlzr::logic
