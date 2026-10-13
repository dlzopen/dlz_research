// game_state_manager.cpp — 游戏状态管理器实现

#include "logic/game_state_manager.hpp"

#include <algorithm>

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
    // RE: DAT_007d06d4 = ending_flag_protagonist (主角柳忻互动计数)
    //     DAT_007d06d8 = ending_flag_xiaofei (萧飞互动计数)
    //     DAT_007d06ec = ending_flag_third (第三标记, production site unknown)
    //
    // 结局分支逻辑 / Ending branch logic:
    // - ending_flag_1 (柳忻): 高→end01, 低→end02
    // - ending_flag_2 (萧飞): 高→end03, 低→end04
    // - 特殊条件: end00 (完美结局), end05 (隐藏结局)
    //
    // 优先检查特殊结局 / Check special endings first
    if (game_data_.ending_flag_protagonist >= 10 &&
        game_data_.ending_flag_xiaofei >= 10 &&
        game_data_.ending_flag_third >= 5) {
        return 0;  // end00: 完美结局 / Perfect ending
    }

    if (game_data_.ending_flag_third >= 10) {
        return 5;  // end05: 隐藏结局 / Hidden ending
    }

    // 根据标记选择普通结局 / Select normal ending based on flags
    if (game_data_.ending_flag_protagonist >= game_data_.ending_flag_xiaofei) {
        return (game_data_.ending_flag_protagonist >= 5) ? 1 : 2;  // end01 or end02
    } else {
        return (game_data_.ending_flag_xiaofei >= 5) ? 3 : 4;  // end03 or end04
    }
}

void GameStateManager::add_gold(int32_t amount) {
    int32_t current = get_gold();
    int32_t new_value = std::max(0, current + amount);
    game_data_.warehouse[0] = static_cast<uint16_t>(std::min(new_value, 65535));
}

bool GameStateManager::buy_wupin(int32_t item_index) {
    if (item_index < 0 || item_index >= core::MAX_WUPIN) return false;
    const auto& item = game_data_.items[item_index];
    if (item.price <= 0) return false;
    if (!can_afford(item.price)) return false;

    add_gold(-item.price);
    if (item_index < static_cast<int32_t>(game_data_.warehouse.size())) {
        uint16_t current_qty = game_data_.warehouse[item_index];
        if (current_qty < 65535) {
            game_data_.warehouse[item_index] = current_qty + 1;
        }
    }
    return true;
}

bool GameStateManager::buy_mine(int32_t mine_index) {
    if (mine_index < 0 || mine_index >= core::MAX_DILEI) return false;
    const auto& mine = game_data_.mines[mine_index];
    if (mine.price <= 0) return false;
    if (!can_afford(mine.price)) return false;

    add_gold(-mine.price);
    int32_t warehouse_slot = core::MAX_WUPIN - core::MAX_DILEI + mine_index;
    if (warehouse_slot >= 0 && warehouse_slot < static_cast<int32_t>(game_data_.warehouse.size())) {
        uint16_t current_qty = game_data_.warehouse[warehouse_slot];
        if (current_qty < 65535) {
            game_data_.warehouse[warehouse_slot] = current_qty + 1;
        }
    }
    return true;
}

bool GameStateManager::equip_item(int32_t char_idx, int32_t slot, int32_t item_idx) {
    if (char_idx < 0 || char_idx >= core::MAX_PEOPLE) return false;
    if (slot < 0 || slot >= 8) return false;
    if (item_idx <= 0 || item_idx >= core::MAX_WUPIN) return false;
    if (game_data_.people[char_idx].items[slot] != 0) return false;
    if (game_data_.warehouse[item_idx] == 0) return false;

    game_data_.warehouse[item_idx]--;
    game_data_.people[char_idx].items[slot] = static_cast<uint8_t>(item_idx);
    return true;
}

bool GameStateManager::unequip_item(int32_t char_idx, int32_t slot) {
    if (char_idx < 0 || char_idx >= core::MAX_PEOPLE) return false;
    if (slot < 0 || slot >= 8) return false;
    uint8_t item_idx = game_data_.people[char_idx].items[slot];
    if (item_idx == 0) return false;

    game_data_.people[char_idx].items[slot] = 0;
    if (item_idx < static_cast<int32_t>(game_data_.warehouse.size())) {
        if (game_data_.warehouse[item_idx] < 65535) {
            game_data_.warehouse[item_idx]++;
        }
    }
    return true;
}

bool GameStateManager::equip_mine(int32_t char_idx, int32_t slot, int32_t mine_idx) {
    if (char_idx < 0 || char_idx >= core::MAX_PEOPLE) return false;
    if (slot < 0 || slot >= 8) return false;
    if (mine_idx <= 0 || mine_idx >= core::MAX_DILEI) return false;
    if (game_data_.people[char_idx].mines[slot] != 0) return false;

    int32_t warehouse_slot = core::MAX_WUPIN - core::MAX_DILEI + mine_idx;
    if (warehouse_slot < 0 || warehouse_slot >= static_cast<int32_t>(game_data_.warehouse.size())) {
        return false;
    }
    if (game_data_.warehouse[warehouse_slot] == 0) return false;

    game_data_.warehouse[warehouse_slot]--;
    game_data_.people[char_idx].mines[slot] = static_cast<uint8_t>(mine_idx);
    return true;
}

bool GameStateManager::unequip_mine(int32_t char_idx, int32_t slot) {
    if (char_idx < 0 || char_idx >= core::MAX_PEOPLE) return false;
    if (slot < 0 || slot >= 8) return false;
    uint8_t mine_idx = game_data_.people[char_idx].mines[slot];
    if (mine_idx == 0) return false;

    game_data_.people[char_idx].mines[slot] = 0;
    int32_t warehouse_slot = core::MAX_WUPIN - core::MAX_DILEI + mine_idx;
    if (warehouse_slot >= 0 && warehouse_slot < static_cast<int32_t>(game_data_.warehouse.size())) {
        if (game_data_.warehouse[warehouse_slot] < 65535) {
            game_data_.warehouse[warehouse_slot]++;
        }
    }
    return true;
}

}  // namespace dlzr::logic
