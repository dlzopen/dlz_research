// test_case9_item_recovery.cpp — case 9 物品回收链（FUN_0042b23a 全链）分支精确红测
//
// 目的：把战斗结算 case 9（武器命中后，从阵亡/被夺单位回收首个非空物品栏槽）的
// 三返回码分支 + 边界语义，用受控 g_game_state 钉死为确定性行为锁。
//
// 铁律零逐指令证据（replica oracle = battle_calc_take_item_to_depot，mirror clean take_item_to_depot）：
//   FUN_0042b23a.c（31 行）：recipient=param_1, donor=param_2；扫 donor.items[0..7] 首个非空槽；
//     item_idx = item_id-1；调 FUN_0042920d(recipient, item_idx)；若返回 1（recipient 满）
//     → DAT_007db3a8[item_idx]++（战斗仓库）；末清 donor 原槽 = 0。
//   FUN_0042920d.c（27 行）：item_idx<0x9c 时找 recipient.items 首个空槽填入并压实 → 返 0；
//     无空槽 → 返 1（→ 仓库）；item_idx>=0x9c → 返 2（丢弃，不进仓库）。
//   FUN_004290f8.c（24 行）：recipient.items 非零项压实到前端（8 字节）。
//
// 与 test_skill_grid_diff 的关系（诚实说明）：test_skill_grid_diff 用 1 万+ 次随机跨层差分
// 间接校验 case 9 整链（clean w.depot ↔ replica g_warehouse_items_battle_0x7db3a8 数组等价），
// 但**不显式断言三返回码分支的各自语义**。本测试显式锁：
//   branch 0：recipient 有空槽 → 物品装入 recipient（donor 原槽清空），仓库不变；
//   branch 1：recipient 满     → 物品收归仓库（donor 原槽清空），recipient 不变；
//   branch 2：item_id>=0x9c    → 丢弃（donor 原槽清空），recipient/仓库均不变；
//   边界：item_id==0x9c（155<0x9c）→ 走 placement，不丢弃；
//   附加：donor 首个非空槽在 slot 3（非 slot 0）也能被取走；donor 无物品 → 纯 no-op。
//
// 受控夹具：保存/清零 g_game_state + g_warehouse_items_battle_0x7db3a8，杜绝跨测试全局污染。

#include <gtest/gtest.h>
#include <cstring>

#include "replica/replica_stubs.h"   // ::GameState g_game_state
#include "replica/battle_sim.hpp"     // battle_calc_take_item_to_depot, g_warehouse_items_battle_0x7db3a8

namespace {

class Case9Fixture : public ::testing::Test {
protected:
    GameState saved_state_;
    int16_t   saved_depot_[256];

