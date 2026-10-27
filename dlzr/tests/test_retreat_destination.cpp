// test_retreat_destination.cpp — 战斗撤退/防御性重定位落点选择 1:1
//   （二进制 find_best_cell_terrain  vs 干净 BattleEngine::select_retreat_cell）
//
// 铁律零范围声明：
//   干净层 ai_retreat 原用 *平方欧氏* 在可达集内选“离敌人最远”的格
//   （battle_engine.cpp 旧 ai_retreat: dist_sq > cur_dist && dist_sq > best_dist）。
//   但二进制对应例程是 find_best_cell_terrain(FUN_0040144e.c:2047-2104)，它 *最大化*
//   曼哈顿距离到敌人 + 地形{9,10,0xb}罚9（FUN_0040144e.c:2084-2091：
//   local_10=|x-attacker_x|+|y-attacker_y|; if(地形∈{9,10,0xb}) local_10+=9;
//   if(local_18 < local_10) 更新 → 取最大；起点格 g_pathfind_grid[start]=1 被排除）。
//   度量方向相同（都是“躲远”），但 *数学量* 不同：平方欧氏 vs 曼哈顿+地形罚。
//   本闸门隔离该度量差异——与 test_bfs_destination.cpp 对 select_destination_cell
//   的处理完全对等（前进用曼哈顿最小、撤退用曼哈顿+地形最大）。
//
// 隔离边界（诚实声明）：二进制以 fighter_array[param_1].attacker_index 为撤退目标敌人；
// 干净层 select_retreat_cell 以 find_nearest_enemy（平方欧氏最近敌人）为之。本测试两侧
// 均用“最近敌人”口径重演，故仅隔离 *度量*；attacker_index vs 最近敌人 属独立待办（见记忆）。

#include "app/resource/dat_loader.hpp"
#include "logic/battle/battle_engine.hpp"
#include "logic/battle/battle_state.hpp"

#include <gtest/gtest.h>
#include <climits>
#include <filesystem>
#include <vector>

using namespace dlzr::logic;

namespace {
const char* kGameDataDir = "C:/seesaw/source/dlz_research/DLZFULL";

// ---- 二进制 find_best_cell_terrain 落点度量：曼哈顿到敌人 + 地形{9,10,0xb}罚9，最大化 ----
// 在 mask(=加权可达集) 内、排除起点格，取 score=|x-ex|+|y-ey|+(9 if 地形∈{9,10,0xb}) 最大者；
// 平局取行优先首次遇到者（y 外 x 内扫描 + 严格 `>`）；无更优候选返回起点格。
static int32_t reenact_binary_retreat(const std::vector<bool>& mask,
                                      const std::vector<int32_t>& terr,
                                      int32_t w, int32_t h,
                                      int32_t fx, int32_t fy,
                                      int32_t ex, int32_t ey) {
    int32_t start_cell = fy * w + fx;
    int32_t best_cell = start_cell;
    int32_t best_score = 0;   // 对应 FUN_0040144e local_18 初值 0
    for (int32_t y = 0; y < h; ++y) {
        for (int32_t x = 0; x < w; ++x) {
            int32_t cell = y * w + x;
            if (cell == start_cell) continue;       // 起点格不被选
            if (!mask[cell]) continue;              // 仅加权可达
            int32_t dx = x - ex;
            int32_t dy = y - ey;
            int32_t score = (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy);  // 曼哈顿到敌人
            int32_t flag = terr[cell] & 0xF;
            if (flag == 9 || flag == 10 || flag == 0xb) score += 9;     // 地形偏好
            if (score > best_score) {               // 严格 >，行优先首次胜
                best_score = score;
                best_cell = cell;
            }
        }
    }
    return best_cell;
}

// 最近敌人（精确重演 battle_engine.cpp::find_nearest_enemy）：平方欧氏、异阵营、存活。
static int32_t reenact_find_nearest_enemy(const BattleState& st, int32_t fighter_idx) {
    const auto& f = st.fighters[fighter_idx];
    int32_t best = -1, best_d = INT32_MAX;
    for (int32_t i = 0; i < BattleState::MAX_FIGHTERS; ++i) {
        if (i == fighter_idx) continue;
        const auto& o = st.fighters[i];
        if (!o.is_active) continue;
        if (o.stats.current_hp <= 0) continue;
        if (o.team == f.team) continue;
        int32_t dx = o.grid_x - f.grid_x;
        int32_t dy = o.grid_y - f.grid_y;
        int32_t d = dx * dx + dy * dy;
        if (d < best_d) { best_d = d; best = i; }
    }
    return best;
}

// 在给定引擎状态上，对单个单位做“二进制重演 vs 干净 select_retreat_cell”对拍，
// 返回分歧数（并输出前若干分歧样例）。
static int32_t compare_retreat_for_unit(const BattleEngine& eng,
                                        int32_t i, int32_t& printed) {
    const auto& st = eng.state();
    const auto& u = st.fighters[i];
    int32_t w = st.map_width, h = st.map_height;

    int32_t enemy = reenact_find_nearest_enemy(st, i);
    if (enemy < 0) return 0;   // 无敌人 → 两侧皆不移动，不计入度量对拍

    // 注意：eng 为 const 引用，select_retreat_cell 会改 state_.movement_mask；
    // 故此处用 const_cast 仅触发计算（掩码为可观测中间量，不影响落点选择结果）。
    BattleEngine& mut = const_cast<BattleEngine&>(eng);
    int32_t cln = mut.select_retreat_cell(i);
    const auto& st2 = mut.state();
    const auto& mask = st2.movement_mask;
    const auto& terr = st2.grid_terrain;

    int32_t bin = reenact_binary_retreat(mask, terr, w, h,
                                         u.grid_x, u.grid_y,
                                         st2.fighters[enemy].grid_x,
                                         st2.fighters[enemy].grid_y);
    if (bin != cln) {
        if (printed < 12) {
            std::cerr << "[Retreat mismatch] unit#" << i << "(" << u.grid_x << "," << u.grid_y
                      << ") enemy#" << enemy << "(" << st2.fighters[enemy].grid_x << ","
                      << st2.fighters[enemy].grid_y
                      << ") bin=" << (bin >= 0 ? (bin % w) : -1) << "," << (bin >= 0 ? (bin / w) : -1)
                      << " cln=" << (cln >= 0 ? (cln % w) : -1) << "," << (cln >= 0 ? (cln / w) : -1)
                      << std::endl;
            ++printed;
        }
        return 1;
    }
    return 0;
}

}  // namespace

