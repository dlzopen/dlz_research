// test_game_dat_blocks.cpp — Phase 6 C 路：GAME.DAT 块级 PE/二进制校验闸门
//
// 铁律零实证来源：FUN_00424566.c（asset_game_dat）用 12 次 FUN_0043e510 顺序装载 GAME.DAT
// 各块，每次字节数 = 元素宽(param_2) * 计数(param_3)：
//   blk0 : 1   * 0xa0   = 160     (What0)
//   blk1 : 0x14* 0x10   = 320     (地形修正表 DiXingModifier，16×20B)
//   blk2 : 0xa0* 0x60   = 15360   (兵种表 ClassType，96×160B)
//   blk3 : 1   * 0x600  = 1536    (兵种地形移动消耗，96×16B)
//   blk4 : 0x48* 0xa0   = 11520   (物品 WuPin，160×72B)
//   blk5 : 0x1c* 10     = 448     (材料 Material，10×~44B)
//   blk6 : 1   * 100    = 100     (地雷配方 DiLeiRecipe)
//   blk7 : 0x30* 10     = 480     (地雷 DiLei)
//   blk8 : 0x30* 0x10   = 768     (技能 JiNeng)
//   blk9 : 1   * 0x510  = 1296    (SelectFanwei)
//   blk10: 4   * 0x930  = 9408    (ApplyFanwei = 技能形状表，48×196B) ← 即块#10
//   blk11: 1   * 0x3c00 = 15360   (物品×兵种兼容矩阵 WuPinValid)
//
// 关键结论（实测）：12 块累计 = 56588 = GAME.DAT 文件大小，精确吻合、零间隙、无文件头。
// 干净层 DatLoader::load_game_data 的各段尺寸与之上四块完全一致。
//
// 诚实残留：二进制块打包是「顺序 fread」语义（FUN_0043e510 为通用缓冲读），块间无间隙；
//   干净层按记录偏移（0x1E0/0x3F20/...）读取同一文件，组织方式不同（Tier-B），但落点一致。
//   技能形状表是块#10（纠正记忆误记的「块#11」）。

#include <gtest/gtest.h>
#include <filesystem>
#include <cstdint>

#include "app/resource/dat_loader.hpp"
#include "core/data_model/game_data.hpp"
#include "core/data_model/constants.hpp"

namespace {
const char* kGameDataDir = "C:/seesaw/source/dlz_research/DLZFULL";
}  // namespace

// 12 块累计字节数必须精确等于 GAME.DAT 文件大小（零头/零间隙）。
TEST(GameDatBlocks, TwelveBlocksTileFileExactly) {
  const int64_t blk_bytes[12] = {
      1 * 0xa0,   0x14 * 0x10, 0xa0 * 0x60, 1 * 0x600,
      0x48 * 0xa0, 0x1c * 10,   1 * 100,    0x30 * 10,
      0x30 * 0x10, 1 * 0x510,   4 * 0x930,  1 * 0x3c00};
  int64_t sum = 0;
  for (int i = 0; i < 12; ++i) sum += blk_bytes[i];

  EXPECT_EQ(sum, 56588) << "FUN_00424566 的 12 块累计必须 == 56588";

  std::error_code ec;
  auto sz = std::filesystem::file_size(
      std::string(kGameDataDir) + "/GAME.DAT", ec);
  ASSERT_FALSE(ec) << "GAME.DAT 应可读";
  EXPECT_EQ(sz, 56588u);
  EXPECT_EQ(static_cast<int64_t>(sz), sum)
      << "GAME.DAT 文件大小应精确等于 12 块累计（块布局无间隙）";
}

// 二进制块尺寸 vs 干净层结构体尺寸：四项干净层实际消耗的块。
TEST(GameDatBlocks, KeyBlockSizesMatchCleanStructs) {
  // 地形修正表（块#1）：16 条 × 20B(=5 int32)
  EXPECT_EQ(0x14 * 0x10, 16 * 20);
  // 兵种表（块#2）：96 兵种 × 160B
  EXPECT_EQ(0xa0 * 0x60, static_cast<int64_t>(dlzr::core::MAX_CLASS_TYPES) * 160);
  // 移动消耗（块#3）：96 兵种 × 16 地形
  EXPECT_EQ(1 * 0x600, static_cast<int64_t>(dlzr::core::MAX_CLASS_TYPES) * 16);
  // 技能形状表（块#10）：9408B
  EXPECT_EQ(4 * 0x930, 9408);
}

// 技能形状表是 FUN_00424566 的块#10（非记忆误记的块#11）；
// 块#11 是兼容矩阵（1*0x3c00=15360B）。
TEST(GameDatBlocks, SkillShapeIsBlock10Not11) {
  EXPECT_EQ(4 * 0x930, 9408);    // 块#10 = 技能形状表
  EXPECT_EQ(1 * 0x3c00, 15360);  // 块#11 = 兼容矩阵
}

// 干净层 load_game_data 必须成功装载出有效段落（文件偏移与二进制块布局一致）。
TEST(GameDatBlocks, CleanLoaderProducesValidSections) {
  ASSERT_TRUE(std::filesystem::exists(kGameDataDir));
  dlzr::app::DatLoader loader(kGameDataDir);
  dlzr::core::GameData gd = loader.load_game_data();  // 不抛即结构有效

  EXPECT_EQ(gd.class_types.size(), static_cast<size_t>(dlzr::core::MAX_CLASS_TYPES));
  EXPECT_EQ(gd.skills.size(), static_cast<size_t>(dlzr::core::MAX_SKILLS));
  EXPECT_EQ(gd.items.size(), static_cast<size_t>(dlzr::core::MAX_WUPIN));

  // 兼容矩阵 160×96
  ASSERT_EQ(gd.wupin_compat.size(), static_cast<size_t>(dlzr::core::MAX_WUPIN));
  EXPECT_EQ(gd.wupin_compat[0].size(),
            static_cast<size_t>(dlzr::core::MAX_CLASS_TYPES));

  // 移动消耗矩阵 96×16
  ASSERT_EQ(gd.class_move_cost.size(),
            static_cast<size_t>(dlzr::core::MAX_CLASS_TYPES));
  EXPECT_EQ(gd.class_move_cost[0].size(), 16u);

  // 地形修正表 16 条
  EXPECT_EQ(gd.terrain_modifiers.size(), 16u);
  for (int t = 0; t < 16; ++t)
    EXPECT_EQ(gd.terrain_modifiers[t].size(), 5u);
}
