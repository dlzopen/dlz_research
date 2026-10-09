#pragma once

// 地雷战 (Landmine War, 1998) — 全局游戏数据聚合
// Aggregated game data loaded from original binary files

#include "core/data_model/types.hpp"
#include "core/data_model/constants.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace dlzr::core {

// ============================================================
// 从原始 DAT 文件加载的全局数据
// Global data loaded from original DAT files at runtime
// ============================================================
struct GameData {
    std::array<People, MAX_PEOPLE> people;
    std::array<ClassType, MAX_CLASS_TYPES> class_types;
    std::array<JiNeng, MAX_SKILLS> skills;
    std::array<Town, MAX_TOWNS> towns;
    std::array<WuPin, MAX_WUPIN> items;
    std::array<DiLei, MAX_DILEI> mines;
    std::array<Material, 10> materials;
    DiLeiRecipe mine_recipe;

    // FIGHT.DAT: 每关数据按需加载，不全部驻留内存
    // Fight data loaded on demand per battle, not all resident

    // 运行时状态（对应原作 28 存档数据块中的全局变量）
    // Runtime state (maps to globals in the 28 save data blocks)
    int32_t current_fight_index{0};   // fight_index @ 0x7D06A0
    int32_t current_town{0};          // town @ 0x7D0484
    std::array<int32_t, 14> story_flags{};
    std::array<uint16_t, 160> warehouse{};  // 仓库物品数量 / Warehouse item quantities

    // 锻造系统运行时 / Forging system runtime
    std::array<int32_t, 10> material_inventory{};   // 材料库存 / Material stock
    std::array<std::array<int16_t, 10>, 4> crafted_stock{};  // 4页×10物品 / 4 tabs × 10 items
};

// ============================================================
// 战斗场景数据（从 FIGHT.DAT 单关加载）
// Battle scene data (loaded per-fight from FIGHT.DAT)
// 每关 20508 字节 = Intro(700) + Fighter[128](8192) + People[88](11616)
// ============================================================
struct FightData {
    FightIntro intro;
    std::array<FightFighter, 128> fighters;
    std::array<FightPeople, 88> people;
};
static_assert(sizeof(FightData) == 20508, "FightData must be 20508 bytes");

}  // namespace dlzr::core