// ============================================================
// 度量差异的确定性证明：曼哈顿+地形罚 vs 平方欧氏 可在同掩码上选不同格
// （说明此前干净层用平方欧氏是真实偏离，须改用曼哈顿+地形罚）
// ============================================================
TEST(RetreatDestination, MetricDivergenceManhattanTerrainVsSquared) {
    // 8x8（w=8 才能表示 x=7），可达集含起点(4,4)与两候选：A=(7,0)、B=(3,3)；敌人(0,0)。
    //   A: 平方欧氏=49, 曼哈顿=7, 地形0 → 分数7
    //   B: 平方欧氏=18, 曼哈顿=6, 地形9(+9) → 分数15
    //   二进制(曼哈顿+地形罚最大)选 B(15>7)。
    //   旧干净层(平方欧氏最大)选 A(49>18)。二者分歧 → 证明度量修复是真实行为变化。
    //   注：cell 编码为 y*w+x（行优先），故 A=(7,0) 索引 = 0*8+7 = 7（x=7<w 合法）。
    const int32_t w = 8, h = 8;
    std::vector<bool> mask(w * h, false);
    std::vector<int32_t> terr(w * h, 0);
    int32_t START = 4 * w + 4;   // (4,4) 起点（被排除）
    int32_t A = 0 * w + 7;       // (7,0) index=7
    int32_t B = 3 * w + 3;       // (3,3) index=27
    mask[START] = true;
    mask[A] = true;
    mask[B] = true;
    terr[B] = 9;                 // 地形旗标 9 → +9 罚
    int32_t bin = reenact_binary_retreat(mask, terr, w, h, 4, 4, 0, 0);
    EXPECT_EQ(bin, B) << "二进制应按曼哈顿+地形罚选 (3,3)";

    // 旧干净层（平方欧氏最大）会选 A——与曼哈顿+地形罚分歧。
    int32_t sq_best = -1, sq_dist = -1;
    for (int32_t y = 0; y < h; ++y)
        for (int32_t x = 0; x < w; ++x) {
            int32_t c = y * w + x;
            if (c == START) continue;
            if (!mask[c]) continue;
            int32_t ddx = x - 0, ddy = y - 0;
            int32_t d = ddx * ddx + ddy * ddy;
            if (d > sq_dist) { sq_dist = d; sq_best = c; }   // 最大化平方欧氏（旧撤退度量）
        }
    EXPECT_EQ(sq_best, A) << "平方欧氏最大会选 (7,0)，与曼哈顿+地形罚分歧";
    EXPECT_NE(bin, sq_best);
}

