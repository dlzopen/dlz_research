// test_forging_system.cpp — Phase 8 业务系统：锻造与真实 Material/DiLeiRecipe 数据联动
//
// 铁律零：用真实 DLZFULL/GAME.DAT 驱动，不 mock 数据。
// 验证对象：ForgingSystem 必须正确消费真实 GAME.DAT 装载的：
//   · Material[10]（@0x70E0，价格 1/1/2/4/2/2/2/10/10/80，python 实测）
//   · DiLeiRecipe 10×10 矩阵（@0x71F8，row0 = 10,0,0,0,5,10,15,0,0,0，python 实测）
// 这是首个把「真实材料/配方数据 → 锻造合成逻辑（门槛/扣减/增产/扣金）」串起来的
// 无头集成测试，直接覆盖 Phase 8 业务系统缺失的验证。

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <string>

#include "app/resource/dat_loader.hpp"
#include "core/data_model/game_data.hpp"
#include "logic/forging/forging_system.hpp"

#ifndef DLZR_DATA_DIR_DEFAULT
#define DLZR_DATA_DIR_DEFAULT "DLZFULL"
#endif

static std::string GameDataDir() {
    const char* env = std::getenv("DLZR_DATA_DIR");
    return env ? std::string(env) : std::string(DLZR_DATA_DIR_DEFAULT);
}

// —— 真实数据装载：ForgingSystem 的数据源必须是真实 GAME.DAT ——
TEST(ForgingSystem, RealMaterialAndRecipeDataLoaded) {
    dlzr::app::DatLoader loader(GameDataDir());
    dlzr::core::GameData gd = loader.load_game_data();

    ASSERT_EQ(gd.materials.size(), 10u);
    // 真实价格（RE：@0x70E0，python 实测 GAME.DAT）
    EXPECT_EQ(gd.materials[0].price, 1);    // 粘土
    EXPECT_EQ(gd.materials[3].price, 4);    // 铁块
    EXPECT_EQ(gd.materials[9].price, 80);   // 电池
    // 真实配方矩阵 row0（RE：@0x71F8，python 实测）：matrix[material][item]
    EXPECT_EQ((int)gd.mine_recipe.matrix[0][0], 10);
    EXPECT_EQ((int)gd.mine_recipe.matrix[0][4], 5);
    EXPECT_EQ((int)gd.mine_recipe.matrix[0][5], 10);
    EXPECT_EQ((int)gd.mine_recipe.matrix[0][6], 15);
    // row0 其余列无需求
    EXPECT_EQ((int)gd.mine_recipe.matrix[0][1], 0);
    EXPECT_EQ((int)gd.mine_recipe.matrix[0][9], 0);
}

// —— 配方门槛：真实矩阵驱动 can_craft / compute_max_quantity ——
// 注：matrix 布局为 [material][item]（与 forging_system.cpp 一致）。
// 实测：item 0/4/5 仅 material 0（粘土）有需求 → 单材料精确值可断言；
//      item 6 为多材料配方 → 仅给 material 0 库存时正确返回 0（缺料）。
TEST(ForgingSystem, RecipeGatesFromRealMatrix) {
    dlzr::app::DatLoader loader(GameDataDir());
    dlzr::core::GameData gd = loader.load_game_data();
    dlzr::logic::ForgingSystem fs;
    fs.init(gd);

    // 仅粘土（material 0）库存 100，其余清零
    for (int m = 0; m < 10; ++m) gd.material_inventory[m] = 0;
    gd.material_inventory[0] = 100;

    // item 0 仅 material 0 需求（matrix[m][0]==0 for m>=1，实测算得 max=10）
    EXPECT_TRUE(fs.can_craft(0, 0, 1, 9999));
    EXPECT_FALSE(fs.can_craft(0, 0, 11, 9999));            // 需 110 > 100
    EXPECT_EQ(fs.compute_max_quantity(0, 0, 9999), 10);     // 100/matrix[0][0]=100/10

    // item 4/5/6 均为多材料配方：仅 material 0 有库存 → 其他材料缺料 → max = 0
    EXPECT_EQ(fs.compute_max_quantity(0, 4, 9999), 0);
    EXPECT_EQ(fs.compute_max_quantity(0, 5, 9999), 0);
    EXPECT_EQ(fs.compute_max_quantity(0, 6, 9999), 0);
    EXPECT_FALSE(fs.can_craft(0, 4, 1, 9999));
    EXPECT_FALSE(fs.can_craft(0, 6, 1, 9999));

    // 给所有材料充足库存 + gold 充足：max 必须 = 各材料下界最小值（oracle 自洽）
    for (int m = 0; m < 10; ++m) gd.material_inventory[m] = 100;
    for (int item = 0; item < 10; ++item) {
        int32_t em = 9999;
        for (int m = 0; m < 10; ++m) {
            uint8_t req = gd.mine_recipe.matrix[m][item];
            if (req > 0) em = std::min(em, gd.material_inventory[m] / (int)req);
        }
        em = (em == 9999) ? 0 : em;
        EXPECT_EQ(fs.compute_max_quantity(0, item, 9999), em)
            << "compute_max_quantity 必须与真实矩阵定义自洽 (item=" << item << ")";
    }

    // tab 价格门槛：tab2 price=80，gold 不足则不可合成
    gd.warehouse[0] = 100;
    EXPECT_FALSE(fs.can_craft(2, 0, 1, 79));               // 80 > 79
    EXPECT_TRUE(fs.can_craft(2, 0, 1, 80));
}

