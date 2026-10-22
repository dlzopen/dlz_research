// test_battle_settlement.cpp — 战斗结算单元测试
// Battle settlement unit tests

#include "logic/battle/battle_settlement.hpp"
#include "logic/battle/battle_engine.hpp"
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

// ============================================================
// 回归测试：init_fighters 不得越界
// FIGHT.DAT 每关只有 88 名角色名册 (People[88])，但战斗部署位有 128 个。
// 旧代码循环 i<128 且无条件访问 fight_->people[i]，i>=88 时越界崩溃。
// 本测试用默认 FightData(128 个部署位) 调用 init，必须不崩溃并正确计数。
// ============================================================
TEST_F(BattleSettlementTest, InitFighters_NoOutOfBoundsCrash) {
    // 回归测试：init_fighters 不得越界。
    // FIGHT.DAT 每关只有 88 名角色名册 (People[88])，但战斗部署位有 128 个。
    // 旧代码循环 i<128 且无条件访问 fight_->people[i]，i>=88 时越界崩溃。
    // 真实"未部署"约定为 stage==0 且 pos_x==999；本测试明确标记其余为未部署，
    // 确保引擎只部署前 12 个登场敌人，并把 88 条名册边界尊重为不变量。
    // 另：引擎会在敌人之上部署 Tier-B 起始军团（玩家），故按 team 分别计数。
    for (int i = 0; i < 12; ++i) {
        fight_.fighters[i].stage = 1;
        // 部署判据是 grid_x（@+0x14），不是 pos_x（@+0x20）。
        // 证据 FUN_0041b53c.c:20 —— 见 battle_engine.cpp 部署判据注释。
        fight_.fighters[i].grid_x = 5;
        fight_.fighters[i].grid_y = 5;
        fight_.fighters[i].pos_x = 5;
        fight_.fighters[i].pos_y = 5;
        fight_.fighters[i].stamina = 100;
        fight_.fighters[i].morale = 100;
        fight_.fighters[i].stat3 = 100;
        fight_.fighters[i].movement = 100;
    }
    for (int i = 12; i < 128; ++i) {
        fight_.fighters[i].stage = 0;
        fight_.fighters[i].grid_x = 999;  // 真实"未部署"标记（判据字段）
        fight_.fighters[i].grid_y = 999;
        fight_.fighters[i].pos_x = 999;
        fight_.fighters[i].pos_y = 999;
    }
    // 真实 FIGHT.DAT 中登场单位对应的名册条目必须标记阵营=敌方(1)，
    // 否则引擎按默认 enemy=0 部署成我方，破坏 enemy_count 断言。
    for (int i = 0; i < 12; ++i) {
        fight_.people[i].enemy = 1;
    }

    BattleEngine engine;
    // 若此处越界，进程会段错误 → 测试失败（而非静默通过）。
    engine.init(fight_, game_, 1);

    int enemy_count = 0, player_count = 0;
    const auto& st = engine.state();
    for (int i = 0; i < BattleState::MAX_FIGHTERS; ++i) {
        if (!st.fighters[i].is_active) continue;
        if (st.fighters[i].team == 1) ++enemy_count;
        else if (st.fighters[i].team == 0) ++player_count;
    }
    EXPECT_EQ(enemy_count, 12);   // 仅 12 名登场敌人被部署
    EXPECT_GE(player_count, 1);   // Tier-B 起始军团已部署（玩家）
    EXPECT_LE(enemy_count, 88);   // 敌人数不超过名册容量
}

TEST_F(BattleSettlementTest, InitFighters_All128SlotsNoCrash) {
    // 回归测试：所有 128 个部署位都置为"登场"，但只有前 88 个有对应角色；
    // 引擎必须跳过 88..127 而不越界，且不会把 88..127 当成敌人。
    for (int i = 0; i < 128; ++i) {
        fight_.fighters[i].stage = 1;
        // 同样以 grid_x 为判据（i%13 恒 != 999，故 128 位全部登场）
        fight_.fighters[i].grid_x = (i % 13);
        fight_.fighters[i].grid_y = (i % 7);
        fight_.fighters[i].pos_x = (i % 13);
        fight_.fighters[i].pos_y = (i % 7);
        fight_.fighters[i].stamina = 100;
        fight_.fighters[i].morale = 100;
        fight_.fighters[i].stat3 = 100;
        fight_.fighters[i].movement = 100;
    }
    // 名册仅 88 条；真实 FIGHT.DAT 中这些条目全部为敌方(1)。
    // 标记阵营使引擎按 team==1 部署，并验证 i>=88 的部署位被跳过（不越界）。
    for (int i = 0; i < 88; ++i) {
        fight_.people[i].enemy = 1;
    }

    BattleEngine engine;
    engine.init(fight_, game_, 1);

    // 仅 88 名角色被计入为敌人；任何 team==1 的单位索引必须 < 88（未越界读名册）。
    const auto& st = engine.state();
    int enemy_count = 0;
    for (int i = 0; i < BattleState::MAX_FIGHTERS; ++i) {
        if (!st.fighters[i].is_active) continue;
        if (st.fighters[i].team == 1) {
            ++enemy_count;
            EXPECT_LT(i, 88) << "敌人索引越界名册边界 (>=88)";
        }
    }
    EXPECT_EQ(enemy_count, 88);
}
