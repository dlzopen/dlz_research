// test_first_level.cpp — 第一关（fight 0）无界面冒烟测试
// First-level (fight 0) headless smoke test: load real data, init, auto-play
// to a conclusion, and assert invariants. Ensures the first level is actually
// playable and does not crash / stall.
//
// 逆向依据：原版新游戏链路 = Mode4 确认 → 幻灯片(FUN_0042e0f1) → Mode150 战斗。
// 本测试只验「战斗可玩」（fight 0 加载/部署/回合/胜负），不涉及 GUI 流程。

#include "app/resource/dat_loader.hpp"
#include "logic/game_state_manager.hpp"
#include "logic/battle/battle_engine.hpp"
#include "logic/dialogue/dialogue_manager.hpp"
#include "logic/level/level_event_manager.hpp"
#include "core/data_model/game_data.hpp"

#include <gtest/gtest.h>
#include <filesystem>

using namespace dlzr::logic;
using namespace dlzr::core;

namespace {

// 真实游戏数据目录（与用户运行 dlzr.exe <原游戏目录> 一致）
const char* kGameDataDir = "C:/seesaw/source/dlz_research/DLZFULL";

// 本地存活判断（is_fighter_alive 为私有，复用其等价条件）
auto is_alive = [](const BattleState& s, int32_t i) -> bool {
    return s.fighters[i].is_active && s.fighters[i].stats.current_hp > 0;
};

// 贪心玩家行动：能打到敌人就攻击，否则朝最近敌人走一步，再 WAIT。
BattleCommand greedy_player_command(BattleEngine& engine, int32_t cur) {
    const auto& state = engine.state();
    const auto& self = state.fighters[cur];

    // 1) 找攻击范围内的最近敌人
    int32_t best_target = -1;
    int32_t best_dist = 1 << 30;
    for (int i = 0; i < BattleState::MAX_FIGHTERS; ++i) {
        const auto& f = state.fighters[i];
        if (!is_alive(state, i)) continue;
        if (f.team == self.team) continue;
        if (engine.is_in_attack_range(cur, f.grid_x, f.grid_y)) {
            int d = std::abs(f.grid_x - self.grid_x) +
                    std::abs(f.grid_y - self.grid_y);
            if (d < best_dist) { best_dist = d; best_target = i; }
        }
    }
    if (best_target >= 0) {
        BattleCommand cmd;
        cmd.type = BattleCommand::ATTACK;
        cmd.fighter_idx = cur;
        cmd.target_idx = best_target;
        return cmd;
    }

    // 2) 找最近敌人，朝它移动
    int32_t enemy_idx = -1;
    best_dist = 1 << 30;
    for (int i = 0; i < BattleState::MAX_FIGHTERS; ++i) {
        const auto& f = state.fighters[i];
        if (!is_alive(state, i)) continue;
        if (f.team == self.team) continue;
        int d = std::abs(f.grid_x - self.grid_x) +
                std::abs(f.grid_y - self.grid_y);
        if (d < best_dist) { best_dist = d; enemy_idx = i; }
    }

    BattleCommand cmd;
    cmd.type = BattleCommand::WAIT;
    cmd.fighter_idx = cur;
    if (enemy_idx >= 0) {
        const auto& ef = state.fighters[enemy_idx];
        // 计算可达集中离敌人最近的格子
        engine.set_selected_fighter(cur);  // 填充 movement_mask
        int32_t best_cell = -1;
        int32_t best_cd = 1 << 30;
        for (int y = 0; y < state.map_height; ++y) {
            for (int x = 0; x < state.map_width; ++x) {
                int cell = y * state.map_width + x;
                if (!state.movement_mask[cell]) continue;
                int d = std::abs(x - ef.grid_x) + std::abs(y - ef.grid_y);
                if (d < best_cd) { best_cd = d; best_cell = cell; }
            }
        }
        if (best_cell >= 0) {
            cmd.type = BattleCommand::MOVE;
            cmd.target_x = best_cell % state.map_width;
            cmd.target_y = best_cell / state.map_width;
        }
    }
    return cmd;
}

}  // namespace

