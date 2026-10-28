// test_game_dat_content.cpp — GAME.DAT 关键战斗表的字节级内容闸门
//
// 铁律零实证：FUN_00424566.c 顺序装载 GAME.DAT 各块；DatLoader::load_game_data
// 按已知偏移抽取。本测试不依赖 DatLoader 的逐字段解析做自洽，而是「独立从
// GAME.DAT 原始字节抽取」同一段，与 DatLoader 产出逐字节对拍——若 DatLoader
// 用了错误偏移/元素宽（如旧实现把地形表读成 40B/地形读穿相邻块的 bug），必红。
//
//   地形修正表 DiXingModifier  @ GAME.DAT +0x00A0  (16 × 20B = 16 × 5 int32)
//   兵种地形移动消耗 BingZhongYiDong @ GAME.DAT +0x3F20 (96 × 16B uint8)
//
// 另含硬锚点（RE 实证，非自洽）：兵种0 移动消耗=[1×14,99,99]；兵种1 地形3/4=2。

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <vector>

#include "app/resource/dat_loader.hpp"
#include "core/data_model/constants.hpp"
#include "core/data_model/game_data.hpp"
#include "logic/combat/combat_formulas.hpp"

namespace {

const char* kGameDataDir = "C:/seesaw/source/dlz_research/DLZFULL";

// 独立抽取 GAME.DAT 原始字节（不经由 DatLoader 解析），作为字节级 oracle。
std::vector<uint8_t> raw_gamedat_slice(const std::string& dir, size_t off, size_t n) {
    std::ifstream f(std::string(dir) + "/GAME.DAT", std::ios::binary);
    EXPECT_TRUE(f.good()) << "GAME.DAT 应可读";
    f.seekg(static_cast<std::streamoff>(off));
    std::vector<uint8_t> buf(n);
    f.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(n));
    return buf;
}

}  // namespace

// ---- 地形修正表（块#1）：GAME.DAT @0x00A0，16×20B(=5 int32) ----
TEST(GameDatContent, TerrainModifierBytesMatchRaw) {
    dlzr::app::DatLoader loader(kGameDataDir);
    auto gd = loader.load_game_data();
    ASSERT_EQ(gd.terrain_modifiers.size(), 16u);

    auto raw = raw_gamedat_slice(kGameDataDir, 0x00A0, 16u * 20u);
    ASSERT_EQ(raw.size(), 16u * 20u);

    for (int t = 0; t < 16; ++t) {
        for (int j = 0; j < 5; ++j) {
            int32_t file_val = 0;
            std::memcpy(&file_val, &raw[static_cast<size_t>(t) * 20 + j * 4], 4);
            EXPECT_EQ(gd.terrain_modifiers[t][j], file_val)
                << "地形修正表[" << t << "][" << j
                << "] 与 GAME.DAT@0x00A0 原始字节不符（DatLoader 偏移/元素宽回归？）";
        }
    }
}

// ---- 兵种地形移动消耗矩阵（块#3）：GAME.DAT @0x3F20，96×16B(uint8) ----
TEST(GameDatContent, ClassMoveCostBytesMatchRaw) {
    dlzr::app::DatLoader loader(kGameDataDir);
    auto gd = loader.load_game_data();
    ASSERT_EQ(gd.class_move_cost.size(),
              static_cast<size_t>(dlzr::core::MAX_CLASS_TYPES));

    auto raw = raw_gamedat_slice(
        kGameDataDir, 0x3F20,
        static_cast<size_t>(dlzr::core::MAX_CLASS_TYPES) * 16u);
    ASSERT_EQ(raw.size(), static_cast<size_t>(dlzr::core::MAX_CLASS_TYPES) * 16u);

    for (int c = 0; c < dlzr::core::MAX_CLASS_TYPES; ++c) {
        for (int terr = 0; terr < 16; ++terr) {
            uint8_t file_val = raw[static_cast<size_t>(c) * 16u + terr];
            EXPECT_EQ(gd.class_move_cost[c][terr], file_val)
                << "移动消耗[" << c << "][" << terr
                << "] 与 GAME.DAT@0x3F20 原始字节不符（DatLoader 偏移/元素宽回归？）";
        }
    }
}

