// test_campaign_recruitment.cpp — 战役招募写回 → 部署分区（红测驱动）
// Campaign recruitment write-back → deployment partition (red-test driven)
//
// 铁律零范围声明：本测试证 "BattleEngine::init 消费 game.campaign_roster 后，
// init_fighters 的现成循环按 slot 索引正确部署同伴（team 0）且不与敌人（team !=0）冲突"，
// 并对拍 RE 行为边界：
//   · 部署判据 = fight.fighters[k].grid_x != 999 && stage == 1（FUN_0041b53c.c:20）
//   · 阵营 = fight.people[k].enemy（init_fighters 行 214）
//   · 同伴体力 = 兵种公式 hp_base+(lv-1)*hp_growth（calc_max_hp FUN_00429e31）
// 不臆造招募"事件"——roster 由测试直接构造（事件 FSM 是写回来源，属下一阶段）。

#include "logic/battle/battle_engine.hpp"
#include "logic/battle/campaign_recruitment.hpp"
#include "core/data_model/game_data.hpp"

#include <cstring>
#include <string>
#include <vector>

#include "gtest/gtest.h"

namespace {

class CampaignRecruitmentTest : public ::testing::Test {
protected:
    dlzr::core::FightData fight_;
    dlzr::core::GameData game_;

    void SetUp() override {
        // 清场：全部 128 部署位未部署（grid_x=999, stage=0）
        for (int i = 0; i < 128; ++i) {
            fight_.fighters[i].stage = 0;
            fight_.fighters[i].grid_x = 999;
            fight_.fighters[i].grid_y = 999;
            fight_.fighters[i].pos_x = 999;
            fight_.fighters[i].pos_y = 999;
        }

        // 主角（槽 0）
        std::strncpy(game_.people[0].name.data(), "主角",
                     game_.people[0].name.size() - 1);
        game_.people[0].name.back() = '\0';
        game_.people[0].face = 103;
        game_.people[0].enemy = 0;
        game_.people[0].class_type = 1;     // 手枪队
        game_.people[0].level = 10;
        game_.class_types[1].hp_base = 80;
        game_.class_types[1].hp_growth = 12;

        // 同伴名册（槽 1..3）——我方战役同伴
        for (int k = 1; k <= 3; ++k) {
            auto& p = game_.people[k];
            std::strncpy(p.name.data(), ("同伴" + std::to_string(k)).c_str(),
                         p.name.size() - 1);
            p.name.back() = '\0';
            p.face = 103 + k;
            p.enemy = 0;
            p.class_type = 4;              // 大刀队
            p.level = 5;
            p.strength = 50;
            p.intelligence = 50;
        }
        game_.class_types[4].hp_base = 100;
        game_.class_types[4].hp_growth = 10;

        // 敌人（槽 40..42，登场，敌方阵营）
        for (int i = 40; i <= 42; ++i) {
            fight_.fighters[i].stage = 1;
            fight_.fighters[i].grid_x = 10 + (i - 40);
            fight_.fighters[i].grid_y = 20;
            fight_.fighters[i].stamina = 100;
            fight_.fighters[i].morale = 100;
            fight_.fighters[i].stat3 = 100;
            fight_.fighters[i].movement = 100;
            fight_.people[i].enemy = 1;    // 敌方
        }

        // 让英雄覆盖分支运行（protagonist_face >= 0 才覆盖 fight.people[0]）
        game_.protagonist_face = 103;
        game_.protagonist_class_type = 1;
    }

