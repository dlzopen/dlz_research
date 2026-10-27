// test_dat_io.cpp — 真实数据 IO 集成测试（铁律零：用真实 DLZ 二进制，不 mock）
//
// 框架设计：
//   1. DatIoTest 夹具把 DatLoader 指向真实数据目录（DLZFULL）。
//      - 优先用环境变量 DLZR_DATA_DIR；
//      - 否则用编译期默认 DLZR_DATA_DIR_DEFAULT（CMake 注入 = 仓库/../DLZFULL）。
//   2. 每个测试用例用「独立原始字节重解析」交叉校验 loader 输出，而非硬编码魔法值，
//      从而真正吃到真实文件字节、捕捉 loader 偏移/步长/结构体尺寸回归。
//   3. 另加 RE 文档化定点（铁律零实证）：地形表 20B/记录、class_move_cost[0]=[1x14,99,99]、
//      593 条对话、map0 20x26/blk11、fight0 名“村庄风云”、PEOPLE[0]=孙悟空 等。
//
// 注意：load_game_data() 会把真实地形表/技能形状表 install 进 replica 全局（与原版运行时
//       装载一致），属预期真实副作用；交叉层差分用例的 clean/oracle 读同一全局，故不受污染。

#include <gtest/gtest.h>

#include <array>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "app/resource/dat_loader.hpp"
#include "core/data_model/constants.hpp"
#include "core/data_model/game_data.hpp"
#include "core/data_model/types.hpp"

using namespace dlzr;  // 测试处于全局命名空间；loader 在 dlzr::app，core:: 可解析为 dlzr::core

namespace fs = std::filesystem;

#ifndef DLZR_DATA_DIR_DEFAULT
#define DLZR_DATA_DIR_DEFAULT "DLZFULL"
#endif

namespace {

// ---- 真实文件读取（独立原始字节源，用于交叉校验）----
std::vector<uint8_t> read_file(const fs::path& p) {
    std::ifstream f(p, std::ios::binary | std::ios::ate);
    if (!f.is_open()) throw std::runtime_error("open failed: " + p.string());
    const size_t n = static_cast<size_t>(f.tellg());
    f.seekg(0);
    std::vector<uint8_t> v(n);
    f.read(reinterpret_cast<char*>(v.data()), static_cast<std::streamsize>(n));
    return v;
}

template <typename T>
T rd_le(const std::vector<uint8_t>& buf, size_t off) {
    T v;
    std::memcpy(&v, buf.data() + off, sizeof(T));
    return v;
}

// 把 char 数组转可打印 std::string（按长度截断，不依赖 null 终结）
template <size_t N>
std::string arr_to_str(const std::array<char, N>& a) {
    size_t len = 0;
    while (len < N && a[len] != '\0') ++len;
    return std::string(a.data(), len);
}

class DatIoTest : public ::testing::Test {
protected:
    fs::path data_dir_;

    void SetUp() override {
        const char* env = std::getenv("DLZR_DATA_DIR");
        data_dir_ = env ? fs::path(env) : fs::path(DLZR_DATA_DIR_DEFAULT);
        ASSERT_TRUE(fs::exists(data_dir_ / "GAME.DAT"))
            << "DLZFULL 数据目录未找到: " << data_dir_.string()
            << " （请设置环境变量 DLZR_DATA_DIR 指向含 GAME.DAT 的目录）";
    }

