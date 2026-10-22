#pragma once

// 游戏状态管理器 / Game state manager
// 处理游戏模式切换：MAP → TOWN → BATTLE
// Handles game mode transitions

#include "core/data_model/constants.hpp"
#include "core/data_model/game_data.hpp"
#include "app/resource/dat_loader.hpp"

#include <functional>
#include <memory>

namespace dlzr::logic {

// 游戏模式枚举 — 对应原始游戏 DAT_007db4e8 的值
// 详见 docs/GUI_MODE_MAP.md
enum class GameMode {
    // 标题画面 (original modes 0-3)
    TITLE_ANIMATION,      // Mode 0: FLC 开场动画
    TITLE,                // Mode 1: 标题菜单 (5个按钮)
    TITLE_RANKING,        // Mode 2: 战斗英雄排行
    TITLE_REST,           // Mode 3: 部队休整

    // 角色选择 (original mode 4)
    CHARACTER_SELECT,     // Mode 4: 角色选择/战前准备

    // 过渡/结算 (original modes 5-7)
    CONTINUE_GAME,        // Mode 5: 继续游戏/读档过渡
    VICTORY_ANIMATION,    // Mode 6: 胜利动画滚动
    LEVEL_SELECT,         // Mode 7: 关卡选择/密码输入

    // 城镇菜单 (original modes 50-58)
    TOWN,                 // Mode 50: 城镇中心 (6按钮枢纽)
    ARMY_INFO,            // Mode 51: 军队信息
    SHOP,                 // Mode 52: 物资装备/商店
    RECRUIT,              // Mode 53: 招募
    FORGING,              // Mode 54: 锻造
    SAVE_LOAD,            // Mode 55: 存档/读档
    RECRUIT_DETAIL,       // Mode 56: 招募详细
    MINE_PLACEMENT,       // Mode 57: 布雷
    CONFIRM_DIALOG,       // Mode 58: 确认对话框

    // 战略/战斗 (original modes 100, 120, 150)
    INIT,                 // Mode 100: 初始化过渡 → 战略地图
    STRATEGIC_MAP,        // Mode 150: 战略地图/战斗
    BATTLE,               // Mode 150 子模式: 战斗中

    // 元模式 (original mode 999)
    RESET,                // Mode 999: 重置到标题

    // 其他系统
    BATTLE_INTRO,         // 战斗开场动画
    BATTLE_RESULT,        // 战斗结算
    DIALOGUE,             // 对话系统
    CUTSCENE,             // 过场动画
    ENDING,               // 结局画面
    OPENING,              // 开场剧情
    PROMOTION,            // 晋升系统
    EQUIPMENT,            // 装备管理
    WAREHOUSE,            // 仓库管理
    HIDDEN_LEVEL_CHOICE,  // 隐藏关卡选择
};

class GameStateManager {
public:
    void init(const app::DatLoader& loader);

    GameMode mode() const { return mode_; }
    void set_mode(GameMode mode);

    core::GameData& game_data() { return game_data_; }
    const core::GameData& game_data() const { return game_data_; }

    const app::DatLoader& loader() const { return *loader_; }

    int32_t selected_town() const { return selected_town_; }
    void set_selected_town(int32_t town) { selected_town_ = town; }

    int32_t selected_fight() const { return selected_fight_; }
    void set_selected_fight(int32_t fight) { selected_fight_ = fight; }

    // 关卡选择界面的返回目标 / Level-select return target
    // 从人物选择(新游戏)进入 → 返回 CHARACTER_SELECT；从城镇进入 → 返回 TOWN
    // ENTERED from new-game character select → return to CHARACTER_SELECT;
    // entered from a town → return to TOWN
    GameMode level_select_return_mode() const {
        return static_cast<GameMode>(level_select_return_mode_);
    }
    void set_level_select_return_mode(GameMode mode) {
        level_select_return_mode_ = static_cast<int32_t>(mode);
    }

