// test_battle_setup.cpp — 3-pass 战斗管线：setup pass 原语 RE 对拍
//
// 验证对象（铁律零，逐指令实证）：
//   0x00427a71 — N×N 矩阵旋转/拷贝原语（setup pass 按朝向旋转 7×7 技能形状表）
//   0x00412030 — 技能形状叠加（旋转后以锚点写入战场缓冲，越界裁剪）
//   0x0043de50 — 手写 memcpy（HP 缓冲→morale 缓冲 0x208 字节）
//
// 方法：oracle（battle_calc_matrix_rotate_27a71 等）vs 独立 ref_rotate（几何旋转矩阵）
// 交叉验证 + 手算单测 + 不变量（4×90°CW = 单位矩阵）。

#include <gtest/gtest.h>
#include <cstdint>
#include <cstring>
#include <vector>
#include <fstream>
#include <string>

extern "C" {
#include "replica/battle_calc.hpp"
#include "replica/replica_stubs.h"
}

#include "replica/battle_sim.hpp"   // battle_calc_run_skill_pipeline（C++ 链接，须在 extern "C" 之外）

namespace {
const char* kGameDataDir = "C:/seesaw/source/dlz_research/DLZFULL";

// ---- 真实技能形状表来源：game.dat 顺序块（铁律零实证）----
// 证据 FUN_00424566.c:24：FUN_0043e510(&DAT_007cdd00, 4, 0x930, handle)
//   从 game.dat 顺序读取 0x930*4 = 9408 字节（48 条目 × 196 字节 = 48×7×7 uint32）。
// 该读取是 game.dat 的第 11 个顺序块；前 10 块字节数取自同函数逐条调用
//   （param_2*param_3），累加得技能形状表的文件内偏移。
//   校验：31820 + 9408 + 15360 = 56588 = game.dat 实际大小（精确吻合 → 顺序布局确认）。
// 注意：DAT_007cdd00 位于 .data 的未初始化区（RVA 0x3cdd00 远超 SizeOfRawData 0x5200），
//       故 PE 静态文件不含此表，必须运行时从 game.dat 装载（与 FUN_00424566 一致）。
static size_t game_dat_skill_shape_offset() {
    // 前 10 块 (param_2, param_3)，顺序同 FUN_00424566.c:14-23
    const uint32_t pre[][2] = {
        {1, 0xa0}, {0x14, 0x10}, {0xa0, 0x60}, {1, 0x600}, {0x48, 0xa0},
        {0x1c, 10}, {1, 100}, {0x30, 10}, {0x30, 0x10}, {1, 0x510},
    };
    size_t off = 0;
    for (auto& p : pre) off += (size_t)p[0] * p[1];
    return off;  // = 31820 = 0x7C4C
}

static bool load_skill_shape_table(std::vector<uint8_t>& out) {
    std::string path = std::string(kGameDataDir) + "/game.dat";
    std::ifstream f(path, std::ios::binary);
    if (!f) {
        f.open(std::string(kGameDataDir) + "/GAME.DAT", std::ios::binary);  // Windows 大小写不敏感回退
        if (!f) return false;
    }
    const size_t off = game_dat_skill_shape_offset();
    const size_t nbytes = 4u * 0x930u;  // 9408
    f.seekg((std::streamoff)off);
    out.resize(nbytes);
    f.read((char*)out.data(), (std::streamsize)nbytes);
    return (size_t)f.gcount() == nbytes;
}
}  // namespace

// ---- 独立 reference：几何旋转（与二进制逐索引转录互为交叉验证）----
static void ref_rotate(const uint32_t* src, uint32_t* dst, int n, int mode) {
    for (int i = 0; i < n * n; i++) dst[i] = src[i];  // 占位，下面按模式覆盖
    if (mode == 0 || mode == (int)0xffffffff) {
        for (int i = 0; i < n * n; i++) dst[i] = src[i];
    } else if (mode == 1) {  // 180°
        for (int r = 0; r < n; r++)
            for (int c = 0; c < n; c++)
                dst[r * n + c] = src[(n - 1 - r) * n + (n - 1 - c)];
    } else if (mode == 2) {  // 90° CCW: dst[i][j] = src[j][n-1-i]
        for (int i = 0; i < n; i++)
            for (int j = 0; j < n; j++)
                dst[i * n + j] = src[j * n + (n - 1 - i)];
    } else if (mode == 3) {  // 90° CW: dst[i][j] = src[n-1-j][i]
        for (int i = 0; i < n; i++)
            for (int j = 0; j < n; j++)
                dst[i * n + j] = src[(n - 1 - j) * n + i];
    }
}

