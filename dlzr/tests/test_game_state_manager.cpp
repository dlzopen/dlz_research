// test_game_state_manager.cpp — GameStateManager 单元测试
// GameStateManager unit tests

#include "logic/game_state_manager.hpp"
#include <gtest/gtest.h>

using namespace dlzr::logic;

class GameStateManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // GameStateManager doesn't require loader for basic tests
    }
};

TEST_F(GameStateManagerTest, InitialMode) {
    GameStateManager gsm;
    // Original flow: Mode 999 (reset) → Mode 0 (title animation) → Mode 1 (title menu).
    // The FLC opening animation (mode 0) is skipped in the remake, so the first
    // visible screen is the title menu (GameMode::TITLE).
    EXPECT_EQ(gsm.mode(), GameMode::TITLE);
}

TEST_F(GameStateManagerTest, SetMode) {
    GameStateManager gsm;
    gsm.set_mode(GameMode::TITLE);
    EXPECT_EQ(gsm.mode(), GameMode::TITLE);

    gsm.set_mode(GameMode::BATTLE);
    EXPECT_EQ(gsm.mode(), GameMode::BATTLE);
}

TEST_F(GameStateManagerTest, ScoreSystem) {
    GameStateManager gsm;
    EXPECT_EQ(gsm.score(), 0);

    gsm.add_score(100);
    EXPECT_EQ(gsm.score(), 100);

    gsm.add_score(50);
    EXPECT_EQ(gsm.score(), 150);
}

TEST_F(GameStateManagerTest, HiddenLevelThreshold_NotTriggered) {
    GameStateManager gsm;
    // Fight 36 not completed, should return 0
    EXPECT_EQ(gsm.get_hidden_level_threshold(), 0);
    EXPECT_FALSE(gsm.check_hidden_level_trigger());
}

TEST_F(GameStateManagerTest, FightCompletionTracking) {
    GameStateManager gsm;

    EXPECT_FALSE(gsm.is_fight_completed(0));
    EXPECT_FALSE(gsm.is_fight_completed(36));

    gsm.complete_fight(0);
    EXPECT_TRUE(gsm.is_fight_completed(0));
    EXPECT_FALSE(gsm.is_fight_completed(1));

    gsm.complete_fight(36);
    EXPECT_TRUE(gsm.is_fight_completed(36));
}

TEST_F(GameStateManagerTest, FightCompletion_Bounds) {
    GameStateManager gsm;

    // Invalid indices should not crash
    gsm.complete_fight(-1);
    gsm.complete_fight(47);
    gsm.complete_fight(100);

    EXPECT_FALSE(gsm.is_fight_completed(-1));
    EXPECT_FALSE(gsm.is_fight_completed(47));
}

TEST_F(GameStateManagerTest, DifficultyLevel) {
    GameStateManager gsm;
    EXPECT_EQ(gsm.difficulty_level(), 1);  // Default: normal

    gsm.set_difficulty_level(0);
    EXPECT_EQ(gsm.difficulty_level(), 0);

    gsm.set_difficulty_level(2);
    EXPECT_EQ(gsm.difficulty_level(), 2);
}

TEST_F(GameStateManagerTest, EndingFlags) {
    GameStateManager gsm;
    EXPECT_EQ(gsm.ending_flag_1(), 0);
    EXPECT_EQ(gsm.ending_flag_2(), 0);
    EXPECT_EQ(gsm.ending_flag_3(), 0);

    gsm.add_ending_flag_1(5);
    EXPECT_EQ(gsm.ending_flag_1(), 5);

    gsm.add_ending_flag_2(3);
    EXPECT_EQ(gsm.ending_flag_2(), 3);

    gsm.add_ending_flag_3(10);
    EXPECT_EQ(gsm.ending_flag_3(), 10);
}

TEST_F(GameStateManagerTest, EndingSelection_PerfectEnding) {
    GameStateManager gsm;
    // Perfect ending: all flags high
    gsm.add_ending_flag_1(10);
    gsm.add_ending_flag_2(10);
    gsm.add_ending_flag_3(5);
    EXPECT_EQ(gsm.select_ending(), 0);  // end00: perfect
}

TEST_F(GameStateManagerTest, EndingSelection_HiddenEnding) {
    GameStateManager gsm;
    // Hidden ending: flag_3 >= 10
    gsm.add_ending_flag_1(5);
    gsm.add_ending_flag_2(5);
    gsm.add_ending_flag_3(10);
    EXPECT_EQ(gsm.select_ending(), 5);  // end05: hidden
}

TEST_F(GameStateManagerTest, EndingSelection_Flag1Dominant) {
    GameStateManager gsm;
    // Flag 1 dominant and high
    gsm.add_ending_flag_1(8);
    gsm.add_ending_flag_2(3);
    gsm.add_ending_flag_3(2);
    EXPECT_EQ(gsm.select_ending(), 1);  // end01
}

TEST_F(GameStateManagerTest, EndingSelection_Flag2Dominant) {
    GameStateManager gsm;
    // Flag 2 dominant and high
    gsm.add_ending_flag_1(3);
    gsm.add_ending_flag_2(8);
    gsm.add_ending_flag_3(2);
    EXPECT_EQ(gsm.select_ending(), 3);  // end03
}

TEST_F(GameStateManagerTest, SelectedTown) {
    GameStateManager gsm;
    EXPECT_EQ(gsm.selected_town(), 0);

    gsm.set_selected_town(5);
    EXPECT_EQ(gsm.selected_town(), 5);
}

TEST_F(GameStateManagerTest, SelectedFight) {
    GameStateManager gsm;
    EXPECT_EQ(gsm.selected_fight(), 0);

    gsm.set_selected_fight(10);
    EXPECT_EQ(gsm.selected_fight(), 10);
}

TEST_F(GameStateManagerTest, SaveLoadMode) {
    GameStateManager gsm;
    EXPECT_EQ(gsm.save_load_mode(), 0);  // Default: save

    gsm.set_save_load_mode(1);
    EXPECT_EQ(gsm.save_load_mode(), 1);  // Load mode
}
