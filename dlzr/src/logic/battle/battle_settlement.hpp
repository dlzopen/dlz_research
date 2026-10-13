#pragma once

// battle_settlement.hpp — 战斗结算
// Battle settlement / finalization
//
// 战斗结束后处理经验分配、物品转移。死亡惩罚在 process_death 中即时处理。
// Post-battle: exp distribution, item transfer. Death penalty is in process_death.

#include "logic/battle/battle_state.hpp"
#include "core/data_model/game_data.hpp"
#include "core/data_model/types.hpp"

#include <array>
#include <cstdint>

namespace dlzr::logic {

struct SettlementResult {
    int32_t total_exp_gained{0};
    int32_t fighters_killed{0};
    int32_t allies_lost{0};
    bool death_penalty_applied{false};
};

SettlementResult finalize_battle(const BattleState& state,
                                 const core::FightData& fight,
                                 core::GameData& game);

}  // namespace dlzr::logic
