// battle_input_handler.cpp — 战斗输入处理实现
// Battle input handler implementation

#include "presentation/battle/battle_input_handler.hpp"

namespace dlzr::presentation {

static constexpr int32_t MENU_ITEM_HEIGHT = 24;
static constexpr int32_t MENU_WIDTH = 80;

void BattleInputHandler::init(int32_t grid_origin_x, int32_t grid_origin_y, int32_t cell_size) {
    grid_origin_x_ = grid_origin_x;
    grid_origin_y_ = grid_origin_y;
    cell_size_ = cell_size;
}

int32_t BattleInputHandler::screen_to_grid_x(int32_t screen_x) const {
    if (screen_x < grid_origin_x_) return -1;
    return (screen_x - grid_origin_x_) / cell_size_;
}

int32_t BattleInputHandler::screen_to_grid_y(int32_t screen_y) const {
    if (screen_y < grid_origin_y_) return -1;
    return (screen_y - grid_origin_y_) / cell_size_;
}

int32_t BattleInputHandler::find_fighter_at_grid(int32_t grid_x, int32_t grid_y,
                                                   const logic::BattleState& state) const {
    for (int i = 0; i < logic::BattleState::MAX_FIGHTERS; ++i) {
        const auto& fighter = state.fighters[i];
        if (!fighter.is_active || fighter.stats.current_hp <= 0) continue;
        if (fighter.grid_x == grid_x && fighter.grid_y == grid_y) {
            return i;
        }
    }
    return -1;
}

void BattleInputHandler::open_menu(int32_t screen_x, int32_t screen_y) {
    menu_x_ = screen_x;
    menu_y_ = screen_y;
    if (menu_x_ + MENU_WIDTH > 640) menu_x_ = 640 - MENU_WIDTH;
    if (menu_y_ + MENU_ITEM_COUNT * MENU_ITEM_HEIGHT > 380) {
        menu_y_ = 380 - MENU_ITEM_COUNT * MENU_ITEM_HEIGHT;
    }
    menu_hover_item_ = -1;
    state_ = State::COMMAND_MENU;
}

int32_t BattleInputHandler::hit_test_menu(int32_t screen_x, int32_t screen_y) const {
    if (screen_x < menu_x_ || screen_x >= menu_x_ + MENU_WIDTH) return -1;
    int32_t rel_y = screen_y - menu_y_;
    if (rel_y < 0) return -1;
    int32_t idx = rel_y / MENU_ITEM_HEIGHT;
    if (idx >= 0 && idx < MENU_ITEM_COUNT) return idx;
    return -1;
}

logic::BattleCommand BattleInputHandler::process_event(const platform::Event& event,
                                                        const logic::BattleState& state) {
    logic::BattleCommand cmd;

    if (event.type == platform::Event::Type::KeyDown) {
        if (event.key == 0x1B) {
            switch (state_) {
            case State::SKILL_SELECT:
            case State::ITEM_SELECT:
                state_ = State::FIGHTER_SELECTED;
                pending_skill_idx_ = -1;
                pending_item_idx_ = -1;
                break;
            case State::ATTACK_TARGET:
            case State::SKILL_TARGET:
            case State::MOVE_TARGET:
            case State::MINE_TARGET:
            case State::ITEM_TARGET:
                state_ = State::FIGHTER_SELECTED;
                pending_skill_idx_ = -1;
                pending_item_idx_ = -1;
                break;
            case State::COMMAND_MENU:
                state_ = State::FIGHTER_SELECTED;
                break;
            default:
                state_ = State::IDLE;
                selected_fighter_ = -1;
                break;
            }
            return cmd;
        }

        if (state_ == State::SKILL_SELECT && event.key >= '1' && event.key <= '5') {
            int32_t idx = event.key - '1';
            pending_skill_idx_ = idx;
            cmd.type = logic::BattleCommand::SKILL;
            cmd.fighter_idx = selected_fighter_;
            cmd.skill_idx = idx;
            state_ = State::SKILL_TARGET;
            return cmd;
        }

        if (state_ == State::ITEM_SELECT && event.key >= '1' && event.key <= '8') {
            int32_t idx = event.key - '1';
            pending_item_idx_ = idx;
            cmd.type = logic::BattleCommand::USE_ITEM;
            cmd.fighter_idx = selected_fighter_;
            cmd.item_idx = idx;
            state_ = State::IDLE;
            selected_fighter_ = -1;
            return cmd;
        }
    }

    if (event.type == platform::Event::Type::MouseDown) {
        if (state_ == State::SKILL_SELECT) {
            if (event.mouse_button == 1) {
                int32_t sub_x = menu_x_ + MENU_WIDTH;
                int32_t sub_y = menu_y_;
                if (event.mouse_x >= sub_x && event.mouse_x < sub_x + MENU_WIDTH) {
                    int32_t rel_y = event.mouse_y - sub_y;
                    if (rel_y >= 0 && rel_y < SKILL_SUBMENU_COUNT * MENU_ITEM_HEIGHT) {
                        int32_t idx = rel_y / MENU_ITEM_HEIGHT;
                        pending_skill_idx_ = idx;
                        cmd.type = logic::BattleCommand::SKILL;
                        cmd.fighter_idx = selected_fighter_;
                        cmd.skill_idx = idx;
                        state_ = State::SKILL_TARGET;
                    }
                }
            } else if (event.mouse_button == 3) {
                state_ = State::FIGHTER_SELECTED;
            }
            return cmd;
        }

        if (state_ == State::ITEM_SELECT) {
            if (event.mouse_button == 1) {
                int32_t sub_x = menu_x_ + MENU_WIDTH;
                int32_t sub_y = menu_y_;
                if (event.mouse_x >= sub_x && event.mouse_x < sub_x + MENU_WIDTH) {
                    int32_t rel_y = event.mouse_y - sub_y;
                    if (rel_y >= 0 && rel_y < ITEM_SUBMENU_COUNT * MENU_ITEM_HEIGHT) {
                        int32_t idx = rel_y / MENU_ITEM_HEIGHT;
                        pending_item_idx_ = idx;
                        cmd.type = logic::BattleCommand::USE_ITEM;
                        cmd.fighter_idx = selected_fighter_;
                        cmd.item_idx = idx;
                        state_ = State::IDLE;
                        selected_fighter_ = -1;
                    }
                }
            } else if (event.mouse_button == 3) {
                state_ = State::FIGHTER_SELECTED;
            }
            return cmd;
        }

        if (state_ == State::COMMAND_MENU) {
            int32_t hit = hit_test_menu(event.mouse_x, event.mouse_y);

            if (event.mouse_button == 1) {
                if (hit >= 0) {
                    switch (hit) {
                        case 0: // 攻击 Attack
                            cmd.type = logic::BattleCommand::ATTACK;
                            cmd.fighter_idx = selected_fighter_;
                            state_ = State::ATTACK_TARGET;
                            break;
                        case 1: // 技能 Skill → skill selection submenu
                            state_ = State::SKILL_SELECT;
                            submenu_hover_item_ = -1;
                            break;
                        case 2: // 物品 Item → item selection submenu
                            state_ = State::ITEM_SELECT;
                            submenu_hover_item_ = -1;
                            break;
                        case 3: // 等待 Wait
                            cmd.type = logic::BattleCommand::WAIT;
                            cmd.fighter_idx = selected_fighter_;
                            state_ = State::IDLE;
                            selected_fighter_ = -1;
                            break;
                        case 4: // 移动 Move
                            state_ = State::MOVE_TARGET;
                            break;
                        case 5: // 地雷 Mine
                            state_ = State::MINE_TARGET;
                            break;
                        default:
                            break;
                    }
                } else {
                    state_ = State::FIGHTER_SELECTED;
                }
            } else if (event.mouse_button == 3) {
                state_ = State::FIGHTER_SELECTED;
            }

            if (event.mouse_button == 1 || event.mouse_button == 3) {
                return cmd;
            }
        }

        if (state_ == State::ATTACK_TARGET && event.mouse_button == 1) {
            int32_t grid_x = screen_to_grid_x(event.mouse_x);
            int32_t grid_y = screen_to_grid_y(event.mouse_y);
            if (grid_x >= 0 && grid_y >= 0 &&
                grid_x < logic::BattleState::GRID_COLS &&
                grid_y < logic::BattleState::GRID_ROWS) {
                int32_t target = find_fighter_at_grid(grid_x, grid_y, state);
                if (target >= 0 && state.fighters[target].team != 0) {
                    cmd.type = logic::BattleCommand::ATTACK;
                    cmd.fighter_idx = selected_fighter_;
                    cmd.target_idx = target;
                    state_ = State::IDLE;
                    selected_fighter_ = -1;
                }
            }
            return cmd;
        }

        if (state_ == State::ATTACK_TARGET && event.mouse_button == 3) {
            state_ = State::FIGHTER_SELECTED;
            return cmd;
        }

        if (state_ == State::SKILL_TARGET && event.mouse_button == 1) {
            int32_t grid_x = screen_to_grid_x(event.mouse_x);
            int32_t grid_y = screen_to_grid_y(event.mouse_y);
            if (grid_x >= 0 && grid_y >= 0 &&
                grid_x < logic::BattleState::GRID_COLS &&
                grid_y < logic::BattleState::GRID_ROWS) {
                cmd.type = logic::BattleCommand::SKILL;
                cmd.fighter_idx = selected_fighter_;
                cmd.skill_idx = pending_skill_idx_;
                cmd.target_x = grid_x;
                cmd.target_y = grid_y;
                int32_t target = find_fighter_at_grid(grid_x, grid_y, state);
                if (target >= 0) {
                    cmd.target_idx = target;
                }
                state_ = State::IDLE;
                selected_fighter_ = -1;
                pending_skill_idx_ = -1;
            }
            return cmd;
        }

        if (state_ == State::SKILL_TARGET && event.mouse_button == 3) {
            state_ = State::FIGHTER_SELECTED;
            pending_skill_idx_ = -1;
            return cmd;
        }

        if (state_ == State::ITEM_TARGET && event.mouse_button == 1) {
            int32_t grid_x = screen_to_grid_x(event.mouse_x);
            int32_t grid_y = screen_to_grid_y(event.mouse_y);
            if (grid_x >= 0 && grid_y >= 0 &&
                grid_x < logic::BattleState::GRID_COLS &&
                grid_y < logic::BattleState::GRID_ROWS) {
                cmd.type = logic::BattleCommand::USE_ITEM;
                cmd.fighter_idx = selected_fighter_;
                cmd.item_idx = pending_item_idx_;
                cmd.target_x = grid_x;
                cmd.target_y = grid_y;
                int32_t target = find_fighter_at_grid(grid_x, grid_y, state);
                if (target >= 0) {
                    cmd.target_idx = target;
                }
                state_ = State::IDLE;
                selected_fighter_ = -1;
                pending_item_idx_ = -1;
            }
            return cmd;
        }

        if (state_ == State::ITEM_TARGET && event.mouse_button == 3) {
            state_ = State::FIGHTER_SELECTED;
            pending_item_idx_ = -1;
            return cmd;
        }

        if (state_ == State::MOVE_TARGET) {
            if (event.mouse_button == 1) {
                int32_t grid_x = screen_to_grid_x(event.mouse_x);
                int32_t grid_y = screen_to_grid_y(event.mouse_y);
                if (grid_x >= 0 && grid_y >= 0 &&
                    grid_x < logic::BattleState::GRID_COLS &&
                    grid_y < logic::BattleState::GRID_ROWS) {
                    cmd.type = logic::BattleCommand::MOVE;
                    cmd.fighter_idx = selected_fighter_;
                    cmd.target_x = grid_x;
                    cmd.target_y = grid_y;
                    state_ = State::IDLE;
                    selected_fighter_ = -1;
                }
            } else if (event.mouse_button == 3) {
                state_ = State::FIGHTER_SELECTED;
            }
            return cmd;
        }

        if (state_ == State::MINE_TARGET) {
            if (event.mouse_button == 1) {
                int32_t grid_x = screen_to_grid_x(event.mouse_x);
                int32_t grid_y = screen_to_grid_y(event.mouse_y);
                if (grid_x >= 0 && grid_y >= 0 &&
                    grid_x < logic::BattleState::GRID_COLS &&
                    grid_y < logic::BattleState::GRID_ROWS) {
                    cmd.type = logic::BattleCommand::PLACE_MINE;
                    cmd.fighter_idx = selected_fighter_;
                    cmd.target_x = grid_x;
                    cmd.target_y = grid_y;
                    cmd.mine_slot = 0;
                    state_ = State::IDLE;
                    selected_fighter_ = -1;
                }
            } else if (event.mouse_button == 3) {
                state_ = State::FIGHTER_SELECTED;
            }
            return cmd;
        }

        int32_t grid_x = screen_to_grid_x(event.mouse_x);
        int32_t grid_y = screen_to_grid_y(event.mouse_y);

        if (grid_x < 0 || grid_y < 0) return cmd;
        if (grid_x >= logic::BattleState::GRID_COLS || grid_y >= logic::BattleState::GRID_ROWS) return cmd;

        if (event.mouse_button == 1) {
            int32_t clicked_fighter = find_fighter_at_grid(grid_x, grid_y, state);

            if (state_ == State::IDLE) {
                if (clicked_fighter >= 0 && state.fighters[clicked_fighter].team == 0 &&
                    !state.fighters[clicked_fighter].has_acted) {
                    selected_fighter_ = clicked_fighter;
                    state_ = State::FIGHTER_SELECTED;
                }
            } else if (state_ == State::FIGHTER_SELECTED) {
                if (clicked_fighter >= 0 && state.fighters[clicked_fighter].team == 0 &&
                    !state.fighters[clicked_fighter].has_acted) {
                    selected_fighter_ = clicked_fighter;
                } else if (clicked_fighter >= 0 && state.fighters[clicked_fighter].team != 0) {
                    cmd.type = logic::BattleCommand::ATTACK;
                    cmd.fighter_idx = selected_fighter_;
                    cmd.target_idx = clicked_fighter;
                    state_ = State::IDLE;
                    selected_fighter_ = -1;
                } else {
                    cmd.type = logic::BattleCommand::MOVE;
                    cmd.fighter_idx = selected_fighter_;
                    cmd.target_x = grid_x;
                    cmd.target_y = grid_y;
                    state_ = State::IDLE;
                    selected_fighter_ = -1;
                }
            }
        } else if (event.mouse_button == 3) {
            if (state_ == State::FIGHTER_SELECTED) {
                open_menu(event.mouse_x, event.mouse_y);
            } else {
                state_ = State::IDLE;
                selected_fighter_ = -1;
            }
        }
    } else if (event.type == platform::Event::Type::MouseMove) {
        if (state_ == State::COMMAND_MENU) {
            menu_hover_item_ = hit_test_menu(event.mouse_x, event.mouse_y);
        }
    }

    return cmd;
}

}  // namespace dlzr::presentation