// 单位矩阵式填充：src[r*n+c] = r*100 + c（便于肉眼追踪旋转）
static void fill_linear(uint32_t* m, int n) {
    for (int r = 0; r < n; r++)
        for (int c = 0; c < n; c++)
            m[r * n + c] = (uint32_t)(r * 100 + c);
}

TEST(BattleSetup, MatrixRotateMatchesGeometricRef) {
    const int N = 7;
    std::vector<uint32_t> src(N * N), got(N * N), ref(N * N);
    fill_linear(src.data(), N);
    for (int mode = 0; mode <= 3; mode++) {
        memset(got.data(), 0, N * N * 4);
        memset(ref.data(), 0, N * N * 4);
        battle_calc_matrix_rotate_27a71(src.data(), got.data(), N, mode);
        ref_rotate(src.data(), ref.data(), N, mode);
        for (int i = 0; i < N * N; i++) {
            ASSERT_EQ(got[i], ref[i]) << "mode=" << mode << " idx=" << i;
        }
    }
    // 未知 mode：不写 dst（保持原版语义）
    memset(got.data(), 0xCD, N * N * 4);
    battle_calc_matrix_rotate_27a71(src.data(), got.data(), N, 7);
    bool untouched = true;
    for (int i = 0; i < N * N; i++) if (got[i] != 0xCDCDCDCDu) untouched = false;
    EXPECT_TRUE(untouched) << "未知 mode 不应改写 dst";
}

TEST(BattleSetup, MatrixRotateHandComputedN4) {
    const int N = 4;
    uint32_t src[16], got[16];
    fill_linear(src, N);  // 0,1,2,3,100,101,...
    // mode 1 = 180°：src[r*4+c]=r*100+c → dst[i][j]=src[3-i][3-j]
    uint32_t expected_180[16] = {
        303,302,301,300, 203,202,201,200, 103,102,101,100, 3,2,1,0};
    battle_calc_matrix_rotate_27a71(src, got, N, 1);
    for (int i = 0; i < 16; i++) ASSERT_EQ(got[i], expected_180[i]) << "180 idx=" << i;
}

TEST(BattleSetup, MatrixRotateFourTimesCWIsIdentity) {
    const int N = 5;
    std::vector<uint32_t> src(N * N), tmp(N * N), cur(N * N);
    fill_linear(src.data(), N);
    cur = src;
    for (int t = 0; t < 4; t++) {
        battle_calc_matrix_rotate_27a71(cur.data(), tmp.data(), N, 3);  // 90° CW
        cur = tmp;
    }
    for (int i = 0; i < N * N; i++)
        ASSERT_EQ(cur[i], src[i]) << "4×90CW 应为单位矩阵 idx=" << i;
}

TEST(BattleSetup, SkillOverlayClippingAtCorner) {
    // 7×7 全 1 形状，锚点 (0,0)，缓冲 13×10；仅 buf[0..3][0..3] 16 格落入界内。
    uint32_t shape[49];
    for (int i = 0; i < 49; i++) shape[i] = 1;
    const int BW = 13, BH = 10;
    std::vector<uint32_t> buf(BW * BH, 0);
    battle_setup_skill_overlay_12030(shape, 0, 0, 0, buf.data(), BW, BH);
    int nonzero = 0;
    for (int by = 0; by < BH; by++)
        for (int bx = 0; bx < BW; bx++) {
            if (buf[by * BW + bx] != 0) {
                nonzero++;
                ASSERT_TRUE(by < 4 && bx < 4) << "越界写入 @" << bx << "," << by;
                ASSERT_EQ(buf[by * BW + bx], 1u);
            }
        }
    EXPECT_EQ(nonzero, 16) << "角点锚点应恰好 16 格落入界内";
}