// 平局时二进制取行优先首次（扫描序 + 严格 `>`）；干净层同序。
TEST(RetreatDestination, TieBreakSmallIndex) {
    const int32_t w = 5, h = 5;
    std::vector<bool> mask(w * h, false);
    std::vector<int32_t> terr(w * h, 0);
    int32_t START = 2 * w + 2;   // (2,2) 起点
    mask[START] = true;
    // (0,4) 与 (4,0) 到敌人(0,0) 曼哈顿都=4（均>0，争夺最大）→ 平局，取索引小者 (0,4)=4。
    mask[0 * w + 4] = true;   // (0,4) index=4
    mask[4 * w + 0] = true;   // (4,0) index=20
    int32_t bin = reenact_binary_retreat(mask, terr, w, h, 2, 2, 0, 0);
    EXPECT_EQ(bin, 4);
}

// ============================================================
// 真实全部 47 关闸门：干净 select_retreat_cell 落点 == 二进制 find_best_cell_terrain
// （两侧均用“最近敌人”口径，仅隔离度量）
// ============================================================
TEST(RetreatDestination, RealMapMetricAgreementAllFights) {
    ASSERT_TRUE(std::filesystem::exists(kGameDataDir));
    dlzr::app::DatLoader loader(kGameDataDir);
    dlzr::core::GameData game = loader.load_game_data();   // 常量，仅载一次

    int total_mismatches = 0;
    int total_pairs = 0;
    int fights_tested = 0;

    for (int j = 0; j < dlzr::core::MAX_FIGHTS; ++j) {
        dlzr::core::FightData fight;
        dlzr::core::FightMap map;
        try {
            fight = loader.load_fight(j);
            map = loader.load_map(j);
        } catch (const std::exception&) {
            continue;   // 该关无数据，跳过
        }

        BattleEngine eng;
        eng.init(fight, game, 1);
        eng.set_map(&map);

        const auto& st = eng.state();
        if ((int)st.movement_mask.size() != st.map_width * st.map_height) continue;
        if ((int)st.grid_terrain.size() != st.map_width * st.map_height) continue;

        auto fighter_alive = [&](int32_t i) {
            if (i < 0 || i >= BattleState::MAX_FIGHTERS) return false;
            const auto& f = st.fighters[i];
            return f.is_active && f.stats.current_hp > 0;
        };

        int printed = 0;
        for (int i = 0; i < BattleState::MAX_FIGHTERS; ++i) {
            if (!fighter_alive(i)) continue;
            const auto& u = st.fighters[i];
            if (u.grid_x < 0 || u.grid_x >= st.map_width ||
                u.grid_y < 0 || u.grid_y >= st.map_height) continue;
            int32_t m = compare_retreat_for_unit(eng, i, printed);
            total_pairs += 1;
            total_mismatches += m;
        }
        ++fights_tested;
    }

    std::cerr << "[Retreat] fights=" << fights_tested
              << " pairs=" << total_pairs
              << " mismatches=" << total_mismatches << std::endl;
    EXPECT_EQ(total_mismatches, 0)
        << "二进制(find_best_cell_terrain 曼哈顿+地形罚最大)与干净(select_retreat_cell)落点选择在 "
        << total_mismatches << "/" << total_pairs << " 对上分歧";
}

