// test_battle_settlement.cpp — 战斗结算单元测试
// Battle settlement unit tests

#include "logic/battle/battle_settlement.hpp"
#include "core/data_model/constants.hpp"
#include <gtest/gtest.h>

using namespace dlzr::logic;
using namespace dlzr::core;

class BattleSettlementTest : public ::testing::Test {
protected:
    void SetUp() override {
        state_ = BattleState{};
        fight_ = FightData{};
        game_ = GameData{};

        // Initialize people with levels
        for (int i = 0; i < MAX_PEOPLE; ++i) {
            game_.people[i].level = 5;
            game_.people[i].experience = 0;
        }
    }

    BattleState state_;
    FightData fight_;
    GameData game_;
};

TEST_F(BattleSettlementTest, NoDeaths_NoPenalty) {
    // All allies survive
    for (int i = 0; i < BattleState::MAX_FIGHTERS; ++i) {
        state_.fighters[i].team = 0;  // Allied
        state_.fighters[i].stats.current_hp = 100;
        state_.fighters[i].is_active = true;
        state_.fighters[i].people_idx = i % MAX_PEOPLE;
    }

    auto result = finalize_battle(state_, fight_, game_);

    EXPECT_EQ(result.allies_lost, 0);
    EXPECT_FALSE(result.death_penalty_applied);
}

TEST_F(BattleSettlementTest, AllyDeath_AppliesPenalty) {
    // One ally dies
    state_.fighters[0].team = 0;
    state_.fighters[0].stats.current_hp = 0;  // Dead
    state_.fighters[0].is_active = true;
    state_.fighters[0].people_idx = 0;
    fight_.fighters[0].stage = 1;

    // Others survive
    for (int i = 1; i < BattleState::MAX_FIGHTERS; ++i) {
        state_.fighters[i].team = 0;
        state_.fighters[i].stats.current_hp = 100;
        state_.fighters[i].is_active = true;
    }

    auto result = finalize_battle(state_, fight_, game_);

    EXPECT_EQ(result.allies_lost, 1);
    EXPECT_TRUE(result.death_penalty_applied);

    // Death penalty (level drop) is applied during battle by BattleEngine,
    // not by finalize_battle. Settlement only reports the deaths.
}

TEST_F(BattleSettlementTest, AllyDeath_LevelMinimum1) {
    // People at level 1 should stay at 1
    for (int i = 0; i < MAX_PEOPLE; ++i) {
        game_.people[i].level = 1;
    }

    state_.fighters[0].team = 0;
    state_.fighters[0].stats.current_hp = 0;
    state_.fighters[0].is_active = true;
    state_.fighters[0].people_idx = 0;
    fight_.fighters[0].stage = 1;

    auto result = finalize_battle(state_, fight_, game_);

    EXPECT_TRUE(result.death_penalty_applied);
    // Death penalty (level drop) is applied during battle, not settlement.
    // Levels remain unchanged by finalize_battle.
    for (int i = 0; i < MAX_PEOPLE; ++i) {
        EXPECT_EQ(game_.people[i].level, 1);
    }
}

TEST_F(BattleSettlementTest, EnemyDeaths_DontTriggerPenalty) {
    // Enemy dies, no penalty
    state_.fighters[0].team = 1;  // Enemy
    state_.fighters[0].stats.current_hp = 0;
    state_.fighters[0].is_active = true;

    // Allies all survive
    for (int i = 1; i < BattleState::MAX_FIGHTERS; ++i) {
        state_.fighters[i].team = 0;
        state_.fighters[i].stats.current_hp = 100;
        state_.fighters[i].is_active = true;
    }

    auto result = finalize_battle(state_, fight_, game_);

    EXPECT_EQ(result.allies_lost, 0);
    EXPECT_FALSE(result.death_penalty_applied);
}

TEST_F(BattleSettlementTest, ExperienceGain) {
    // Set up ally
    state_.fighters[0].team = 0;
    state_.fighters[0].stats.current_hp = 100;
    state_.fighters[0].is_active = true;
    state_.fighters[0].people_idx = 0;

    // Set up enemy that dies
    state_.fighters[1].team = 1;
    state_.fighters[1].stats.current_hp = 0;
    state_.fighters[1].is_active = true;
    state_.fighters[1].people_idx = 1;
    fight_.people[1].level = 10;

    game_.people[0].level = 5;
    game_.people[0].experience = 0;

    auto result = finalize_battle(state_, fight_, game_);

    EXPECT_GT(result.total_exp_gained, 0);
    EXPECT_GT(game_.people[0].experience, 0);
}

TEST_F(BattleSettlementTest, SettlementResult_Defaults) {
    SettlementResult result;
    EXPECT_EQ(result.allies_lost, 0);
    EXPECT_EQ(result.fighters_killed, 0);
    EXPECT_EQ(result.total_exp_gained, 0);
    EXPECT_FALSE(result.death_penalty_applied);
}