    int32_t current_shop_type() const { return current_shop_type_; }
    void set_current_shop_type(int32_t type) { current_shop_type_ = type; }

    int32_t save_load_mode() const { return save_load_mode_; }
    void set_save_load_mode(int32_t mode) { save_load_mode_ = mode; }

    int32_t selected_equipment_char() const { return selected_equipment_char_; }
    void set_selected_equipment_char(int32_t char_index) { selected_equipment_char_ = char_index; }

    // 隐藏关卡系统 / Hidden level system
    int32_t score() const { return score_; }
    void add_score(int32_t points) { score_ += points; }
    bool check_hidden_level_trigger() const;

    // 获取隐藏关卡阈值等级（0-4）/ Get hidden level threshold (0-4)
    // 0=未触发, 1=2800分, 2=3000分, 3=3300分, 4=3600分
    // 0=not triggered, 1=2800pts, 2=3000pts, 3=3300pts, 4=3600pts
    int32_t get_hidden_level_threshold() const;

    // 检查是否解锁模拟模式 / Check if simulation mode is unlocked
    bool is_simulation_mode_unlocked() const;

    // 检查是否解锁虚构模式 / Check if narrative mode is unlocked
    bool is_narrative_mode_unlocked() const;

    // 结局标记系统 / Ending flag system
    // These read/write game_data_ fields that are persisted in save files
    int32_t ending_flag_1() const { return game_data_.ending_flag_protagonist; }
    int32_t ending_flag_2() const { return game_data_.ending_flag_xiaofei; }
    int32_t ending_flag_3() const { return game_data_.ending_flag_third; }

    void add_ending_flag_1(int32_t value) { game_data_.ending_flag_protagonist += value; }
    void add_ending_flag_2(int32_t value) { game_data_.ending_flag_xiaofei += value; }
    void add_ending_flag_3(int32_t value) { game_data_.ending_flag_third += value; }

    // 战斗完成追踪 / Fight completion tracking
    void complete_fight(int32_t fight_index);
    bool is_fight_completed(int32_t fight_index) const;

    // 结局选择 / Ending selection
    int32_t select_ending() const;

    // 商店系统 / Shop system
    int32_t get_gold() const { return static_cast<int32_t>(game_data_.warehouse[0]); }
    void add_gold(int32_t amount);
    bool can_afford(int32_t price) const { return get_gold() >= price; }
    bool buy_wupin(int32_t item_index);
    bool buy_mine(int32_t mine_index);

    // 装备系统 / Equipment system
    bool equip_item(int32_t char_idx, int32_t slot, int32_t item_idx);
    bool unequip_item(int32_t char_idx, int32_t slot);
    bool equip_mine(int32_t char_idx, int32_t slot, int32_t mine_idx);
    bool unequip_mine(int32_t char_idx, int32_t slot);

    // 难度系统 / Difficulty system
    int32_t difficulty_level() const { return difficulty_level_; }
    void set_difficulty_level(int32_t level) { difficulty_level_ = level; }

private:
    GameMode mode_{GameMode::TITLE};
    core::GameData game_data_;
    const app::DatLoader* loader_{nullptr};

    int32_t selected_town_{0};
    int32_t selected_fight_{0};
    int32_t level_select_return_mode_{static_cast<int32_t>(GameMode::TOWN)};
    int32_t current_shop_type_{0};
    int32_t save_load_mode_{0};  // 0=save, 1=load
    int32_t selected_equipment_char_{0};

    // 隐藏关卡 / Hidden levels
    int32_t score_{0};
    int32_t difficulty_level_{1};  // 0=简单, 1=普通, 2=困难
    std::array<bool, 47> completed_fights_{};

    // 结局标记已移至 game_data_ / Ending flags moved to game_data_
    // DAT_007d06d4: game_data_.ending_flag_protagonist
    // DAT_007d06d8: game_data_.ending_flag_xiaofei
    // DAT_007d06ec: game_data_.ending_flag_third
};

}  // namespace dlzr::logic