// ---- 硬锚点（RE 实证，非自洽）：二进制实测值 ----
TEST(GameDatContent, MoveCostKnownValuesMatchREEvidence) {
    dlzr::app::DatLoader loader(kGameDataDir);
    auto gd = loader.load_game_data();

    // 兵种0：前 14 地形移动消耗=1，后 2 地形(14/15)=99 不可通行。
    const uint8_t expect_cls0[16] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 99, 99};
    for (int terr = 0; terr < 16; ++terr) {
        EXPECT_EQ(gd.class_move_cost[0][terr], expect_cls0[terr])
            << "兵种0 移动消耗[" << terr << "] 与 RE 实证 [1×14,99,99] 不符";
    }
    // 兵种1：地形3/4 移动消耗=2（RE 实证）。
    EXPECT_EQ(gd.class_move_cost[1][3], 2);
    EXPECT_EQ(gd.class_move_cost[1][4], 2);
}

// 取 GAME.DAT 中某记录从 off 起的 16 字节名称（GBK），作为字节级 oracle。
static std::array<char, 16> raw_name_at(const std::string& dir, size_t off) {
    auto buf = raw_gamedat_slice(dir, off, 16u);
    std::array<char, 16> name{};
    for (int i = 0; i < 16 && i < static_cast<int>(buf.size()); ++i) name[i] = static_cast<char>(buf[i]);
    return name;
}

// ---- JiNeng（技能）记录对齐闸门 ----
// RE 实证（权威 010 模板 template/dlz_game.bt JiNeng 结构 + DLZFULL/GAME.DAT @0x743C 逐字节抽取）：
// 每条 48B，名称位于记录 +0x00（无前导文件头），字段顺序 name[16]/para1/para2/type/
// select_range/apply_range/anm_index/flm_index/audio_index。旧结构体误加 8B header 并把
// name 推到 +0x08，致 para1/para2/type 整体错位 8B（读到名称尾字节）。本测试钉死正确对齐。
TEST(GameDatContent, JiNengNameAndParamsAligned) {
    dlzr::app::DatLoader loader(kGameDataDir);
    auto gd = loader.load_game_data();
    ASSERT_GE(gd.skills.size(), 16u);

    // 名称位于记录 +0x00（前 8 字节不再是文件头）。
    auto name_raw0 = raw_name_at(kGameDataDir, 0x743C);
    EXPECT_TRUE(std::memcmp(gd.skills[0].name.data(), name_raw0.data(), 16) == 0)
        << "skills[0].name 应与 GAME.DAT@0x743C 的 16 字节一致（补给…）";

    // 真实文件值（python 抽取 DLZFULL/GAME.DAT @0x743C，16 条技能名
    // 补给/激励/医疗/火攻/滚木/擂石/威慑/骚扰/破坏/掠夺/反间/疑兵/督战/暗杀/施毒/空袭）：
    //   补给 para1=120, type=0(TILI_ADD)；医疗 para1=120 para2=10 type=2(TILI_SHIQI_ADD)；
    //   火攻 para1=80 type=3(TILI_REM)；掠夺 type=7(ZHUANGBEI_REM)；施毒 para1=180 para2=40；空袭 para1=400。
    EXPECT_EQ(gd.skills[0].para1, 120);
    EXPECT_EQ(gd.skills[0].type, 0);
    EXPECT_EQ(gd.skills[2].para1, 120);
    EXPECT_EQ(gd.skills[2].para2, 10);
    EXPECT_EQ(gd.skills[2].type, 2);
    EXPECT_EQ(gd.skills[3].para1, 80);
    EXPECT_EQ(gd.skills[3].type, 3);
    EXPECT_EQ(gd.skills[9].type, 7);
    EXPECT_EQ(gd.skills[14].para1, 180);
    EXPECT_EQ(gd.skills[14].para2, 40);
    EXPECT_EQ(gd.skills[15].para1, 400);
    // 名称逐条与文件字节对拍（钉死表格顺序：skills[15]=空袭）。
    auto name_raw15 = raw_name_at(kGameDataDir, 0x743C + 15 * 48);
    EXPECT_TRUE(std::memcmp(gd.skills[15].name.data(), name_raw15.data(), 16) == 0)
        << "skills[15].name 应与 GAME.DAT@0x743C+15*48 的 16 字节一致（空袭…）";
}

