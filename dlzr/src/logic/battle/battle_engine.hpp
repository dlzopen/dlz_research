#pragma once

// battle_engine.hpp — 战斗引擎
// Battle engine
//
// 管理战斗的初始化、回合调度、胜负判定。
// Manages battle init, turn scheduling, victory/defeat.

#include "logic/battle/battle_state.hpp"
#include "logic/combat/combat_formulas.hpp"
#include "core/data_model/game_data.hpp"

namespace dlzr::logic {

class BattleEngine {
public:
    void init(core::FightData& fight, core::GameData& game,
              int32_t difficulty_level = 1);

    // 注入 MAP.DAT 地图（须在 init 前调用；地图尺寸决定战斗网格与相机边界）。
    // 证据: FUN_0041e97a.c 以 MAP.DAT 宽高为相机钳制边界；
    // FIGHT.DAT 单位坐标即 MAP.DAT 瓦片坐标。
    void set_map(const core::FightMap* map) {
        map_ = map;
        if (fight_ != nullptr) {
            init_grid();  // 已 init 后注入：按真实 MAP.DAT 尺寸重算网格与相机
        }
    }

    void execute_command(const BattleCommand& cmd);

    void advance_turn();

    bool needs_ai_input() const;
    void execute_ai_turn();

    const BattleState& state() const { return state_; }
    bool is_over() const { return state_.is_over; }
    bool is_victory() const { return state_.is_victory; }

    void clear_events() { state_.clear_events(); }

    int32_t compute_score() const;

    void calc_movement_range(int32_t fighter_idx);
    void calc_attack_range(int32_t fighter_idx);
    bool is_valid_move_target(int32_t grid_x, int32_t grid_y) const;
    bool is_in_attack_range(int32_t attacker_idx, int32_t target_x, int32_t target_y) const;
    int32_t get_attack_range(int32_t fighter_idx) const;
    int32_t get_movement_range(int32_t fighter_idx) const;
    // 前进/追击落点选择（对齐二进制 find_nearest_free_cell：加权可达集内曼哈顿最小、严格优于起点）
    int32_t select_destination_cell(int32_t fighter_idx, int32_t to_x, int32_t to_y);
    // 撤退/防御性重定位落点选择（对齐二进制 find_best_cell_terrain FUN_0040144e：
    // 加权可达集(排除起点)内 *最大化* 曼哈顿距离到敌人 + 地形{9,10,0xb}罚9；平局取行优先首次）
    int32_t select_retreat_cell(int32_t fighter_idx);

    // 选中单位：记录选中索引并立即计算可达/攻击范围（与原版"选中即亮出可达区"一致）
    void set_selected_fighter(int32_t idx);

    struct ScoreBreakdown {
        int32_t base{0};
        int32_t kill_bonus{0};
        int32_t loss_penalty{0};
        int32_t round_bonus{0};
        int32_t difficulty_pct{100};
        int32_t total{0};
        int32_t enemy_kills{0};
        int32_t player_losses{0};
        int32_t rounds{0};
    };
    ScoreBreakdown compute_score_breakdown() const;

private:
    BattleState state_;
    const core::FightData* fight_{nullptr};
    const core::FightMap* map_{nullptr};
    core::GameData* game_{nullptr};
    int32_t difficulty_level_{1};
    int32_t death_penalty_count_{0};

    void init_fighters();
    void init_grid();

    int32_t find_next_active_fighter(int32_t from_idx);
    bool is_fighter_alive(int32_t fighter_idx) const;
    // 地质硬障碍（所有兵种均不可通行，class_move_cost[任意兵种][地形旗标] >= 99）。
    // 证据: FUN_00418e29.c 移动范围洪水填充以 class_move_cost[entered_terrain>>12][兵种]
    //       作步进代价，>=99 即不可进入；此处取"全兵种均 99"作为与兵种无关的硬墙判定。
    bool is_hard_obstacle(int32_t cell) const;
    int32_t count_alive_on_team(int32_t team) const;

    void execute_move(const BattleCommand& cmd);
    void execute_attack(const BattleCommand& cmd);
    void execute_skill(const BattleCommand& cmd);
    void execute_wait(const BattleCommand& cmd);
    void execute_place_mine(const BattleCommand& cmd);
    void execute_use_item(const BattleCommand& cmd);

    void apply_damage(int32_t attacker_idx, int32_t defender_idx);
    void process_death(int32_t fighter_idx);
    void award_kill_exp(int32_t killer_idx, int32_t victim_idx);

    void check_mine_trigger(int32_t fighter_idx, int32_t grid_x, int32_t grid_y);

    void check_victory_conditions();
    void check_round_exhaustion();
    void check_milestone_events();
    bool check_all_enemies_dead();
    bool check_protect_target();
    bool check_survive_rounds();
    bool check_boss_defeated();
    bool check_reach_position();
    bool check_rescue_target();

    void end_battle(bool victory);

    BattleCommand generate_enemy_command(int32_t fighter_idx);
    int32_t find_nearest_enemy(int32_t fighter_idx) const;
    int32_t find_best_attack_target(int32_t fighter_idx);
    int32_t bfs_step_toward(int32_t fighter_idx, int32_t to_x, int32_t to_y);

    // 敌方 AI 派发（忠实复刻 FUN_00403f8e + FUN_00402ddc + DAT_00446030 表）。
    // dispatch_ai_type 复刻 FUN_00402ddc 的单个 ai_type 分支：若触发条件满足则
    // 返回对应动作（攻击/移动/技能/撤退/待命），否则返回 NONE（交由上一层回退到
    // 表的下一列 ai_type）。ai_attack_in_place 复刻"原地攻击"(DAT_0044b230=0)。
    BattleCommand dispatch_ai_type(int32_t fighter_idx, int ai_type);
    BattleCommand ai_attack_in_place(int32_t fighter_idx);
    // 技能类 ai_type（31/41/42/51/52/61/71/72）→ 精确二进制技能 id（FUN_00402ddc 写
    // DAT_0044b240）：31→15(空袭) 41/42→13(暗杀) 51/52→14(施毒) 61/62→12(督战) 71/72→6(威慑)。
    BattleCommand ai_try_skill(int32_t fighter_idx, int32_t ai_type);
    BattleCommand ai_try_mine(int32_t fighter_idx);
    BattleCommand ai_patrol(int32_t fighter_idx);
    BattleCommand ai_guard(int32_t fighter_idx);
    BattleCommand ai_assist(int32_t fighter_idx);
    // 支援类 ai_type（7/8/17/18）：二进制 FUN_004018fb/004018cc/FUN_00401c73/FUN_00401c44
    // 以同队扫描选中友军格后施放支援技能（治疗/增益）。干净层以"治疗受伤友军(medical,
    // skill_idx=3)"近似；无 medical 技能或无受伤友军 → 返回 NONE（上层回退到下一列）。
    BattleCommand ai_support_ally(int32_t fighter_idx);
    BattleCommand ai_ambush(int32_t fighter_idx);
    BattleCommand ai_chase(int32_t fighter_idx);
    BattleCommand ai_retreat(int32_t fighter_idx);
    int32_t find_ally_to_heal(int32_t fighter_idx) const;

    const core::FightPeople& get_fight_people(int32_t fighter_idx) const;
    const core::ClassType& get_class_type(int32_t class_idx) const;

    // 测试友元：允许 test_enemy_ai_dispatch 直接设定 state_ 的 hp/士气并调用私有派发，
    // 以覆盖 hp<25% 等需手动致伤的门控路径（不引入生产代码副作用）。
    friend class EnemyAiDispatchTest;
};

}  // namespace dlzr::logic
