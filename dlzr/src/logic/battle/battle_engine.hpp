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
    void init(const core::FightData& fight, const core::GameData& game,
              int32_t difficulty_level = 1);

    void execute_command(const BattleCommand& cmd);

    void advance_turn();

    bool needs_ai_input() const;
    void execute_ai_turn();

    const BattleState& state() const { return state_; }
    bool is_over() const { return state_.is_over; }
    bool is_victory() const { return state_.is_victory; }

    void clear_events() { state_.clear_events(); }

    int32_t compute_score() const;

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
    const core::GameData* game_{nullptr};
    int32_t difficulty_level_{1};

    void init_fighters();
    void init_grid();

    int32_t find_next_active_fighter(int32_t from_idx);
    bool is_fighter_alive(int32_t fighter_idx) const;
    int32_t count_alive_on_team(int32_t team) const;

    void execute_move(const BattleCommand& cmd);
    void execute_attack(const BattleCommand& cmd);
    void execute_skill(const BattleCommand& cmd);
    void execute_wait(const BattleCommand& cmd);
    void execute_place_mine(const BattleCommand& cmd);

    void apply_damage(int32_t attacker_idx, int32_t defender_idx);
    void process_death(int32_t fighter_idx);
    void award_kill_exp(int32_t killer_idx, int32_t victim_idx);

    void check_mine_trigger(int32_t fighter_idx, int32_t grid_x, int32_t grid_y);

    void check_victory_conditions();
    bool check_all_enemies_dead();
    bool check_protect_target();
    bool check_survive_rounds();
    bool check_boss_defeated();
    bool check_reach_position();
    bool check_rescue_target();

    BattleCommand generate_enemy_command(int32_t fighter_idx);
    int32_t find_nearest_enemy(int32_t fighter_idx);

    const core::FightPeople& get_fight_people(int32_t fighter_idx) const;
    const core::ClassType& get_class_type(int32_t class_idx) const;
};

}  // namespace dlzr::logic
