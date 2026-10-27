#pragma once

// map_model.hpp — 战斗地图模型 + 寻路器（Phase 5，2026-10-25）
//
// 铁律零依据（二进制实测，非推测）：
//   · MAP.DAT 瓦片 = uint16，地形类型 = (tile >> 12) & 0xF（output/map_dat_analysis.md §2）。
//   · 兵种地形移动消耗 BingZhongYiDong @ GAME.DAT+0x3F20，96×16×uint8，99=不可通行
//     （Python 抽取 DLZFULL/GAME.DAT 实证：class[0]=[1×14,99,99]）。
//   · 地形修正 DiXingModifier @ GAME.DAT+0x00A0，16×40B(=10×int32)。
//
// 原版移动范围算法 = FUN_00418e29（已反编译，output/ghidra_all/FUN_00418e29.c）：
// 4 向洪水填充、预算(param_3)受限、步进代价 = DAT_0095e4f8[ (tile>>12) + 兵种*0x10 ]
// = class_move_cost[兵种][地形]（与本 MapModel::move_cost 同源）。原版用 3 个方向性地形
// 缓冲(0058438e/90/92)承载"左/右用当前格地形、上/下用邻格地形"的非对称代价；本 PathFinder
// 用非方向性 move_cost（Tier-B 实现自由）。代价模型一致时可达集 0 分歧；方向性代价差异
// 为已知 residual（见 replica_roadmap.md §7 候选 1）。算法选择与二进制无关，只要可观测的
// "可达集合 / 路径总代价"一致即满足保真契约。

#include "core/data_model/game_data.hpp"
#include "core/data_model/types.hpp"

#include <cstdint>
#include <vector>

namespace dlzr::logic {

struct MapPoint {
    int x{0};
    int y{0};
    bool operator==(const MapPoint& o) const { return x == o.x && y == o.y; }
};

// 不可通行代价标记
constexpr uint8_t kTerrainImpassable = 99;

class MapModel {
public:
    MapModel() = default;

    // 由 FIGHT.DAT 解析出的 FightMap（瓦片网格）+ GameData（移动消耗/地形修正）构造。
    void build(const core::FightMap& map, const core::GameData& gd);

    int width()  const { return width_; }
    int height() const { return height_; }

    // 单元地形类型（0..15），越界返回 -1。
    int terrain_at(int x, int y) const;

    // 某兵种经 (x,y) 的移动消耗；不可通行返回 kTerrainImpassable 之上的 INF（用 255 标识）。
    // 越界或不可通行 → 255（>99，统一"不可走"语义）。
    uint8_t move_cost(int class_type, int x, int y) const;

    bool in_bounds(int x, int y) const {
        return x >= 0 && y >= 0 && x < width_ && y < height_;
    }
    bool passable(int class_type, int x, int y) const {
        return in_bounds(x, y) && move_cost(class_type, x, y) < kTerrainImpassable;
    }

    const core::GameData* game_data() const { return gd_; }

private:
    int width_{0};
    int height_{0};
    const core::GameData* gd_{nullptr};
    const std::vector<uint16_t>* tiles_{nullptr};
};

// 标准 Dijkstra 寻路（Tier-B 算法自由）。正交四邻接，按移动消耗加权。
// 返回从 start 到 goal 的最短路径（含 start 与 goal）；不可达返回空。
// class_type 决定每格移动消耗（见 MapModel::move_cost）。
class PathFinder {
public:
    // 最大代价上限（单格移动消耗 ≤ 99，路径代价可能很大，用 int 累加）。
    static constexpr int kUnreachable = -1;

    std::vector<MapPoint> find_path(const MapModel& model,
                                    const MapPoint& start,
                                    const MapPoint& goal,
                                    int class_type) const;

    // 从 start 起 BFS/Dijkstra 可达的单元集合大小（用于差分断言"可达集确定性"）。
    int reachable_count(const MapModel& model,
                        const MapPoint& start,
                        int class_type,
                        int max_cost) const;
};

}  // namespace dlzr::logic