// ---- DiLei（地雷）记录对齐闸门 ----
// RE 实证（权威 010 模板 template/dlz_game.bt DiLei 结构 + DLZFULL/GAME.DAT @0x725C 逐字节抽取）：
// 每条 48B，名称位于记录 +0x00。模板在 name 与 price 间标了 pad0(@+0x10)，但真实文件该位
// 实为 weili（模板命名瑕疵、字节位置一致）；C++ 以 weili@+0x10 起。旧结构体误加 8B header
// 且 name 推到 +0x08，致 weili/price 错位。真实 10 条：土雷(weili60/price?/audio100)…轰天雷(800/800/109)。
// 本测试钉死正确对齐（@0x725C，name@+0x00，weili@+0x10，audio_index@+0x2C）。
TEST(GameDatContent, DiLeiNameAndParamsAligned) {
    dlzr::app::DatLoader loader(kGameDataDir);
    auto gd = loader.load_game_data();
    ASSERT_GT(gd.mines.size(), 9u);

    // 名称位于记录 +0x00（前 8 字节不再是文件头）。
    auto name_raw0 = raw_name_at(kGameDataDir, 0x725C);
    EXPECT_TRUE(std::memcmp(gd.mines[0].name.data(), name_raw0.data(), 16) == 0)
        << "mines[0].name 应与 GAME.DAT@0x725C 的 16 字节一致（土雷…）";

    // mines[9]=轰天雷：真实文件值（python 抽取 @0x725C+9*48）：weili=800, price=800, audio_index=109。
    auto name_raw9 = raw_name_at(kGameDataDir, 0x725C + 9 * 48);
    EXPECT_TRUE(std::memcmp(gd.mines[9].name.data(), name_raw9.data(), 16) == 0)
        << "mines[9].name 应与 GAME.DAT@0x725C+9*48 的 16 字节一致（轰天雷…）";
    EXPECT_EQ(gd.mines[9].weili, 800);
    EXPECT_EQ(gd.mines[9].price, 800);
    EXPECT_EQ(gd.mines[9].audio_index, 109);
}

// ---- Material（锻造材料）记录对齐闸门 ----
// RE 实证（权威 010 模板 template/dlz_game.bt Material 结构 + DLZFULL/GAME.DAT @0x70E0 逐字节抽取）：
// 每条 28B = name[16]/unit[8]/price[4]，名称位于记录 +0x00。旧 C++ 误加成 32B 且装载器偏移
// 误置 0x7260（落入 DiLei 块读脏数据）。真实 10 条：粘土/石头/木料/铁块/火硝/硫磺/木炭粉/
// 蝎钩/毒瘴菇/电池（单位 斤/块/两/只/个，价格 1/1/2/4/2/2/2/10/10/80）。本测试钉死 28B 对齐
// 与真实值（@0x70E0，name@+0x00，price@+0x18）。
TEST(GameDatContent, MaterialNameAndPriceAligned) {
    dlzr::app::DatLoader loader(kGameDataDir);
    auto gd = loader.load_game_data();
    ASSERT_GE(gd.materials.size(), 10u);

    // 名称位于记录 +0x00；独立从 GAME.DAT@0x70E0 + i*28 抽取 16 字节对拍。
    for (int i = 0; i < 10; ++i) {
        auto name_raw = raw_name_at(kGameDataDir, 0x70E0 + static_cast<size_t>(i) * 28u);
        EXPECT_TRUE(std::memcmp(gd.materials[i].name.data(), name_raw.data(), 16) == 0)
            << "materials[" << i << "].name 应与 GAME.DAT@0x70E0+i*28 的 16 字节一致";
    }

    // 真实文件值（python 抽取 @0x70E0，28B 步长）：
    //   粘土 price=1 / 石头 price=1 / 木料 price=2 / 铁块 price=4 /
    //   火硝 price=2 / 硫磺 price=2 / 木炭粉 price=2 / 蝎钩 price=10 / 毒瘴菇 price=10 / 电池 price=80。
    const int32_t expect_price[10] = {1, 1, 2, 4, 2, 2, 2, 10, 10, 80};
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(gd.materials[i].price, expect_price[i])
            << "materials[" << i << "].price 应=文件真实值 " << expect_price[i]
            << "（旧 0x7260 偏移/32B 误读会落到 DiLei 块→垃圾）";
    }
}

