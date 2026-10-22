#pragma once

// battle_input_handler.hpp — 战斗输入处理
// Battle input handling
//
// 处理玩家输入，生成战斗命令。
// Handles player input, generates battle commands.

#include "logic/battle/battle_state.hpp"
#include "platform/platform.hpp"

namespace dlzr::presentation {

class BattleInputHandler {
public:
    enum class State {
        IDLE,
        FIGHTER_SELECTED,
        COMMAND_MENU,
        MOVE_TARGET,
        ATTACK_TARGET,
        SKILL_SELECT,
        SKILL_TARGET,
        ITEM_SELECT,
        ITEM_TARGET,
        MINE_TARGET,
    };

    static constexpr int32_t MENU_ITEM_COUNT = 6;
    static constexpr int32_t SKILL_SUBMENU_COUNT = 5;
    static constexpr int32_t ITEM_SUBMENU_COUNT = 4;

    void init(int32_t grid_origin_x, int32_t grid_origin_y, int32_t cell_size);

    logic::BattleCommand process_event(const platform::Event& event,
                                       const logic::BattleState& state);

    State state() const { return state_; }
    int32_t selected_fighter() const { return selected_fighter_; }
    int32_t menu_hover_item() const { return menu_hover_item_; }

private:
    State state_{State::IDLE};
    int32_t selected_fighter_{-1};
    int32_t menu_hover_item_{-1};
    int32_t submenu_hover_item_{-1};
    int32_t pending_skill_idx_{-1};
    int32_t pending_item_idx_{-1};

    int32_t grid_origin_x_{0};
    int32_t grid_origin_y_{0};
    int32_t cell_size_{0};
    int32_t cam_x_{0};   // 视口相机左上角瓦片坐标（与渲染器共用，保证"点哪=画哪"）
    int32_t cam_y_{0};

    int32_t menu_x_{0};
    int32_t menu_y_{0};

    int32_t screen_to_grid_x(int32_t screen_x) const;
    int32_t screen_to_grid_y(int32_t screen_y) const;

    int32_t find_fighter_at_grid(int32_t grid_x, int32_t grid_y,
                                  const logic::BattleState& state) const;

    void open_menu(int32_t screen_x, int32_t screen_y);
    int32_t hit_test_menu(int32_t screen_x, int32_t screen_y) const;
};

}  // namespace dlzr::presentation
