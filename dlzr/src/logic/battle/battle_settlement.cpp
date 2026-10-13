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

    // 统计我方损失 / Count allied losses
    for (int i = 0; i < BattleState::MAX_FIGHTERS; ++i) {
        const auto& fighter = state.fighters[i];
        if (fighter.team != 0) continue;
        if (fight.fighters[i].stage == 0) continue;
        if (fighter.stats.current_hp > 0) continue;
        ++result.allies_lost;
    }

    if (result.allies_lost > 0) {
        result.death_penalty_applied = true;
    }

    // 统计击杀敌人总数（不重复计算）/ Count total enemy kills (no double-counting)
    int32_t total_enemies_killed = 0;
    for (int j = 0; j < BattleState::MAX_FIGHTERS; ++j) {
        const auto& victim = state.fighters[j];
        if (victim.team == 0) continue;  // skip allies
        if (victim.stats.current_hp > 0) continue;  // skip survivors
        if (victim.people_idx < 0) continue;
        ++total_enemies_killed;
    }
    result.fighters_killed = total_enemies_killed;

    // 为每个存活的我方角色分配经验 / Distribute EXP to each surviving ally
    int32_t survivor_count = 0;
    for (int i = 0; i < BattleState::MAX_FIGHTERS; ++i) {
        const auto& fighter = state.fighters[i];
        if (fighter.team != 0) continue;
        if (fighter.stats.current_hp <= 0) continue;
        if (fighter.people_idx < 0 || fighter.people_idx >= core::MAX_PEOPLE) continue;
        ++survivor_count;

        auto& person = game.people[fighter.people_idx];
        int32_t exp_gained = 0;

        for (int j = 0; j < BattleState::MAX_FIGHTERS; ++j) {
            const auto& victim = state.fighters[j];
            if (victim.team == 0) continue;
            if (victim.stats.current_hp > 0) continue;
            if (victim.people_idx < 0) continue;

            const auto& victim_fp = fight.people[victim.people_idx];
            int32_t base_exp = victim_fp.level * 10;
            exp_gained += calc_experience(person.level, victim_fp.level, base_exp);
        }

        if (exp_gained > 0) {
            person.experience += exp_gained;
            result.total_exp_gained += exp_gained;
        }
    }

    return result;
}

}  // namespace dlzr::logic
