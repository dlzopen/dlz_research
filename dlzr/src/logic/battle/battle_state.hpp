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
#include <vector>

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

    // 解耦的战斗属性：部署时从来源名册(FIGHT.DAT people / PEOPLE.DAT)拷贝，
    // 战斗逻辑直接读这里，不再按运行时索引反查名册（避免我方/敌方名册错配）。
    // Decoupled combat attributes copied at deploy time so battle logic no
    // longer re-indexes the roster by runtime index.
    int32_t class_type{0};              // 兵种索引 / Class type
    int32_t level{1};                   // 等级 / Level
    int32_t strength{0};                // 武力 / Strength
    int32_t intelligence{0};            // 智力 / Intelligence
    int32_t skill_attack{0};            // 基础攻击技能使用次数 / Basic attack skill exp
    std::array<uint8_t, 8> items{};     // 物品栏 / Item slots
    std::array<uint8_t, 8> mines{};     // 地雷栏 / Mine slots

    int32_t attacker{-1};         // 正在攻击本单位的敌人索引 / Index of unit currently attacking this one
                                  // 二进制 fighter 记录 +0x38 = attacker_index（FUN_0040144e.c 撤退目标）；
                                  // 由 BattleEngine::apply_damage 命中写回。
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
    DRAW = 7,
};

enum class TacticalSubState : int32_t {
    IDLE = 0,
    AWAITING_MOVE_DEST = 8,
    AWAITING_ITEM_DEST = 9,
    SKILL_SELECT = 2,
    DEFEND = 3,
    SKILL_TARGET_SELECT = 4,
    SPECIAL_ACTION = 5,
    SUB_ACTION_7 = 7,
    EQUIPMENT_USE = 0xB,
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
        MILESTONE_REACHED,
        ROUNDS_EXHAUSTED,
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
    // 战斗网格尺寸每关不同，由 FIGHT.DAT 的 grid_param 决定（动态，非固定）。
    // Grid dimensions are per-fight (from FIGHT.DAT grid_param_x/y), so all grid
    // buffers are dynamically sized vectors. MAX_GRID_* are safety caps only.
    static constexpr int32_t MAX_GRID_COLS = 64;
    static constexpr int32_t MAX_GRID_ROWS = 64;

    struct PlacedMine {
        int32_t mine_type{0};   // DiLei索引 (0=无) / DiLei index (0=empty)
        int32_t owner_team{0};  // 放置者阵营 / Placer's team
    };

    std::array<RuntimeFighter, MAX_FIGHTERS> fighters;
    int32_t fighter_count{0};

    // 网格地形/地雷/掩码按 MAP.DAT 地图尺寸动态尺寸；0=可行, 99=障碍
    // Grid terrain/mines/masks sized to the MAP.DAT map dims at init.
    // 证据: 原版战斗在 MAP.DAT 瓦片网格上进行（FUN_0041e97a 视口以 MAP.DAT
    // 宽高 DAT_0057ba20/22 为边界做相机钳制）；FIGHT.DAT 单位 grid_x/grid_y
    // 即 MAP.DAT 瓦片坐标（范围 0-63/0-57，与地图尺寸上限一致）。
    std::vector<int32_t> grid_terrain;
    std::vector<PlacedMine> grid_mines;
    // 初始相机目标（视口中心瓦片坐标）= FIGHT.DAT intro.grid_param_x/y。
    // 证据: map_dat_analysis.md §4.3 —— grid_param_x/y 是 MAP.DAT 瓦片网格上的
    // 初始相机位置（0-999），并非战斗网格尺寸；战斗网格尺寸 = MAP.DAT 宽高。
    int32_t cam_target_x{0};
    int32_t cam_target_y{0};
    int32_t map_width{0};      // MAP.DAT 地图宽（瓦片列数）= 战斗网格列数（证据: 0x57BA20 equipment_stride）
    int32_t map_height{0};     // MAP.DAT 地图高（瓦片行数）= 战斗网格行数（证据: 0x57BA22 grid_rows）

    // 相机（视口左上角瓦片坐标）。证据: FUN_0041e97a.c
    //   普通模式(DAT_007d06b8==0): 9×9 视口, 4 格边距,
    //   cam_x = clamp(target_x-4, 0, map_w-9), cam_y = clamp(target_y-4, 0, map_h-9)
    //   单元 48px。
    int32_t cam_x{0};
    int32_t cam_y{0};

    // 屏幕映射（渲染器与输入处理器共用，保证"画在哪、点哪"一致）：
    //   screen_x = (grid_x - cam_x) * 48 + 14 ; screen_y = (grid_y - cam_y) * 48 + 32
    // 证据: FUN_0041be02.c:31 FUN_00426810(p1,p2,0xe,0x20,0x1b0,0x1b0) 即视口命中矩形
    //   (14,32,432,432)，且 hovered cell = (mouse_x-14)/48 -> 真 client 原点 = (14,32)，非 48px 外。
    //   注: FUN_0041e97a.c 的 (0x26,0x38)=(38,56) 是 SetCursorPos 的**屏幕坐标**（含 24px 窗框），
    //   并非 client 空间原点。所有绘制以 (14,32) 为准。
    int32_t grid_origin_x{14};  // 视口像素原点 X (client) / screen origin X of viewport
    int32_t grid_origin_y{32};  // 视口像素原点 Y (client) / screen origin Y of viewport
    int32_t cell_size{48};      // 像素格边长 / pixel cell size (48, 证据 FUN_0041be02)

    std::vector<bool> movement_mask;
    std::vector<bool> attack_mask;

    BattlePhase phase{BattlePhase::INIT};
    TacticalSubState tactical_sub_state{TacticalSubState::IDLE};
    int32_t battle_round{0};
    int32_t current_fighter_idx{0};
    int32_t selected_fighter_idx{-1};   // 玩家当前选中的单位 / Player-selected unit (for highlight+range)
    int32_t battle_phase_counter{0};
    int32_t selected_skill_idx{-1};
    int32_t selected_item_idx{-1};

    bool is_over{false};
    bool is_victory{false};

    int32_t max_rounds{0};
    int32_t victory_condition{0};   // 0=all dead, 1=protect, 2=survive, 3=boss, 4=reach, 5=rescue
    int32_t failure_condition{0};

    // 胜利条件参数 / Victory condition parameters
    int32_t protect_target_idx{-1};     // people_idx of VIP to protect (condition 1)
    int32_t survive_round_target{0};    // rounds to survive (condition 2)
    int32_t boss_target_idx{-1};        // people_idx of boss to defeat (condition 3)
    int32_t reach_target_people_idx{-1}; // people_idx of fighter who must reach (condition 4)
    int32_t reach_target_x{-1};         // target grid X (condition 4)
    int32_t reach_target_y{-1};         // target grid Y (condition 4)
    int32_t rescue_target_idx{-1};      // people_idx of target to rescue (condition 5)

    static constexpr int32_t EVENT_BUFFER_SIZE = 64;
    std::array<BattleEvent, EVENT_BUFFER_SIZE> events;
    int32_t event_count{0};

    void clear_events() { event_count = 0; }
    void push_event(const BattleEvent& ev) {
        if (event_count < EVENT_BUFFER_SIZE) {
            events[event_count++] = ev;
        }
    }

    // 动态网格辅助 / Dynamic-grid helpers（以 MAP.DAT 地图尺寸为准）
    int32_t grid_cells() const { return map_width * map_height; }
    bool in_grid(int32_t x, int32_t y) const {
        return x >= 0 && x < map_width && y >= 0 && y < map_height;
    }
};

}  // namespace dlzr::logic
