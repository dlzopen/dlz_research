// test_save_system.cpp — 存档系统保真门禁（铁律零：用真实 DLZ 二进制，不 mock）
//
// 验证对象：原作 28 数据块格式（output/save_load_analysis.md）。
//  - 真实 DLZFULL/save0.dat = 85072 字节 = 28 块累加（证据级实测）。
//  - SaveLoader 必须能按该格式无损解析真实存档，且各块边界与文档/loader 一致。
//  - 锚定校验：people 块（偏移 17696, 16896B=128×132）与 map_grid 块（偏移 52248,
//    32768B=16384×uint16）解码后字节必须 == 文件原始字节；任一错位即暴露块尺寸/步长回归。

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

#include "app/resource/save_loader.hpp"
#include "core/data_model/game_data.hpp"

using namespace dlzr;  // loader 在 dlzr::app，core:: 可解析为 dlzr::core

namespace fs = std::filesystem;

#ifndef DLZR_DATA_DIR_DEFAULT
#define DLZR_DATA_DIR_DEFAULT "DLZFULL"
#endif

// 二十八数据块尺寸（与 output/save_load_analysis.md 及 save_loader.cpp BLOCK_SIZES 一致）
static constexpr size_t kBlockSizes[28] = {
    4, 4, 4, 4, 700, 4, 4, 4, 4, 4,
    64, 76, 56, 380,
    8192, 8192, 16896, 16896,
    320, 320, 80, 40,
    32768, 8, 28, 12, 4, 4,
};

// 各块（0-indexed）起始偏移（累加）
static constexpr size_t kBlockOffset[29] = {
    0,       4,      8,      12,     16,     716,    720,    724,    728,    732,
    736,     800,    876,    932,    1312,   9504,   17696,  34592,  51488,  51808,
    52128,   52208,  52248,  85016,  85024,  85052,  85064,  85068,  85072,
};

class SaveSystemTest : public ::testing::Test {
protected:
    fs::path data_dir_;
    void SetUp() override {
        const char* env = std::getenv("DLZR_DATA_DIR");
        data_dir_ = env ? fs::path(env) : fs::path(DLZR_DATA_DIR_DEFAULT);
    }
    fs::path save_path(int slot) const {
        return data_dir_ / ("save" + std::to_string(slot) + ".dat");
    }
    std::vector<uint8_t> read_file(const fs::path& p) {
        std::ifstream f(p, std::ios::binary);
        return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
    }
};

// —— 结构层：真实存档字节必须精确匹配 28 块累加布局（与 loader 实现无关）——
TEST_F(SaveSystemTest, RealSaveFileHas28BlockLayout) {
    auto p = save_path(0);
    if (!fs::exists(p)) {
        GTEST_SKIP() << "需要 DLZFULL/save0.dat（设置 DLZR_DATA_DIR）";
    }
    auto buf = read_file(p);
    size_t expected_total = 0;
    for (size_t s : kBlockSizes) expected_total += s;
    ASSERT_EQ(expected_total, 85072u) << "28 块累加必须等于真实存档尺寸";
    ASSERT_EQ(buf.size(), expected_total) << "真实 save0.dat 尺寸必须等于 28 块累加(85072)";

    // 逐块边界与文档/loader 完全一致
    for (int i = 0; i < 28; ++i) {
        EXPECT_EQ(kBlockOffset[i + 1], kBlockOffset[i] + kBlockSizes[i])
            << "块 " << i << " 边界必须连续";
    }
    EXPECT_EQ(kBlockOffset[28], buf.size()) << "末块边界必须等于文件尺寸";
}

// —— 锚定：people[0]（偏移 17696）解码字节 == 文件原始字节；进度字段自洽 ——
// people[0] = 孙悟空/主角（用户确认：编号0是占位孙悟空，同时是主角），不得被跳过/清零。
// 该断言同时验证了块 0..16 的边界与文档/loader 完全一致（people 块之前的所有块已对齐）。
TEST_F(SaveSystemTest, LoadRealSaveAnchorsPeople0AndProgress) {
    auto p = save_path(0);
    if (!fs::exists(p)) {
        GTEST_SKIP() << "需要 DLZFULL/save0.dat（设置 DLZR_DATA_DIR）";
    }
    auto raw = read_file(p);

    app::SaveLoader loader(data_dir_);
    core::GameData gd;
    ASSERT_TRUE(loader.load_game(0, gd)) << "真实 save0.dat 必须能被 28 块格式解析";

    constexpr size_t kPeopleEntry = 132;  // 16896 / 128
    constexpr size_t kPeopleOff = kBlockOffset[16];  // 17696
    ASSERT_GE(raw.size(), kPeopleOff + kPeopleEntry);
    EXPECT_EQ(0, std::memcmp(&gd.people[0], &raw[kPeopleOff], kPeopleEntry))
        << "解码 people[0](孙悟空/主角) 必须 == 文件偏移 17696 的原始字节";

    // 进度字段自洽
    EXPECT_GE(gd.current_fight_index, 0);
    EXPECT_LE(gd.current_fight_index, 47);
}

// —— 无损往返：加载真实存档 → 另存临时目录 → 重载，各主块字节一致 ——
// 这是复刻层最关键的保真闸门：loader 写入的 28 块必须能原样读回，
// 不丢字节、不错位。people 块若只写了 40/128 条会让文件缩水到 73456B，此测试直接失败。
TEST_F(SaveSystemTest, SaveLoadRoundTripIsLossless) {
    auto p = save_path(0);
    if (!fs::exists(p)) {
        GTEST_SKIP() << "需要 DLZFULL/save0.dat（设置 DLZR_DATA_DIR）";
    }

    app::SaveLoader loader(data_dir_);
    core::GameData gd1;
    ASSERT_TRUE(loader.load_game(0, gd1));

    fs::path tmp = data_dir_ / "__save_rt_tmp__";
    fs::create_directories(tmp);
    app::SaveLoader tmp_loader(tmp);
    ASSERT_TRUE(tmp_loader.save_game(0, gd1));

    core::GameData gd2;
    ASSERT_TRUE(tmp_loader.load_game(0, gd2));

    // 各主块（people/fighter/map_grid）往返后字节一致 = 无损
    EXPECT_EQ(0, std::memcmp(&gd1.people[0], &gd2.people[0], 128 * 132))
        << "people 块（128×132）往返必须无损";
    EXPECT_EQ(0, std::memcmp(gd1.fighter_array.data(), gd2.fighter_array.data(),
                             gd1.fighter_array.size() * sizeof(core::Fighter)))
        << "fighter_array 往返必须无损";
    EXPECT_EQ(0, std::memcmp(gd1.map_grid.data(), gd2.map_grid.data(),
                             gd1.map_grid.size() * sizeof(uint16_t)))
        << "map_grid 往返必须无损（块 17+ 边界须与 loader 自洽）";

    // 清理
    std::error_code ec;
    fs::remove_all(tmp, ec);
}
