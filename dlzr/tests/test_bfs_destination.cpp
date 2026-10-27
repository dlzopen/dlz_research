// test_bfs_destination.cpp — 战斗 BFS 流场：目的地选择度量 1:1
//   （二进制 find_nearest_free_cell  vs 干净 BattleEngine::select_destination_cell）
//
// 铁律零范围声明（重要勘误）：
//   此前版本误将干净层 bfs_step_toward 的落点度量与 *撤退* 例程
//   find_best_cell_terrain(FUN_0040144e) 对齐——后者在 HP/士气过低时 *最大化*
//   曼哈顿+地形{9,10,0xb}罚9 距离（FUN_0040144e.c:2091 `if(local_18<local_10)` 取最大），
//   方向与“朝目标前进”相反，属错误映射。
//   本测试的正确二进制对齐对象是 find_nearest_free_cell（replica_stubs.c:1998，被
//   AI 派发 FUN_00402ddc 的 case 4/0xe/0xf 等前进/追击分支调用）：它在加权可达集
//   （掩码==0=可达，=1 被 floodfill_move_8e29 清零）内 *最小化* 曼哈顿距离
//   （FUN_00402ddc 调用点 + find_nearest_free_cell.c:2027-2035：
//   local_10=|x-param2|+|y-param3|；if(local_10<local_1c) 更新，local_1c 初值=起点曼哈顿距离
//   → 严格优于起点才移动；*无*地形罚）。
//
// 因此本闸门隔离的是“落点选度量的数学量”差异：干净层须用曼哈顿（与二进制一致），
// 而非此前的平方欧氏。可达集本身已由 test_movement_range.cpp 证与 FUN_00418e29 1:1，
// 故本测试两侧共用 st.movement_mask（干净层 calc_movement_range 产物）作为候选集，
// 仅比较度量——这是合法隔离，因可达集已被前序闸门锁定。

#include "app/resource/dat_loader.hpp"
#include "logic/battle/battle_engine.hpp"

#include <gtest/gtest.h>
#include <climits>
#include <filesystem>
#include <vector>

using namespace dlzr::logic;

namespace {
const char* kGameDataDir = "C:/seesaw/source/dlz_research/DLZFULL";

// ---- 二进制 find_nearest_free_cell 落点度量：曼哈顿距离最小（严格优于起点，无地形罚）----
// 返回在 mask(=加权可达集) 内、离 (tx,ty) 曼哈顿距离最小、且严格小于起点距离的格索引；
// 无更优候选返回 -1。平局取首次遇到者（行优先扫描 + 严格 `<` → 索引最小者胜）。
static int32_t reenact_binary_best_cell(const std::vector<bool>& mask,
                                        int32_t w, int32_t h,
                                        int32_t fx, int32_t fy,
                                        int32_t tx, int32_t ty) {
    auto ai = [](int32_t v) { return v < 0 ? -v : v; };
    int32_t start_dist = ai(fx - tx) + ai(fy - ty);
    int32_t best = -1;
    int32_t best_score = start_dist;   // 严格优于起点才移动
    for (int32_t y = 0; y < h; ++y) {
        for (int32_t x = 0; x < w; ++x) {
            int32_t cell = y * w + x;
            if (!mask[cell]) continue;
            int32_t d = ai(x - tx) + ai(y - ty);
            if (d < best_score) {
                best_score = d;
                best = cell;
            }
        }
    }
    return best;
}

}  // namespace

// ============================================================
// 度量差异的确定性证明：曼哈顿 vs 平方欧氏 可在同掩码上选不同格
// （说明此前干净层用平方欧氏是真实偏离，须改用曼哈顿）
// ============================================================
TEST(BfsDestination, MetricDivergenceManhattanVsSquared) {
    // 7x7 网格，可达集含起点(4,4)与两候选：A=(5,0)、B=(3,3)，目标 (0,0)。
    //   曼哈顿：起点=8, A=5, B=6 → 二进制(严格优于起点且曼哈顿最小)选 A。
    //   平方欧氏：起点=32, A=25, B=18 → 平方选 B。
    // 二者分歧 → 证明干净层此前用平方欧氏是真实偏离，须改用曼哈顿。
    const int32_t w = 7, h = 7;
    std::vector<bool> mask(w * h, false);
    int32_t START = 4 * w + 4;   // (4,4) 起点（须在可达集内）
    int32_t A = 0 * w + 5;       // (5,0)
    int32_t B = 3 * w + 3;       // (3,3)
    mask[START] = true;
    mask[A] = true;
    mask[B] = true;
    int32_t bin = reenact_binary_best_cell(mask, w, h, 4, 4, 0, 0);
    EXPECT_EQ(bin, A) << "二进制应按曼哈顿选 (5,0)";

    // 平方欧氏会选 B——与曼哈顿分歧，证明干净层此前度量错误。
    int32_t sq_best = -1, sq_dist = INT32_MAX;
    auto ai = [](int32_t v) { return v < 0 ? -v : v; };
    for (int32_t y = 0; y < h; ++y)
        for (int32_t x = 0; x < w; ++x) {
            int32_t c = y * w + x;
            if (!mask[c]) continue;
            int32_t ddx = x - 0, ddy = y - 0;
            int32_t d = ddx * ddx + ddy * ddy;
            if (d < sq_dist) { sq_dist = d; sq_best = c; }
        }
    EXPECT_EQ(sq_best, B) << "平方欧氏会选 (3,3)，与曼哈顿分歧";
    EXPECT_NE(bin, sq_best);
}

