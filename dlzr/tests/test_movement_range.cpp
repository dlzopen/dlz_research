// test_movement_range.cpp — Phase 5 战斗内移动范围 1:1（地形加权）
//
// 铁律零范围声明：本测试证 "BattleEngine::calc_movement_range 的可达集 == 二进制
// FUN_00418e29 洪水填充的可达集"，代价模型 = class_move_cost[进入格地形>>12][兵种]、
// 预算 = 移动力（FUN_00418e29.c:14-54 / FUN_00416c70.c:24 / FUN_0042a95b.c:10）。
// 地形来自 MAP.DAT 瓦片（grid_terrain 存 (tile>>12)&0xF），非 FIGHT.DAT grid_terrain[92]
// （后者经全量扫描从未写入地形缓冲，实为坐标/覆盖层）。

#include "app/resource/dat_loader.hpp"
#include "logic/battle/battle_engine.hpp"

#include <gtest/gtest.h>
#include <climits>
#include <algorithm>
#include <filesystem>
#include <queue>
#include <vector>

using namespace dlzr::logic;

namespace {
const char* kGameDataDir = "C:/seesaw/source/dlz_research/DLZFULL";
}  // namespace

// 与 BattleEngine::is_fighter_alive 完全一致的可达性判定（测试内复刻，避免调用私有方法）
static bool fighter_alive(const BattleState& st, int32_t i) {
    if (i < 0 || i >= BattleState::MAX_FIGHTERS) return false;
    return st.fighters[i].is_active && st.fighters[i].stats.current_hp > 0;
}

// 从零重演 FUN_00418e29 的地形加权可达集（与 calc_movement_range 同源代价模型），
// 用于交叉校验实现未偏离二进制算法。
static std::vector<bool> reenact_movement_range(const BattleState& st,
                                                const dlzr::core::GameData& game,
                                                int32_t fighter_idx,
                                                int32_t budget) {
    const int32_t w = st.map_width, h = st.map_height;
    const auto& fighter = st.fighters[fighter_idx];
    const int32_t cls = fighter.class_type;
    const int32_t sx = fighter.grid_x, sy = fighter.grid_y;
    std::vector<bool> reach(st.grid_cells(), false);
    if (sx < 0 || sx >= w || sy < 0 || sy >= h) return reach;

    const int32_t start = sy * w + sx;
    std::vector<int32_t> best(st.grid_cells(), INT32_MAX);
    auto cmp = [](const std::pair<int32_t, int32_t>& a,
                  const std::pair<int32_t, int32_t>& b) { return a.first > b.first; };
    std::priority_queue<std::pair<int32_t, int32_t>,
                       std::vector<std::pair<int32_t, int32_t>>, decltype(cmp)> pq(cmp);

    best[start] = 0;
    pq.push({0, start});
    reach[start] = true;

    constexpr int dx[] = {0, 0, -1, 1};
    constexpr int dy[] = {-1, 1, 0, 0};

    while (!pq.empty()) {
        auto [cd, cell] = pq.top();
        pq.pop();
        if (cd > best[cell]) continue;
        int32_t cx = cell % w, cy = cell / w;
        for (int d = 0; d < 4; ++d) {
            int32_t nx = cx + dx[d], ny = cy + dy[d];
            if (nx < 0 || nx >= w || ny < 0 || ny >= h) continue;
            int32_t ncell = ny * w + nx;
            if (ncell == start) continue;

            int32_t flag = st.grid_terrain[ncell] & 0xF;
            int32_t step = (flag < 16) ? game.class_move_cost[cls][flag] : 99;
            if (step >= 99) continue;

            bool blocked = false;
            for (int i = 0; i < BattleState::MAX_FIGHTERS; ++i) {
                if (i == fighter_idx) continue;
                const auto& f = st.fighters[i];
                if (fighter_alive(st, i) && f.grid_x == nx && f.grid_y == ny) {
                    blocked = true;
                    break;
                }
            }
            if (blocked) continue;

            int32_t nd = cd + step;
            if (nd > budget) continue;
            if (nd < best[ncell]) {
                best[ncell] = nd;
                reach[ncell] = true;
                pq.push({nd, ncell});
            }
        }
    }
    return reach;
}

// grid_terrain 必须由 MAP.DAT 瓦片填充（非全 0 占位），否则移动范围退化为均匀步进。
TEST(MovementRangeTest, GridTerrainFilledFromMap) {
    ASSERT_TRUE(std::filesystem::exists(kGameDataDir));
    dlzr::app::DatLoader loader(kGameDataDir);
    auto map = loader.load_map(0);

    BattleEngine eng;
    dlzr::core::GameData game = loader.load_game_data();
    dlzr::core::FightData fight = loader.load_fight(0);
    eng.init(fight, game, 1);
    eng.set_map(&map);

    const auto& st = eng.state();
    ASSERT_EQ((int)st.grid_terrain.size(), map.width * map.height);
    int nonzero = 0;
    for (int32_t i = 0; i < (int)st.grid_terrain.size(); ++i) {
        EXPECT_GE(st.grid_terrain[i], 0);
        EXPECT_LE(st.grid_terrain[i], 15);
        if (st.grid_terrain[i] != 0) ++nonzero;
    }
    // 真实地图必有非 0 地形（至少存在 !=0 的旗标），证明已接真实数据。
    EXPECT_GT(nonzero, 0) << "grid_terrain 仍全 0：MAP.DAT 地形未接入";
}

