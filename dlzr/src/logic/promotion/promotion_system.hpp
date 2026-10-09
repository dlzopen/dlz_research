#pragma once

// promotion_system.hpp — 转职系统
// Class promotion system
//
// 12 promotion chains, 90 classes, max 3 tiers.
// Characters can promote when they have the required item and meet level requirements.

#include "core/data_model/game_data.hpp"

#include <cstdint>

namespace dlzr::logic {

class PromotionSystem {
public:
    void init(core::GameData& game_data);

    bool can_promote(int32_t people_idx) const;
    void execute_promotion(int32_t people_idx);

    int32_t get_promotion_target(int32_t people_idx) const;
    int32_t get_required_item(int32_t people_idx) const;

private:
    core::GameData* game_data_{nullptr};
    static constexpr int32_t MIN_PROMOTION_LEVEL = 10;
};

}  // namespace dlzr::logic