// 平局时二进制取小索引优先（与扫描序 + 严格 `<` 一致）；干净层同序。
TEST(BfsDestination, TieBreakSmallIndex) {
    const int32_t w = 5, h = 5;
    std::vector<bool> mask(w * h, false);
    int32_t START = 2 * w + 2;   // (2,2) 起点，到(0,0)曼哈顿=4
    mask[START] = true;
    // (0,1) 与 (1,0) 到 (0,0) 曼哈顿都=1（均严格优于起点4）→ 平局，取索引小者 (0,1)=1。
    mask[0 * w + 1] = true;   // (0,1) index=1
    mask[1 * w + 0] = true;   // (1,0) index=5
    int32_t bin = reenact_binary_best_cell(mask, w, h, 2, 2, 0, 0);
    EXPECT_EQ(bin, 1);
}

// ============================================================
// 真实 fight0/map0 闸门：干净 select_destination_cell 落点 == 二进制 find_nearest_free_cell
// ============================================================
TEST(BfsDestination, RealMapMetricAgreement) {
    ASSERT_TRUE(std::filesystem::exists(kGameDataDir));
    dlzr::app::DatLoader loader(kGameDataDir);
    auto map = loader.load_map(0);
    dlzr::core::GameData game = loader.load_game_data();
    dlzr::core::FightData fight = loader.load_fight(0);

    BattleEngine eng;
    eng.init(fight, game, 1);
    eng.set_map(&map);

    const auto& st = eng.state();
    const int32_t w = st.map_width, h = st.map_height;
    ASSERT_EQ((int)st.movement_mask.size(), w * h);

    auto fighter_alive = [&](int32_t i) {
        if (i < 0 || i >= BattleState::MAX_FIGHTERS) return false;
        const auto& f = st.fighters[i];
        return f.is_active && f.stats.current_hp > 0;
    };

    // 对每个存活单位：先算其专属加权可达掩码（与二进制 0x833418 1:1），
    // 再对地图上每一个格作假想目标，比较真实干净落点选择与二进制重演。
    int mismatches = 0;
    int pairs = 0;
    for (int i = 0; i < BattleState::MAX_FIGHTERS; ++i) {
        if (!fighter_alive(i)) continue;
        const auto& u = st.fighters[i];
        if (u.grid_x < 0 || u.grid_x >= w || u.grid_y < 0 || u.grid_y >= h) continue;

        eng.calc_movement_range(i);          // 刷新 st.movement_mask 为该单位专属掩码
        const auto& mask = st.movement_mask;

        for (int ty = 0; ty < h; ++ty) {
            for (int tx = 0; tx < w; ++tx) {
                ++pairs;
                int32_t bin = reenact_binary_best_cell(mask, w, h,
                                                       u.grid_x, u.grid_y, tx, ty);
                int32_t cln = eng.select_destination_cell(i, tx, ty);
                if (bin != cln) {
                    ++mismatches;
                    if (mismatches <= 12) {
                        std::cerr << "[BfsDest mismatch] unit#" << i << "(" << u.grid_x << "," << u.grid_y
                                  << ") target=(" << tx << "," << ty
                                  << ") bin=" << (bin >= 0 ? (bin % w) : -1) << "," << (bin >= 0 ? (bin / w) : -1)
                                  << " cln=" << (cln >= 0 ? (cln % w) : -1) << "," << (cln >= 0 ? (cln / w) : -1)
                                  << std::endl;
                    }
                }
            }
        }
    }

    std::cerr << "[BfsDest] pairs=" << pairs << " mismatches=" << mismatches << std::endl;
    EXPECT_EQ(mismatches, 0)
        << "二进制(find_nearest_free_cell 曼哈顿)与干净(select_destination_cell)落点选择在 "
        << mismatches << "/" << pairs << " 对上分歧";
}
