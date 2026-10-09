#pragma once

// battle_state.hpp — 战斗运行时状态
// Battle runtime state
//
// 从 FIGHT.DAT 磁盘数据转换为战斗运行时状态。
// Converted from FIGHT.DAT on-disk data to battle runtime state.

#include "core/data_model/types.hpp"
#include "core/data_model/constants.hpp"
#include "core/data_model/game_data.hpp"

#include <array>
#include <cstdint>

namespace dlzr::logic {

// ============================================================
// 战斗中的运行时角色 / Runtime fighter in battle
// ============================================================
struct RuntimeFighter {
    core::Fighter stats;          // 运行时战斗属性 / Runtime combat stats
    int32_t grid_x{0};            // 网格X坐标 / Grid X position
    int32_t grid_y{0};            // 网格Y坐标 / Grid Y position
    int32_t direction{1};         // 朝向 0=N 1=S 2=W 3=E
    int32_t people_idx{-1};       // 关联的 FightPeople 索引 / FightPeople index
    int32_t team{-1};             // 0=玩家, 1=敌方, 2=NPC
    bool is_active{false};        // 是否已部署 / Whether deployed
    bool has_acted{false};        // 本回合是否已行动 / Whether acted this round
    int32_t movement{100};        // 移动力 / Movement points
    int32_t stamina{0};           // 体力(初始HP) / Stamina (initial HP)
};

// ============================================================
// 战斗阶段 / Battle phase
// ============================================================
enum class BattlePhase : int32_t {
    INIT = 0,
    PLAYER_TURN = 1,
    ENEMY_TURN = 2,
    ANIMATING = 3,
    CHECK_RESULT = 4,
    VICTORY = 5,
    DEFEAT = 6,
};

// ============================================================
// 战斗命令（玩家输入）/ Battle command (player input)
// ============================================================
struct BattleCommand {
    enum Type : int32_t {
        NONE = 0,
        MOVE,
        ATTACK,
        SKILL,
        USE_ITEM,
        WAIT,
        FLEE,
        PLACE_MINE,
    };

    Type type{NONE};
    int32_t fighter_idx{-1};      // 执行命令的角色 / Fighter executing
    int32_t target_x{-1};         // 目标X / Target X
    int32_t target_y{-1};         // 目标Y / Target Y
    int32_t target_idx{-1};       // 目标角色索引 / Target fighter index
    int32_t skill_idx{-1};        // 技能索引 / Skill index
    int32_t item_idx{-1};         // 物品索引 / Item index
    int32_t mine_slot{-1};        // 地雷栏位索引 / Mine slot index (0-7)
};

// ============================================================
// 战斗事件（引擎输出）/ Battle event (engine output)
// ============================================================
struct BattleEvent {
    enum Type : int32_t {
        NONE = 0,
        DAMAGE_DEALT,
        FIGHTER_DIED,
        SKILL_USED,
        ITEM_USED,
        FIGHTER_MOVED,
        ROUND_ADVANCED,
        BATTLE_WON,
        BATTLE_LOST,
        MESSAGE,
        MINE_PLACED,
        MINE_TRIGGERED,
    };

    Type type{NONE};
    int32_t source_idx{-1};
    int32_t target_idx{-1};
    int32_t target_x{-1};
    int32_t target_y{-1};
    int32_t value{0};
};

// ============================================================
// 战斗运行时状态 / Battle runtime state
// ============================================================
struct BattleState {
    static constexpr int32_t MAX_FIGHTERS = 128;
    static constexpr int32_t GRID_COLS = 13;
    static constexpr int32_t GRID_ROWS = 7;

    struct PlacedMine {
        int32_t mine_type{0};   // DiLei索引 (0=无) / DiLei index (0=empty)
        int32_t owner_team{0};  // 放置者阵营 / Placer's team
    };

    std::array<RuntimeFighter, MAX_FIGHTERS> fighters;
    int32_t fighter_count{0};

    std::array<int32_t, 92> grid_terrain{};
    std::array<PlacedMine, 92> grid_mines{};
    int32_t grid_param_x{0};
    int32_t grid_param_y{0};

    BattlePhase phase{BattlePhase::INIT};
    int32_t battle_round{0};
    int32_t current_fighter_idx{0};
    int32_t battle_phase_counter{0};

    bool is_over{false};
    bool is_victory{false};

    int32_t max_rounds{0};
    int32_t victory_condition{0};
    int32_t failure_condition{0};

    static constexpr int32_t EVENT_BUFFER_SIZE = 64;
    std::array<BattleEvent, EVENT_BUFFER_SIZE> events;
    int32_t event_count{0};

    void clear_events() { event_count = 0; }
    void push_event(const BattleEvent& ev) {
        if (event_count < EVENT_BUFFER_SIZE) {
            events[event_count++] = ev;
        }
    }
};

}  // namespace dlzr::logic
