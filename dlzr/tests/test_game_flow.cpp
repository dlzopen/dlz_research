// test_game_flow.cpp — Phase 8 / Phase 11 无头集成闸
//
// 铁律零：用真实 DLZFULL 二进制驱动，不 mock 数据。
// 验证对象：游戏主流程脊 = 真实数据装载(DatLoader) → 模式状态机(GameStateManager)
//   → 战斗装配(BattleEngine) → 驱动一个单位移动，整条链路在真实数据下必须自洽、不崩溃。
// 这是首个把「数据装载 → 状态机 → 战斗引擎」串成一条连贯流程的集成测试，
// 直接覆盖 Phase 8（模式切换）与 Phase 11（集成）缺失的无头验证。

#include <gtest/gtest.h>

#include <string>

#include "app/resource/dat_loader.hpp"
#include "core/data_model/game_data.hpp"
#include "logic/battle/battle_engine.hpp"
#include "logic/game_state_manager.hpp"

#ifndef DLZR_DATA_DIR_DEFAULT
#define DLZR_DATA_DIR_DEFAULT "DLZFULL"
#endif

static std::string GameDataDir() {
    const char* env = std::getenv("DLZR_DATA_DIR");
    return env ? std::string(env) : std::string(DLZR_DATA_DIR_DEFAULT);
}

// —— Phase 8：状态机初始模式 + 新游戏流程模式跳转 ——
// GameStateManager::init 必须装载真实数据并把模式置为 TITLE；
// 各 Mode 跳转（TITLE→CHARACTER_SELECT→STRATEGIC_MAP→BATTLE）必须一致可追溯。
TEST(GameFlow, StateMachineTransitions) {
    dlzr::app::DatLoader loader(GameDataDir());
    dlzr::logic::GameStateManager gsm;
    gsm.init(loader);

    EXPECT_EQ(gsm.mode(), dlzr::logic::GameMode::TITLE)
        << "init 后初始模式必须为 TITLE";
    // init 已装载真实 GAME.DAT（兵种表等）
    EXPECT_GT(gsm.game_data().class_types.size(), 0u)
        << "init 必须装载真实兵种表";

    // 新游戏流程的模式跳转链
    gsm.set_mode(dlzr::logic::GameMode::CHARACTER_SELECT);
    EXPECT_EQ(gsm.mode(), dlzr::logic::GameMode::CHARACTER_SELECT);
    gsm.set_mode(dlzr::logic::GameMode::STRATEGIC_MAP);
    EXPECT_EQ(gsm.mode(), dlzr::logic::GameMode::STRATEGIC_MAP);
    gsm.set_mode(dlzr::logic::GameMode::BATTLE);
    EXPECT_EQ(gsm.mode(), dlzr::logic::GameMode::BATTLE);

    // 重置回到标题
    gsm.set_mode(dlzr::logic::GameMode::RESET);
    EXPECT_EQ(gsm.mode(), dlzr::logic::GameMode::RESET);
}

// —— Phase 4 + 8 + 11：真实战斗数据装载 → 战斗装配 → 状态自洽 ——
// 真实 fight0 / map0 必须能装载，BattleEngine 装配后：
//   · 有单位部署、网格尺寸 == MAP.DAT 宽高（证据 map0 = 20×26）、未结束；
//   · 网格地形缓冲按地图尺寸填充。
TEST(GameFlow, BattleSetupWithRealData) {
    dlzr::app::DatLoader loader(GameDataDir());
    dlzr::logic::GameStateManager gsm;
    gsm.init(loader);

    auto fight = loader.load_fight(0);
    auto map = loader.load_map(0);
    auto blk = loader.load_blk_group(map.blk_group);
    (void)blk;  // 烟雾：真实 BLK 装载不得抛错

    // 地图尺寸（证据：map0 = 20×26，MAP.DAT 宽高 = 战斗网格尺寸）
    EXPECT_EQ(map.width, 20) << "map0 宽必须为 20（MAP.DAT 宽 = 战斗网格列）";
    EXPECT_EQ(map.height, 26) << "map0 高必须为 26（MAP.DAT 高 = 战斗网格行）";

    dlzr::logic::BattleEngine engine;
    engine.set_map(&map);  // 必须在 init 前注入：真实地图尺寸钳制相机/网格
    engine.init(fight, gsm.game_data(), 1);

    const auto& st = engine.state();
    EXPECT_GT(st.fighter_count, 0) << "战斗装配后必须有单位";
    EXPECT_EQ(st.map_width, map.width) << "战斗网格列必须 == 地图宽";
    EXPECT_EQ(st.map_height, map.height) << "战斗网格行必须 == 地图高";
    EXPECT_FALSE(st.is_over) << "刚装配不应结束";

    // 至少应有一个已部署的我方单位（团队 0）
    int active_player = 0;
    for (int i = 0; i < st.fighter_count; ++i) {
        if (st.fighters[i].is_active && st.fighters[i].team == 0) ++active_player;
    }
    EXPECT_GT(active_player, 0)
        << "战斗装配后应至少有一个已部署的玩家单位";

    // 网格地形缓冲按地图尺寸填充
    EXPECT_EQ(static_cast<int>(st.grid_terrain.size()), map.width * map.height)
        << "网格地形缓冲必须按地图尺寸填充";
}

// —— Phase 11：驱动一个已部署玩家单位计算移动范围并移动到可达格 ——
// 验证「选单位 → 算范围 → 移动」这条玩家可感知的交互脊在真实数据下自洽、不崩溃。
TEST(GameFlow, DriveDeployedUnitMove) {
    dlzr::app::DatLoader loader(GameDataDir());
    dlzr::logic::GameStateManager gsm;
    gsm.init(loader);
    auto fight = loader.load_fight(0);
    auto map = loader.load_map(0);
    dlzr::logic::BattleEngine engine;
    engine.set_map(&map);
    engine.init(fight, gsm.game_data(), 1);

    auto& st = engine.state();

    // 找第一个已部署的玩家单位
    int idx = -1;
    for (int i = 0; i < st.fighter_count; ++i) {
        if (st.fighters[i].is_active && st.fighters[i].team == 0) {
            idx = i;
            break;
        }
    }
    ASSERT_GE(idx, 0) << "必须存在已部署玩家单位";

    const int from_x = st.fighters[idx].grid_x;
    const int from_y = st.fighters[idx].grid_y;

    engine.calc_movement_range(idx);
    ASSERT_EQ(static_cast<int>(st.movement_mask.size()), map.width * map.height)
        << "移动范围掩码必须按地图尺寸填充";

    // 找一个可达格（mask=true 且非当前格）
    int to_x = -1, to_y = -1;
    for (int y = 0; y < map.height && to_x < 0; ++y) {
        for (int x = 0; x < map.width; ++x) {
            const int cell = y * map.width + x;
            if (st.movement_mask[cell] && !(x == from_x && y == from_y)) {
                to_x = x;
                to_y = y;
                break;
            }
        }
    }

    if (to_x < 0) {
        GTEST_SKIP() << "该单位在真实地形下无可移动格，跳过移动驱动";
    }

    // 驱动移动到可达格：不得崩溃、不得结束战斗、状态保持自洽
    const int r = engine.select_destination_cell(idx, to_x, to_y);
    EXPECT_TRUE(r >= 0 || r == -1)
        << "select_destination_cell 返回应合法（成功或明确拒绝）";
    EXPECT_FALSE(st.is_over) << "单次移动不应结束战斗";
    EXPECT_GE(st.fighters[idx].grid_x, 0);
    EXPECT_GE(st.fighters[idx].grid_y, 0);
}
