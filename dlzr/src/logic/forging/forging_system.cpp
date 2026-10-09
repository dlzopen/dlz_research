// forging_system.cpp — 锻造系统实现
// Forging/crafting system implementation

#include "logic/forging/forging_system.hpp"

#include <algorithm>

namespace dlzr::logic {

void ForgingSystem::init(core::GameData& game_data) {
    game_data_ = &game_data;
}

int32_t ForgingSystem::get_price_per_unit(int32_t tab) const {
    if (tab < 0 || tab >= NUM_TABS) return 0;
    return TAB_PRICES[tab];
}

int32_t ForgingSystem::compute_max_quantity(int32_t tab, int32_t item,
                                             int32_t gold) const {
    if (!game_data_ || tab < 0 || tab >= NUM_TABS ||
        item < 0 || item >= ITEMS_PER_TAB) {
        return 0;
    }

    int32_t max_qty = 9999;

    int32_t price = get_price_per_unit(tab);
    if (price > 0) {
        max_qty = std::min(max_qty, gold / price);
    }

    for (int m = 0; m < NUM_MATERIALS; ++m) {
        uint8_t required = game_data_->mine_recipe.matrix[m][item];
        if (required > 0) {
            int32_t available = game_data_->material_inventory[m];
            max_qty = std::min(max_qty, available / required);
        }
    }

    return (max_qty == 9999) ? 0 : max_qty;
}

bool ForgingSystem::can_craft(int32_t tab, int32_t item, int32_t quantity,
                               int32_t gold) const {
    if (quantity <= 0) return false;

    int32_t price = get_price_per_unit(tab);
    if (price > 0 && gold < price * quantity) return false;

    for (int m = 0; m < NUM_MATERIALS; ++m) {
        uint8_t required = game_data_->mine_recipe.matrix[m][item];
        if (required > 0) {
            if (game_data_->material_inventory[m] < required * quantity) {
                return false;
            }
        }
    }

    return true;
}

void ForgingSystem::execute_craft(int32_t tab, int32_t item, int32_t quantity) {
    if (!game_data_ || !can_craft(tab, item, quantity,
                                   game_data_->warehouse[0])) {
        return;
    }

    game_data_->crafted_stock[tab][item] += static_cast<int16_t>(quantity);

    int32_t price = get_price_per_unit(tab);
    if (price > 0) {
        int32_t cost = price * quantity;
        int32_t gold = static_cast<int32_t>(game_data_->warehouse[0]);
        game_data_->warehouse[0] = static_cast<uint16_t>(std::max(0, gold - cost));
    }

    for (int m = 0; m < NUM_MATERIALS; ++m) {
        uint8_t required = game_data_->mine_recipe.matrix[m][item];
        if (required > 0) {
            game_data_->material_inventory[m] -= required * quantity;
        }
    }
}

const core::Material& ForgingSystem::get_material(int32_t index) const {
    return game_data_->materials[index];
}

int32_t ForgingSystem::get_material_stock(int32_t index) const {
    if (!game_data_ || index < 0 || index >= NUM_MATERIALS) return 0;
    return game_data_->material_inventory[index];
}

int16_t ForgingSystem::get_crafted_stock(int32_t tab, int32_t item) const {
    if (!game_data_ || tab < 0 || tab >= NUM_TABS ||
        item < 0 || item >= ITEMS_PER_TAB) {
        return 0;
    }
    return game_data_->crafted_stock[tab][item];
}

}  // namespace dlzr::logic
