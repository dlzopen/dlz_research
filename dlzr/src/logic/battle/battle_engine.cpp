// battle_engine.cpp — 战斗引擎实现
// Battle engine implementation

#include "logic/battle/battle_engine.hpp"

#include <algorithm>
#include <climits>

namespace dlzr::logic {

void BattleEngine::init(const core::FightData& fight, const core::GameData& game,
                         int32_t difficulty_level) {
    fight_ = &fight;
    game_ = &game;
    difficulty_level_ = difficulty_level;
    state_ = BattleState{};

    init_grid();
    init_fighters();

    state_.phase = BattlePhase::PLAYER_TURN;
    state_.current_fighter_idx = find_next_active_fighter(0);
}

void BattleEngine::init_grid() {
    state_.grid_terrain = fight_->intro.grid_terrain;
    state_.grid_param_x = fight_->intro.grid_param_x;
    state_.grid_param_y = fight_->intro.grid_param_y;
}

void BattleEngine::init_fighters() {
    state_.fighter_count = 0;

    // 难度系数 / Difficulty multiplier (percent, 100 = normal)
    int32_t diff_pct = 100;
    if (difficulty_level_ == 0) diff_pct = 80;
    else if (difficulty_level_ == 2) diff_pct = 130;

    for (int i = 0; i < 128; ++i) {
        const auto& src = fight_->fighters[i];
        if (src.stage == 0 && src.pos_x == 999) {
            continue;
        }

        auto& rt = state_.fighters[i];
        rt.grid_x = src.pos_x;
        rt.grid_y = src.pos_y;
        rt.direction = src.direction;
        rt.is_active = (src.stage != 0);
        rt.stamina = src.stamina;
        rt.movement = src.movement;
        rt.people_idx = i;

        const auto& fp = fight_->people[i];
        rt.team = fp.enemy;

        rt.stats.max_hp = src.stamina;
        rt.stats.current_hp = src.stamina;
        rt.stats.morale = src.morale;
        rt.stats.equip_integrity = 100;
        rt.stats.defense_modifier = src.stat3;

        if (rt.team != 0 && rt.is_active) {
            rt.stats.max_hp = src.stamina * diff_pct / 100;
            rt.stats.current_hp = rt.stats.max_hp;
            rt.stats.morale = src.morale * diff_pct / 100;
        }

        ++state_.fighter_count;
    }
}

int32_t BattleEngine::find_next_active_fighter(int32_t from_idx) {
    for (int i = from_idx; i < BattleState::MAX_FIGHTERS; ++i) {
        if (state_.fighters[i].is_active && is_fighter_alive(i)) {
            return i;
        }
    }
    return -1;
}

bool BattleEngine::is_fighter_alive(int32_t fighter_idx) const {
    return state_.fighters[fighter_idx].is_active
        && state_.fighters[fighter_idx].stats.current_hp > 0;
}

int32_t BattleEngine::count_alive_on_team(int32_t team) const {
    int32_t count = 0;
    for (int i = 0; i < BattleState::MAX_FIGHTERS; ++i) {
        if (state_.fighters[i].team == team && is_fighter_alive(i)) {
            ++count;
        }
    }
    return count;
}

const core::FightPeople& BattleEngine::get_fight_people(int32_t fighter_idx) const {
    return fight_->people[fighter_idx];
}

const core::ClassType& BattleEngine::get_class_type(int32_t class_idx) const {
    if (class_idx >= 0 && class_idx < core::MAX_CLASS_TYPES) {
        return game_->class_types[class_idx];
    }
    return game_->class_types[0];
}

void BattleEngine::execute_command(const BattleCommand& cmd) {
    if (state_.is_over) return;

    switch (cmd.type) {
    case BattleCommand::MOVE:
        execute_move(cmd);
        break;
    case BattleCommand::ATTACK:
        execute_attack(cmd);
        break;
    case BattleCommand::SKILL:
        execute_skill(cmd);
        break;
    case BattleCommand::WAIT:
        execute_wait(cmd);
        break;
    case BattleCommand::PLACE_MINE:
        execute_place_mine(cmd);
        break;
    default:
        break;
    }

    state_.fighters[cmd.fighter_idx].has_acted = true;
    check_victory_conditions();
}

void BattleEngine::execute_move(const BattleCommand& cmd) {
    auto& fighter = state_.fighters[cmd.fighter_idx];
    fighter.grid_x = cmd.target_x;
    fighter.grid_y = cmd.target_y;

    BattleEvent ev;
    ev.type = BattleEvent::FIGHTER_MOVED;
    ev.source_idx = cmd.fighter_idx;
    ev.target_x = cmd.target_x;
    ev.target_y = cmd.target_y;
    state_.push_event(ev);

    check_mine_trigger(cmd.fighter_idx, cmd.target_x, cmd.target_y);
}

void BattleEngine::execute_attack(const BattleCommand& cmd) {
    apply_damage(cmd.fighter_idx, cmd.target_idx);
}

void BattleEngine::execute_skill(const BattleCommand& cmd) {
    const auto& fp = get_fight_people(cmd.fighter_idx);

    core::People person{};
    person.strength = fp.strength;
    person.intelligence = fp.intelligence;
    person.level = fp.level;
    person.class_type = fp.class_type;

    int32_t skill_level = 0;
    if (cmd.skill_idx >= 0 && cmd.skill_idx < core::MAX_SKILLS) {
        const auto& skill = game_->skills[cmd.skill_idx];
        auto effect = calc_skill_effect(person, skill, skill_level);

        if (cmd.target_idx >= 0) {
            auto& target = state_.fighters[cmd.target_idx];

            if (effect.hp_effect > 0) {
                target.stats.current_hp -= effect.hp_effect;
            } else if (effect.hp_effect < 0) {
                target.stats.current_hp -= effect.hp_effect;

                // 治疗技能累积结局标记 / Healing skills accumulate ending flags
                // 主动治疗（治疗他人）+2 / Active heal (healing others) +2
                if (target.team == 0) {  // Only for allies
                    // Note: In a full implementation, this would update GameStateManager's ending flags
                    // For now, this is a placeholder for the ending flag accumulation logic
                }
            }

            target.stats.morale = std::max(0, target.stats.morale - effect.morale_effect);

            BattleEvent ev;
            ev.type = BattleEvent::SKILL_USED;
            ev.source_idx = cmd.fighter_idx;
            ev.target_idx = cmd.target_idx;
            ev.value = effect.hp_effect;
            state_.push_event(ev);

            if (target.stats.current_hp <= 0) {
                process_death(cmd.target_idx);
                award_kill_exp(cmd.fighter_idx, cmd.target_idx);
            }
        }
    }
}

void BattleEngine::execute_wait(const BattleCommand& cmd) {
    (void)cmd;
}

void BattleEngine::execute_place_mine(const BattleCommand& cmd) {
    auto& fighter = state_.fighters[cmd.fighter_idx];
    const auto& fp = fight_->people[fighter.people_idx];

    if (cmd.mine_slot < 0 || cmd.mine_slot >= 8) return;
    uint8_t mine_idx = fp.mines[cmd.mine_slot];
    if (mine_idx == 0 || mine_idx >= core::MAX_DILEI) return;

    int32_t cell = cmd.target_y * BattleState::GRID_COLS + cmd.target_x;
    if (cell < 0 || cell >= 92) return;
    if (state_.grid_terrain[cell] == 99) return;
    if (state_.grid_mines[cell].mine_type != 0) return;

    state_.grid_mines[cell].mine_type = mine_idx;
    state_.grid_mines[cell].owner_team = fighter.team;

    BattleEvent ev;
    ev.type = BattleEvent::MINE_PLACED;
    ev.source_idx = cmd.fighter_idx;
    ev.target_x = cmd.target_x;
    ev.target_y = cmd.target_y;
    ev.value = mine_idx;
    state_.push_event(ev);
}

void BattleEngine::check_mine_trigger(int32_t fighter_idx, int32_t grid_x, int32_t grid_y) {
    int32_t cell = grid_y * BattleState::GRID_COLS + grid_x;
    if (cell < 0 || cell >= 92) return;

    auto& mine = state_.grid_mines[cell];
    if (mine.mine_type == 0) return;

    auto& fighter = state_.fighters[fighter_idx];
    if (fighter.team == mine.owner_team) return;

    if (mine.mine_type > 0 && mine.mine_type < core::MAX_DILEI) {
        const auto& dilei = game_->mines[mine.mine_type];
        int32_t damage = dilei.damage;

        if (dilei.effect_type == 0) {
            fighter.stats.current_hp -= damage;
        } else {
            fighter.stats.morale = std::max(0, fighter.stats.morale - damage);
        }

        BattleEvent ev;
        ev.type = BattleEvent::MINE_TRIGGERED;
        ev.source_idx = fighter_idx;
        ev.target_x = grid_x;
        ev.target_y = grid_y;
        ev.value = damage;
        state_.push_event(ev);

        if (fighter.stats.current_hp <= 0) {
            process_death(fighter_idx);
        }
    }

    mine.mine_type = 0;
    mine.owner_team = 0;
}

void BattleEngine::apply_damage(int32_t attacker_idx, int32_t defender_idx) {
    const auto& atk_fp = get_fight_people(attacker_idx);
    const auto& def_fp = get_fight_people(defender_idx);

    core::People atk_person{};
    atk_person.strength = atk_fp.strength;
    atk_person.intelligence = atk_fp.intelligence;
    atk_person.level = atk_fp.level;
    atk_person.class_type = atk_fp.class_type;

    core::People def_person{};
    def_person.strength = def_fp.strength;
    def_person.intelligence = def_fp.intelligence;
    def_person.level = def_fp.level;
    def_person.class_type = def_fp.class_type;

    const auto& atk_class = get_class_type(atk_fp.class_type);
    const auto& def_class = get_class_type(def_fp.class_type);

    const auto& atk_fighter = state_.fighters[attacker_idx];
    const auto& def_fighter = state_.fighters[defender_idx];

    int32_t atk_power = calc_attack_power(atk_person, atk_class, atk_fighter.stats, 0, true);
    int32_t def_power = calc_defense(def_person, def_class, def_fighter.stats, 0, true);
    int32_t damage = calc_damage(atk_power, def_power);

    auto& target = state_.fighters[defender_idx];
    target.stats.current_hp -= damage;

    BattleEvent ev;
    ev.type = BattleEvent::DAMAGE_DEALT;
    ev.source_idx = attacker_idx;
    ev.target_idx = defender_idx;
    ev.value = damage;
    state_.push_event(ev);

    if (target.stats.current_hp <= 0) {
        process_death(defender_idx);
        award_kill_exp(attacker_idx, defender_idx);
    }
}

void BattleEngine::process_death(int32_t fighter_idx) {
    auto& fighter = state_.fighters[fighter_idx];
    fighter.stats.current_hp = 0;
    fighter.is_active = false;

    BattleEvent ev;
    ev.type = BattleEvent::FIGHTER_DIED;
    ev.target_idx = fighter_idx;
    state_.push_event(ev);
}

void BattleEngine::award_kill_exp(int32_t killer_idx, int32_t victim_idx) {
    const auto& victim_fp = get_fight_people(victim_idx);
    int32_t killer_level = get_fight_people(killer_idx).level;
    int32_t victim_level = victim_fp.level;
    int32_t base_exp = victim_level * 10;

    int32_t exp = calc_experience(killer_level, victim_level, base_exp);
    (void)exp;
}

void BattleEngine::advance_turn() {
    int32_t next = find_next_active_fighter(state_.current_fighter_idx + 1);
    if (next < 0) {
        state_.battle_round++;
        state_.current_fighter_idx = find_next_active_fighter(0);

        for (auto& f : state_.fighters) {
            f.has_acted = false;
        }

        BattleEvent ev;
        ev.type = BattleEvent::ROUND_ADVANCED;
        ev.value = state_.battle_round;
        state_.push_event(ev);
    } else {
        state_.current_fighter_idx = next;
    }

    check_victory_conditions();
}

bool BattleEngine::needs_ai_input() const {
    if (state_.is_over) return false;
    int32_t idx = state_.current_fighter_idx;
    if (idx < 0 || idx >= BattleState::MAX_FIGHTERS) return false;
    const auto& fighter = state_.fighters[idx];
    return fighter.is_active && fighter.team != 0 && !fighter.has_acted;
}

void BattleEngine::execute_ai_turn() {
    if (!needs_ai_input()) return;
    BattleCommand cmd = generate_enemy_command(state_.current_fighter_idx);
    execute_command(cmd);
    advance_turn();
}

int32_t BattleEngine::find_nearest_enemy(int32_t fighter_idx) {
    const auto& fighter = state_.fighters[fighter_idx];
    int32_t best_idx = -1;
    int32_t best_dist = INT32_MAX;

    for (int i = 0; i < BattleState::MAX_FIGHTERS; ++i) {
        if (i == fighter_idx) continue;
        if (!is_fighter_alive(i)) continue;
        if (state_.fighters[i].team == fighter.team) continue;

        int32_t dx = state_.fighters[i].grid_x - fighter.grid_x;
        int32_t dy = state_.fighters[i].grid_y - fighter.grid_y;
        int32_t dist = dx * dx + dy * dy;
        if (dist < best_dist) {
            best_dist = dist;
            best_idx = i;
        }
    }
    return best_idx;
}

BattleCommand BattleEngine::generate_enemy_command(int32_t fighter_idx) {
    BattleCommand cmd;
    cmd.fighter_idx = fighter_idx;

    int32_t target = find_nearest_enemy(fighter_idx);
    if (target < 0) {
        cmd.type = BattleCommand::WAIT;
        return cmd;
    }

    auto& fighter = state_.fighters[fighter_idx];
    const auto& target_fighter = state_.fighters[target];

    int32_t dx = target_fighter.grid_x - fighter.grid_x;
    int32_t dy = target_fighter.grid_y - fighter.grid_y;
    int32_t dist_sq = dx * dx + dy * dy;

    if (dist_sq <= 4) {
        cmd.type = BattleCommand::ATTACK;
        cmd.target_idx = target;
    } else {
        cmd.type = BattleCommand::MOVE;
        int32_t step_x = (dx > 0) ? 1 : (dx < 0) ? -1 : 0;
        int32_t step_y = (dy > 0) ? 1 : (dy < 0) ? -1 : 0;
        cmd.target_x = fighter.grid_x + step_x;
        cmd.target_y = fighter.grid_y + step_y;
    }

    return cmd;
}

void BattleEngine::check_victory_conditions() {
    if (state_.is_over) return;

    int32_t player_alive = count_alive_on_team(0);
    int32_t enemy_alive = count_alive_on_team(1);

    if (player_alive == 0) {
        state_.is_over = true;
        state_.is_victory = false;
        state_.phase = BattlePhase::DEFEAT;
        BattleEvent ev;
        ev.type = BattleEvent::BATTLE_LOST;
        state_.push_event(ev);
        return;
    }

    if (enemy_alive == 0) {
        state_.is_over = true;
        state_.is_victory = true;
        state_.phase = BattlePhase::VICTORY;
        BattleEvent ev;
        ev.type = BattleEvent::BATTLE_WON;
        state_.push_event(ev);
        return;
    }
}

int32_t BattleEngine::compute_score() const {
    return compute_score_breakdown().total;
}

BattleEngine::ScoreBreakdown BattleEngine::compute_score_breakdown() const {
    ScoreBreakdown result;
    result.rounds = state_.battle_round;

    if (!state_.is_victory) return result;

    for (int i = 0; i < BattleState::MAX_FIGHTERS; ++i) {
        const auto& f = state_.fighters[i];
        if (!f.is_active) continue;
        if (f.team != 0 && f.stats.current_hp <= 0) ++result.enemy_kills;
        if (f.team == 0 && f.stats.current_hp <= 0) ++result.player_losses;
    }

    result.base = 50;
    result.kill_bonus = result.enemy_kills * 10;
    result.loss_penalty = result.player_losses * 5;
    result.round_bonus = (state_.battle_round < 15) ? (30 - state_.battle_round * 2) : 0;

    if (difficulty_level_ == 0) result.difficulty_pct = 80;
    else if (difficulty_level_ == 2) result.difficulty_pct = 130;

    int32_t raw = result.base + result.kill_bonus - result.loss_penalty + result.round_bonus;
    if (raw < 0) raw = 0;
    result.total = raw * result.difficulty_pct / 100;

    return result;
}

}  // namespace dlzr::logic