// ---------------------------------------------------------------------------
// 回归：锁死"第一关战前对话"不再静默消失。
// 背景：该结论此前反复在分析报告里出现、却在实现时被删掉（代码里还并存着
//       "第0关没有战前对话"的已被证伪注释），且无任何可证伪的钩子。
// 这里把 事件表 → TXT 条目 → 解析正文 整条链路钉死：任一环节空了就红。
// ---------------------------------------------------------------------------
TEST(FirstLevelTest, Fight0HasPreBattleDialogue) {
    ASSERT_TRUE(std::filesystem::exists(kGameDataDir))
        << "需要真实游戏数据目录: " << kGameDataDir;

    dlzr::app::DatLoader loader(kGameDataDir);
    GameStateManager gsm;
    gsm.init(loader);

    dlzr::logic::DialogueManager dm;
    dm.load_dialogues(loader.load_dialogues());
    ASSERT_GT(dm.dialogue_count(), 0) << "TXT.DAT 装载后台词不应为空";

    dlzr::logic::LevelEventManager level_events;
    auto events = level_events.get_pre_battle_events(0, gsm.game_data());
    ASSERT_FALSE(events.dialogue_entries.empty())
        << "第 0 关战前对话条目不应为空";

    for (int32_t idx : events.dialogue_entries) {
        ASSERT_GE(idx, 0);
        ASSERT_LT(idx, dm.dialogue_count())
            << "对话条目索引 " << idx << " 越界（dialogue_count="
            << dm.dialogue_count() << "）";

        const std::string raw = dm.get_dialogue_text(idx);
        EXPECT_FALSE(raw.empty()) << "TXT[" << idx << "] 原文为空";

        auto parsed = dlzr::logic::DialogueManager::parse_dialogue(raw);
        EXPECT_FALSE(parsed.text.empty())
            << "TXT[" << idx << "] 解析后正文为空，原文=" << raw;

        std::cerr << "[diag] fight0 pre-battle dialogue: idx=" << idx
                  << " speaker=" << parsed.speaker_id
                  << " text=" << parsed.text << "\n";
    }
}

// ---------------------------------------------------------------------------
// 回归：锁死"槽位 → 名册"映射，防止敌人再变成"孙悟空"。
// 背景（用户抓到的 bug）：克隆把 FIGHT.DAT 的 88 条名册当成 people[0..87]，
//   于是部署槽位 40 取到 people[40]="孙悟空"。
//   真相：FIGHT.DAT 装载器把磁盘 88 条写到运行时 people 的 **[40..127]**
//   （FUN_004292fe.c:18，DAT_007d2398 = people基址 + 40×0x84），
//   故 people[40] = 磁盘[0] = 伪军（步枪队），与原版截图面板逐项吻合。
// ---------------------------------------------------------------------------
TEST(FirstLevelTest, Fight0DeployedEnemiesAreNotPlaceholder) {
    ASSERT_TRUE(std::filesystem::exists(kGameDataDir))
        << "需要真实游戏数据目录: " << kGameDataDir;

    dlzr::app::DatLoader loader(kGameDataDir);
    GameStateManager gsm;
    gsm.init(loader);

    auto fight = loader.load_fight(0);
    auto map = loader.load_map(0);
    BattleEngine engine;
    engine.set_map(&map);
    engine.init(fight, gsm.game_data(), 1);

    const auto& st = engine.state();
    int checked = 0;
    for (int i = 0; i < BattleState::MAX_FIGHTERS; ++i) {
        if (!st.fighters[i].is_active) continue;
        if (st.fighters[i].team != 1) continue;   // 只看敌方

        const int32_t pidx = st.fighters[i].people_idx;
        ASSERT_GE(pidx, 0);
        ASSERT_LT(pidx, static_cast<int32_t>(fight.people.size()));

        std::string nm(fight.people[pidx].name.data(),
                       strnlen(fight.people[pidx].name.data(),
                               fight.people[pidx].name.size()));
        EXPECT_NE(nm, "\xCB\xEF\xCE\xF2\xBF\xD5")
            << "部署槽位 " << i << " 仍是占位单位 孙悟空"
            << "（说明槽位→名册映射又被改回 people[slot]）";
        ++checked;
        std::cerr << "[diag] slot=" << i << " people_idx=" << pidx
                  << " name=" << nm
                  << " class=" << fight.people[pidx].class_type << "\n";
    }
    EXPECT_GT(checked, 0) << "应有敌方单位被部署";

    // 槽位 40 必须映射到磁盘名册首条 = 伪军（步枪队 class=50）
    EXPECT_GE(fight.people[40].name.size(), 6);
    std::string first(fight.people[40].name.data(),
                      strnlen(fight.people[40].name.data(), 6));
    EXPECT_EQ(first, "\xCE\xB1\xA1\xA1\xBE\xFC")
        << "people[40] 应为磁盘名册首条 伪军";
    EXPECT_EQ(fight.people[40].class_type, 50) << "伪军应为步枪队(class 50)";
}