// ---- DiLeiRecipe（地雷配方矩阵）记录对齐闸门 ----
// RE 实证（权威 010 模板 template/dlz_game.bt DiLeiRecipe（char recipe[10]）+
//   DLZFULL/GAME.DAT @0x71F8 逐字节抽取）：10×10 矩阵，matrix[row=材料][col=地雷]=数量。
// 旧装载器偏移误置 0x73A0（落入 DiLei 块读脏数据）。本测试钉死 @0x71F8 与真实矩阵值。
TEST(GameDatContent, DiLeiRecipeMatrixAligned) {
    dlzr::app::DatLoader loader(kGameDataDir);
    auto gd = loader.load_game_data();

    // 独立从 GAME.DAT@0x71F8 抽取 100 字节与 DatLoader 产出逐字节对拍。
    auto raw = raw_gamedat_slice(kGameDataDir, 0x71F8, 100u);
    ASSERT_EQ(raw.size(), 100u);
    for (int r = 0; r < 10; ++r) {
        for (int c = 0; c < 10; ++c) {
            EXPECT_EQ(gd.mine_recipe.matrix[r][c], raw[static_cast<size_t>(r) * 10u + c])
                << "mine_recipe[" << r << "][" << c
                << "] 与 GAME.DAT@0x71F8 原始字节不符（旧 0x73A0 偏移回归？）";
        }
    }

    // 真实文件值（python 抽取 @0x71F8，10×10）：石雷配方 row0=[10,0,0,0,5,10,15,0,0,0]。
    const uint8_t expect_row0[10] = {10, 0, 0, 0, 5, 10, 15, 0, 0, 0};
    for (int c = 0; c < 10; ++c) {
        EXPECT_EQ(gd.mine_recipe.matrix[0][c], expect_row0[c])
            << "mine_recipe[0][" << c << "] 应=石雷配方真实值";
    }
}

// ---- ClassType（兵种）名称偏移闸门（钉死 0x1E0，杜绝 data_dictionary 误标 0x0320）----
// RE 实证：兵种表真实基址 0x1E0（class[1]=手枪队）；data_dictionary.md 误标 0x0320
// （该处 class[1]=冲锋队）。本测试同时核对正确偏移内容、并断言不等于错误偏移内容。
TEST(GameDatContent, ClassTypeNameOffsetCorrect) {
    dlzr::app::DatLoader loader(kGameDataDir);
    auto gd = loader.load_game_data();
    ASSERT_GT(gd.class_types.size(), 1u);

    // 正确基址 0x1E0 + 兵种1 记录(160B) + 名称@+0x00。
    auto correct = raw_name_at(kGameDataDir, 0x1E0 + 160u);
    EXPECT_TRUE(std::memcmp(gd.class_types[1].name.data(), correct.data(), 16) == 0)
        << "class_types[1].name 应=0x1E0+160 处 16 字节（手枪队…）";

    // 错误基址 0x0320（data_dictionary 误标）处为冲锋队，必须不等于正确加载值。
    auto wrong = raw_name_at(kGameDataDir, 0x0320 + 160u);
    EXPECT_TRUE(std::memcmp(gd.class_types[1].name.data(), wrong.data(), 16) != 0)
        << "class_types[1] 不应来自误标基址 0x0320（该处=冲锋队）；若相等说明偏移回归。";
}

// ---- wupin_compat（兵种×物品兼容矩阵）硬锚点 ----
// RE：FUN_0042e2d6.c:257 武器映射 class1→物品100→WuPin[99]=转轮式手枪，
// 故手枪队(class1) 可装备转轮式手枪(item99)：wupin_compat[99][1] != 0。
TEST(GameDatContent, WuPinCompatRevolverUsableByPistolClass) {
    dlzr::app::DatLoader loader(kGameDataDir);
    auto gd = loader.load_game_data();
    ASSERT_GT(gd.wupin_compat.size(), 99u);

    EXPECT_NE(gd.wupin_compat[99][1], 0)
        << "wupin_compat[99][1] 应非零：手枪队(class1) 可装备转轮式手枪(item99)；"
           "若为零说明兼容矩阵偏移/布局回归。";
}

