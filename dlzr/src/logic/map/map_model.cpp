// map_model.cpp — MapModel + PathFinder 实现（Phase 5，2026-10-25）
//
// 铁律零依据见 map_model.hpp 文件头（MAP.DAT 瓦片位布局、BingZhongYiDong 移动消耗、
// DiXingModifier 地形修正，均来自 DLZFULL/GAME.DAT 二进制实测）。

#include "logic/map/map_model.hpp"

#include <queue>
#include <tuple>
#include <vector>

namespace dlzr::logic {

void MapModel::build(const core::FightMap& map, const core::GameData& gd) {
    width_ = map.width;
    height_ = map.height;
    gd_ = &gd;
    tiles_ = &map.tiles;
}

int MapModel::terrain_at(int x, int y) const {
    if (!in_bounds(x, y)) return -1;
    const uint16_t t = (*tiles_)[static_cast<size_t>(y) * width_ + x];
    return static_cast<int>((t >> 12) & 0x0F);
}

uint8_t MapModel::move_cost(int class_type, int x, int y) const {
    if (!in_bounds(x, y)) return 255;
    if (class_type < 0 || class_type >= core::MAX_CLASS_TYPES) return 255;
    const int terr = terrain_at(x, y);
    if (terr < 0 || terr > 15) return 255;
    const uint8_t c = gd_->class_move_cost[class_type][terr];
    return (c >= kTerrainImpassable) ? 255 : c;
}

// Dijkstra：优先队列按累计代价最小扩展；返回最短路径（含端点），不可达返回空。
std::vector<MapPoint> PathFinder::find_path(const MapModel& model,
                                           const MapPoint& start,
                                           const MapPoint& goal,
                                           int class_type) const {
    if (!model.in_bounds(start.x, start.y) || !model.in_bounds(goal.x, goal.y))
        return {};
    if (start == goal) return {start};

    const int W = model.width(), H = model.height();
    const int N = W * H;
    std::vector<int> best(N, kUnreachable);
    std::vector<MapPoint> prev(N, {-1, -1});

    auto idx = [&](int x, int y) { return y * W + x; };

    // (cost, x, y)
    using Node = std::tuple<int, int, int>;
    auto cmp = [](const Node& a, const Node& b) { return std::get<0>(a) > std::get<0>(b); };
    std::priority_queue<Node, std::vector<Node>, decltype(cmp)> pq(cmp);

    const int s = idx(start.x, start.y);
    best[s] = 0;
    pq.push({0, start.x, start.y});

    const int dx[4] = {1, -1, 0, 0};
    const int dy[4] = {0, 0, 1, -1};

    int goal_idx = idx(goal.x, goal.y);
    while (!pq.empty()) {
        auto [cost, cx, cy] = pq.top();
        pq.pop();
        const int ci = idx(cx, cy);
        if (cost > best[ci]) continue;
        if (ci == goal_idx) break;

        for (int d = 0; d < 4; ++d) {
            int nx = cx + dx[d], ny = cy + dy[d];
            if (!model.in_bounds(nx, ny)) continue;
            uint8_t step = model.move_cost(class_type, nx, ny);
            if (step >= 255) continue;  // 不可通行
            int ni = idx(nx, ny);
            int nc = cost + static_cast<int>(step);
            if (best[ni] == kUnreachable || nc < best[ni]) {
                best[ni] = nc;
                prev[ni] = {cx, cy};
                pq.push({nc, nx, ny});
            }
        }
    }

    if (best[goal_idx] == kUnreachable) return {};
    // 回溯路径
    std::vector<MapPoint> path;
    for (MapPoint cur = goal; !(cur.x == start.x && cur.y == start.y); ) {
        path.push_back(cur);
        MapPoint p = prev[idx(cur.x, cur.y)];
        if (p.x < 0) return {};  // 理论不会到这（goal 可达性已确认）
        cur = p;
    }
    path.push_back(start);
    std::reverse(path.begin(), path.end());
    return path;
}

int PathFinder::reachable_count(const MapModel& model,
                               const MapPoint& start,
                               int class_type,
                               int max_cost) const {
    if (!model.in_bounds(start.x, start.y)) return 0;
    const int W = model.width(), H = model.height();
    const int N = W * H;
    std::vector<int> best(N, kUnreachable);
    auto idx = [&](int x, int y) { return y * W + x; };
    using Node = std::tuple<int, int, int>;
    auto cmp = [](const Node& a, const Node& b) { return std::get<0>(a) > std::get<0>(b); };
    std::priority_queue<Node, std::vector<Node>, decltype(cmp)> pq(cmp);

    const int s = idx(start.x, start.y);
    best[s] = 0;
    pq.push({0, start.x, start.y});

    const int dx[4] = {1, -1, 0, 0};
    const int dy[4] = {0, 0, 1, -1};
    int count = 0;
    while (!pq.empty()) {
        auto [cost, cx, cy] = pq.top();
        pq.pop();
        const int ci = idx(cx, cy);
        if (cost > best[ci]) continue;
        if (cost <= max_cost) ++count;
        for (int d = 0; d < 4; ++d) {
            int nx = cx + dx[d], ny = cy + dy[d];
            if (!model.in_bounds(nx, ny)) continue;
            uint8_t step = model.move_cost(class_type, nx, ny);
            if (step >= 255) continue;
            int ni = idx(nx, ny);
            int nc = cost + static_cast<int>(step);
            if ((best[ni] == kUnreachable || nc < best[ni]) && nc <= max_cost) {
                best[ni] = nc;
                pq.push({nc, nx, ny});
            }
        }
    }
    return count;
}

}  // namespace dlzr::logic
