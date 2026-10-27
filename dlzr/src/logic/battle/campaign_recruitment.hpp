#pragma once

// campaign_recruitment.hpp — 战役招募写回（Tier-B 接缝）
// Campaign recruitment write-back (Tier-B seam)
//
// 把已招募的战役同伴（game.people[k], k∈[1,39]）写回本关战斗数据：
//   fight.people[k]  ← 同伴名册（People → FightPeople 逐字段）
//   fight.fighters[k] ← 登场标记 + 部署坐标 + 体力(兵种公式)
// 原版部署按"同一槽位索引"消费这两者（证据 FUN_0041b53c.c:19-29），
// 故本函数不新增部署逻辑，仅填充数据，init_fighters 的现成循环即部署同伴。
// 招募"事件"（决定哪些 k）由事件 FSM 负责，此处不臆造（铁律零）。

#include "core/data_model/game_data.hpp"

namespace dlzr::logic {

// 把 roster 中每个同伴写回 fight（须在 BattleEngine::init_fighters 之前调用）。
// 仅处理 k∈[1,39]；越界/空名册安全跳过。
void apply_campaign_recruitment(core::FightData& fight,
                                 const core::CampaignRoster& roster,
                                 const core::GameData& game);

// 招募生产者 seam（事件 FSM 调用点）：把一个战役同伴 k 加入 game.campaign_roster。
// 仅做数据写回，不决定"何时/是否招募"（那是事件 FSM 的职责，铁律零不伪造招募逻辑）。
// 合法范围 1<=people_index<40 且 < game.people.size()；非法返回 false 且不修改名册。
// 部署坐标(grid_x,grid_y)/朝向(facing)由调用方（事件 UI）给定，沿用 FIGHT.DAT 约定(999=未部署)。
bool recruit_companion(core::GameData& game,
                       int32_t people_index,
                       int32_t grid_x = 999,
                       int32_t grid_y = 999,
                       int32_t facing = 1);

}  // namespace dlzr::logic