    dlzr::app::DatLoader make_loader() const {
        return dlzr::app::DatLoader(data_dir_);
    }
};

// ============================================================
// 文件完整性（真实文件事实，固定数据目录）
// ============================================================
TEST_F(DatIoTest, RealFileSizes) {
    EXPECT_EQ(56588u, read_file(data_dir_ / "GAME.DAT").size());
    EXPECT_EQ(5280u, read_file(data_dir_ / "PEOPLE.DAT").size());   // 40 × 132
    EXPECT_EQ(110900u, read_file(data_dir_ / "TXT.DAT").size());
    EXPECT_EQ(120690u, read_file(data_dir_ / "MAP.DAT").size());
    EXPECT_EQ(963876u, read_file(data_dir_ / "FIGHT.DAT").size());  // 47 × 20508
    EXPECT_EQ(15805952u, read_file(data_dir_ / "BLK.DAT").size());
    EXPECT_EQ(12992170u, read_file(data_dir_ / "WAVE.DAT").size());
}

// ============================================================
// PEOPLE.DAT — 字节级保真交叉校验
// ============================================================
TEST_F(DatIoTest, PeopleLoadedByteIdentical) {
    auto gd = make_loader().load_game_data();
    const auto raw = read_file(data_dir_ / "PEOPLE.DAT");
    ASSERT_GE(raw.size(), static_cast<size_t>(core::MAX_PEOPLE) * core::PEOPLE_RECORD_SIZE);
    static_assert(sizeof(core::People) == core::PEOPLE_RECORD_SIZE,
                  "People 结构体须与磁盘记录 132B 同尺寸");
    for (int i = 0; i < core::MAX_PEOPLE; ++i) {
        const uint8_t* rec = raw.data() + static_cast<size_t>(i) * core::PEOPLE_RECORD_SIZE;
        ASSERT_EQ(0, std::memcmp(&gd.people[i], rec, sizeof(core::People)))
            << "PEOPLE.DAT 记录 " << i << " 字节与 loader 输出不一致";
    }
}

TEST_F(DatIoTest, People0IsSunWukong) {
    auto gd = make_loader().load_game_data();
    // “孙悟空” GBK = CB EF CE F2 BF D5
    static const uint8_t kSunWukong[] = {0xcb, 0xef, 0xce, 0xf2, 0xbf, 0xd5};
    ASSERT_EQ(0, std::memcmp(gd.people[0].name.data(), kSunWukong, sizeof(kSunWukong)))
        << "PEOPLE[0] 应为孙悟空，实际: " << arr_to_str(gd.people[0].name);
    // 孙悟空 bingzhong=0 → 神仙队（loader 注释实证）
    EXPECT_EQ(0, gd.people[0].class_type);
}

// ============================================================
// GAME.DAT 地形修正表 — 偏移/步长回归 + RE 定点
// ============================================================
TEST_F(DatIoTest, TerrainTableMatchesRawParse) {
    auto gd = make_loader().load_game_data();
    const auto g = read_file(data_dir_ / "GAME.DAT");
    // 块#1 @ 0x00A0，16 × 20B（5×int32）
    for (int t = 0; t < 16; ++t) {
        int32_t rec[5];
        std::memcpy(rec, g.data() + 0x00A0 + static_cast<size_t>(t) * 20, 20);
        for (int j = 0; j < 5; ++j) {
            ASSERT_EQ(rec[j], gd.terrain_modifiers[t][j])
                << "地形表[" << t << "][" << j << "] 与原始字节不一致";
        }
    }
    // RE 定点（Python 独立抽取 DLZFULL/GAME.DAT 实证）
    EXPECT_EQ((std::array<int32_t, 5>{40, -20, 10, 0, 0}), gd.terrain_modifiers[0]);
    EXPECT_EQ((std::array<int32_t, 5>{20, 5, 10, 0, 0}), gd.terrain_modifiers[1]);
}

TEST_F(DatIoTest, TerrainStrideIs20Not40) {
    // 历史 bug 守卫：旧实现读 40B/地形会读穿进相邻块。
    // 真实 20B/记录时 terrain[1] == 原始@0xA0+20，且 != 原始@0xA0+40。
    auto gd = make_loader().load_game_data();
    const auto g = read_file(data_dir_ / "GAME.DAT");
    int32_t at20[5];
    std::memcpy(at20, g.data() + 0x00A0 + 20, 20);
    int32_t at40[5];
    std::memcpy(at40, g.data() + 0x00A0 + 40, 20);
    EXPECT_EQ(std::vector<int32_t>(at20, at20 + 5),
              std::vector<int32_t>(gd.terrain_modifiers[1].begin(),
                                   gd.terrain_modifiers[1].end()));
    EXPECT_NE(std::vector<int32_t>(at40, at40 + 5),
              std::vector<int32_t>(gd.terrain_modifiers[1].begin(),
                                   gd.terrain_modifiers[1].end()))
        << "地形表疑似用了 40B/记录（读穿相邻块）";
}

// ============================================================
// GAME.DAT 兵种地形移动消耗 + 兼容矩阵 — 交叉校验
// ============================================================
TEST_F(DatIoTest, ClassMoveCostMatchesRaw) {
    auto gd = make_loader().load_game_data();
    const auto g = read_file(data_dir_ / "GAME.DAT");
    // @ 0x3F20，96 兵种 × 16 × uint8
    uint8_t expect[16];
    std::memcpy(expect, g.data() + 0x3F20, 16);
    for (int k = 0; k < 16; ++k) {
        ASSERT_EQ(expect[k], gd.class_move_cost[0][k]) << "class_move_cost[0][" << k << "]";
    }
    // RE 定点：class[0]=[1×14, 99, 99]
    for (int k = 0; k < 14; ++k) EXPECT_EQ(1, gd.class_move_cost[0][k]);
    EXPECT_EQ(99, gd.class_move_cost[0][14]);
    EXPECT_EQ(99, gd.class_move_cost[0][15]);
}

TEST_F(DatIoTest, WupinCompatIsLastBlock) {
    auto gd = make_loader().load_game_data();
    const auto g = read_file(data_dir_ / "GAME.DAT");
    const size_t kCompatSize =
        static_cast<size_t>(core::MAX_WUPIN) * core::MAX_CLASS_TYPES;  // 15360
    ASSERT_GE(g.size(), kCompatSize);
    const size_t off = g.size() - kCompatSize;
    for (int item = 0; item < core::MAX_WUPIN; ++item) {
        for (int cls = 0; cls < core::MAX_CLASS_TYPES; ++cls) {
            ASSERT_EQ(g[off + static_cast<size_t>(item) * core::MAX_CLASS_TYPES + cls],
                      gd.wupin_compat[item][cls])
                << "wupin_compat[" << item << "][" << cls << "]";
        }
    }
}

// ============================================================
// TXT.DAT — XOR 0x51 解码 + 593 条定点 + 独立计数交叉校验
// ============================================================
TEST_F(DatIoTest, DialoguesDecode593) {
    auto dlg = make_loader().load_dialogues();
    EXPECT_EQ(593u, dlg.size());
    ASSERT_FALSE(dlg.empty());
    // 条目体保留“xx”尾标记（parse_dialogue 剥离前的原始体），不变量：首条以 xx 起头
    EXPECT_EQ(0u, dlg[0].find("xx"));
}

TEST_F(DatIoTest, DialogueDecodeIndependentCount) {
    auto dlg = make_loader().load_dialogues();
    // 独立再解码 + 同启发式计数，校验 loader 计数不漂移
    const auto raw = read_file(data_dir_ / "TXT.DAT");
    std::string dec;
    dec.reserve(raw.size());
    for (uint8_t b : raw) dec.push_back(static_cast<char>(b ^ core::TXT_XOR_KEY));
    auto is_start = [](const std::string& s) {
        if (s.size() >= 4 && std::isdigit(static_cast<unsigned char>(s[0])) &&
            std::isdigit(static_cast<unsigned char>(s[1])) &&
            std::isdigit(static_cast<unsigned char>(s[2])) && s[3] == ' ')
            return true;
        if (s.size() >= 3 && s[0] == '-' && s[1] == '1' && s[2] == ' ')
            return true;
        return false;
    };
    int indep = 0;
    std::istringstream iss(dec);
    std::string line;
    while (std::getline(iss, line)) {
        if (is_start(line)) ++indep;
    }
    EXPECT_EQ(indep, static_cast<int>(dlg.size()))
        << "独立解码计数与 loader 条目数不一致（解码/聚合逻辑漂移）";
    // XOR 可逆：再 XOR 一次须还原为原始密文
    std::vector<uint8_t> re;
    re.reserve(dec.size());
    for (char c : dec) re.push_back(static_cast<uint8_t>(c) ^ core::TXT_XOR_KEY);
    EXPECT_EQ(raw, re);
}

// ============================================================
// MAP.DAT — 第 0 关尺寸/blk 组 + 瓦片交叉校验
// ============================================================
TEST_F(DatIoTest, Map0Dimensions) {
    auto fm = make_loader().load_map(0);
    const auto m = read_file(data_dir_ / "MAP.DAT");
    const uint16_t w = rd_le<uint16_t>(m, 0x200);
    const uint16_t h = rd_le<uint16_t>(m, 0x202);
    EXPECT_EQ(w, fm.width);
    EXPECT_EQ(h, fm.height);
    EXPECT_EQ(20, fm.width);
    EXPECT_EQ(26, fm.height);
    // blk_file = "dlz-11.blk" → blk_group = 11（loader 实证于 dat_loader.cpp:248-266）
    char blk[13] = {};
    std::memcpy(blk, m.data() + 0x204, 12);
    const int bg = (blk[4] - '0') * 10 + (blk[5] - '0');
    EXPECT_EQ(bg, fm.blk_group);
    EXPECT_EQ(11, fm.blk_group);
    EXPECT_EQ(static_cast<size_t>(w) * h, fm.tiles.size());
    // 首瓦片交叉校验
    const uint16_t t0 = rd_le<uint16_t>(m, 0x200 + 0x1C);
    EXPECT_EQ(t0, fm.tiles[0]);
}

// ============================================================
// FIGHT.DAT — 第 0 关装载（运行时 people[40..] 映射）+ 名称定点
// ============================================================
TEST_F(DatIoTest, Fight0Loads) {
    auto fd = make_loader().load_fight(0);
    // 关卡名“村庄风云” GBK = B4 A9 D4 BD B7 E2 CB F8 CF DF
    static const uint8_t kName[] = {0xb4, 0xa9, 0xd4, 0xbd, 0xb7, 0xe2,
                                    0xcb, 0xf8, 0xcf, 0xdf};
    ASSERT_EQ(0, std::memcmp(fd.intro.name.data(), kName, sizeof(kName)))
        << "fight0 关卡名应为“村庄风云”，实际: " << arr_to_str(fd.intro.name);
    EXPECT_EQ(128u, fd.fighters.size());
    EXPECT_EQ(128u, fd.people.size());
    // 运行时映射：磁盘名册 → people[40..]（loader 注释实证，fight0 people[40]=伪军）
    bool people40_named = false;
    for (char c : fd.people[40].name) {
        if (c != '\0') { people40_named = true; break; }
    }
    EXPECT_TRUE(people40_named) << "fight0 名册未装载到运行时 people[40]（运行时映射回归）";
}

// ============================================================
// BLK.DAT — 第 11 组（map0 引用）瓦片装载
// ============================================================
TEST_F(DatIoTest, BlkGroup11Loads) {
    auto bg = make_loader().load_blk_group(11);
    ASSERT_FALSE(bg.tiles.empty());
    for (const auto& tile : bg.tiles) {
        EXPECT_EQ(static_cast<size_t>(dlzr::core::BlkGroupData::kTileSize) *
                      dlzr::core::BlkGroupData::kTileSize,
                  tile.size());
    }
}

// ============================================================
// WAVE.DAT — 首个非空 clip 装载 + 字节交叉校验
// ============================================================
TEST_F(DatIoTest, WaveClipLoads) {
    const auto w = read_file(data_dir_ / "WAVE.DAT");
    int found = -1;
    uint32_t pcm_off = 0, pcm_size = 0;
    for (int id = 0; id < 1024; ++id) {
        const uint32_t o = rd_le<uint32_t>(w, static_cast<size_t>(id) * 8);
        const uint32_t s = rd_le<uint32_t>(w, static_cast<size_t>(id) * 8 + 4);
        if (s != 0) {
            found = id;
            pcm_off = o;
            pcm_size = s;
            break;
        }
    }
    ASSERT_GE(found, 0) << "WAVE.DAT 无非空 clip";
    const auto clip = make_loader().load_wave_clip(found);
    EXPECT_EQ(pcm_size, clip.size());
    std::vector<uint8_t> expect(w.begin() + 0x2000 + pcm_off,
                                w.begin() + 0x2000 + pcm_off + pcm_size);
    EXPECT_EQ(expect, clip);
}

// ============================================================
// 负向：缺失文件须抛异常（真实 IO 错误处理）
// ============================================================
TEST_F(DatIoTest, MissingFileThrows) {
    dlzr::app::DatLoader bad(data_dir_ / "no_such_dir_xyz");
    EXPECT_THROW(bad.load_game_data(), std::runtime_error);
    EXPECT_THROW(bad.load_map(0), std::runtime_error);
    EXPECT_THROW(bad.load_dialogues(), std::runtime_error);
}

}  // namespace
