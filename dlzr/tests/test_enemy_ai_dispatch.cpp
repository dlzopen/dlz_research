// test_enemy_ai_dispatch.cpp — 敌方 AI 派发 + 攻击目标选择 忠实性（红测驱动）
// Enemy AI dispatch & attack-target selection fidelity (red-test driven)
//
// 铁律零范围声明：本测试证 "BattleEngine::generate_enemy_command 忠实复刻
// 原版三级派发 FUN_00403f8e → DAT_00446030 → FUN_00402ddc"：
//   · 主循环 FUN_00403f8e：按 behavior&0xF 取 DAT_00446030 行，列 0..15 依次
//     试派发，首个 FUN_00402ddc 返"决策成立"的列生效；全列不成立→待命。
//   · 派发表 DAT_00446030：从 reverse/DLZ.exe .data 节 VA 0x00446030 字节级抽取
//     （1024B=16×16 uint32），值即传给 FUN_00402ddc 的 ai_type。
//   · 单 ai_type 派发 FUN_00402ddc：阈值语义
//       hp<25%   : current_hp*4 < max_hp*1
//       hp<60%   : current_hp*5 < max_hp*3
//       hp受损   : current_hp != max_hp
//       morale<25: morale < 0x19
//       morale<60: morale < 0x3c
//       morale非满: morale != 100
//     动作编码（DAT_0044b230=0 原地攻 / 1 撤退 / 2 待命；DAT_0044b23c=0 移动 / 1 技能 / 2 攻击选定）。
//
// 攻击目标选择契约（find_best_attack_target / 各 ai_type chooser）：
//   原版 chooser 体系（FUN_00402ddc 调用图）与副本启发式机制不同——
//     · ai_type 2/3/12/13 用 FUN_00401697/FUN_004016ce：返回威胁索引 0/2 或 1/2
//       （读 per-fighter DAT_007d0f2c/0x34/0x30 旗标，非扫描）。
//     · ai_type 7/17/42/52/72 用 FUN_004018fb/FUN_00401c73/FUN_00401fff：
//       425B 同构扫描器，洪泛可达格(FUN_00418e29)后"自上而下、自左而右"取首个
//       命中敌人的格（per-cell 评估 FUN_00401708/FUN_00401aa4/FUN_00401e1c，
//       后者用技能形状戳 FUN_00418a70 + 单位栅 DAT_0057bf10/11）。
//   副本当前用 lowest-HP 启发式（见 battle_engine.cpp:find_best_attack_target）。
//   本测试的攻击选择闸门只锁"有效目标"契约（任何忠实重刻必满足），不锁具体
//   选择规则；具体机制差异记录于当日记忆 .workbuddy/memory/YYYY-MM-DD.md。
//
// 测试通过 friend class EnemyAiDispatchTest 直接操控私有 state_（设定 hp/士气）与
// 私有 generate_enemy_command / find_best_attack_target，覆盖需手动致伤/hp 门控路径；
// 不引入生产代码副作用。
// 注意：本 fixture 必须位于 namespace dlzr::logic，以匹配 battle_engine.hpp 中
// `friend class EnemyAiDispatchTest;` 的友元声明。

#include "logic/battle/battle_engine.hpp"
#include "core/data_model/game_data.hpp"
#include "core/data_model/types.hpp"

#include <cstring>
#include <vector>

#include "gtest/gtest.h"

namespace dlzr::logic {

class EnemyAiDispatchTest : public ::testing::Test {
protected:
    BattleEngine engine_;
    dlzr::core::FightData fight_;
    dlzr::core::GameData game_;
    dlzr::core::FightMap map_;

    void SetUp() override {
        for (int i = 0; i < 128; ++i) {
            fight_.fighters[i].stage = 0;
            fight_.fighters[i].grid_x = 999;
            fight_.fighters[i].grid_y = 999;
            fight_.fighters[i].behavior = 0;
        }
        game_.protagonist_face = -1;

        map_.width = 30;
        map_.height = 30;
        map_.tiles.assign(900, 0);
    }