    void SetUp() override {
        saved_state_ = g_game_state;
        std::memset(&g_game_state, 0, sizeof(GameState));
        std::memcpy(saved_depot_, g_warehouse_items_battle_0x7db3a8, sizeof(saved_depot_));
        std::memset(g_warehouse_items_battle_0x7db3a8, 0, sizeof(g_warehouse_items_battle_0x7db3a8));
    }
    void TearDown() override {
        g_game_state = saved_state_;
        std::memcpy(g_warehouse_items_battle_0x7db3a8, saved_depot_, sizeof(saved_depot_));
    }
};

// 把 people_array[idx].items 设为给定 8 字节（item_id，0=空）。
static void set_items(uint32_t idx, const uint8_t v[8]) {
    for (int i = 0; i < 8; ++i) g_game_state.people_array[idx].items[i] = v[i];
}

// branch 0：recipient 有空槽 → 物品装入 recipient，donor 原槽清空，仓库不变。
TEST_F(Case9Fixture, RecipientHasSpaceTakesItem) {
    uint8_t donor[8] = {5, 0, 0, 0, 0, 0, 0, 0};   // 物品栏 item_id=5 在 slot 0
    uint8_t recip[8] = {0, 0, 0, 0, 0, 0, 0, 0};   // 空
    set_items(0, recip);
    set_items(1, donor);

    battle_calc_take_item_to_depot(/*recipient=*/0, /*donor=*/1);

    // recipient 取得 item_id=5（item_idx=4），装入首个空槽 0（已在前端，压实无变化）
    EXPECT_EQ((int)g_game_state.people_array[0].items[0], 5);
    for (int i = 1; i < 8; ++i)
        EXPECT_EQ((int)g_game_state.people_array[0].items[i], 0) << "recip slot " << i;
    // donor 原槽清空
    EXPECT_EQ((int)g_game_state.people_array[1].items[0], 0);
    // 仓库不变（item_idx=4 应保持 0）
    EXPECT_EQ((int)g_warehouse_items_battle_0x7db3a8[4], 0);
}

// branch 1：recipient 满 → 物品收归仓库，recipient 不变，donor 原槽清空。
TEST_F(Case9Fixture, RecipientFullGoesToDepot) {
    uint8_t donor[8] = {9, 0, 0, 0, 0, 0, 0, 0};   // item_id=9 → item_idx=8
    uint8_t recip[8] = {1, 2, 3, 4, 5, 6, 7, 8};   // 8 槽全满
    set_items(0, recip);
    set_items(1, donor);

    battle_calc_take_item_to_depot(0, 1);

    // recipient 8 槽保持原样（满，无空位）
    for (int i = 0; i < 8; ++i)
        EXPECT_EQ((int)g_game_state.people_array[0].items[i], (i + 1)) << "recip slot " << i;
    // donor 原槽清空
    EXPECT_EQ((int)g_game_state.people_array[1].items[0], 0);
    // 仓库 item_idx=8 递增 1（二进制 FUN_0042b23a.c:25 DAT_007db3a8[item_idx]++）
    EXPECT_EQ((int)g_warehouse_items_battle_0x7db3a8[8], 1);
}

// branch 2：item_id>=0x9c → 丢弃，recipient/仓库均不变，donor 原槽清空。
TEST_F(Case9Fixture, InvalidItemIdDiscarded) {
    uint8_t donor[8] = {0x9d, 0, 0, 0, 0, 0, 0, 0};  // item_id=0x9d=157 → item_idx=156>=0x9c
    uint8_t recip[8] = {0, 0,0,0,0,0,0,0};
    set_items(0, recip);
    set_items(1, donor);

    battle_calc_take_item_to_depot(0, 1);

    // recipient 不变
    for (int i = 0; i < 8; ++i)
        EXPECT_EQ((int)g_game_state.people_array[0].items[i], 0) << "recip slot " << i;
    // donor 原槽清空（无论走哪条路径都清）
    EXPECT_EQ((int)g_game_state.people_array[1].items[0], 0);
    // 仓库不变（item_idx=156 丢弃，不进仓库）
    EXPECT_EQ((int)g_warehouse_items_battle_0x7db3a8[156], 0);
}

// 边界：item_id==0x9c（item_idx=155 < 0x9c）→ 走 placement，不丢弃。
TEST_F(Case9Fixture, ItemIdBoundary0x9cStillPlaced) {
    uint8_t donor[8] = {0x9c, 0, 0, 0, 0, 0, 0, 0};  // item_id=156 → item_idx=155 < 0x9c
    uint8_t recip[8] = {0, 0,0,0,0,0,0,0};
    set_items(0, recip);
    set_items(1, donor);

    battle_calc_take_item_to_depot(0, 1);

    // recipient 取得 item_id=0x9c（非丢弃）
    EXPECT_EQ((int)g_game_state.people_array[0].items[0], 0x9c);
    EXPECT_EQ((int)g_game_state.people_array[1].items[0], 0);
    EXPECT_EQ((int)g_warehouse_items_battle_0x7db3a8[155], 0);
}

// 附加：donor 首个非空槽在 slot 3（非 slot 0）也能被取走，且只清该槽。
TEST_F(Case9Fixture, FirstNonEmptySlotTakenNotSlotZero) {
    uint8_t donor[8] = {0, 0, 0, 7, 0, 0, 0, 0};   // 首个非空在 slot 3，item_id=7
    uint8_t recip[8] = {0, 0,0,0,0,0,0,0};
    set_items(0, recip);
    set_items(1, donor);

    battle_calc_take_item_to_depot(0, 1);

    // recipient 取得 item_id=7（slot 3 的首个非空语义，FUN_0042b23a.c:19 扫 0..7）
    EXPECT_EQ((int)g_game_state.people_array[0].items[0], 7);
    for (int i = 1; i < 8; ++i)
        EXPECT_EQ((int)g_game_state.people_array[0].items[i], 0) << "recip slot " << i;
    // donor 仅 slot 3 清空，slot 0..2 本就空
    EXPECT_EQ((int)g_game_state.people_array[1].items[3], 0);
    EXPECT_EQ((int)g_warehouse_items_battle_0x7db3a8[6], 0);  // item_idx=6
}

// 附加：donor 无物品 → 纯 no-op，任何状态不变。
TEST_F(Case9Fixture, DonorEmptyIsNoOp) {
    uint8_t donor[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    uint8_t recip[8] = {1, 2, 0, 0, 0, 0, 0, 0};
    set_items(0, recip);
    set_items(1, donor);

    battle_calc_take_item_to_depot(0, 1);

    for (int i = 0; i < 8; ++i)
        EXPECT_EQ((int)g_game_state.people_array[0].items[i], (i < 2 ? i + 1 : 0)) << "recip slot " << i;
    for (int i = 0; i < 8; ++i)
        EXPECT_EQ((int)g_game_state.people_array[1].items[i], 0) << "donor slot " << i;
}

}  // namespace
