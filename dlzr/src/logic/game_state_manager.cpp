// game_state_manager.cpp — 游戏状态管理器实现

#include "logic/game_state_manager.hpp"

namespace dlzr::logic {

void GameStateManager::init(const app::DatLoader& loader) {
    loader_ = &loader;
    game_data_ = loader.load_game_data();
    loader.load_towns(game_data_);
    completed_fights_.fill(false);
    mode_ = GameMode::TITLE;
}

void GameStateManager::set_mode(GameMode mode) {
    mode_ = mode;
}

bool GameStateManager::check_hidden_level_trigger() const {
    // 隐藏关卡触发条件 / Hidden level trigger conditions
    // 战斗 36 完成后，分数达到阈值
    // After fight 36, when score reaches thresholds
    if (!completed_fights_[36]) {
        return false;
    }

    // 4 个阈值 / 4 thresholds: 2800, 3000, 3300, 3600
    return score_ >= 2800;
}

int32_t GameStateManager::get_hidden_level_threshold() const {
    if (!completed_fights_[36]) {
        return 0;
    }

    if (score_ >= 3600) return 4;
    if (score_ >= 3300) return 3;
    if (score_ >= 3000) return 2;
    if (score_ >= 2800) return 1;
    return 0;
}

bool GameStateManager::is_simulation_mode_unlocked() const {
    // 模拟模式在 2800 分后解锁
    // Simulation mode unlocks at 2800 points
    return completed_fights_[36] && score_ >= 2800;
}

bool GameStateManager::is_narrative_mode_unlocked() const {
    // 虚构模式在 3300 分后解锁，进入战斗 40
    // Narrative mode unlocks at 3300 points, enters fight 40
    return completed_fights_[36] && score_ >= 3300;
}

void GameStateManager::complete_fight(int32_t fight_index) {
    if (fight_index >= 0 && fight_index < 47) {
        completed_fights_[fight_index] = true;
    }
}

bool GameStateManager::is_fight_completed(int32_t fight_index) const {
    if (fight_index >= 0 && fight_index < 47) {
        return completed_fights_[fight_index];
    }
    return false;
}

int32_t GameStateManager::select_ending() const {
    // 6 个结局（end00-05）/ 6 endings (end00-05)
    // 基于 3 个结局标记选择 / Based on 3 ending flags
    //
    // 结局分支逻辑 / Ending branch logic:
    // - ending_flag_1 (柳忻): 高→end01, 低→end02
    // - ending_flag_2 (萧飞): 高→end03, 低→end04
    // - 特殊条件: end00 (完美结局), end05 (隐藏结局)
    //
    // 优先检查特殊结局 / Check special endings first
    if (ending_flag_1_ >= 10 && ending_flag_2_ >= 10 && ending_flag_3_ >= 5) {
        return 0;  // end00: 完美结局 / Perfect ending
    }

    if (ending_flag_3_ >= 10) {
        return 5;  // end05: 隐藏结局 / Hidden ending
    }

    // 根据标记选择普通结局 / Select normal ending based on flags
    if (ending_flag_1_ >= ending_flag_2_) {
        return (ending_flag_1_ >= 5) ? 1 : 2;  // end01 or end02
    } else {
        return (ending_flag_2_ >= 5) ? 3 : 4;  // end03 or end04
    }
}

}  // namespace dlzr::logic