// ============================================================
// attacker_index 分叉闭合闸门（铁律零 · 可证）：
//   二进制 FUN_0040144e 以 fighter_array[param_1].attacker_index（记录 +0x38）为撤离远离对象；
//   干净层 select_retreat_cell 现消费 RuntimeFighter.attacker（由 apply_damage 命中写回），
//   仅当 attacker 无效时回退 find_nearest_enemy。
//   本测试确定性证明该字段被 *实际消费*：同掩码下，attacker=E_far 与 attacker=E_near
//   给出不同落点，且二者分别等于“远离 E_far / 远离 E_near”的二进制重演；attacker=-1 时
//   回退到最近敌人(=E_near)结果与 attacker=E_near 完全一致。
// ============================================================
TEST(RetreatDestination, AttackerFieldDrivesRetreatTarget) {
    ASSERT_TRUE(std::filesystem::exists(kGameDataDir));
    dlzr::app::DatLoader loader(kGameDataDir);
    dlzr::core::GameData game = loader.load_game_data();

    // 取第一关尺寸足够大者，保证可达集非空
    int32_t w = 0, h = 0;
    dlzr::core::FightData fight;
    dlzr::core::FightMap map;
    for (int j = 0; j < dlzr::core::MAX_FIGHTS; ++j) {
        try {
            fight = loader.load_fight(j);
            map = loader.load_map(j);
        } catch (...) { continue; }
        if (map.width > 6 && map.height > 6) { w = map.width; h = map.height; break; }
    }
    ASSERT_GT(w, 6); ASSERT_GT(h, 6);

    BattleEngine eng;
    eng.init(fight, game, 1);
    eng.set_map(&map);
    BattleState& st = const_cast<BattleState&>(eng.state());   // 受控注入场景

    // 选一名玩家单位 F 作被试
    int32_t F = -1;
    for (int i = 0; i < BattleState::MAX_FIGHTERS; ++i) {
        if (st.fighters[i].team == 0 && st.fighters[i].is_active &&
            st.fighters[i].stats.current_hp > 0) { F = i; break; }
    }
    ASSERT_GE(F, 0) << "need a player fighter";

    // 全地形置为可行（旗标 0，class_move_cost[class][0]=1 <99，见 GAME.DAT 0x3F20）
    for (int c = 0; c < w * h; ++c) st.grid_terrain[c] = 0;
    st.map_width = w; st.map_height = h;

    // 仅保留 F 与两名敌人 E_near(近, 3,2) / E_far(远, w-1,h-1)
    for (int i = 0; i < BattleState::MAX_FIGHTERS; ++i)
        if (i != F) st.fighters[i].is_active = false;
    int32_t E_near = -1, E_far = -1;
    for (int i = 0; i < BattleState::MAX_FIGHTERS && E_far < 0; ++i) {
        if (i == F || st.fighters[i].is_active) continue;
        st.fighters[i].is_active = true;
        st.fighters[i].stats.current_hp = 50;
        st.fighters[i].team = 1;
        st.fighters[i].class_type = 0;
        if (E_near < 0) { E_near = i; st.fighters[i].grid_x = 3;     st.fighters[i].grid_y = 2; }
        else            { E_far  = i; st.fighters[i].grid_x = w - 1; st.fighters[i].grid_y = h - 1; }
    }
    ASSERT_GE(E_near, 0); ASSERT_GE(E_far, 0);
    st.fighters[F].grid_x = 2; st.fighters[F].grid_y = 2;
    st.fighters[F].class_type = 0;   // 确保旗标0可行、移动力有效

    // 1) attacker = E_far → 落点应远离 E_far
    st.fighters[F].attacker = E_far;
    int32_t cln_far = eng.select_retreat_cell(F);
    int32_t bin_far = reenact_binary_retreat(st.movement_mask, st.grid_terrain, w, h,
                                             2, 2, w - 1, h - 1);
    EXPECT_EQ(cln_far, bin_far) << "attacker=E_far 时须远离 E_far";

    // 2) attacker = E_near → 落点应远离 E_near
    st.fighters[F].attacker = E_near;
    int32_t cln_near = eng.select_retreat_cell(F);
    int32_t bin_near = reenact_binary_retreat(st.movement_mask, st.grid_terrain, w, h,
                                              2, 2, 3, 2);
    EXPECT_EQ(cln_near, bin_near) << "attacker=E_near 时须远离 E_near";

    // 3) attacker 字段必须真实决定目标（E_far≠E_near 位置 → 落点不同）
    EXPECT_NE(cln_far, cln_near)
        << "attacker 字段必须改变撤退目标（远离攻击者，而非恒用最近敌人）";

    // 4) attacker 无效(-1) → 回退 find_nearest_enemy = 最近敌人 E_near，等价于 attacker=E_near
    st.fighters[F].attacker = -1;
    int32_t cln_fb = eng.select_retreat_cell(F);
    EXPECT_EQ(cln_fb, cln_near)
        << "attacker 无效时须回退到最近敌人(=E_near)，与 attacker=E_near 一致";
}