    static void count_teams(const dlzr::logic::BattleEngine& engine,
                            int& team0, int& team1) {
        team0 = team1 = 0;
        const auto& st = engine.state();
        for (int i = 0; i < dlzr::logic::BattleState::MAX_FIGHTERS; ++i) {
            if (!st.fighters[i].is_active) continue;
            if (st.fighters[i].team == 0) ++team0;
            else if (st.fighters[i].team == 1) ++team1;
        }
    }
};

// 招募 3 名同伴 → 部署分区 = 主角(1) + 同伴(3) 在 team0，敌人(3) 在 team1
TEST_F(CampaignRecruitmentTest, RecruitedCompanionsDeployOnTeamZero) {
    game_.campaign_roster.companions = {
        {1, 8, 24, 1},
        {2, 9, 24, 1},
        {3, 10, 24, 1},
    };

    dlzr::logic::BattleEngine engine;
    engine.init(fight_, game_, 1);

    int team0 = 0, team1 = 0;
    count_teams(engine, team0, team1);
    EXPECT_EQ(team1, 3);    // 3 名敌人
    EXPECT_EQ(team0, 4);    // 主角 + 3 同伴

    // 同伴的 people_idx 必须落在 [1,3]（即确为招募槽），主角 people_idx==0
    const auto& st = engine.state();
    int companion_slots = 0, hero_slots = 0;
    for (int i = 0; i < dlzr::logic::BattleState::MAX_FIGHTERS; ++i) {
        if (!st.fighters[i].is_active) continue;
        if (st.fighters[i].team != 0) continue;
        if (st.fighters[i].people_idx >= 1 && st.fighters[i].people_idx <= 3)
            ++companion_slots;
        else if (st.fighters[i].people_idx == 0)
            ++hero_slots;
    }
    EXPECT_EQ(companion_slots, 3);
    EXPECT_EQ(hero_slots, 1);

    // 同伴部署坐标不得与敌人(10..12,20)重叠，且均有效(非 999)
    for (int i = 0; i < dlzr::logic::BattleState::MAX_FIGHTERS; ++i) {
        if (!st.fighters[i].is_active) continue;
        if (st.fighters[i].team != 0) continue;
        EXPECT_NE(st.fighters[i].grid_x, 999);
        EXPECT_NE(st.fighters[i].grid_y, 999);
        if (st.fighters[i].people_idx >= 1 && st.fighters[i].people_idx <= 3) {
            EXPECT_FALSE((st.fighters[i].grid_x == 10 || st.fighters[i].grid_x == 11 ||
                          st.fighters[i].grid_x == 12) &&
                         st.fighters[i].grid_y == 20)
                << "同伴与敌人部署坐标重叠";
        }
    }
}

// 空名册 → 仅主角登场（team0==1），同伴缺口未引入回归
TEST_F(CampaignRecruitmentTest, EmptyRosterDeploysOnlyHero) {
    game_.campaign_roster.companions.clear();

    dlzr::logic::BattleEngine engine;
    engine.init(fight_, game_, 1);

    int team0 = 0, team1 = 0;
    count_teams(engine, team0, team1);
    EXPECT_EQ(team1, 3);    // 敌人不变
    EXPECT_EQ(team0, 1);    // 仅主角（无同伴）
}

// 招募同伴的体力须按兵种公式（class 4: hp_base=100, growth=10, lv=5 → 140）
TEST_F(CampaignRecruitmentTest, CompanionHpFromClassFormula) {
    game_.campaign_roster.companions = {{1, 8, 24, 1}};

    dlzr::logic::BattleEngine engine;
    engine.init(fight_, game_, 1);

    const auto& st = engine.state();
    int idx = -1;
    for (int i = 0; i < dlzr::logic::BattleState::MAX_FIGHTERS; ++i) {
        if (st.fighters[i].is_active && st.fighters[i].people_idx == 1) {
            idx = i;
            break;
        }
    }
    ASSERT_GE(idx, 0) << "同伴 1 应被部署";
    EXPECT_EQ(st.fighters[idx].stats.max_hp, 140);
    EXPECT_EQ(st.fighters[idx].stats.current_hp, 140);
}

// 越界/非法名册索引被安全忽略（不崩溃、不影响既有部署）
TEST_F(CampaignRecruitmentTest, InvalidRosterEntriesIgnored) {
    game_.campaign_roster.companions = {
        {0, 8, 24, 1},     // 0=主角槽，跳过
        {40, 8, 24, 1},    // 40=本关名册槽，跳过
        {200, 8, 24, 1},   // 越界，跳过
        {2, 9, 24, 1},     // 合法同伴
    };

    dlzr::logic::BattleEngine engine;
    engine.init(fight_, game_, 1);   // 不得崩溃

    int team0 = 0, team1 = 0;
    count_teams(engine, team0, team1);
    EXPECT_EQ(team1, 3);
    EXPECT_EQ(team0, 2);             // 主角 + 仅 1 名合法同伴
}

// 招募生产者 API：非法索引被拒、合法索引入册（不决定"何时招募"，仅数据写回）
TEST_F(CampaignRecruitmentTest, RecruitCompanionApiRejectsInvalidAndAcceptsValid) {
    EXPECT_FALSE(dlzr::logic::recruit_companion(game_, 0, 8, 24));    // 主角槽
    EXPECT_FALSE(dlzr::logic::recruit_companion(game_, 40, 8, 24));   // 本关名册槽
    EXPECT_FALSE(dlzr::logic::recruit_companion(game_, 200, 8, 24));  // 越界
    EXPECT_EQ(game_.campaign_roster.companions.size(), 0u);

    EXPECT_TRUE(dlzr::logic::recruit_companion(game_, 2, 8, 24, 1));
    EXPECT_TRUE(dlzr::logic::recruit_companion(game_, 3, 9, 24, 1));
    ASSERT_EQ(game_.campaign_roster.companions.size(), 2u);
    EXPECT_EQ(game_.campaign_roster.companions[0].people_index, 2);
    EXPECT_EQ(game_.campaign_roster.companions[0].grid_x, 8);
    EXPECT_EQ(game_.campaign_roster.companions[0].grid_y, 24);
    EXPECT_EQ(game_.campaign_roster.companions[0].facing, 1);
    EXPECT_EQ(game_.campaign_roster.companions[1].people_index, 3);
}

// 全链路：生产者(recruit) → 名册 → 消费者(apply_campaign_recruitment, 经 init) → 部署
// 不手填 roster，验证"事件系统调 recruit_companion"即可让同伴上场（team0）
TEST_F(CampaignRecruitmentTest, ProducerToDeployFullChain) {
    ASSERT_TRUE(dlzr::logic::recruit_companion(game_, 1, 8, 24, 1));
    ASSERT_TRUE(dlzr::logic::recruit_companion(game_, 2, 9, 24, 1));
    ASSERT_EQ(game_.campaign_roster.companions.size(), 2u);

    dlzr::logic::BattleEngine engine;
    engine.init(fight_, game_, 1);

    int team0 = 0, team1 = 0;
    count_teams(engine, team0, team1);
    EXPECT_EQ(team1, 3);    // 敌人不变
    EXPECT_EQ(team0, 3);    // 主角 + 2 名经 recruit 入册的同伴

    const auto& st = engine.state();
    int companion_slots = 0;
    for (int i = 0; i < dlzr::logic::BattleState::MAX_FIGHTERS; ++i) {
        if (!st.fighters[i].is_active) continue;
        if (st.fighters[i].team != 0) continue;
        if (st.fighters[i].people_idx >= 1 && st.fighters[i].people_idx <= 2)
            ++companion_slots;
    }
    EXPECT_EQ(companion_slots, 2);
}

}  // namespace
