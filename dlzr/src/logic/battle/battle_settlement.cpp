// battle_settlement.cpp — 战斗结算实现
// Battle settlement implementation

#include "logic/battle/battle_settlement.hpp"
#include "logic/combat/combat_formulas.hpp"

#include <algorithm>

namespace dlzr::logic {

SettlementResult finalize_battle(const BattleState& state,
                                 const core::FightData& fight,
                                 core::GameData& game) {
    SettlementResult result;

    for (int i = 0; i < BattleState::MAX_FIGHTERS; ++i) {
        const auto& fighter = state.fighters[i];
        if (fighter.team != 0) continue;
        if (fight.fighters[i].stage == 0) continue;
        if (fighter.stats.current_hp > 0) continue;
        ++result.allies_lost;
    }

    if (result.allies_lost > 0) {
        result.death_penalty_applied = true;
        for (int i = 0; i < core::MAX_PEOPLE; ++i) {
            auto& person = game.people[i];
            if (person.level > 1) {
                person.level -= 1;
            }
        }
    }

    for (int i = 0; i < BattleState::MAX_FIGHTERS; ++i) {
        const auto& fighter = state.fighters[i];
        if (fighter.team != 0) continue;
        if (fighter.stats.current_hp <= 0) continue;
        if (fighter.people_idx < 0 || fighter.people_idx >= 88) continue;

        const auto& fp = fight.people[fighter.people_idx];
        int32_t people_idx = fighter.people_idx;
        if (people_idx >= 0 && people_idx < core::MAX_PEOPLE) {
            auto& person = game.people[people_idx];

            int32_t exp_gained = 0;
            for (int j = 0; j < BattleState::MAX_FIGHTERS; ++j) {
                const auto& victim = state.fighters[j];
                if (victim.team == 0) continue;
                if (victim.stats.current_hp > 0) continue;
                if (victim.people_idx < 0) continue;

                const auto& victim_fp = fight.people[victim.people_idx];
                int32_t base_exp = victim_fp.level * 10;
                exp_gained += calc_experience(person.level, victim_fp.level, base_exp);
                ++result.fighters_killed;
            }

            if (exp_gained > 0) {
                person.experience += exp_gained;
                result.total_exp_gained += exp_gained;
            }
        }
    }

    result.fighters_killed /= std::max(1, static_cast<int>(
        std::count_if(state.fighters.begin(), state.fighters.end(),
            [](const RuntimeFighter& f) {
                return f.team == 0 && f.is_active && f.stats.current_hp > 0;
            })));

    return result;
}

}  // namespace dlzr::logic