// ---- 端到端：技能参数对齐 → 实际效果计算（可被玩家感知的 bug 闭环）----
// 旧 JiNeng 结构体漏 8B 头，gd.skills 整体错位 8B，para1/para2 读到名称尾字节（垃圾），
// calc_skill_effect 用垃圾算技能 HP/士气效果。修正后 施毒=skills[14]（RE 实证 @0x743C+14*48），
// para1=180, para2=40，施毒效果应 = (para1*智力/100)*factor。本测试钉死「对齐修复真正流入效果计算」。
TEST(GameDatContent, SkillParamsFlowToEffectComputation) {
    dlzr::app::DatLoader loader(kGameDataDir);
    auto gd = loader.load_game_data();
    ASSERT_GT(gd.skills.size(), 14u);

    // 施毒 (skill[14])：RE 实证 para1=180, para2=40。若仍错位，此断言必红。
    ASSERT_EQ(gd.skills[14].para1, 180)
        << "skills[14].para1 应=文件真实值 180（施毒）；若为名称垃圾说明仍漏 8B 头。";
    ASSERT_EQ(gd.skills[14].para2, 40);

    dlzr::core::People caster{};
    caster.intelligence = 100;
    int32_t skill_level = 1;            // factor = (1+7)>>3 = 1
    auto eff = dlzr::logic::calc_skill_effect(caster, gd.skills[14], skill_level);
    EXPECT_EQ(eff.hp_effect, (180 * 100 / 100) * 1)
        << "技能 HP 效果应 = para1*智力/100*factor = 180；若错配为垃圾则不为 180。";
    EXPECT_EQ(eff.morale_effect, (40 * 100 / 100) * 1)
        << "技能士气效果应 = para2*智力/100*factor = 40。";
}

// =====================================================================
// 防御性回归锚点：ClassType / WuPin / Material / People / Town 结构对齐
// 目的：JiNeng/DiLei 曾因漏 8B 文件头而整体错位 8B（玩家可感知 bug，且交叉层
// 差分测不出——clean/replica 同读错误加载）。同类 bug 可能发生在其他表；下列测试
// 用「独立从原始字节抽取」逐字段对拍，钉死每条表的名称偏移与关键数值字段对齐，
// 任何未来的偏移/步长回归都会立即红。
// =====================================================================

// ClassType：GAME.DAT @0x1E0，160B 记录步长，名称@+0x00（无文件头；与 JiNeng/DiLei 不同）。
TEST(GameDatContent, ClassTypeFieldsAligned) {
    dlzr::app::DatLoader loader(kGameDataDir);
    auto gd = loader.load_game_data();
    ASSERT_GT(gd.class_types.size(), 1u);

    // 名称@+0x00 应与原始字节一致（铁证：class[1]=手枪队，非误标基址 0x0320 的冲锋队）。
    auto name0 = raw_name_at(kGameDataDir, 0x1E0 + 0u * 160u);
    auto name1 = raw_name_at(kGameDataDir, 0x1E0 + 1u * 160u);
    EXPECT_TRUE(std::memcmp(gd.class_types[0].name.data(), name0.data(), 16) == 0)
        << "class_types[0].name 应=@0x1E0 的 16 字节（神仙队…）";
    EXPECT_TRUE(std::memcmp(gd.class_types[1].name.data(), name1.data(), 16) == 0)
        << "class_types[1].name 应=@0x1E0+160 的 16 字节（手枪队…）";

    // 关键数值字段（python 抽取 DLZFULL/GAME.DAT 实测）：
    //   [0] 神仙队: img=81, hp_base=1000, hp_growth=100, atk_growth=99, movement=8
    //   [1] 手枪队: img=30, hp_base=80,  hp_growth=12,  atk_growth=6,  movement=4
    EXPECT_EQ(gd.class_types[0].img_index, 81);
    EXPECT_EQ(gd.class_types[0].hp_base, 1000);
    EXPECT_EQ(gd.class_types[0].hp_growth, 100);
    EXPECT_EQ(gd.class_types[0].attack_growth, 99);
    EXPECT_EQ(gd.class_types[0].movement, 8);
    EXPECT_EQ(gd.class_types[1].img_index, 30);
    EXPECT_EQ(gd.class_types[1].hp_base, 80);
    EXPECT_EQ(gd.class_types[1].hp_growth, 12);
    EXPECT_EQ(gd.class_types[1].attack_growth, 6);
    EXPECT_EQ(gd.class_types[1].movement, 4);
}