    // 清场并激活 E(team1)@(15,15) 与若干我方单位(team0)。
    // heroes: 每项为 (slot, dx, dy)。返回后调用 generate_enemy_command(E) 即可派发。
    // 注意：behavior 由调用方在 setup_duel 之前设定（dispatch/直接调用均已设），
    // 本函数不得重置 behavior，否则会覆盖调用方指定的敌方 AI 类型。
    void setup_duel(int E, int e_hp, int e_max, int e_morale,
                    const std::vector<std::tuple<int,int,int>>& heroes) {
        engine_.init(fight_, game_, 1);
        engine_.set_map(&map_);

        for (int i = 0; i < BattleState::MAX_FIGHTERS; ++i) {
            engine_.state_.fighters[i].is_active = false;
        }

        auto& ef = engine_.state_.fighters[E];
        ef.is_active = true; ef.team = 1;
        ef.grid_x = 15; ef.grid_y = 15; ef.people_idx = E;
        ef.class_type = 1; ef.level = 1; ef.movement = 100;
        ef.stats.current_hp = e_hp; ef.stats.max_hp = e_max;
        ef.stats.morale = e_morale;

        for (auto& h : heroes) {
            int H = std::get<0>(h), hdx = std::get<1>(h), hdy = std::get<2>(h);
            auto& hf = engine_.state_.fighters[H];
            hf.is_active = true; hf.team = 0;
            hf.grid_x = 15 + hdx; hf.grid_y = 15 + hdy; hf.people_idx = H;
            hf.class_type = 1; hf.level = 1; hf.movement = 100;
            hf.stats.current_hp = 100; hf.stats.max_hp = 100;
            hf.stats.morale = 100;
        }
    }

    BattleCommand dispatch(int E, int H, int behavior,
                           int e_hp, int e_max, int e_morale,
                           int hdx, int hdy) {
        fight_.fighters[E].behavior = static_cast<uint8_t>(behavior);
        setup_duel(E, e_hp, e_max, e_morale, {{H, hdx, hdy}});
        return engine_.generate_enemy_command(E);
    }