// calc_movement_range 可达集 == 二进制 FUN_00418e29 地形加权可达集（真实 fight0/map0）。
TEST(MovementRangeTest, MatchesBinaryFloodFill) {
    ASSERT_TRUE(std::filesystem::exists(kGameDataDir));
    dlzr::app::DatLoader loader(kGameDataDir);
    auto map = loader.load_map(0);
    dlzr::core::GameData game = loader.load_game_data();
    dlzr::core::FightData fight = loader.load_fight(0);

    BattleEngine eng;
    eng.init(fight, game, 1);
    eng.set_map(&map);

    // 找一个在地图范围内、已部署的单位作为移动主体。
    int idx = -1;
    const auto& st = eng.state();
    for (int i = 0; i < BattleState::MAX_FIGHTERS; ++i) {
        const auto& f = st.fighters[i];
        if (fighter_alive(st, i) && f.grid_x >= 0 && f.grid_x < map.width &&
            f.grid_y >= 0 && f.grid_y < map.height) {
            idx = i;
            break;
        }
    }
    ASSERT_GE(idx, 0) << "未找到可移动单位";

    const int32_t budget = eng.get_movement_range(idx);
    ASSERT_GT(budget, 0);

    eng.calc_movement_range(idx);
    const auto& after = eng.state();
    auto expect = reenact_movement_range(after, game, idx, budget);

    ASSERT_EQ(expect.size(), after.movement_mask.size());
    int mismatches = 0;
    for (size_t i = 0; i < expect.size(); ++i) {
        if (expect[i] != after.movement_mask[i]) ++mismatches;
    }
    EXPECT_EQ(mismatches, 0)
        << "calc_movement_range 与二进制 FUN_00418e29 地形加权模型分歧 "
        << mismatches << " 格 (单位#" << idx << " 预算=" << budget << ")";
}

// 保真闸门（铁律零，红测驱动）：干净层移动预算必须 1:1 对齐二进制求和公式，
// 分叉已闭合（此前锁「求积/100、不含物品」的契约已作废）。
//   二进制 FUN_0040124c.c:32 预算 param_3 = iVar3 + iVar2：
//     iVar3 = 运行时兵种表[class].movement（FUN_0040124c.c:30, stride 0xa0）；
//     iVar2 = FUN_0042b192(unit) = 8 槽装备物品的最大移动加成
//             （FUN_0042b192.c:16-21: 物品存在 & item-1>0x50 & wupin_compat[item-1][class]!=0
//              → max(WuPin[item-1].para1)）。
//   干净层 BattleEngine::get_movement_range 现 = cls.movement + max_item_bonus（求和）。
//   本测试复刻同公式（与引擎同源），断言两者一致——证明引擎已对齐二进制，不再锁旧契约。
TEST(MovementRangeTest, BudgetIsSumOfClassMovementPlusMaxItemBonus) {
    ASSERT_TRUE(std::filesystem::exists(kGameDataDir));
    dlzr::app::DatLoader loader(kGameDataDir);
    dlzr::core::GameData game = loader.load_game_data();
    dlzr::core::FightData fight = loader.load_fight(0);

    BattleEngine eng;
    eng.init(fight, game, 1);
    const auto& st = eng.state();

    int found = 0;
    for (int i = 0; i < BattleState::MAX_FIGHTERS; ++i) {
        const auto& f = st.fighters[i];
        if (!(f.is_active && f.grid_x >= 0 && f.grid_x < st.map_width &&
              f.grid_y >= 0 && f.grid_y < st.map_height))
            continue;

        // 复刻二进制公式（与 get_movement_range 同源，避免调用私有方法）。
        const auto& cls = (f.class_type >= 0 && f.class_type < dlzr::core::MAX_CLASS_TYPES)
                              ? game.class_types[f.class_type]
                              : game.class_types[0];
        int32_t base = cls.movement;
        int32_t bonus = 0;
        for (size_t s = 0; s < f.items.size(); ++s) {
            uint8_t item_id = f.items[s];
            if (item_id == 0) continue;
            int item_idx = static_cast<int>(item_id) - 1;
            if (item_idx < 0 || item_idx >= dlzr::core::MAX_WUPIN) continue;
            if (!(item_idx > 0x50)) continue;                       // FUN_0042b192.c:17
            if (f.class_type < 0 || f.class_type >= dlzr::core::MAX_CLASS_TYPES) continue;
            if (game.wupin_compat[item_idx][f.class_type] == 0) continue;
            bonus = std::max(bonus, game.items[item_idx].para1);
        }
        int32_t expect = base + bonus;

        EXPECT_EQ(eng.get_movement_range(i), expect)
            << "干净层预算必须 = 兵种移动力 + max(兼容装备物品 WuPin.para1)（求和，对齐 "
               "FUN_0040124c+FUN_0042b192）；单位#" << i << " class=" << f.class_type;
        ++found;
    }
    EXPECT_GT(found, 0) << "应至少校验一个在地图内的单位";
}