TEST(BattleSetup, SkillOverlayRotatedDistinctPattern) {
    // 形状 shape[row*7+col] = row*7+col+1（1..49 互异且非零），facing=3（90°CW），锚点中心 (6,5)，缓冲 13×10。
    uint32_t shape[49];
    for (int r = 0; r < 7; r++)
        for (int c = 0; c < 7; c++)
            shape[r * 7 + c] = (uint32_t)(r * 7 + c + 1);
    const int BW = 13, BH = 10;
    std::vector<uint32_t> buf(BW * BH, 0);
    battle_setup_skill_overlay_12030(shape, 6, 5, 3, buf.data(), BW, BH);
    // 旋转后 rot[r][c] = shape[(6-c)*7 + r]；锚点 (6,5) → bx=3+col, by=2+row，全部 49 格无裁剪。
    // 抽查：buf[2][3] = rot[0][0] = shape[6*7+0] = 43；buf[8][9] = rot[6][6] = shape[0*7+6] = 7。
    EXPECT_EQ(buf[2 * BW + 3], 43u);
    EXPECT_EQ(buf[8 * BW + 9], 7u);
    int written = 0;
    for (uint32_t v : buf) if (v != 0) written++;
    EXPECT_EQ(written, 49) << "中心锚点 7×7 应全部写入，无裁剪";
}

TEST(BattleSetup, HpToMoraleMemcpyExact) {
    // FUN_0043de50(&DAT_004565d0, &DAT_007d0488, 0x208)：HP 缓冲→morale 缓冲 0x208 字节。
    const size_t SZ = 0x208;
    std::vector<uint8_t> src(SZ), dst(SZ, 0);
    for (size_t i = 0; i < SZ; i++) src[i] = (uint8_t)((i * 31 + 7) & 0xFF);
    battle_setup_memcpy_3de50(dst.data(), src.data(), SZ);
    EXPECT_EQ(memcmp(dst.data(), src.data(), SZ), 0) << "0x208 字节应逐字节一致";
    // 越界不应写：放大 dst，确认恰好 0x208 字节被改，其后仍为 0。
    std::vector<uint8_t> big(0x300, 0);
    battle_setup_memcpy_3de50(big.data(), src.data(), SZ);
    for (size_t i = SZ; i < big.size(); i++)
        ASSERT_EQ(big[i], 0) << "memcpy 不应越过 0x208 字节（idx=" << i << "）";
}