    // 友元访问器：本 fixture（EnemyAiDispatchTest）是友元类，但其派生的 TEST_F 子类
    // 不继承友元，故私有引擎方法须经本类（基类）方法转发，不得在测试体直接调用。
    BattleCommand gen_cmd(int E) { return engine_.generate_enemy_command(E); }
    int32_t best_target(int E) { return engine_.find_best_attack_target(E); }
};

// —— behavior=1（登场最多 899 例）：低血(hp<25%) → 防御撤退(MOVE) ——
TEST_F(EnemyAiDispatchTest, Behavior1LowHpRetreats) {
    auto cmd = dispatch(40, 99, 1, 10, 100, 100, 1, 0);
    EXPECT_EQ(cmd.type, BattleCommand::MOVE)
        << "behavior=1 低血(hp<25%)应触发防御撤退(MOVE)，原副本错为巡逻";
    if (cmd.type == BattleCommand::MOVE) {
        EXPECT_TRUE(cmd.target_x != 15 || cmd.target_y != 15)
            << "撤退落点须异于当前格";
    }
}

// —— behavior=1 满血满士气 + 敌在射程 → 列序走到无条件攻击 → ATTACK ——
TEST_F(EnemyAiDispatchTest, Behavior1FullHpAttacksWhenEnemyInRange) {
    auto cmd = dispatch(40, 99, 1, 100, 100, 100, 1, 0);
    EXPECT_EQ(cmd.type, BattleCommand::ATTACK)
        << "behavior=1 满血且敌在射程应攻击(列序 ai_type=42/52/72/31)";
    EXPECT_EQ(cmd.target_idx, 99);
}

// —— behavior=1 仅低士气(morale<25) → 列1 ai_type=11 撤退(MOVE) ——
TEST_F(EnemyAiDispatchTest, Behavior1LowMoraleRetreats) {
    auto cmd = dispatch(40, 99, 1, 100, 100, 20, 1, 0);
    EXPECT_EQ(cmd.type, BattleCommand::MOVE)
        << "behavior=1 仅低士气(morale<25)应触发撤退(MOVE)";
}

// —— behavior=0 满血满士气且敌在射程外 → 全列不成立 → 待命(WAIT) ——
TEST_F(EnemyAiDispatchTest, Behavior0NoColumnFiresWaits) {
    auto cmd = dispatch(40, 99, 0, 100, 100, 100, 14, 14);
    EXPECT_EQ(cmd.type, BattleCommand::WAIT)
        << "behavior=0 满血且敌在射程外，所有列不成立应待命";
}

// —— 列回退：behavior=0 满血但 morale<60 → 列0 ai_type=2 不成立，列1 ai_type=12 成立 → ATTACK ——
TEST_F(EnemyAiDispatchTest, Behavior0ColumnFallbackToMoraleGate) {
    auto cmd = dispatch(40, 99, 0, 100, 100, 30, 1, 0);
    EXPECT_EQ(cmd.type, BattleCommand::ATTACK)
        << "behavior=0 满血但 morale<60 应回退到列1(ai_type=12)攻击";
}

// —— behavior=12（激进型，列0=ai_type=31 无条件攻击）→ 敌在射程即 ATTACK ——
TEST_F(EnemyAiDispatchTest, Behavior12UnconditionalAttack) {
    auto cmd = dispatch(40, 99, 12, 100, 100, 100, 1, 0);
    EXPECT_EQ(cmd.type, BattleCommand::ATTACK)
        << "behavior=12 列0=ai_type=31 无条件攻击，敌在射程应 ATTACK";
    EXPECT_EQ(cmd.target_idx, 99);
}

// —— behavior=2 低士气(morale<25) → 列0 ai_type=1 不成立 → 列1 ai_type=11 撤退 → MOVE ——
TEST_F(EnemyAiDispatchTest, Behavior2LowMoraleRetreats) {
    auto cmd = dispatch(40, 99, 2, 100, 100, 20, 1, 0);
    EXPECT_EQ(cmd.type, BattleCommand::MOVE)
        << "behavior=2 低士气(morale<25)应触发撤退(MOVE)";
}

// —— behavior=11（全零行）→ case 0 待命 → 立即 WAIT ——
TEST_F(EnemyAiDispatchTest, Behavior11EmptyRowWaits) {
    auto cmd = dispatch(40, 99, 11, 100, 100, 100, 1, 0);
    EXPECT_EQ(cmd.type, BattleCommand::WAIT)
        << "behavior=11 全零行(case 0 待命)应直接待命";
}

// ===================== 攻击目标选择契约闸门 =====================
// 锁"敌方攻击时选择的攻击目标"可观测契约：任何忠实重刻（含 FUN_00401697 威胁索引 /
// FUN_00401fff 可达格扫描）都必须满足，避免静默回归到"选错目标/越界/选自己人"。
// 诚实残留：具体选择规则（lowest-HP 启发式 vs 威胁索引/可达扫描）机制不同，见当日记忆。

// 唯一在射程内敌人 → 必须攻击该敌人（无歧义，任何重刻必满足）。
TEST_F(EnemyAiDispatchTest, SingleInRangeTargetIsAttacked) {
    auto cmd = dispatch(40, 99, 12, 100, 100, 100, 1, 0);  // behavior=12 col0=ai_type31 无条件攻
    EXPECT_EQ(cmd.type, BattleCommand::ATTACK);
    EXPECT_EQ(cmd.target_idx, 99)
        << "射程内唯一敌人必须被选中为攻击目标";
}

// 无敌人可攻击（远距）→ 不发起攻击（不越界/不选错目标）。
TEST_F(EnemyAiDispatchTest, NoTargetInRangeDoesNotAttack) {
    auto cmd = dispatch(40, 99, 12, 100, 100, 100, 14, 14);  // 远距≈20格
    EXPECT_NE(cmd.type, BattleCommand::ATTACK)
        << "射程外不应发起攻击";
}

// 两个在射程内敌人 → 确定性选中有效其一（不选中自己/友军/越界）。
TEST_F(EnemyAiDispatchTest, MultipleInRangeTargetsDeterministicValid) {
    fight_.fighters[40].behavior = 12;
    setup_duel(40, 100, 100, 100, {{98, 1, 0}, {99, -1, 0}});
    auto cmd = gen_cmd(40);
    ASSERT_EQ(cmd.type, BattleCommand::ATTACK);
    EXPECT_TRUE(cmd.target_idx == 98 || cmd.target_idx == 99)
        << "多目标时须选中有效在射程敌人之一";
    EXPECT_NE(cmd.target_idx, 40) << "不得选中自己";
}

// find_best_attack_target 直接契约：射程内唯一敌人 → 返回其索引；射程外 → 返回 -1；且确定性。
TEST_F(EnemyAiDispatchTest, FindBestAttackTargetContract) {
    // 唯一在射程内
    fight_.fighters[40].behavior = 12;
    setup_duel(40, 100, 100, 100, {{99, 1, 0}});
    int32_t t1 = best_target(40);
    EXPECT_EQ(t1, 99);
    int32_t t2 = best_target(40);
    EXPECT_EQ(t1, t2) << "选择须确定";

    // 射程外
    setup_duel(40, 100, 100, 100, {{99, 14, 14}});
    EXPECT_EQ(best_target(40), -1)
        << "射程外须返回 -1（无目标）";
}

}  // namespace dlzr::logic