// WuPin（物品）：GAME.DAT @0x43E0，72B 记录步长，名称@+0x00。
TEST(GameDatContent, WuPinFieldsAligned) {
    dlzr::app::DatLoader loader(kGameDataDir);
    auto gd = loader.load_game_data();
    ASSERT_GT(gd.items.size(), 99u);

    // WuPin[99]=转轮式手枪（名称含全角空格，逐字节对拍最稳）。
    auto name99 = raw_name_at(kGameDataDir, 0x43E0 + 99u * 72u);
    EXPECT_TRUE(std::memcmp(gd.items[99].name.data(), name99.data(), 16) == 0)
        << "items[99].name 应=@0x43E0+99*72 的 16 字节（转轮式手　枪…）";

    // 关键数值：price=300（python 抽取实测）。
    EXPECT_EQ(gd.items[99].price, 300)
        << "items[99].price 应=300（转轮式手枪）；若错位则不为 300。";
}

// Material（锻造材料）：GAME.DAT @0x7260，32B 记录步长，名称@+0x00。
TEST(GameDatContent, MaterialNameAligned) {
    dlzr::app::DatLoader loader(kGameDataDir);
    auto gd = loader.load_game_data();
    ASSERT_GT(gd.materials.size(), 0u);

    // 权威 010 模板 template/dlz_game.bt Material 基址 0x70E0（28B 步长，名称@+0x00）。
    auto name0 = raw_name_at(kGameDataDir, 0x70E0);
    EXPECT_TRUE(std::memcmp(gd.materials[0].name.data(), name0.data(), 16) == 0)
        << "materials[0].name 应=@0x70E0 的 16 字节（粘土…）";
}

// People（PEOPLE.DAT）：132B 记录步长，名称@+0x00，class_type@+0x2C。
TEST(GameDatContent, PeopleFieldsAligned) {
    dlzr::app::DatLoader loader(kGameDataDir);
    auto gd = loader.load_game_data();
    ASSERT_GT(gd.people.size(), 1u);

    auto name0 = raw_name_at(kGameDataDir, 0u * 132u);       // 注意：读自 PEOPLE.DAT
    // PEOPLE.DAT 与 GAME.DAT 不同文件；复用 raw_name_at 需指定文件，故内联读取。
    {
        std::ifstream f(std::string(kGameDataDir) + "/PEOPLE.DAT", std::ios::binary);
        EXPECT_TRUE(f.good());
        std::array<char, 16> buf{};
        f.read(buf.data(), 16);
        EXPECT_TRUE(std::memcmp(gd.people[0].name.data(), buf.data(), 16) == 0)
            << "people[0].name 应=PEOPLE.DAT@0 的 16 字节（孙悟空…）";
        f.seekg(0x2C);
        int32_t ct = 0;
        f.read(reinterpret_cast<char*>(&ct), 4);
        EXPECT_EQ(gd.people[0].class_type, ct)
            << "people[0].class_type 应与 PEOPLE.DAT@0x2C 一致（孙悟空=0…神仙队）";
        EXPECT_EQ(gd.people[0].class_type, 0);
    }
    // 第二人：朱老忠，class_type=4（步枪队）。
    {
        std::ifstream f(std::string(kGameDataDir) + "/PEOPLE.DAT", std::ios::binary);
        EXPECT_TRUE(f.good());
        std::array<char, 16> buf{};
        f.seekg(1u * 132u);
        f.read(buf.data(), 16);
        EXPECT_TRUE(std::memcmp(gd.people[1].name.data(), buf.data(), 16) == 0)
            << "people[1].name 应=PEOPLE.DAT@132 的 16 字节（朱老忠…）";
        f.seekg(1u * 132u + 0x2C);
        int32_t ct = 0;
        f.read(reinterpret_cast<char*>(&ct), 4);
        EXPECT_EQ(gd.people[1].class_type, 4)
            << "people[1].class_type 应=4（朱老忠→步枪队）；若错位则不为 4。";
    }
}

// Town（TOWN.DAT）：346B 记录步长，#pragma pack(1)。Town[0] 固定字段对拍。
TEST(GameDatContent, TownFieldsAligned) {
    dlzr::app::DatLoader loader(kGameDataDir);
    auto gd = loader.load_game_data();
    loader.load_towns(gd);
    ASSERT_GT(gd.towns.size(), 0u);

    // python 抽取 TOWN.DAT[0]：reserved0=0, reserved1=-1, sound1_id=803, flags[1]=1。
    EXPECT_EQ(gd.towns[0].reserved0, 0);
    EXPECT_EQ(gd.towns[0].reserved1, -1);
    EXPECT_EQ(gd.towns[0].sound1_id, 803);
    ASSERT_GT(gd.towns[0].flags.size(), 1u);
    EXPECT_EQ(gd.towns[0].flags[1], 1u);
}