// —— execute_craft 扣减材料 + 增产 + 扣金（真实数据联动 + 边界）——
TEST(ForgingSystem, ExecuteCraftDebitsAndProduces) {
    dlzr::app::DatLoader loader(GameDataDir());
    dlzr::core::GameData gd = loader.load_game_data();
    dlzr::logic::ForgingSystem fs;
    fs.init(gd);

    for (int m = 0; m < 10; ++m) gd.material_inventory[m] = 0;
    gd.material_inventory[0] = 100;
    gd.warehouse[0] = 1000;                                // gold
    for (int t = 0; t < 4; ++t)
        for (int i = 0; i < 10; ++i) gd.crafted_stock[t][i] = 0;

    // tab0 免费：item0 合成 2 个（需 10 粘土/个）
    fs.execute_craft(0, 0, 2);
    EXPECT_EQ(gd.crafted_stock[0][0], 2);
    EXPECT_EQ(gd.material_inventory[0], 80);                // 100 - 10*2
    EXPECT_EQ((int)gd.warehouse[0], 1000);                  // 免费 tab 不扣金

    // tab2 price=80：item0 合成 1 个
    fs.execute_craft(2, 0, 1);
    EXPECT_EQ(gd.crafted_stock[2][0], 1);
    EXPECT_EQ(gd.material_inventory[0], 70);                // 80 - 10
    EXPECT_EQ((int)gd.warehouse[0], 920);                   // 1000 - 80

    // 边界：材料不足 → execute_craft 不改动任何状态
    gd.material_inventory[0] = 5;                           // < 10
    fs.execute_craft(0, 0, 1);
    EXPECT_EQ(gd.crafted_stock[0][0], 2);                   // 不变
    EXPECT_EQ(gd.material_inventory[0], 5);                 // 不变
    EXPECT_EQ((int)gd.warehouse[0], 920);                   // 不变
}

// —— 价格表 + 库存/产物访问器（与真实 Material 数据一致）——
TEST(ForgingSystem, PriceTableAndAccessors) {
    dlzr::app::DatLoader loader(GameDataDir());
    dlzr::core::GameData gd = loader.load_game_data();
    dlzr::logic::ForgingSystem fs;
    fs.init(gd);

    EXPECT_EQ(fs.get_price_per_unit(0), 0);
    EXPECT_EQ(fs.get_price_per_unit(1), 0);
    EXPECT_EQ(fs.get_price_per_unit(2), 80);
    EXPECT_EQ(fs.get_price_per_unit(3), 300);

    gd.material_inventory[3] = 42;
    EXPECT_EQ(fs.get_material_stock(3), 42);
    EXPECT_EQ(fs.get_material(3).price, 4);                 // 铁块真实价格
    EXPECT_EQ(fs.get_crafted_stock(0, 0), 0);
}