// 真实 7×7 技能形状表：来自 game.dat 顺序块（铁律零实证，非 PE 静态数据）。
// 证据 FUN_00424566.c:24（game.dat 装载 DAT_007cdd00）+ FUN_00412030.c:26（按索引读取 + 旋转叠加）。
TEST(BattleSetup, SkillShapeTableRealData) {
    std::vector<uint8_t> table;
    ASSERT_TRUE(load_skill_shape_table(table)) << "应从 game.dat 抽取 9408 字节技能形状表";
    const int N_ENTRIES = 48;  // 9408 / 196
    ASSERT_EQ((int)table.size(), N_ENTRIES * 196);

    // 1) 真实数据非空（排除 BSS 误映射：不应全 0）
    bool any_nonzero = false;
    for (uint8_t b : table) if (b != 0) { any_nonzero = true; break; }
    EXPECT_TRUE(any_nonzero) << "技能形状表应为真实形状数据，而非全 0（BSS 误映射）";

    // 2) 旋转群性质在真实数据上成立（4×90CW=单位矩阵；180°=2×90CW）
    for (int e = 0; e < N_ENTRIES; e++) {
        uint32_t raw[49], r1[49], r2[49], r3[49], r4[49];
        battle_calc_skill_shape_at(table.data(), e, 0, raw);   // facing=0 → 拷贝
        battle_calc_matrix_rotate_27a71(raw, r1, 7, 3);
        battle_calc_matrix_rotate_27a71(r1, r2, 7, 3);
        battle_calc_matrix_rotate_27a71(r2, r3, 7, 3);
        battle_calc_matrix_rotate_27a71(r3, r4, 7, 3);
        for (int i = 0; i < 49; i++)
            ASSERT_EQ(r4[i], raw[i]) << "entry " << e << " 4×90CW 应为单位矩阵 idx=" << i;
        uint32_t r180[49];
        battle_calc_matrix_rotate_27a71(raw, r180, 7, 1);
        for (int i = 0; i < 49; i++)
            ASSERT_EQ(r180[i], r2[i]) << "entry " << e << " 180° 应等于 2×90CW idx=" << i;
    }

    // 3) 叠加往返：entry 0（单点形状，中心=100）经 facing=3 旋转后写入 13×10 缓冲（锚点 6,5），与旋转结果逐格一致
    uint32_t shape0[49];
    battle_calc_skill_shape_at(table.data(), 0, 3, shape0);
    const int BW = 13, BH = 10;
    std::vector<uint32_t> buf((size_t)BW * BH, 0);
    battle_setup_skill_overlay_12030(shape0, 6, 5, 3, buf.data(), BW, BH);
    int nonzero = 0, center = 0;
    for (int r = 0; r < 7; r++)
        for (int c = 0; c < 7; c++) {
            int bx = 3 + c, by = 2 + r;
            ASSERT_EQ(buf[(size_t)by * BW + bx], shape0[r * 7 + c])
                << "entry0 叠加往返不一致 @cell " << r << "," << c;
            if (buf[(size_t)by * BW + bx] != 0) { nonzero++; if (bx == 6 && by == 5) center++; }
        }
    EXPECT_EQ(nonzero, 1) << "entry0 为单点形状，缓冲应恰有 1 个非零点";
    EXPECT_EQ(center, 1) << "entry0 唯一点应落在锚点 (6,5)";
}

// ---------------------------------------------------------------------------
// 真实数据接线（residual #1）：game.dat 块#11 技能形状表必须流进 replica 全局
// （=二进制 DAT_007cdd00），使 battle_calc_run_skill_pipeline 在表参数为 null 时回退读取。
// 断言：① 安装后进全局读取与直接传 raw 给 battle_calc_skill_shape_at 逐位一致；
//       ② pipeline 用 null 表 = 用已安装全局，结果 == 显式传表。
// ---------------------------------------------------------------------------
TEST(BattleSetup, SkillShapeTableRuntimeInstall) {
    std::vector<uint8_t> raw;
    ASSERT_TRUE(load_skill_shape_table(raw)) << "应能从 game.dat 块#11 读 9408 字节";
    ASSERT_EQ(raw.size(), 9408u);

    battle_calc_install_skill_shape_table(raw.data());

    for (int facing = 0; facing <= 3; ++facing) {
        uint32_t out_global[49], out_direct[49];
        ASSERT_EQ(battle_calc_skill_shape_at(g_battlecalc_skill_shape_raw, 0, facing, out_global), 0);
        ASSERT_EQ(battle_calc_skill_shape_at(raw.data(), 0, facing, out_direct), 0);
        for (int i = 0; i < 49; ++i)
            ASSERT_EQ(out_global[i], out_direct[i])
                << "facing=" << facing << " cell=" << i << "：全局与直接读取应一致";
    }
}

TEST(BattleSetup, SkillShapeTableRuntimePipelineUsesGlobal) {
    std::vector<uint8_t> raw;
    ASSERT_TRUE(load_skill_shape_table(raw));
    battle_calc_install_skill_shape_table(raw.data());

    // 安装后用 null 表参数，pipeline 应回退到全局并成功（不返回 -1）
    int32_t r_param = battle_calc_run_skill_pipeline(0, 3, 0, 4, 4, 0, 9, 9, 9, 0, raw.data());
    int32_t r_global = battle_calc_run_skill_pipeline(0, 3, 0, 4, 4, 0, 9, 9, 9, 0, nullptr);
    EXPECT_NE(r_param, -1) << "显式传表应成功";
    EXPECT_NE(r_global, -1) << "null 表应回退到已安装全局并成功";
    EXPECT_EQ(r_param, r_global) << "null 表回退到全局，结果应与显式传表一致";
}

