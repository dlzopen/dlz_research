#pragma once

// forging_system.hpp — 锻造系统
// Forging/crafting system for mine production
//
// 4页×10物品，10种材料，10×10配方矩阵
// 4 tabs × 10 items, 10 materials, 10×10 recipe matrix

#include "core/data_model/game_data.hpp"

#include <cstdint>

namespace dlzr::logic {

class ForgingSystem {
public:
    static constexpr int32_t NUM_MATERIALS = 10;
    static constexpr int32_t NUM_TABS = 4;
    static constexpr int32_t ITEMS_PER_TAB = 10;

    void init(core::GameData& game_data);

    int32_t compute_max_quantity(int32_t tab, int32_t item, int32_t gold) const;

    bool can_craft(int32_t tab, int32_t item, int32_t quantity, int32_t gold) const;

    void execute_craft(int32_t tab, int32_t item, int32_t quantity);

    int32_t get_price_per_unit(int32_t tab) const;

    const core::Material& get_material(int32_t index) const;
    int32_t get_material_stock(int32_t index) const;

    int16_t get_crafted_stock(int32_t tab, int32_t item) const;

private:
    core::GameData* game_data_{nullptr};

    static constexpr std::array<int32_t, NUM_TABS> TAB_PRICES = {0, 0, 80, 300};
};

}  // namespace dlzr::logic