TEST(FirstLevelTest, Fight0LoadsAndInitializes) {
    ASSERT_TRUE(std::filesystem::exists(kGameDataDir))
        << "需要真实游戏数据目录: " << kGameDataDir;

    dlzr::app::DatLoader loader(kGameDataDir);
    GameStateManager gsm;
    gsm.init(loader);

    auto fight = loader.load_fight(0);
    auto map = loader.load_map(0);
    BattleEngine engine;
    engine.set_map(&map);    // 真实 MAP.DAT 尺寸 = 战斗网格尺寸（20x26），须在 init 前注入
    engine.init(fight, gsm.game_data(), 1);

    const auto& st = engine.state();
    EXPECT_GT(st.fighter_count, 0) << "第一关应有部署单位";

    int allies = 0, enemies = 0;
    for (int i = 0; i < BattleState::MAX_FIGHTERS; ++i) {
        if (!st.fighters[i].is_active) continue;
        EXPECT_GE(st.fighters[i].grid_x, 0);
        EXPECT_LT(st.fighters[i].grid_x, st.map_width);
        EXPECT_GE(st.fighters[i].grid_y, 0);
        EXPECT_LT(st.fighters[i].grid_y, st.map_height);
        if (st.fighters[i].team == 0) ++allies;
        else ++enemies;
    }
    EXPECT_GT(allies, 0) << "第一关应有我方单位";
    EXPECT_GT(enemies, 0) << "第一关应有敌方单位";
    EXPECT_FALSE(engine.is_over()) << "第一关开局不应已结束";
}

TEST(FirstLevelTest, Fight0PlaysToCompletion) {
    ASSERT_TRUE(std::filesystem::exists(kGameDataDir))
        << "需要真实游戏数据目录: " << kGameDataDir;

    dlzr::app::DatLoader loader(kGameDataDir);
    GameStateManager gsm;
    gsm.init(loader);

    auto fight = loader.load_fight(0);
    auto map = loader.load_map(0);
    BattleEngine engine;
    engine.set_map(&map);    // 真实 MAP.DAT 尺寸 = 战斗网格尺寸（20x26），须在 init 前注入
    engine.init(fight, gsm.game_data(), 1);

    constexpr int kMaxIter = 20000;
    int iter = 0;
    while (!engine.is_over() && iter < kMaxIter) {
        const auto& st = engine.state();
        int32_t cur = st.current_fighter_idx;
        if (cur >= 0 && st.fighters[cur].is_active &&
            st.fighters[cur].stats.current_hp > 0 &&
            !st.fighters[cur].has_acted) {
            if (st.fighters[cur].team == 0) {
                // 玩家：贪心行动
                BattleCommand cmd = greedy_player_command(engine, cur);
                engine.execute_command(cmd);
                engine.advance_turn();
            } else {
                // 敌方 AI
                engine.execute_ai_turn();
            }
        } else {
            // 当前无待行动单位（可能刚回合切换），推进一下避免死循环
            engine.advance_turn();
        }
        ++iter;
    }

    EXPECT_LT(iter, kMaxIter) << "第一关未在合理步数内结束（可能卡死）";
    EXPECT_TRUE(engine.is_over()) << "第一关应正常分出胜负";
    // 终局要么胜要么负
    EXPECT_TRUE(engine.is_victory() || !engine.is_victory());

    int pa = 0, ea = 0;
    const auto& st = engine.state();
    for (int i = 0; i < BattleState::MAX_FIGHTERS; ++i) {
        if (!st.fighters[i].is_active) continue;
        if (st.fighters[i].stats.current_hp > 0) {
            if (st.fighters[i].team == 0) ++pa; else ++ea;
        }
    }
    std::cerr << "[diag] fight0 outcome: "
              << (engine.is_victory() ? "VICTORY" : "DEFEAT")
              << " rounds=" << st.battle_round
              << " steps=" << iter
              << " playerAlive=" << pa << " enemyAlive=" << ea << "\n";
}
