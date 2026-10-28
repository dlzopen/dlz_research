// 商店系统无头集成闸 / Shop system headless integration gate
//
// Phase 8 业务系统验证：真实 GAME.DAT 装载 (WuPin@0x43E0 / DiLei@0x725C，已 RE python 实测)
// → GameStateManager::buy_wupin / buy_mine 交易逻辑。
//
// 真实价格锚点（权威 010 模板 dlz_game.bt 内部字段 + DLZFULL/GAME.DAT 逐字节实测，铁律零坐标）：
//   WuPin.price  @ +0x1C：items[0]=蟠桃 10000 / items[1]=核桃 40 / items[2]=山楂 60
//   DiLei.price  @ +0x14：mines[1]=石雷 240 / mines[2]=铁雷 400
//   （mines[0]=土雷 在 +0x14 为脏值 0x66616666，系数据文件本身怪值，非代码 bug；
//     本测试用 mines[1] 锚定，避开该数据异常）
//
// buy 逻辑映射（RE 实证）：
//   buy_wupin(i)：扣 gold(warehouse[0]) = items[i].price，增 warehouse[i]
//   buy_mine(m) ：扣 gold = mines[m].price，增 warehouse[MAX_WUPIN-MAX_DILEI+m] = warehouse[150+m]

#include <gtest/gtest.h>

#include <cstdint>
#include <string>

#include "app/resource/dat_loader.hpp"
#include "core/data_model/constants.hpp"
#include "core/data_model/game_data.hpp"
#include "logic/game_state_manager.hpp"

namespace {

// 与 kGameDataDir（其它测试共用）一致的真实数据目录。
const char* GameDataDir() { return "C:/seesaw/source/dlz_research/DLZFULL"; }

}  // namespace

// —— 真实数据装载 + 价格锚点（内容闸门）——
TEST(ShopSystem, RealDataPricesMatchBinary) {
    dlzr::app::DatLoader loader(GameDataDir());
    dlzr::core::GameData gd = loader.load_game_data();

    // WuPin.price @ +0x1C（template/dlz_game.bt 内部字段 + 实测）
    ASSERT_EQ(gd.items[0].price, 10000);  // 蟠桃
    ASSERT_EQ(gd.items[1].price, 40);     // 核桃
    ASSERT_EQ(gd.items[2].price, 60);     // 山楂

    // DiLei.price @ +0x14（template/dlz_game.bt 内部字段 + 实测；避开 mines[0] 数据异常）
    ASSERT_EQ(gd.mines[1].price, 240);    // 石雷
    ASSERT_EQ(gd.mines[2].price, 400);    // 铁雷
}

// —— buy_wupin：扣金 + 增库存 + 可负担门控 ——
TEST(ShopSystem, BuyWuPinDeductsGoldAndIncrementsStock) {
    dlzr::app::DatLoader loader(GameDataDir());
    dlzr::logic::GameStateManager gsm;
    gsm.init(loader);

    // 设置初始金库（warehouse[0] = gold）
    gsm.game_data().warehouse[0] = 1000;
    const int32_t price = gsm.game_data().items[1].price;  // 核桃 = 40
    ASSERT_GT(price, 0);

    const uint16_t before_stock = gsm.game_data().warehouse[1];
    const int32_t before_gold = gsm.get_gold();

    EXPECT_TRUE(gsm.buy_wupin(1));
    EXPECT_EQ(gsm.get_gold(), before_gold - price);          // 扣金精确
    EXPECT_EQ(gsm.game_data().warehouse[1],
              static_cast<uint16_t>(before_stock + 1));       // 库存 +1

    // 金不足则拒绝且状态不变
    gsm.game_data().warehouse[0] = static_cast<uint16_t>(price - 1);
    const int32_t poor_gold = gsm.get_gold();
    const uint16_t poor_stock = gsm.game_data().warehouse[1];
    EXPECT_FALSE(gsm.buy_wupin(1));
    EXPECT_EQ(gsm.get_gold(), poor_gold);                    // 金不变
    EXPECT_EQ(gsm.game_data().warehouse[1], poor_stock);      // 库存不变
}

// —— buy_mine：映射到 correct warehouse 槽位（150+mine）+ 扣金 ——
TEST(ShopSystem, BuyMineMapsToWarehouseSlot150PlusMine) {
    dlzr::app::DatLoader loader(GameDataDir());
    dlzr::logic::GameStateManager gsm;
    gsm.init(loader);

    gsm.game_data().warehouse[0] = 1000;
    const int32_t mine_price = gsm.game_data().mines[1].price;  // 石雷 = 240
    ASSERT_GT(mine_price, 0);

    const int32_t slot = dlzr::core::MAX_WUPIN - dlzr::core::MAX_DILEI + 1;  // 150 + 1 = 151
    ASSERT_EQ(slot, 151);
    ASSERT_LT(slot, static_cast<int32_t>(gsm.game_data().warehouse.size()));

    const uint16_t before_stock = gsm.game_data().warehouse[slot];
    const int32_t before_gold = gsm.get_gold();

    EXPECT_TRUE(gsm.buy_mine(1));
    EXPECT_EQ(gsm.get_gold(), before_gold - mine_price);      // 扣金精确
    EXPECT_EQ(gsm.game_data().warehouse[slot],
              static_cast<uint16_t>(before_stock + 1));        // 映射到 150+mine 槽

    // 金不足则拒绝且状态不变
    gsm.game_data().warehouse[0] = static_cast<uint16_t>(mine_price - 1);
    const int32_t poor_gold = gsm.get_gold();
    const uint16_t poor_stock = gsm.game_data().warehouse[slot];
    EXPECT_FALSE(gsm.buy_mine(1));
    EXPECT_EQ(gsm.get_gold(), poor_gold);
    EXPECT_EQ(gsm.game_data().warehouse[slot], poor_stock);
}

// —— 非法索引：越界返回 false，不改状态 ——
TEST(ShopSystem, InvalidIndexRejected) {
    dlzr::app::DatLoader loader(GameDataDir());
    dlzr::logic::GameStateManager gsm;
    gsm.init(loader);

    gsm.game_data().warehouse[0] = 1000;
    const int32_t gold_before = gsm.get_gold();

    EXPECT_FALSE(gsm.buy_wupin(-1));
    EXPECT_FALSE(gsm.buy_wupin(999));
    EXPECT_FALSE(gsm.buy_wupin(dlzr::core::MAX_WUPIN));      // 越界 1
    EXPECT_FALSE(gsm.buy_mine(-1));
    EXPECT_FALSE(gsm.buy_mine(99));
    EXPECT_FALSE(gsm.buy_mine(dlzr::core::MAX_DILEI));        // 越界 1

    EXPECT_EQ(gsm.get_gold(), gold_before);                  // 任何非法购买都不改金
}
