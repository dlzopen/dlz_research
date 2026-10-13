// battle_engine.cpp — 战斗引擎实现
// Battle engine implementation

#include "logic/battle/battle_engine.hpp"

#include <algorithm>
#include <climits>
#include <cstring>
#include <queue>

namespace dlzr::logic {

void BattleEngine::init(const core::FightData& fight, core::GameData& game,
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

    state_.max_rounds = state_.fighter_count * 2 - 2;
    if (state_.max_rounds < 2) state_.max_rounds = 2;
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

int32_t BattleEngine::get_movement_range(int32_t fighter_idx) const {
    if (fighter_idx < 0 || fighter_idx >= BattleState::MAX_FIGHTERS) return 0;
    const auto& fighter = state_.fighters[fighter_idx];
    if (!fighter.is_active) return 0;

    const auto& fp = fight_->people[fighter.people_idx];
    const auto& cls = get_class_type(fp.class_type);
    int32_t base_range = cls.movement;
    if (base_range <= 0) base_range = 3;

    int32_t move_pct = fighter.movement;
    if (move_pct <= 0) move_pct = 100;

    return std::max(1, base_range * move_pct / 100);
}

int32_t BattleEngine::get_attack_range(int32_t fighter_idx) const {
    if (fighter_idx < 0 || fighter_idx >= BattleState::MAX_FIGHTERS) return 1;
    const auto& fighter = state_.fighters[fighter_idx];
    if (!fighter.is_active) return 1;

    const auto& fp = fight_->people[fighter.people_idx];
    const auto& cls = get_class_type(fp.class_type);
    int32_t range = cls.select_range;
    if (range <= 0) range = 1;
    if (range > 5) range = 5;
    return range;
}

void BattleEngine::calc_movement_range(int32_t fighter_idx) {
    state_.movement_mask.fill(false);

    if (fighter_idx < 0 || fighter_idx >= BattleState::MAX_FIGHTERS) return;
    const auto& fighter = state_.fighters[fighter_idx];
    if (!fighter.is_active) return;

    int32_t max_dist = get_movement_range(fighter_idx);
    int32_t start_x = fighter.grid_x;
    int32_t start_y = fighter.grid_y;

    struct Pos { int32_t x, y, dist; };
    std::queue<Pos> bfs;
    std::array<bool, BattleState::GRID_COLS * BattleState::GRID_ROWS> visited{};

    int32_t start_cell = start_y * BattleState::GRID_COLS + start_x;
    if (start_cell < 0 || start_cell >= BattleState::GRID_COLS * BattleState::GRID_ROWS) return;

    visited[start_cell] = true;
    bfs.push({start_x, start_y, 0});
    state_.movement_mask[start_cell] = true;

    constexpr int dx[] = {0, 0, -1, 1};
    constexpr int dy[] = {-1, 1, 0, 0};

    while (!bfs.empty()) {
        auto [cx, cy, cd] = bfs.front();
        bfs.pop();

        if (cd >= max_dist) continue;

        for (int d = 0; d < 4; ++d) {
            int32_t nx = cx + dx[d];
            int32_t ny = cy + dy[d];
            if (nx < 0 || nx >= BattleState::GRID_COLS) continue;
            if (ny < 0 || ny >= BattleState::GRID_ROWS) continue;

            int32_t ncell = ny * BattleState::GRID_COLS + nx;
            if (visited[ncell]) continue;
            if (state_.grid_terrain[ncell] == 99) continue;

            bool blocked = false;
            for (int i = 0; i < BattleState::MAX_FIGHTERS; ++i) {
                if (i == fighter_idx) continue;
                const auto& f = state_.fighters[i];
                if (f.is_active && is_fighter_alive(i) &&
                    f.grid_x == nx && f.grid_y == ny) {
                    blocked = true;
                    break;
                }
            }
            if (blocked) continue;

            visited[ncell] = true;
            state_.movement_mask[ncell] = true;
            bfs.push({nx, ny, cd + 1});
        }
    }
}

void BattleEngine::calc_attack_range(int32_t fighter_idx) {
    state_.attack_mask.fill(false);

    if (fighter_idx < 0 || fighter_idx >= BattleState::MAX_FIGHTERS) return;
    const auto& fighter = state_.fighters[fighter_idx];
    if (!fighter.is_active) return;

    int32_t range = get_attack_range(fighter_idx);
    int32_t cx = fighter.grid_x;
    int32_t cy = fighter.grid_y;

    for (int y = 0; y < BattleState::GRID_ROWS; ++y) {
        for (int x = 0; x < BattleState::GRID_COLS; ++x) {
            int32_t dx = x - cx;
            int32_t dy = y - cy;
            int32_t dist = dx * dx + dy * dy;
            if (dist > 0 && dist <= range * range) {
                state_.attack_mask[y * BattleState::GRID_COLS + x] = true;
            }
        }
    }
}

bool BattleEngine::is_valid_move_target(int32_t grid_x, int32_t grid_y) const {
    if (grid_x < 0 || grid_x >= BattleState::GRID_COLS) return false;
    if (grid_y < 0 || grid_y >= BattleState::GRID_ROWS) return false;
    int32_t cell = grid_y * BattleState::GRID_COLS + grid_x;
    return state_.movement_mask[cell];
}

bool BattleEngine::is_in_attack_range(int32_t attacker_idx,
                                       int32_t target_x, int32_t target_y) const {
    if (attacker_idx < 0 || attacker_idx >= BattleState::MAX_FIGHTERS) return false;
    const auto& fighter = state_.fighters[attacker_idx];
    int32_t range = get_attack_range(attacker_idx);
    int32_t dx = target_x - fighter.grid_x;
    int32_t dy = target_y - fighter.grid_y;
    return (dx * dx + dy * dy) <= range * range;
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
    case BattleCommand::USE_ITEM:
        execute_use_item(cmd);
        break;
    default:
        break;
    }

    state_.fighters[cmd.fighter_idx].has_acted = true;
    check_victory_conditions();
}

void BattleEngine::execute_move(const BattleCommand& cmd) {
    auto& fighter = state_.fighters[cmd.fighter_idx];

    calc_movement_range(cmd.fighter_idx);
    if (!is_valid_move_target(cmd.target_x, cmd.target_y)) {
        return;
    }

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
    if (cmd.target_idx < 0 || cmd.target_idx >= BattleState::MAX_FIGHTERS) return;
    const auto& target = state_.fighters[cmd.target_idx];
    if (!is_in_attack_range(cmd.fighter_idx, target.grid_x, target.grid_y)) {
        return;
    }
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
        int32_t skill_exp = (&fp.skill_attack)[cmd.skill_idx];
        skill_level = get_skill_level(skill_exp);
        auto effect = calc_skill_effect(person, skill, skill_level);

        if (cmd.target_idx >= 0) {
            auto& target = state_.fighters[cmd.target_idx];

            if (effect.hp_effect > 0) {
                target.stats.current_hp -= effect.hp_effect;
            } else if (effect.hp_effect < 0) {
                target.stats.current_hp -= effect.hp_effect;

                if (target.team == 0 && game_) {
                    int32_t healer_pidx = state_.fighters[cmd.fighter_idx].people_idx;
                    int32_t target_pidx = state_.fighters[cmd.target_idx].people_idx;
                    if (healer_pidx == 0 && target_pidx == 7) {
                        game_->ending_flag_protagonist += 2;
                    } else if (healer_pidx == 7 && target_pidx == 0) {
                        game_->ending_flag_protagonist += 1;
                    } else if (healer_pidx == 8 && target_pidx == 7) {
                        game_->ending_flag_xiaofei += 2;
                    } else if (healer_pidx == 7 && target_pidx == 8) {
                        game_->ending_flag_xiaofei += 1;
                    }
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

void BattleEngine::execute_use_item(const BattleCommand& cmd) {
    if (cmd.item_idx < 0 || cmd.item_idx >= core::MAX_WUPIN) return;

    auto& fighter = state_.fighters[cmd.fighter_idx];
    const auto& wupin = game_->items[cmd.item_idx];

    if (wupin.para1 != 0) {
        fighter.stats.current_hp = std::min(fighter.stats.max_hp,
                                             fighter.stats.current_hp + wupin.para1);
    }
    if (wupin.para2 != 0) {
        fighter.stats.morale = std::min(100, fighter.stats.morale + wupin.para2);
    }

    BattleEvent ev;
    ev.type = BattleEvent::ITEM_USED;
    ev.source_idx = cmd.fighter_idx;
    ev.value = cmd.item_idx;
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

    // 地形防御加成 / Terrain defense bonus for defender
    int32_t def_cell = def_fighter.grid_y * BattleState::GRID_COLS + def_fighter.grid_x;
    int32_t terrain_type = (def_cell >= 0 && def_cell < 92) ? state_.grid_terrain[def_cell] : 0;
    int32_t terrain_bonus = get_terrain_defense_bonus(terrain_type);

    // 物品攻防加成 / Item attack/defense bonuses (simplified: 0 for now)
    // TODO: Implement get_item_attack_bonus() and get_item_defense_bonus() from equipped items
    int32_t item_atk_bonus = 0;
    int32_t item_def_bonus = 0;

    // 攻击者技能等级 / Attacker's skill level for attack
    int32_t atk_skill_exp = (&atk_fp.skill_attack)[0];  // Basic attack skill
    int32_t atk_skill_level = get_skill_level(atk_skill_exp);

    int32_t atk_power = calc_attack_power(atk_person, atk_class, atk_fighter.stats, atk_skill_level, true, item_atk_bonus);
    int32_t def_power = calc_defense(def_person, def_class, def_fighter.stats, terrain_bonus, true, item_def_bonus);
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

    if (game_ && fighter.team == 0) {
        int32_t pidx = fighter.people_idx;
        if (pidx != 0 && pidx < 0x20) {
            bool apply = false;
            if (difficulty_level_ == 2) {
                apply = true;
            } else if (difficulty_level_ == 1 && death_penalty_count_ < 3) {
                apply = true;
            }
            if (apply) {
                auto& protagonist = game_->people[0];
                if (protagonist.level > 1) {
                    protagonist.level -= 1;
                }
                if (state_.fighters[0].stats.current_hp >
                    state_.fighters[0].stats.max_hp) {
                    state_.fighters[0].stats.current_hp =
                        state_.fighters[0].stats.max_hp;
                }
                ++death_penalty_count_;
            }
        }
    }

    BattleEvent ev;
    ev.type = BattleEvent::FIGHTER_DIED;
    ev.target_idx = fighter_idx;
    state_.push_event(ev);
}

void BattleEngine::award_kill_exp(int32_t killer_idx, int32_t victim_idx) {
    const auto& killer_fighter = state_.fighters[killer_idx];
    if (killer_fighter.team != 0 || !game_) return;

    int32_t people_idx = killer_fighter.people_idx;
    if (people_idx < 0 || people_idx >= core::MAX_PEOPLE) return;

    const auto& victim_fp = get_fight_people(victim_idx);
    int32_t killer_level = get_fight_people(killer_idx).level;
    int32_t victim_level = victim_fp.level;
    int32_t base_exp = victim_level * 10;

    int32_t exp = calc_experience(killer_level, victim_level, base_exp);
    game_->people[people_idx].experience += exp;
}

void BattleEngine::advance_turn() {
    int32_t next = find_next_active_fighter(state_.current_fighter_idx + 1);
    if (next < 0) {
        state_.battle_round++;
        check_milestone_events();
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

int32_t BattleEngine::find_nearest_enemy(int32_t fighter_idx) const {
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

int32_t BattleEngine::find_best_attack_target(int32_t fighter_idx) {
    const auto& fighter = state_.fighters[fighter_idx];
    int32_t range = get_attack_range(fighter_idx);
    int32_t best_idx = -1;
    int32_t best_score = INT32_MIN;

    for (int i = 0; i < BattleState::MAX_FIGHTERS; ++i) {
        if (i == fighter_idx) continue;
        if (!is_fighter_alive(i)) continue;
        if (state_.fighters[i].team == fighter.team) continue;

        const auto& target = state_.fighters[i];
        int32_t dx = target.grid_x - fighter.grid_x;
        int32_t dy = target.grid_y - fighter.grid_y;
        int32_t dist_sq = dx * dx + dy * dy;
        if (dist_sq > range * range) continue;

        int32_t score = 0;
        if (target.stats.current_hp <= 30) score += 50;
        score -= target.stats.current_hp;
        score += (100 - target.stats.morale) / 2;

        if (score > best_score) {
            best_score = score;
            best_idx = i;
        }
    }
    return best_idx;
}

int32_t BattleEngine::bfs_step_toward(int32_t from_x, int32_t from_y,
                                       int32_t to_x, int32_t to_y,
                                       int32_t max_dist) {
    struct Pos { int32_t x, y; };
    std::queue<Pos> bfs;
    std::array<bool, BattleState::GRID_COLS * BattleState::GRID_ROWS> visited{};
    std::array<int32_t, BattleState::GRID_COLS * BattleState::GRID_ROWS> first_step{};
    first_step.fill(-1);

    int32_t start_cell = from_y * BattleState::GRID_COLS + from_x;
    visited[start_cell] = true;
    bfs.push({from_x, from_y});

    constexpr int dx[] = {0, 0, -1, 1};
    constexpr int dy[] = {-1, 1, 0, 0};

    while (!bfs.empty()) {
        auto [cx, cy] = bfs.front();
        bfs.pop();

        int32_t ccell = cy * BattleState::GRID_COLS + cx;
        int32_t cdist = 0;
        {
            int32_t ddx = cx - from_x;
            int32_t ddy = cy - from_y;
            cdist = (ddx < 0 ? -ddx : ddx) + (ddy < 0 ? -ddy : ddy);
        }
        if (cdist >= max_dist) continue;

        for (int d = 0; d < 4; ++d) {
            int32_t nx = cx + dx[d];
            int32_t ny = cy + dy[d];
            if (nx < 0 || nx >= BattleState::GRID_COLS) continue;
            if (ny < 0 || ny >= BattleState::GRID_ROWS) continue;

            int32_t ncell = ny * BattleState::GRID_COLS + nx;
            if (visited[ncell]) continue;
            if (state_.grid_terrain[ncell] == 99) continue;

            bool blocked = false;
            for (int i = 0; i < BattleState::MAX_FIGHTERS; ++i) {
                const auto& f = state_.fighters[i];
                if (f.is_active && is_fighter_alive(i) &&
                    f.grid_x == nx && f.grid_y == ny) {
                    blocked = true;
                    break;
                }
            }
            if (blocked) continue;

            visited[ncell] = true;
            first_step[ncell] = (cdist == 0) ? d : first_step[ccell];
            bfs.push({nx, ny});

            if (nx == to_x && ny == to_y) {
                int32_t step = first_step[ncell];
                if (step >= 0 && step < 4) {
                    return step;
                }
            }
        }
    }

    int32_t best_cell = -1;
    int32_t best_dist = INT32_MAX;
    for (int y = 0; y < BattleState::GRID_ROWS; ++y) {
        for (int x = 0; x < BattleState::GRID_COLS; ++x) {
            int32_t cell = y * BattleState::GRID_COLS + x;
            if (!visited[cell]) continue;
            int32_t ddx = x - to_x;
            int32_t ddy = y - to_y;
            int32_t dist = ddx * ddx + ddy * ddy;
            if (dist < best_dist) {
                best_dist = dist;
                best_cell = cell;
            }
        }
    }

    if (best_cell >= 0 && first_step[best_cell] >= 0) {
        return first_step[best_cell];
    }
    return -1;
}

BattleCommand BattleEngine::generate_enemy_command(int32_t fighter_idx) {
    BattleCommand cmd;
    cmd.fighter_idx = fighter_idx;

    const auto& fighter = state_.fighters[fighter_idx];
    uint8_t behavior = fight_->fighters[fighter_idx].behavior;

    if (should_retreat(fighter_idx)) {
        return ai_retreat(fighter_idx);
    }

    BattleCommand skill_cmd = ai_try_skill(fighter_idx);
    if (skill_cmd.type != BattleCommand::NONE) {
        return skill_cmd;
    }

    int32_t attack_target = find_best_attack_target(fighter_idx);
    if (attack_target >= 0) {
        cmd.type = BattleCommand::ATTACK;
        cmd.target_idx = attack_target;
        return cmd;
    }

    BattleCommand mine_cmd = ai_try_mine(fighter_idx);
    if (mine_cmd.type != BattleCommand::NONE) {
        return mine_cmd;
    }

    switch (behavior) {
    case 0:  // idle — hold position
        cmd.type = BattleCommand::WAIT;
        return cmd;
    case 1:  // patrol — move between patrol points
        return ai_patrol(fighter_idx);
    case 4:  // chase — aggressively pursue nearest enemy
        return ai_chase(fighter_idx);
    case 5:  // guard — stay near guard position
        return ai_guard(fighter_idx);
    case 6:  // assist — support a specific ally
        return ai_assist(fighter_idx);
    case 7:  // ambush — wait then strike
        return ai_ambush(fighter_idx);
    default: // chase by default
        return ai_chase(fighter_idx);
    }
}

bool BattleEngine::should_retreat(int32_t fighter_idx) const {
    const auto& f = state_.fighters[fighter_idx];
    if (f.stats.current_hp <= 0) return false;
    float hp_ratio = static_cast<float>(f.stats.current_hp) /
                     static_cast<float>(f.stats.max_hp);
    if (hp_ratio > 0.2f) return false;
    int32_t nearest_enemy = find_nearest_enemy(fighter_idx);
    if (nearest_enemy < 0) return false;
    const auto& enemy = state_.fighters[nearest_enemy];
    int32_t dx = f.grid_x - enemy.grid_x;
    int32_t dy = f.grid_y - enemy.grid_y;
    return (dx * dx + dy * dy) <= 9;
}

int32_t BattleEngine::find_ally_to_heal(int32_t fighter_idx) const {
    const auto& self = state_.fighters[fighter_idx];
    int32_t best_idx = -1;
    int32_t best_need = 0;

    for (int i = 0; i < BattleState::MAX_FIGHTERS; ++i) {
        if (i == fighter_idx) continue;
        const auto& ally = state_.fighters[i];
        if (!ally.is_active || ally.stats.current_hp <= 0) continue;
        if (ally.team != self.team) continue;

        int32_t missing = ally.stats.max_hp - ally.stats.current_hp;
        if (missing <= best_need) continue;

        int32_t dx = self.grid_x - ally.grid_x;
        int32_t dy = self.grid_y - ally.grid_y;
        int32_t dist_sq = dx * dx + dy * dy;
        if (dist_sq > 25) continue;

        best_need = missing;
        best_idx = i;
    }
    return best_idx;
}

BattleCommand BattleEngine::ai_try_skill(int32_t fighter_idx) {
    BattleCommand cmd;
    cmd.type = BattleCommand::NONE;
    cmd.fighter_idx = fighter_idx;

    const auto& fp = get_fight_people(fighter_idx);
    const auto& fighter = state_.fighters[fighter_idx];

    if (fp.skill_medical > 0) {
        int32_t heal_target = find_ally_to_heal(fighter_idx);
        if (heal_target >= 0) {
            const auto& target = state_.fighters[heal_target];
            int32_t dx = fighter.grid_x - target.grid_x;
            int32_t dy = fighter.grid_y - target.grid_y;
            if (dx * dx + dy * dy <= 4) {
                cmd.type = BattleCommand::SKILL;
                cmd.skill_idx = 3;  // 医疗 / Medical skill index
                cmd.target_idx = heal_target;
                cmd.target_x = target.grid_x;
                cmd.target_y = target.grid_y;
                return cmd;
            }
        }
    }

    if (fp.skill_fire > 0 || fp.skill_poison > 0) {
        int32_t best_target = -1;
        int32_t best_score = -1;
        int32_t atk_range = get_attack_range(fighter_idx);

        for (int i = 0; i < BattleState::MAX_FIGHTERS; ++i) {
            const auto& enemy = state_.fighters[i];
            if (!enemy.is_active || enemy.stats.current_hp <= 0) continue;
            if (enemy.team == fighter.team) continue;

            int32_t dx = fighter.grid_x - enemy.grid_x;
            int32_t dy = fighter.grid_y - enemy.grid_y;
            if (dx * dx + dy * dy > atk_range * atk_range + 4) continue;

            int32_t score = enemy.stats.max_hp - enemy.stats.current_hp;
            if (score > best_score) {
                best_score = score;
                best_target = i;
            }
        }

        if (best_target >= 0) {
            const auto& target = state_.fighters[best_target];
            cmd.type = BattleCommand::SKILL;
            cmd.skill_idx = (fp.skill_fire > 0) ? 4 : 15;  // 火攻=4, 施毒=15
            cmd.target_idx = best_target;
            cmd.target_x = target.grid_x;
            cmd.target_y = target.grid_y;
            return cmd;
        }
    }

    if (fp.skill_intimidate > 0) {
        int32_t atk_range = get_attack_range(fighter_idx);
        int32_t best_target = -1;
        int32_t best_morale = -1;

        for (int i = 0; i < BattleState::MAX_FIGHTERS; ++i) {
            const auto& enemy = state_.fighters[i];
            if (!enemy.is_active || enemy.stats.current_hp <= 0) continue;
            if (enemy.team == fighter.team) continue;

            int32_t dx = fighter.grid_x - enemy.grid_x;
            int32_t dy = fighter.grid_y - enemy.grid_y;
            if (dx * dx + dy * dy > atk_range * atk_range + 4) continue;

            if (enemy.stats.morale > best_morale) {
                best_morale = enemy.stats.morale;
                best_target = i;
            }
        }

        if (best_target >= 0) {
            const auto& target = state_.fighters[best_target];
            cmd.type = BattleCommand::SKILL;
            cmd.skill_idx = 7;  // 威慑 / Intimidate skill index
            cmd.target_idx = best_target;
            cmd.target_x = target.grid_x;
            cmd.target_y = target.grid_y;
            return cmd;
        }
    }

    return cmd;
}

BattleCommand BattleEngine::ai_try_mine(int32_t fighter_idx) {
    BattleCommand cmd;
    cmd.type = BattleCommand::NONE;
    cmd.fighter_idx = fighter_idx;

    const auto& fp = get_fight_people(fighter_idx);
    int32_t mine_type = -1;
    int32_t mine_slot_idx = -1;
    for (int i = 0; i < 8; ++i) {
        if (fp.mines[i] > 0) {
            mine_type = fp.mines[i];
            mine_slot_idx = i;
            break;
        }
    }
    if (mine_type < 0) return cmd;

    const auto& fighter = state_.fighters[fighter_idx];
    int32_t nearest_enemy = find_nearest_enemy(fighter_idx);
    if (nearest_enemy < 0) return cmd;

    const auto& enemy = state_.fighters[nearest_enemy];
    int32_t dx = fighter.grid_x - enemy.grid_x;
    int32_t dy = fighter.grid_y - enemy.grid_y;
    if (dx * dx + dy * dy > 16) return cmd;

    int32_t best_x = -1, best_y = -1;
    int32_t best_score = -1;

    for (int oy = -1; oy <= 1; ++oy) {
        for (int ox = -1; ox <= 1; ++ox) {
            if (ox == 0 && oy == 0) continue;
            int32_t mx = enemy.grid_x + ox;
            int32_t my = enemy.grid_y + oy;
            if (mx < 0 || mx >= BattleState::GRID_COLS) continue;
            if (my < 0 || my >= BattleState::GRID_ROWS) continue;

            int32_t cell = my * BattleState::GRID_COLS + mx;
            if (state_.grid_mines[cell].mine_type > 0) continue;
            if (state_.fighters[fighter_idx].grid_x == mx &&
                state_.fighters[fighter_idx].grid_y == my) continue;

            int32_t score = 10;
            for (int i = 0; i < BattleState::MAX_FIGHTERS; ++i) {
                const auto& f = state_.fighters[i];
                if (!f.is_active || f.stats.current_hp <= 0) continue;
                if (f.team == fighter.team) {
                    int32_t adx = f.grid_x - mx;
                    int32_t ady = f.grid_y - my;
                    if (adx * adx + ady * ady <= 1) { score = -1; break; }
                }
            }
            if (score < 0) continue;
            if (score > best_score) {
                best_score = score;
                best_x = mx;
                best_y = my;
            }
        }
    }

    if (best_x >= 0) {
        cmd.type = BattleCommand::PLACE_MINE;
        cmd.target_x = best_x;
        cmd.target_y = best_y;
        cmd.mine_slot = mine_slot_idx;
        return cmd;
    }

    return cmd;
}

BattleCommand BattleEngine::ai_patrol(int32_t fighter_idx) {
    BattleCommand cmd;
    cmd.fighter_idx = fighter_idx;

    const auto& fighter = state_.fighters[fighter_idx];
    const auto& src = fight_->fighters[fighter_idx];
    int32_t px = src.target_pos_x;
    int32_t py = src.target_pos_y;

    if (px < 0 || px >= BattleState::GRID_COLS ||
        py < 0 || py >= BattleState::GRID_ROWS) {
        return ai_chase(fighter_idx);
    }

    int32_t dx = fighter.grid_x - px;
    int32_t dy = fighter.grid_y - py;
    if (dx * dx + dy * dy <= 1) {
        int32_t nearest = find_nearest_enemy(fighter_idx);
        if (nearest >= 0) {
            const auto& enemy = state_.fighters[nearest];
            int32_t edx = fighter.grid_x - enemy.grid_x;
            int32_t edy = fighter.grid_y - enemy.grid_y;
            if (edx * edx + edy * edy <= 4) {
                cmd.type = BattleCommand::ATTACK;
                cmd.target_idx = nearest;
                return cmd;
            }
        }
        cmd.type = BattleCommand::WAIT;
        return cmd;
    }

    int32_t step = bfs_step_toward(fighter.grid_x, fighter.grid_y, px, py,
                                    get_movement_range(fighter_idx));
    if (step >= 0 && step < 4) {
        constexpr int sdx[] = {0, 0, -1, 1};
        constexpr int sdy[] = {-1, 1, 0, 0};
        int32_t nx = fighter.grid_x + sdx[step];
        int32_t ny = fighter.grid_y + sdy[step];
        int32_t ncell = ny * BattleState::GRID_COLS + nx;
        if (nx >= 0 && nx < BattleState::GRID_COLS &&
            ny >= 0 && ny < BattleState::GRID_ROWS &&
            state_.movement_mask[ncell]) {
            cmd.type = BattleCommand::MOVE;
            cmd.target_x = nx;
            cmd.target_y = ny;
            return cmd;
        }
    }

    cmd.type = BattleCommand::WAIT;
    return cmd;
}

BattleCommand BattleEngine::ai_guard(int32_t fighter_idx) {
    BattleCommand cmd;
    cmd.fighter_idx = fighter_idx;

    const auto& fighter = state_.fighters[fighter_idx];
    const auto& src = fight_->fighters[fighter_idx];
    int32_t guard_x = src.target_pos_x;
    int32_t guard_y = src.target_pos_y;

    int32_t nearest = find_nearest_enemy(fighter_idx);
    if (nearest >= 0) {
        const auto& enemy = state_.fighters[nearest];
        int32_t edx = guard_x - enemy.grid_x;
        int32_t edy = guard_y - enemy.grid_y;
        if (edx * edx + edy * edy <= 9) {
            int32_t atk_target = find_best_attack_target(fighter_idx);
            if (atk_target >= 0) {
                cmd.type = BattleCommand::ATTACK;
                cmd.target_idx = atk_target;
                return cmd;
            }
        }
    }

    if (guard_x >= 0 && guard_x < BattleState::GRID_COLS &&
        guard_y >= 0 && guard_y < BattleState::GRID_ROWS) {
        int32_t dx = fighter.grid_x - guard_x;
        int32_t dy = fighter.grid_y - guard_y;
        if (dx * dx + dy * dy <= 4) {
            cmd.type = BattleCommand::WAIT;
            return cmd;
        }

        int32_t step = bfs_step_toward(fighter.grid_x, fighter.grid_y,
                                        guard_x, guard_y,
                                        get_movement_range(fighter_idx));
        if (step >= 0 && step < 4) {
            constexpr int sdx[] = {0, 0, -1, 1};
            constexpr int sdy[] = {-1, 1, 0, 0};
            int32_t nx = fighter.grid_x + sdx[step];
            int32_t ny = fighter.grid_y + sdy[step];
            int32_t ncell = ny * BattleState::GRID_COLS + nx;
            if (nx >= 0 && nx < BattleState::GRID_COLS &&
                ny >= 0 && ny < BattleState::GRID_ROWS &&
                state_.movement_mask[ncell]) {
                cmd.type = BattleCommand::MOVE;
                cmd.target_x = nx;
                cmd.target_y = ny;
                return cmd;
            }
        }
    }

    cmd.type = BattleCommand::WAIT;
    return cmd;
}

BattleCommand BattleEngine::ai_assist(int32_t fighter_idx) {
    BattleCommand cmd;
    cmd.fighter_idx = fighter_idx;

    const auto& fighter = state_.fighters[fighter_idx];
    const auto& src = fight_->fighters[fighter_idx];
    int32_t target_people = src.target_people;

    int32_t ally_idx = -1;
    for (int i = 0; i < BattleState::MAX_FIGHTERS; ++i) {
        if (i == fighter_idx) continue;
        const auto& ally = state_.fighters[i];
        if (!ally.is_active || ally.stats.current_hp <= 0) continue;
        if (ally.team != fighter.team) continue;
        if (target_people >= 0 && ally.people_idx == target_people) {
            ally_idx = i;
            break;
        }
    }

    if (ally_idx < 0) {
        return ai_chase(fighter_idx);
    }

    const auto& ally = state_.fighters[ally_idx];
    int32_t nearest_to_ally = -1;
    int32_t best_dist = INT32_MAX;
    for (int i = 0; i < BattleState::MAX_FIGHTERS; ++i) {
        const auto& enemy = state_.fighters[i];
        if (!enemy.is_active || enemy.stats.current_hp <= 0) continue;
        if (enemy.team == fighter.team) continue;
        int32_t dx = ally.grid_x - enemy.grid_x;
        int32_t dy = ally.grid_y - enemy.grid_y;
        int32_t dist_sq = dx * dx + dy * dy;
        if (dist_sq < best_dist) {
            best_dist = dist_sq;
            nearest_to_ally = i;
        }
    }

    if (nearest_to_ally >= 0 && best_dist <= 9) {
        int32_t atk_target = find_best_attack_target(fighter_idx);
        if (atk_target >= 0) {
            cmd.type = BattleCommand::ATTACK;
            cmd.target_idx = atk_target;
            return cmd;
        }
    }

    int32_t step = bfs_step_toward(fighter.grid_x, fighter.grid_y,
                                    ally.grid_x, ally.grid_y,
                                    get_movement_range(fighter_idx));
    if (step >= 0 && step < 4) {
        constexpr int sdx[] = {0, 0, -1, 1};
        constexpr int sdy[] = {-1, 1, 0, 0};
        int32_t nx = fighter.grid_x + sdx[step];
        int32_t ny = fighter.grid_y + sdy[step];
        int32_t ncell = ny * BattleState::GRID_COLS + nx;
        if (nx >= 0 && nx < BattleState::GRID_COLS &&
            ny >= 0 && ny < BattleState::GRID_ROWS &&
            state_.movement_mask[ncell]) {
            cmd.type = BattleCommand::MOVE;
            cmd.target_x = nx;
            cmd.target_y = ny;
            return cmd;
        }
    }

    cmd.type = BattleCommand::WAIT;
    return cmd;
}

BattleCommand BattleEngine::ai_ambush(int32_t fighter_idx) {
    BattleCommand cmd;
    cmd.fighter_idx = fighter_idx;

    const auto& fighter = state_.fighters[fighter_idx];
    const auto& src = fight_->fighters[fighter_idx];
    int32_t ambush_x = src.target_pos_x;
    int32_t ambush_y = src.target_pos_y;

    int32_t nearest = find_nearest_enemy(fighter_idx);
    if (nearest >= 0) {
        const auto& enemy = state_.fighters[nearest];
        int32_t dx = fighter.grid_x - enemy.grid_x;
        int32_t dy = fighter.grid_y - enemy.grid_y;
        if (dx * dx + dy * dy <= 4) {
            int32_t atk_target = find_best_attack_target(fighter_idx);
            if (atk_target >= 0) {
                cmd.type = BattleCommand::ATTACK;
                cmd.target_idx = atk_target;
                return cmd;
            }
        }
    }

    if (ambush_x >= 0 && ambush_x < BattleState::GRID_COLS &&
        ambush_y >= 0 && ambush_y < BattleState::GRID_ROWS) {
        int32_t dx = fighter.grid_x - ambush_x;
        int32_t dy = fighter.grid_y - ambush_y;
        if (dx * dx + dy * dy <= 1) {
            cmd.type = BattleCommand::WAIT;
            return cmd;
        }

        int32_t step = bfs_step_toward(fighter.grid_x, fighter.grid_y,
                                        ambush_x, ambush_y,
                                        get_movement_range(fighter_idx));
        if (step >= 0 && step < 4) {
            constexpr int sdx[] = {0, 0, -1, 1};
            constexpr int sdy[] = {-1, 1, 0, 0};
            int32_t nx = fighter.grid_x + sdx[step];
            int32_t ny = fighter.grid_y + sdy[step];
            int32_t ncell = ny * BattleState::GRID_COLS + nx;
            if (nx >= 0 && nx < BattleState::GRID_COLS &&
                ny >= 0 && ny < BattleState::GRID_ROWS &&
                state_.movement_mask[ncell]) {
                cmd.type = BattleCommand::MOVE;
                cmd.target_x = nx;
                cmd.target_y = ny;
                return cmd;
            }
        }
    }

    return ai_chase(fighter_idx);
}

BattleCommand BattleEngine::ai_chase(int32_t fighter_idx) {
    BattleCommand cmd;
    cmd.fighter_idx = fighter_idx;

    const auto& fighter = state_.fighters[fighter_idx];
    int32_t nearest = find_nearest_enemy(fighter_idx);
    if (nearest < 0) {
        cmd.type = BattleCommand::WAIT;
        return cmd;
    }

    const auto& target_fighter = state_.fighters[nearest];
    int32_t move_range = get_movement_range(fighter_idx);

    calc_movement_range(fighter_idx);

    int32_t atk_range = get_attack_range(fighter_idx);
    int32_t best_x = -1, best_y = -1;
    int32_t best_dist = INT32_MAX;

    for (int y = 0; y < BattleState::GRID_ROWS; ++y) {
        for (int x = 0; x < BattleState::GRID_COLS; ++x) {
            int32_t cell = y * BattleState::GRID_COLS + x;
            if (!state_.movement_mask[cell]) continue;

            int32_t dx = x - target_fighter.grid_x;
            int32_t dy = y - target_fighter.grid_y;
            int32_t dist_sq = dx * dx + dy * dy;

            if (dist_sq <= atk_range * atk_range) {
                if (dist_sq < best_dist) {
                    best_dist = dist_sq;
                    best_x = x;
                    best_y = y;
                }
            }
        }
    }

    if (best_x >= 0) {
        cmd.type = BattleCommand::MOVE;
        cmd.target_x = best_x;
        cmd.target_y = best_y;
        return cmd;
    }

    int32_t step = bfs_step_toward(fighter.grid_x, fighter.grid_y,
                                    target_fighter.grid_x, target_fighter.grid_y,
                                    move_range);
    if (step >= 0 && step < 4) {
        constexpr int sdx[] = {0, 0, -1, 1};
        constexpr int sdy[] = {-1, 1, 0, 0};
        int32_t nx = fighter.grid_x + sdx[step];
        int32_t ny = fighter.grid_y + sdy[step];
        int32_t ncell = ny * BattleState::GRID_COLS + nx;
        if (nx >= 0 && nx < BattleState::GRID_COLS &&
            ny >= 0 && ny < BattleState::GRID_ROWS &&
            state_.movement_mask[ncell]) {
            cmd.type = BattleCommand::MOVE;
            cmd.target_x = nx;
            cmd.target_y = ny;
            return cmd;
        }
    }

    cmd.type = BattleCommand::WAIT;
    return cmd;
}

BattleCommand BattleEngine::ai_retreat(int32_t fighter_idx) {
    BattleCommand cmd;
    cmd.fighter_idx = fighter_idx;

    const auto& fighter = state_.fighters[fighter_idx];
    int32_t nearest = find_nearest_enemy(fighter_idx);
    if (nearest < 0) {
        cmd.type = BattleCommand::WAIT;
        return cmd;
    }

    const auto& enemy = state_.fighters[nearest];
    calc_movement_range(fighter_idx);

    int32_t best_x = fighter.grid_x;
    int32_t best_y = fighter.grid_y;
    int32_t best_dist = 0;

    int32_t edx = fighter.grid_x - enemy.grid_x;
    int32_t edy = fighter.grid_y - enemy.grid_y;
    int32_t cur_dist = edx * edx + edy * edy;

    for (int y = 0; y < BattleState::GRID_ROWS; ++y) {
        for (int x = 0; x < BattleState::GRID_COLS; ++x) {
            int32_t cell = y * BattleState::GRID_COLS + x;
            if (!state_.movement_mask[cell]) continue;

            int32_t dx = x - enemy.grid_x;
            int32_t dy = y - enemy.grid_y;
            int32_t dist_sq = dx * dx + dy * dy;

            if (dist_sq > cur_dist && dist_sq > best_dist) {
                best_dist = dist_sq;
                best_x = x;
                best_y = y;
            }
        }
    }

    if (best_x != fighter.grid_x || best_y != fighter.grid_y) {
        cmd.type = BattleCommand::MOVE;
        cmd.target_x = best_x;
        cmd.target_y = best_y;
    } else {
        cmd.type = BattleCommand::WAIT;
    }
    return cmd;
}

void BattleEngine::check_victory_conditions() {
    if (state_.is_over) return;

    int32_t player_alive = count_alive_on_team(0);
    if (player_alive == 0) {
        end_battle(false);
        return;
    }

    switch (state_.victory_condition) {
    case 0:
        if (check_all_enemies_dead()) { end_battle(true); return; }
        break;
    case 1:
        if (!check_protect_target()) { end_battle(false); return; }
        if (check_all_enemies_dead()) { end_battle(true); return; }
        break;
    case 2:
        if (check_survive_rounds()) { end_battle(true); return; }
        break;
    case 3:
        if (check_boss_defeated()) { end_battle(true); return; }
        break;
    case 4:
        if (check_reach_position()) { end_battle(true); return; }
        break;
    case 5:
        if (check_rescue_target()) { end_battle(true); return; }
        break;
    }

    check_round_exhaustion();
}

bool BattleEngine::check_all_enemies_dead() {
    return count_alive_on_team(1) == 0;
}

bool BattleEngine::check_protect_target() {
    if (state_.protect_target_idx < 0) return true;
    for (const auto& f : state_.fighters) {
        if (f.people_idx == state_.protect_target_idx && f.team == 0 && f.is_active) {
            return f.stats.current_hp > 0;
        }
    }
    return false;
}

bool BattleEngine::check_survive_rounds() {
    if (state_.survive_round_target <= 0) return false;
    return state_.battle_round >= state_.survive_round_target;
}

bool BattleEngine::check_boss_defeated() {
    if (state_.boss_target_idx < 0) return false;
    for (const auto& f : state_.fighters) {
        if (f.people_idx == state_.boss_target_idx && f.is_active) {
            return f.stats.current_hp <= 0;
        }
    }
    return true;
}

bool BattleEngine::check_reach_position() {
    if (state_.reach_target_people_idx < 0 ||
        state_.reach_target_x < 0 || state_.reach_target_y < 0) return false;
    for (const auto& f : state_.fighters) {
        if (f.people_idx == state_.reach_target_people_idx &&
            f.team == 0 && f.is_active && f.stats.current_hp > 0) {
            return f.grid_x == state_.reach_target_x &&
                   f.grid_y == state_.reach_target_y;
        }
    }
    return false;
}

bool BattleEngine::check_rescue_target() {
    if (state_.rescue_target_idx < 0) return false;
    int32_t target_x = -1, target_y = -1;
    for (const auto& f : state_.fighters) {
        if (f.people_idx == state_.rescue_target_idx && f.is_active) {
            target_x = f.grid_x;
            target_y = f.grid_y;
            break;
        }
    }
    if (target_x < 0) return false;
    for (const auto& f : state_.fighters) {
        if (f.team == 0 && f.is_active && f.stats.current_hp > 0 &&
            f.people_idx != state_.rescue_target_idx) {
            int32_t dx = f.grid_x - target_x;
            int32_t dy = f.grid_y - target_y;
            if (dx * dx + dy * dy <= 1) return true;
        }
    }
    return false;
}

void BattleEngine::check_round_exhaustion() {
    if (state_.is_over) return;
    if (state_.max_rounds <= 0) return;

    if (state_.battle_round >= state_.max_rounds) {
        int32_t player_alive = count_alive_on_team(0);
        int32_t enemy_alive = count_alive_on_team(1);
        bool victory = player_alive >= enemy_alive;

        BattleEvent ev;
        ev.type = BattleEvent::ROUNDS_EXHAUSTED;
        ev.value = state_.battle_round;
        state_.push_event(ev);

        end_battle(victory);
    }
}

void BattleEngine::check_milestone_events() {
    int32_t round = state_.battle_round;
    if (round == 10 || round == 150 || round == 270) {
        BattleEvent ev;
        ev.type = BattleEvent::MILESTONE_REACHED;
        ev.value = round;
        state_.push_event(ev);
    }
}

void BattleEngine::end_battle(bool victory) {
    if (state_.is_over) return;

    state_.is_over = true;
    state_.is_victory = victory;
    state_.phase = victory ? BattlePhase::VICTORY : BattlePhase::DEFEAT;

    BattleEvent ev;
    ev.type = victory ? BattleEvent::BATTLE_WON : BattleEvent::BATTLE_LOST;
    state_.push_event(ev);
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
