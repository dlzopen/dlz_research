// test_boot.cpp — 真实数据统一装载接线闸门（Phase 8, residual #1 收口）
//
// 铁律零：本测试不重复「独立暴力抽取 vs loader」对拍，而是验证
//   **DatLoader 真实装载路径本身**把二进制数据写入 replica / clean 全局：
//   · DatLoader::load_game_data() 必须让
//       g_battlecalc_terrain_modifier / g_terrain_mod_a_0x475aa8 /
//       g_terrain_mod_b_0x475aac / g_battlecalc_skill_shape_raw / g_terrain_defense_bonus
//     等于 GAME.DAT 字节实测值（off 0xA0 地形块 / off 31820 技能形状表块#11）。
//   · DatLoader::load_map(0) 必须让 g_battlecalc_terrain_grid[t] == (fm.tiles[t] >> 12)
//     且 g_battlecalc_map_width == fm.width（MAP.DAT 瓦片 → 0x584390 全局）。
//   即「游戏 boot 即装载真实地形/技能形状表/地形修正」闭环，不再仅测试显式注入。

#include "app/resource/dat_loader.hpp"
#include "logic/combat/combat_formulas.hpp"
#include "replica/battle_calc.hpp"
#include "replica/battle_sim.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <fstream>
#include <vector>

namespace {

static const char* kGameDataDir = "C:/seesaw/source/dlz_research/DLZFULL";

// 独立直接读 GAME.DAT（不经由 DatLoader）作为跨校基准。
std::vector<int32_t> read_terrain_records80() {
    std::ifstream f(std::string(kGameDataDir) + "/GAME.DAT", std::ios::binary);
    std::vector<int32_t> rec(80);
    f.seekg(0xA0);
    f.read(reinterpret_cast<char*>(rec.data()), 320);
    return rec;
}

std::vector<uint8_t> read_skill_shape_raw() {
    std::ifstream f(std::string(kGameDataDir) + "/GAME.DAT", std::ios::binary);
    std::vector<uint8_t> buf(9408);
    f.seekg(31820);
    f.read(reinterpret_cast<char*>(buf.data()), 9408);
    return buf;
}

}  // namespace

// ---- GAME.DAT 块 → replica / clean 三层全局 ----
TEST(BootRealData, GameDataInstallsTerrainAndSkillShapeGlobals) {
    dlzr::app::DatLoader loader(kGameDataDir);
    (void)loader.load_game_data();  // 触发 FUN_00424566 等价装载

    auto rec = read_terrain_records80();
    // 地形防御加成%（int2）：g_battlecalc_terrain_modifier[t] == rec[t*5+2]
    for (int t = 0; t < 16; ++t) {
        EXPECT_EQ(g_battlecalc_terrain_modifier[t], rec[t * 5 + 2]) << "def t" << t;
        // mod_a / mod_b（FUN_004138ea case 3/4/5 / case 6）
        EXPECT_EQ(g_terrain_mod_a_0x475aa8[t], rec[t * 5 + 0]) << "moda t" << t;
        EXPECT_EQ(g_terrain_mod_b_0x475aac[t], rec[t * 5 + 1]) << "modb t" << t;
        // clean 层防御加成（g_terrain_defense_bonus）
        EXPECT_EQ((int)dlzr::logic::get_terrain_defense_bonus(t), rec[t * 5 + 2])
            << "clean def t" << t;
    }
    // 二进制钉死关键点（证据 FUN_0042a98e.c:21 / FUN_004138ea.c:262/310）
    EXPECT_EQ(g_battlecalc_terrain_modifier[0], 10);    // terr[0]  def=10
    EXPECT_EQ(g_battlecalc_terrain_modifier[9], 15);    // terr[9]  def=15
    EXPECT_EQ(g_battlecalc_terrain_modifier[11], 40);   // terr[11] def=40
    EXPECT_EQ(g_terrain_mod_a_0x475aa8[9], 20);
    EXPECT_EQ(g_terrain_mod_b_0x475aac[9], 0);

    // 技能形状表块#11 → g_battlecalc_skill_shape_raw(=DAT_007cdd00)
    auto raw = read_skill_shape_raw();
    bool all_zero = true;
    for (size_t i = 0; i < 9408; ++i) {
        ASSERT_EQ(g_battlecalc_skill_shape_raw[i], raw[i]) << "skill raw byte " << i;
        if (g_battlecalc_skill_shape_raw[i] != 0) all_zero = false;
    }
    EXPECT_FALSE(all_zero)
        << "技能形状表不应全零（应含 100=填充/0=空 的真实 7×7 形状，48 条目×196B）";
}

// ---- MAP.DAT 瓦片 → 0x584390 地形网格全局 ----
TEST(BootRealData, MapLoadInstallsTerrainGrid) {
    dlzr::app::DatLoader loader(kGameDataDir);
    dlzr::core::FightMap fm = loader.load_map(0);  // 触发 FUN_00416c70 等价装载

    EXPECT_EQ(g_battlecalc_map_width, (int32_t)fm.width);
    ASSERT_EQ((size_t)g_battlecalc_map_width * fm.height, fm.tiles.size());
    for (size_t t = 0; t < fm.tiles.size(); ++t) {
        // 网格存完整瓦片值（oracle battle_calc_equip_defense 在内部做 >>12 取地形类型），
        // 故此处锁死「完整瓦片 1:1 拷贝」，与既有 TerrainGridWiredFromMapDat 一致。
        EXPECT_EQ((int)g_battlecalc_terrain_grid[t], (int)fm.tiles[t]) << "grid tile " << t;
        EXPECT_EQ((int)(g_battlecalc_terrain_grid[t] >> 12), (int)(fm.tiles[t] >> 12))
            << "grid terrain type tile " << t;
    }
}
