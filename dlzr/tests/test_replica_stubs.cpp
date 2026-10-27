// test_replica_stubs.cpp — 方向②：真实 C 转写桩的转写验证（borrow 验证工作流）
//
// 铁律零：桩函数本身即"开证坐标"，每个 RF:xxxxxxxxx 由 output/ghidra_all/FUN_*.c 转写而来。
// 本测试不验证 755 个桩的全集，而是建立「真实桩 + brute-force oracle」的可续作验证范式：
//   选一个自洽、纯函数、可独立 oracle 的已转写桩 rand_impl()（RF:0043e310），
//   用 Borland LCG 的精确算式（seed=seed*0x343fd+0x269ec3，返回 (seed>>16)&0x7fff，
//   证据 FUN_0043e310.c）做逐步对拍，确认转写与二进制一致且确定性可复现。

#include <gtest/gtest.h>

#include <windows.h>

#include <cstring>
#include <vector>

#include "replica/replica_stubs.h"  // ::GameState g_game_state、uint32_t rand_impl()、memmove_impl()

namespace {

// 纯函数 brute-force oracle：Borland LCG（FUN_0043e310.c 实证）
// 二进制为 32-bit 整数运算；uint32 回绕与 int32 在两补码下位模式一致。
uint32_t lcg_step(uint32_t& seed) {
    uint32_t s = seed * 0x343fdu + 0x269ec3u;
    seed = s;
    return (s >> 16) & 0x7fff;
}

}  // namespace

// 多种子逐步骤对拍：replica 桩输出必须 == 独立 LCG oracle。
TEST(ReplicaStubs, RandImplMatchesLCGOracle) {
    const std::vector<uint32_t> seeds = {0u, 1u, 42u, 0x12345678u, 0xffffffffu};
    for (uint32_t init : seeds) {
        g_game_state.g_pow2_1_448790 = static_cast<int32_t>(init);
        uint32_t oracle_seed = init;
        for (int i = 0; i < 200; ++i) {
            const uint32_t got = rand_impl();
            const uint32_t exp = lcg_step(oracle_seed);
            EXPECT_EQ(got, exp) << "seed=0x" << std::hex << init
                                << " step=" << std::dec << i;
        }
    }
}

// ---------------------------------------------------------------------------
// 候选 4 ④b：FUN_0043de50 = 标准 memmove（重叠安全内存搬移）。
// replica 转录为 memmove_impl（replica_stubs.c:28751，委托标准 memmove()），
// 由 FUN_00412030.c:51 用于把 HP 伤害缓冲(0x7d0488) 播种进士气缓冲(0x4565d0)（0x208B）。
// 证据 output/ghidra_all/FUN_0043de50.c（664B RTL：前向/后向拷贝 + 4 字节对齐快路径 + 字节尾）。
// 本测试钉死其字节级语义含重叠反向拷贝（区分 memmove 与 memcpy 的关键）。
// ---------------------------------------------------------------------------
TEST(ReplicaStubs, MemmoveImplByteExact) {
    // 重叠正向拷贝：dst 落在 src 区间内（dst = src+20，区间重叠）→ 必须正确。
    {
        std::vector<uint8_t> buf(256);
        for (size_t i = 0; i < buf.size(); ++i) buf[i] = static_cast<uint8_t>(0x20 + i);
        std::vector<uint8_t> ref = buf;
        std::memmove(ref.data() + 20, ref.data() + 10, 236);
        memmove_impl(buf.data() + 20, buf.data() + 10, 236);
        EXPECT_EQ(buf, ref) << "重叠正向拷贝(dst>src)应逐字节==标准 memmove";
    }
    // 不重叠拷贝
    {
        std::vector<uint8_t> src(256), dst(256, 0);
        for (size_t i = 0; i < src.size(); ++i) src[i] = static_cast<uint8_t>(0x10 + i);
        std::vector<uint8_t> ref(256, 0);
        std::memmove(ref.data(), src.data(), 256);
        memmove_impl(dst.data(), src.data(), 256);
        EXPECT_EQ(dst, ref) << "不重叠拷贝应逐字节==标准 memmove";
    }
    // 部分拷贝（17B）
    {
        std::vector<uint8_t> src(64), dst(64, 0);
        for (size_t i = 0; i < src.size(); ++i) src[i] = static_cast<uint8_t>(0x30 + i);
        std::vector<uint8_t> ref(64, 0);
        std::memmove(ref.data(), src.data(), 17);
        memmove_impl(dst.data(), src.data(), 17);
        EXPECT_EQ(dst, ref) << "部分拷贝(17B)应==标准 memmove";
    }
    // 反向重叠：src 落在 dst 区间内（src = dst+10，区间重叠）→ 必须正确反向拷贝。
    {
        std::vector<uint8_t> buf(256);
        for (size_t i = 0; i < buf.size(); ++i) buf[i] = static_cast<uint8_t>(0x40 + i);
        std::vector<uint8_t> ref = buf;
        std::memmove(ref.data() + 10, ref.data() + 20, 236);  // src=dst+20 落在 dst 区间
        memmove_impl(buf.data() + 10, buf.data() + 20, 236);
        EXPECT_EQ(buf, ref) << "重叠反向拷贝(src>dst)应逐字节==标准 memmove";
    }
}

// 具体锚点：seed=0 首步 = (0x269ec3 >> 16) & 0x7fff = 0x26 = 38（手算可核）。
TEST(ReplicaStubs, RandImplSeedZeroAnchor) {
    g_game_state.g_pow2_1_448790 = 0;
    EXPECT_EQ(rand_impl(), 38u);
    // 继续两步，确保状态推进符合 LCG
    uint32_t s = 0u;
    lcg_step(s);            // 推进到 step1 对应的 oracle 种子
    EXPECT_EQ(rand_impl(), lcg_step(s));
}

// 确定性：同种子 → 同序列（与 battle 命中抽签/事件随机的可复现性前提一致）。
TEST(ReplicaStubs, RandImplDeterministic) {
    g_game_state.g_pow2_1_448790 = 12345;
    uint32_t s = 12345u;
    for (int i = 0; i < 50; ++i) {
        EXPECT_EQ(rand_impl(), lcg_step(s));
    }
}

// ---------------------------------------------------------------------------
// 方向② 规模化示范：未解析参数函数经 ABI 还原后做 oracle 差分。
// check_span_room (RF:00425d1f) 原 Ghidra 未解析参数（param_N 局部），
// 由调用方 FUN_00425d7c.c:27/38/49 还原为 (byte,byte,uint) 位置参数并落地签名。
// 本测试 = 转写 + ABI 还原后的行为 oracle（独立转写同一文档化逻辑，交叉验证）。
// ---------------------------------------------------------------------------
namespace {
// 独立参考：文档化逻辑（FUN_00425d1f.c 实证，1:1）。
uint32_t check_span_room_ref(uint8_t a, uint8_t b, uint32_t c) {
    if (b < a) {
        return (c < static_cast<uint32_t>(a) - static_cast<uint32_t>(b)) ? 1u : 0u;
    }
    return (c < static_cast<uint32_t>(b) - static_cast<uint32_t>(a)) ? 0xffffffffu : 0u;
}
}  // namespace

TEST(ReplicaStubs, CheckSpanRoomMatchesRef) {
    // 覆盖三类分支：b<a / b>=a，及 c 在差值的边界上下。
    for (uint8_t a = 0; a < 32; a += 3) {
        for (uint8_t b = 0; b < 32; b += 5) {
            for (uint32_t c : {0u, 1u, 7u, 15u, 31u, 63u, 127u, 255u}) {
                EXPECT_EQ(check_span_room(a, b, c), check_span_room_ref(a, b, c))
                    << "a=" << (int)a << " b=" << (int)b << " c=" << c;
            }
        }
    }
}

// 锚点：已知若干具体三元组的返回值（b<a 且 c<差 → 1；b>=a 且 c>=差 → 0；b>=a 且 c<差 → -1）。
TEST(ReplicaStubs, CheckSpanRoomAnchors) {
    EXPECT_EQ(check_span_room(10, 4, 3), 1u);    // b<a, c(3)<6 → 1
    EXPECT_EQ(check_span_room(10, 4, 6), 0u);    // b<a, c(6)>=6 → 0
    EXPECT_EQ(check_span_room(4, 10, 3), 0xffffffffu);  // b>=a, c(3)<6 → -1
    EXPECT_EQ(check_span_room(4, 10, 6), 0u);    // b>=a, c(6)>=6 → 0
    EXPECT_EQ(check_span_room(7, 7, 0), 0u);     // b>=a 且差=0, c(0)<0 永假 → 0
}

// ---------------------------------------------------------------------------
// 规模化示范 (续)：delta_direction_code (RF:00427a0a) 与 copy_int_block (RF:004220a1)
// 经 ABI 还原 (调用方 FUN_004043ea / FUN_00427a71 等实参证据) 后做 oracle 差分。
// ---------------------------------------------------------------------------
namespace {
// 独立参考：delta_direction_code 文档化逻辑（FUN_00427a0a.c:538-559 实证，1:1）。
uint32_t delta_direction_code_ref(int32_t x1, int32_t y1, int32_t x2, int32_t y2) {
    if (x1 == x2 && y1 == y2) return 0xffffffffu;
    int32_t dx = (x2 > x1) ? (x2 - x1) : (x1 - x2);  // abs_int_de40
    int32_t dy = (y2 > y1) ? (y2 - y1) : (y1 - y2);
    if (dy < dx) {
        return (x1 < x2) ? 3u : 2u;
    } else if (y1 < y2) {
        return 1u;
    }
    return 0u;
}
}  // namespace

// 穷举小坐标网格：方向码必须 == 独立参考。
TEST(ReplicaStubs, DeltaDirectionMatchesRef) {
    for (int x1 = -6; x1 <= 6; ++x1) {
        for (int y1 = -6; y1 <= 6; ++y1) {
            for (int x2 = -6; x2 <= 6; ++x2) {
                for (int y2 = -6; y2 <= 6; ++y2) {
                    EXPECT_EQ(delta_direction_code(x1, y1, x2, y2),
                              delta_direction_code_ref(x1, y1, x2, y2))
                        << "x1=" << x1 << " y1=" << y1 << " x2=" << x2 << " y2=" << y2;
                }
            }
        }
    }
}

// 锚点：四类方向 + 相等。
TEST(ReplicaStubs, DeltaDirectionAnchors) {
    EXPECT_EQ(delta_direction_code(0, 0, 0, 0), 0xffffffffu);  // 相等
    EXPECT_EQ(delta_direction_code(0, 0, 0, 5), 1u);           // 向下 (y+)
    EXPECT_EQ(delta_direction_code(0, 0, 0, -5), 0u);          // 向上 (y-)
    EXPECT_EQ(delta_direction_code(0, 0, 5, 1), 3u);           // 右 (x+，且|dx|>|dy|)
    EXPECT_EQ(delta_direction_code(0, 0, -5, 1), 2u);          // 左 (x-，且|dx|>|dy|)
    EXPECT_EQ(delta_direction_code(0, 0, 3, 3), 1u);           // |dx|==|dy| 归到 y 主导 → 下
}

// copy_int_block (RF:004220a1)：纯内存搬移 n 个 int，无全局状态。
// 独立参考：逐 int 从 src 拷到 dst。
TEST(ReplicaStubs, CopyIntBlockMatchesRef) {
    const int kN = 17;
    std::vector<int32_t> src(kN), dst(kN, -1);
    for (int i = 0; i < kN; ++i) src[i] = (i * 1103515245 + 12345) & 0xfff;
    copy_int_block(dst.data(), src.data(), kN);
    for (int i = 0; i < kN; ++i) {
        EXPECT_EQ(dst[i], src[i]) << "i=" << i;
    }
}

// 锚点：部分搬移 + 重叠区不踩（纯前向拷贝，源在左不重叠）。
TEST(ReplicaStubs, CopyIntBlockPartial) {
    std::vector<int32_t> a(8), b(8, 0);
    for (int i = 0; i < 8; ++i) a[i] = i + 1;
    copy_int_block(b.data(), a.data(), 4);  // 仅拷前 4
    for (int i = 0; i < 4; ++i) EXPECT_EQ(b[i], a[i]);
    for (int i = 4; i < 8; ++i) EXPECT_EQ(b[i], 0);
}

// ---------------------------------------------------------------------------
// 规模化示范 (续二)：remap_bytes_lut (RF:00421e97) 与 nearest_color_match_21d10
// (RF:00421d10) 经 ABI 还原后做 oracle 差分。两者皆为纯缓冲操作（无 g_game_state）。
//   remap_bytes_lut : dst[i] = lut[src[i]]  (FUN_00421e97.c 实证, 1:1)
//   nearest_color_match : 扫 0x100 调色板项(每项4B: R/G/B) 取加权重(0x1e/0x3b/0xb)距离最小项
//                         (FUN_00421d10.c 实证)。B 项在反编译中被混淆为含 param_2 低位字节，
//                         故用"全部 B=0 且目标蓝=0"把 B 项做成对所有项恒定的常数，仅由 R/G 决定排序。
// ---------------------------------------------------------------------------
#include <cstdint>
#include <cstring>

// remap_bytes_lut：纯缓冲查表，无全局状态。独立参考 = dst[i]=lut[src[i]]。
TEST(ReplicaStubs, RemapBytesLutMatchesRef) {
    const uint32_t kN = 64;
    uint8_t lut[256];
    for (int i = 0; i < 256; ++i) lut[i] = static_cast<uint8_t>((i * 31 + 7) & 0xff);
    std::vector<uint8_t> src(kN), dst(kN, 0);
    for (uint32_t i = 0; i < kN; ++i) src[i] = static_cast<uint8_t>((i * 17 + 3) & 0xff);
    const int32_t ret = remap_bytes_lut(src.data(), dst.data(), kN, lut);
    EXPECT_EQ(ret, 0);  // Ghidra 原函数返回 void，转写为 0
    for (uint32_t i = 0; i < kN; ++i) {
        EXPECT_EQ(dst[i], lut[src[i]]) << "i=" << i;
    }
}

// 锚点：恒等表 (lut[i]=i) → dst 应逐字节等于 src；乱序表锚点。
TEST(ReplicaStubs, RemapBytesLutIdentityAnchor) {
    uint8_t lut[256];
    for (int i = 0; i < 256; ++i) lut[i] = static_cast<uint8_t>(i);
    uint8_t src[8] = {0, 1, 200, 255, 10, 128, 77, 42};
    uint8_t dst[8] = {0};
    remap_bytes_lut(src, dst, 8, lut);
    for (int i = 0; i < 8; ++i) EXPECT_EQ(dst[i], src[i]) << "i=" << i;
}

// 锚点：长度为 0 与单字节边界。
TEST(ReplicaStubs, RemapBytesLutLengthEdges) {
    uint8_t lut[256];
    for (int i = 0; i < 256; ++i) lut[i] = static_cast<uint8_t>((i * 13) & 0xff);
    uint8_t src[4] = {3, 9, 1, 4}, dst0[4] = {9, 9, 9, 9}, dst1[4] = {9};
    remap_bytes_lut(src, dst0, 0, lut);  // 长度 0：不写
    EXPECT_EQ(dst0[0], 9);
    remap_bytes_lut(src, dst1, 1, lut);  // 单字节
    EXPECT_EQ(dst1[0], lut[src[0]]);
}

// nearest_color_match：构造"唯一精确 R/G 命中项"，绕开混淆的 B 项 (全部 B=0, 目标蓝=0)。
TEST(ReplicaStubs, NearestColorMatchExactHit) {
    uint8_t palette[0x100 * 4];
    std::memset(palette, 0, sizeof(palette));
    // 非命中项置为最远色 (R=G=255, B=0)
    for (int i = 0; i < 0x100; ++i) {
        palette[i * 4 + 0] = 255;
        palette[i * 4 + 1] = 255;
        palette[i * 4 + 2] = 0;
    }
    // 若干命中项 k：R/G 与目标完全一致，B=0 → 距离最小(仅含恒定积分 B 项)
    for (int k : {0, 1, 5, 42, 127, 200, 255}) {
        const uint8_t tr = static_cast<uint8_t>(k * 3 + 1);
        const uint8_t tg = static_cast<uint8_t>(k * 7 + 2);
        palette[k * 4 + 0] = tr;
        palette[k * 4 + 1] = tg;
        palette[k * 4 + 2] = 0;
        const uint32_t target = static_cast<uint32_t>(tr) | (static_cast<uint32_t>(tg) << 8);
        EXPECT_EQ(nearest_color_match_21d10(target, palette), k)
            << "k=" << k << " tr=" << (int)tr << " tg=" << (int)tg;
    }
}

// 锚点：最近项胜过较远项（R/G 距离主导，B 项恒定不影响排序）。
TEST(ReplicaStubs, NearestColorMatchCloserWins) {
    uint8_t palette[0x100 * 4];
    std::memset(palette, 0, sizeof(palette));
    for (int i = 0; i < 0x100; ++i) {
        palette[i * 4 + 0] = 255;  // 远
        palette[i * 4 + 1] = 255;
        palette[i * 4 + 2] = 0;
    }
    // 目标 R=100,G=100；近项 k=10 设 R=102,G=102，远项 k=20 设 R=200,G=200
    palette[10 * 4 + 0] = 102; palette[10 * 4 + 1] = 102; palette[10 * 4 + 2] = 0;
    palette[20 * 4 + 0] = 200; palette[20 * 4 + 1] = 200; palette[20 * 4 + 2] = 0;
    const uint32_t target = (100u) | (100u << 8);
    EXPECT_EQ(nearest_color_match_21d10(target, palette), 10);
    // 反转：让 k=20 更近
    palette[10 * 4 + 0] = 200; palette[10 * 4 + 1] = 200;
    palette[20 * 4 + 0] = 102; palette[20 * 4 + 1] = 102;
    EXPECT_EQ(nearest_color_match_21d10(target, palette), 20);
}



// ===== 本批还原函数的独立 oracle (证据: output/ghidra_all/FUN_0042254f.c / _00426810.c / _00418551.c) =====

// 交换两 int (FUN_0042254f.c: 直接 *p1<->*p2)。
TEST(ReplicaStubs, SwapIntsBasic) {
    int32_t a = 5, b = 9;
    swap_ints(&a, &b);
    EXPECT_EQ(a, 9);
    EXPECT_EQ(b, 5);
}
TEST(ReplicaStubs, SwapIntsSameValue) {
    int32_t a = 42, b = 42;
    swap_ints(&a, &b);
    EXPECT_EQ(a, 42);
    EXPECT_EQ(b, 42);
}

// 点 (p1,p2) 是否落在矩形 (p3,p4,p5,p6=x,y,w,h) 内：左上含、右下不含。
TEST(ReplicaStubs, RectOverlapInside) {
    EXPECT_EQ(rect_overlap(5, 5, 0, 0, 10, 10), 1);
    EXPECT_EQ(rect_overlap(0, 0, 0, 0, 10, 10), 1);  // 左上角含
}
TEST(ReplicaStubs, RectOverlapOutside) {
    EXPECT_EQ(rect_overlap(15, 5, 0, 0, 10, 10), 0);  // 右外
    EXPECT_EQ(rect_overlap(-1, 5, 0, 0, 10, 10), 0);   // 左外
    EXPECT_EQ(rect_overlap(5, 15, 0, 0, 10, 10), 0);   // 下外
    EXPECT_EQ(rect_overlap(5, -1, 0, 0, 10, 10), 0);   // 上外
    EXPECT_EQ(rect_overlap(10, 5, 0, 0, 10, 10), 0);   // 右边界不含 (x+w=10)
    EXPECT_EQ(rect_overlap(5, 10, 0, 0, 10, 10), 0);   // 下边界不含 (y+h=10)
}

// RLE 字节展开 (FUN_00418551.c)：token>=1 → 复制该字节 token 次；token 为负(>=0x80) → 从后继字节原样拷 -token 字节。
TEST(ReplicaStubs, RleByteExpandReplicate) {
    std::vector<uint8_t> src = {1, 3, 0xAB};  // count=1, token=3, byte=0xAB
    std::vector<uint8_t> dst(8, 0);
    int32_t ret = rle_byte_expand(src.data(), dst.data(), (uint32_t)src.size());
    EXPECT_EQ(ret, 1);
    EXPECT_EQ(dst[0], 0xAB);
    EXPECT_EQ(dst[1], 0xAB);
    EXPECT_EQ(dst[2], 0xAB);
    EXPECT_EQ(dst[3], 0);  // 未写区域不变
}
TEST(ReplicaStubs, RleByteExpandLiteral) {
    std::vector<uint8_t> src = {1, (uint8_t)0xFD, 0x11, 0x22, 0x33};  // count=1, token=0xFD(-3) → 拷贝3字节
    std::vector<uint8_t> dst(8, 0);
    int32_t ret = rle_byte_expand(src.data(), dst.data(), (uint32_t)src.size());
    EXPECT_EQ(ret, 1);
    EXPECT_EQ(dst[0], 0x11);
    EXPECT_EQ(dst[1], 0x22);
    EXPECT_EQ(dst[2], 0x33);
    EXPECT_EQ(dst[3], 0);
}

// 临时隔离测试：确认 memmove_impl 本身在 64 位下是否工作。
TEST(ReplicaStubs, MemmoveImplBasic) {
    uint8_t dst[4] = {0};
    uint8_t src[4] = {1, 2, 3, 4};
    memmove_impl(dst, src, 3);
    EXPECT_EQ(dst[0], 1);
    EXPECT_EQ(dst[1], 2);
    EXPECT_EQ(dst[2], 3);
}

// ---------------------------------------------------------------------------
// 规模化示范 (续三)：copy_skip_char (RF:0042207c) 与 palette_find_nearest_ex
// (RF:00421ddb) 经 ABI 落地后做 oracle 差分。两者皆纯函数（无 g_game_state）。
//   copy_skip_char : 将 src 拷到 dst 共 n 字节；遇 stop 字符(==param_4)跳过不写
//     （dst 保留原值），指针与计数各自推进（证据 FUN_0042207c.c:7-18, 1:1）。
//   palette_find_nearest_ex : 扫 0x100 调色板(每项4B)取 RGB 平方距离；
//     d>=阈值 时继续维护局部最优 local_1c；d<阈值 时返回当前索引 local_18
//     （首个低于阈值项）；全程 d>=阈值 则扫完返回全局最近 local_1c
//     （证据 FUN_00421ddb.c:17-38）。
//     反编译 B 项被混淆为调色板指针低字节(param_2&0xff)；本测试将调色板对齐
//      256B(低字节=0)且全 B=0，使混淆 B 项恒为 0，等价真实蓝分量(目标蓝=0)
//      语义，故锚点可纯按 R/G 距离手算。
// ---------------------------------------------------------------------------

// 独立参考：copy_skip_char 文档化逻辑（FUN_0042207c.c:7-18, 1:1）。
namespace {
void copy_skip_char_ref(char* dst, const char* src, int32_t n, char stop) {
    for (int32_t i = 0; i < n; ++i) {
        if (src[i] != stop) dst[i] = src[i];  // 遇 stop 字符跳过（dst 保留原值）
    }
}
}  // namespace

TEST(ReplicaStubs, CopySkipCharBasic) {
    char src[] = "HELLO";
    char dst[6]; std::memset(dst, 'X', 5); dst[5] = '\0';
    const int32_t ret = copy_skip_char(dst, src, 5, 'L');
    EXPECT_EQ(ret, 0);  // 桩恒返回 0
    // 位置 2,3 是 'L' → 跳过；其余拷入 → "HEXXO"
    EXPECT_STREQ(dst, "HEXXO");
    char expect[6]; std::memset(expect, 'X', 5); expect[5] = '\0';
    copy_skip_char_ref(expect, src, 5, 'L');
    EXPECT_EQ(std::memcmp(dst, expect, 5), 0);
}

TEST(ReplicaStubs, CopySkipCharStopAtStart) {
    char src[] = "Xabc";
    char dst[5]; std::memset(dst, 'Y', 4); dst[4] = '\0';
    copy_skip_char(dst, src, 4, 'X');
    EXPECT_STREQ(dst, "Yabc");  // src[0]=='X' 跳过(dst[0]保留'Y')，其余 a/b/c 拷入
}

TEST(ReplicaStubs, CopySkipCharNoStop) {
    char src[] = "abc";
    char dst[4]; std::memset(dst, 0, 4);
    copy_skip_char(dst, src, 3, '#');  // stop 不在 src → 全拷
    EXPECT_STREQ(dst, "abc");
}

// 注：FUN_0042207c.c:10-17 转录为 do { … } while(param_3!=0) 后测循环，故 n 必须 >0；
// n==0 会先执行一次再递减为负数永不满足退出条件（原二进制调用方不传 0），属契约外行为，
// 不对其断言。以下仅覆盖 n>0 的语义。
TEST(ReplicaStubs, CopySkipCharPostTestLoopContract) {
    // n=1 且 stop 命中：仅首字节跳过；do-while 保证至少执行一次，随后 param_3=-1≠0 退出。
    char src[] = "Z";
    char dst[2]; std::memset(dst, 'W', 1); dst[1] = '\0';
    copy_skip_char(dst, src, 1, 'Z');  // src[0]=='Z'==stop → 跳过，dst[0]保留 'W'
    EXPECT_STREQ(dst, "W");
}

TEST(ReplicaStubs, CopySkipCharAllStop) {
    char src[] = "aaa";
    char dst[4]; std::memset(dst, 'Z', 3); dst[3] = '\0';
    copy_skip_char(dst, src, 3, 'a');  // 全 stop → 全部跳过
    EXPECT_STREQ(dst, "ZZZ");
}

// ---------------------------------------------------------------------------
// palette_find_nearest_ex 参考实现：与桩体 1:1 转录（含混淆 B 项），用于交叉验证。
// 控制流严格镜像 FUN_00421ddb.c：
//   while 外层先 iVar1=local_1c；若 local_18>=0x100 退出返回 local_1c；
//   否则算 d，iVar1=local_18；若 d<阈值(threshold<=iVar2 为假)退出返回 local_18；
//   否则(继续)在 iVar2<local_14 时更新局部最优，p+=4, local_18++。
// ---------------------------------------------------------------------------
namespace {
int32_t palette_find_nearest_ex_ref(uint32_t target, int32_t pbase_int, int32_t threshold) {
    const uint8_t* p = reinterpret_cast<const uint8_t*>(
        static_cast<uintptr_t>(static_cast<uint32_t>(pbase_int)));
    int32_t local_14 = 20000;
    int32_t local_18 = 0;
    int32_t local_1c = 0;
    int32_t iVar1, iVar2, iVar3;
    while (true) {
        iVar1 = local_1c;                                  // 外层先取最优
        if (!(local_18 < 0x100)) break;                    // 扫完 → 返回 local_1c
        iVar1 = static_cast<int32_t>(*p) - static_cast<int32_t>(target & 0xff);            // Rdiff
        iVar2 = static_cast<int32_t>(*(p + 1)) - static_cast<int32_t>((target >> 8) & 0xff); // Gdiff
        iVar3 = static_cast<int32_t>(*(p + 2)) - static_cast<int32_t>((target >> 16) & 0xff);    // Bdiff(蓝分量，已订正)
        iVar2 = iVar1 * iVar1 + iVar2 * iVar2 + iVar3 * iVar3;                             // d
        iVar1 = local_18;
        if (!(threshold <= iVar2)) break;                  // d<阈值 → 返回当前索引 local_18
        if (iVar2 < local_14) { local_1c = local_18; local_14 = iVar2; }
        p = p + 4;
        local_18 = local_18 + 1;
    }
    return iVar1;
}

// 全蓝=0 的"远色"初始化：R=G=255, B=0（d 远大于测试阈值）。
void fill_palette_all_blue_zero(uint8_t* pal) {
    std::memset(pal, 0, 0x100 * 4);
    for (int i = 0; i < 0x100; ++i) { pal[i * 4 + 0] = 255; pal[i * 4 + 1] = 255; pal[i * 4 + 2] = 0; }
}

// 原二进制为 32 位，palette 指针以 int32_t 传递；64 位测试下须分配在 4GB 以下，
// 使 32 位截断无损。本机 VirtualAlloc 的 MEM_32BIT(0x40) 返回 err=87（不支持），
// 故改为在 <4GB 指定低基址预约（64KB 对齐的低地址通常空闲）；页对齐低字节必为 0。
uint8_t* alloc_palette_low32() {
    // 先试 MEM_32BIT（兼容其它平台）。
    uint8_t* p = static_cast<uint8_t*>(VirtualAlloc(nullptr, 0x100 * 4,
        MEM_COMMIT | MEM_RESERVE | 0x40 /*MEM_32BIT*/, PAGE_READWRITE));
    if (p && reinterpret_cast<uintptr_t>(p) < 0x100000000u) return p;
    if (p) VirtualFree(p, 0, MEM_RELEASE);
    // 退路：在 <4GB 内指定多个低基址尝试预约。
    static const uintptr_t kCandidates[] = {0x10000, 0x100000, 0x1000000,
                                            0x8000000, 0x40000000u, 0x70000000u};
    for (uintptr_t base : kCandidates) {
        p = static_cast<uint8_t*>(VirtualAlloc(reinterpret_cast<void*>(base), 0x100 * 4,
            MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
        if (p) {
            if (reinterpret_cast<uintptr_t>(p) < 0x100000000u) return p;
            VirtualFree(p, 0, MEM_RELEASE);
        }
    }
    return nullptr;
}
void free_palette_low32(uint8_t* p) {
    if (p) VirtualFree(p, 0, MEM_RELEASE);
}

// 通用低 32 位分配器：供以 uint32_t* 传 32-bit 指针语义的桩（strcpy_impl/strcat_impl 等）。
// 64 位测试下须落在 4GB 以下，使 32 位截断无损；页对齐低字节必为 0。
uint8_t* alloc_low32_bytes(size_t n) {
    uint8_t* p = static_cast<uint8_t*>(VirtualAlloc(nullptr, n,
        MEM_COMMIT | MEM_RESERVE | 0x40 /*MEM_32BIT*/, PAGE_READWRITE));
    if (p && reinterpret_cast<uintptr_t>(p) < 0x100000000u) return p;
    if (p) VirtualFree(p, 0, MEM_RELEASE);
    static const uintptr_t kCandidates[] = {0x10000, 0x100000, 0x1000000,
                                            0x8000000, 0x40000000u, 0x70000000u};
    for (uintptr_t base : kCandidates) {
        p = static_cast<uint8_t*>(VirtualAlloc(reinterpret_cast<void*>(base), n,
            MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
        if (p) {
            if (reinterpret_cast<uintptr_t>(p) < 0x100000000u) return p;
            VirtualFree(p, 0, MEM_RELEASE);
        }
    }
    return nullptr;
}
void free_low32(uint8_t* p) {
    if (p) VirtualFree(p, 0, MEM_RELEASE);
}
}  // namespace

// 锚点：首个低于阈值的精确命中项被返回（d=0<阈值）；多命中返回最靠前索引。
TEST(ReplicaStubs, PaletteFindNearestFirstBelowThreshold) {
    uint8_t* palette = alloc_palette_low32();
    ASSERT_NE(palette, nullptr);
    ASSERT_LT(reinterpret_cast<uintptr_t>(palette), 0x100000000u);  // 32 位截断无损
    const uint8_t tr = 10, tg = 20;
    const uint32_t target = static_cast<uint32_t>(tr) | (static_cast<uint32_t>(tg) << 8);
    const int32_t threshold = 10000;  // 远色项 d≈130050 >> 阈值 → 继续；命中项 d=0<阈值 → 返回
    for (int k : {0, 1, 5, 42, 127, 200, 255}) {
        fill_palette_all_blue_zero(palette);
        palette[k * 4 + 0] = tr; palette[k * 4 + 1] = tg;  // 精确命中：R/G 一致，B=0 → d=0
        const int32_t got = palette_find_nearest_ex(target, (int32_t)(uintptr_t)palette, threshold);
        const int32_t ref = palette_find_nearest_ex_ref(target, (int32_t)(uintptr_t)palette, threshold);
        EXPECT_EQ(got, k) << "k=" << k;
        EXPECT_EQ(got, ref) << "k=" << k;
    }
    // 首个低于阈值：k=3 与 k=7 同时精确命中，应返回更靠前的 3。
    fill_palette_all_blue_zero(palette);
    palette[3 * 4 + 0] = tr; palette[3 * 4 + 1] = tg;
    palette[7 * 4 + 0] = tr; palette[7 * 4 + 1] = tg;
    EXPECT_EQ(palette_find_nearest_ex(target, (int32_t)(uintptr_t)palette, threshold), 3);
    free_palette_low32(palette);
}

// 锚点：全程 d>=阈值 → 扫完返回全局最近（局部最优 local_1c）。
TEST(ReplicaStubs, PaletteFindNearestNoneBelowReturnsGlobalBest) {
    uint8_t* palette = alloc_palette_low32();
    ASSERT_NE(palette, nullptr);
    ASSERT_LT(reinterpret_cast<uintptr_t>(palette), 0x100000000u);
    fill_palette_all_blue_zero(palette);
    const uint32_t target = 0u;       // R=G=B=0
    const int32_t threshold = 1;      // 所有项 d>=1 → 扫完返回"cap 内最近"
    const int m = 50;
    // 仅 index m 距 < local_14(20000) 上限 → local_1c 更新为 m；其余 R=G=255 d=130050 不更新。
    palette[m * 4 + 0] = 10; palette[m * 4 + 1] = 10;  // d=10²*2=200 < 20000
    const int32_t got = palette_find_nearest_ex(target, (int32_t)(uintptr_t)palette, threshold);
    const int32_t ref = palette_find_nearest_ex_ref(target, (int32_t)(uintptr_t)palette, threshold);
    EXPECT_EQ(got, m) << "global best (within 20000 cap) index";
    EXPECT_EQ(got, ref);
    free_palette_low32(palette);
}

// 锚点：d==阈值 项不触发返回(threshold<=d 为真→继续)，首个 d<阈值 项(k+1)返回。
TEST(ReplicaStubs, PaletteFindNearestThresholdBoundary) {
    uint8_t* palette = alloc_palette_low32();
    ASSERT_NE(palette, nullptr);
    ASSERT_LT(reinterpret_cast<uintptr_t>(palette), 0x100000000u);
    fill_palette_all_blue_zero(palette);
    const uint32_t target = 0u;
    const int32_t threshold = 10000;
    const int k = 10;
    // 同时设 R=G（否则 G 残留 255 使距离爆炸）。R=G=71→d=71²*2=10082(>=阈值,继续)；
    // R=G=70→d=70²*2=9800(<阈值,返回 k+1)。
    palette[k * 4 + 0] = 71; palette[k * 4 + 1] = 71;          // d=10082 == 越过阈值 → 继续
    palette[(k + 1) * 4 + 0] = 70; palette[(k + 1) * 4 + 1] = 70;  // d=9800 < 阈值 → 返回 k+1
    const int32_t got = palette_find_nearest_ex(target, (int32_t)(uintptr_t)palette, threshold);
    const int32_t ref = palette_find_nearest_ex_ref(target, (int32_t)(uintptr_t)palette, threshold);
    EXPECT_EQ(got, k + 1) << "d==阈值项不返回，首个 d<阈值项(k+1)返回";
    EXPECT_EQ(got, ref);
    free_palette_low32(palette);
}

// ---------------------------------------------------------------------------
// 候选 4 ④c：FUN_0043e190 = CRT 风格 strcpy（字对齐 + 0x7efefeff 双字零检测）。
// 逐字节拷 src→dst 直至含 '\0'（含 null 本身），返回 dest 起始。
// 二进制为 32 位，指针以 uint32_t* 传递；须低 32 位分配器（见 alloc_low32_bytes）。
// 证据 output/ghidra_all/FUN_0043e190.c。

namespace {
// 逐字节 oracle：拷 src→dst 至含 null（含），返回后 dst 不变。
size_t strcpy_oracle_span(const uint8_t* src) {
    size_t i = 0;
    while (src[i] != 0) ++i;
    return i + 1;  // 含 null 的长度
}
// strcat oracle：找到 dst 末尾 null，再追加 src（含其 null）。
size_t strcat_oracle_span(uint8_t* dst, const uint8_t* src) {
    size_t i = 0;
    while (dst[i] != 0) ++i;
    size_t j = 0;
    while (true) {
        uint8_t c = src[j];
        dst[i + j] = c;
        if (c == 0) break;
        ++j;
    }
    return i + j;  // 总长度（含末尾 null）
}
}  // namespace

TEST(ReplicaStubs, StrcpyImplBasic) {
    const char* s = "Hello,World!";
    const size_t slen = std::strlen(s);
    const size_t cap = 256;
    uint8_t* dst = alloc_low32_bytes(cap);
    uint8_t* src = alloc_low32_bytes(cap);
    ASSERT_NE(dst, nullptr);
    ASSERT_NE(src, nullptr);
    ASSERT_LT(reinterpret_cast<uintptr_t>(dst), 0x100000000u);
    ASSERT_LT(reinterpret_cast<uintptr_t>(src), 0x100000000u);
    std::memset(dst, 0xCD, cap);
    std::memset(src, 0xCD, cap);
    std::memcpy(src, s, slen + 1);  // 含 null
    uint32_t* ret = strcpy_impl(reinterpret_cast<uint32_t*>(dst),
                                reinterpret_cast<uint32_t*>(src));
    EXPECT_EQ(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(ret)),
              static_cast<uint32_t>(reinterpret_cast<uintptr_t>(dst)));
    const size_t span = slen + 1;  // 含 null
    for (size_t i = 0; i < span; ++i) EXPECT_EQ(dst[i], src[i]) << "i=" << i;
    for (size_t i = span; i < cap; ++i) EXPECT_EQ(dst[i], 0xCD) << "after i=" << i;
    free_low32(dst); free_low32(src);
}

TEST(ReplicaStubs, StrcpyImplEmpty) {
    const size_t cap = 64;
    uint8_t* dst = alloc_low32_bytes(cap);
    uint8_t* src = alloc_low32_bytes(cap);
    ASSERT_NE(dst, nullptr); ASSERT_NE(src, nullptr);
    ASSERT_LT(reinterpret_cast<uintptr_t>(dst), 0x100000000u);
    ASSERT_LT(reinterpret_cast<uintptr_t>(src), 0x100000000u);
    std::memset(dst, 0xCD, cap);
    std::memset(src, 0xCD, cap);
    src[0] = 0;  // 空串
    uint32_t* ret = strcpy_impl(reinterpret_cast<uint32_t*>(dst),
                                reinterpret_cast<uint32_t*>(src));
    EXPECT_EQ(dst[0], 0);
    for (size_t i = 1; i < cap; ++i) EXPECT_EQ(dst[i], 0xCD) << "i=" << i;
    (void)ret;
    free_low32(dst); free_low32(src);
}

TEST(ReplicaStubs, StrcpyImplLongMultiWord) {
    const char* s = "This is a longer string that spans many words to exercise the "
                    "4-byte word copy loop repeatedly without hitting a null for a "
                    "while, testing the bulk path of the CRT strcpy implementation.";
    const size_t slen = std::strlen(s);
    const size_t cap = 512;
    uint8_t* dst = alloc_low32_bytes(cap);
    uint8_t* src = alloc_low32_bytes(cap);
    ASSERT_NE(dst, nullptr); ASSERT_NE(src, nullptr);
    ASSERT_LT(reinterpret_cast<uintptr_t>(dst), 0x100000000u);
    ASSERT_LT(reinterpret_cast<uintptr_t>(src), 0x100000000u);
    std::memset(dst, 0xCD, cap);
    std::memset(src, 0xCD, cap);
    std::memcpy(src, s, slen + 1);
    strcpy_impl(reinterpret_cast<uint32_t*>(dst), reinterpret_cast<uint32_t*>(src));
    for (size_t i = 0; i < slen + 1; ++i) EXPECT_EQ(dst[i], src[i]) << "i=" << i;
    for (size_t i = slen + 1; i < cap; ++i) EXPECT_EQ(dst[i], 0xCD) << "after i=" << i;
    free_low32(dst); free_low32(src);
}

// 非对齐路径：dst/src 各偏移 0..3 字节，触发逐字节预对齐循环（dest 与 src 双向非对齐）。
TEST(ReplicaStubs, StrcpyImplUnalignedSrcDst) {
    const char* s = "UnalignedTestString";
    const size_t slen = std::strlen(s);
    const size_t cap = 256;
    uint8_t* buf = alloc_low32_bytes(cap);
    ASSERT_NE(buf, nullptr);
    ASSERT_LT(reinterpret_cast<uintptr_t>(buf), 0x100000000u);
    for (int off = 0; off < 4; ++off) {
        uint8_t* dst = buf + off;
        uint8_t* src = buf + 96 + off;  // 避开 dst 区
        std::memset(buf, 0xCD, cap);
        std::memcpy(src, s, slen + 1);
        uint32_t* ret = strcpy_impl(reinterpret_cast<uint32_t*>(dst),
                                    reinterpret_cast<uint32_t*>(src));
        EXPECT_EQ(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(ret)),
                  static_cast<uint32_t>(reinterpret_cast<uintptr_t>(dst))) << "off=" << off;
        for (size_t i = 0; i < slen + 1; ++i)
            EXPECT_EQ(dst[i], src[i]) << "off=" << off << " i=" << i;
    }
    free_low32(buf);
}

// ---------------------------------------------------------------------------
// 候选 4 ④d：FUN_0043e1a0 = CRT 风格 strcat（先定位 dst 末尾 '\0'，再追加 src）。
// 证据 output/ghidra_all/FUN_0043e1a0.c。

TEST(ReplicaStubs, StrcatImplBasic) {
    const char* base = "Hello";
    const char* app = "World";
    const size_t cap = 256;
    uint8_t* dst = alloc_low32_bytes(cap);
    uint8_t* src = alloc_low32_bytes(cap);
    ASSERT_NE(dst, nullptr); ASSERT_NE(src, nullptr);
    ASSERT_LT(reinterpret_cast<uintptr_t>(dst), 0x100000000u);
    ASSERT_LT(reinterpret_cast<uintptr_t>(src), 0x100000000u);
    std::memset(dst, 0xCD, cap);
    std::memset(src, 0xCD, cap);
    std::memcpy(dst, base, std::strlen(base) + 1);
    std::memcpy(src, app, std::strlen(app) + 1);
    uint32_t* ret = strcat_impl(reinterpret_cast<uint32_t*>(dst),
                                reinterpret_cast<uint32_t*>(src));
    EXPECT_EQ(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(ret)),
              static_cast<uint32_t>(reinterpret_cast<uintptr_t>(dst)));
    std::string exp = std::string(base) + std::string(app);
    for (size_t i = 0; i < exp.size(); ++i) EXPECT_EQ((char)dst[i], exp[i]) << "i=" << i;
    EXPECT_EQ(dst[exp.size()], 0);
    for (size_t i = exp.size() + 1; i < cap; ++i) EXPECT_EQ(dst[i], 0xCD) << "after i=" << i;
    free_low32(dst); free_low32(src);
}

TEST(ReplicaStubs, StrcatImplEmptyDest) {
    const char* app = "xy";
    const size_t cap = 128;
    uint8_t* dst = alloc_low32_bytes(cap);
    uint8_t* src = alloc_low32_bytes(cap);
    ASSERT_NE(dst, nullptr); ASSERT_NE(src, nullptr);
    ASSERT_LT(reinterpret_cast<uintptr_t>(dst), 0x100000000u);
    ASSERT_LT(reinterpret_cast<uintptr_t>(src), 0x100000000u);
    std::memset(dst, 0xCD, cap);
    std::memset(src, 0xCD, cap);
    dst[0] = 0;  // 空 dest
    std::memcpy(src, app, std::strlen(app) + 1);
    strcat_impl(reinterpret_cast<uint32_t*>(dst), reinterpret_cast<uint32_t*>(src));
    EXPECT_EQ((char)dst[0], 'x');
    EXPECT_EQ((char)dst[1], 'y');
    EXPECT_EQ(dst[2], 0);
    for (size_t i = 3; i < cap; ++i) EXPECT_EQ(dst[i], 0xCD) << "i=" << i;
    free_low32(dst); free_low32(src);
}

TEST(ReplicaStubs, StrcatImplUnaligned) {
    const char* base = "AB";
    const char* app = "CDE";
    const size_t cap = 256;
    uint8_t* buf = alloc_low32_bytes(cap);
    ASSERT_NE(buf, nullptr);
    ASSERT_LT(reinterpret_cast<uintptr_t>(buf), 0x100000000u);
    for (int off = 0; off < 4; ++off) {
        std::memset(buf, 0xCD, cap);
        uint8_t* dst = buf + off;
        uint8_t* src = buf + 96 + off;
        std::memcpy(dst, base, std::strlen(base) + 1);
        std::memcpy(src, app, std::strlen(app) + 1);
        strcat_impl(reinterpret_cast<uint32_t*>(dst), reinterpret_cast<uint32_t*>(src));
        std::string exp = std::string(base) + std::string(app);
        for (size_t i = 0; i < exp.size(); ++i)
            EXPECT_EQ((char)dst[i], exp[i]) << "off=" << off << " i=" << i;
        EXPECT_EQ(dst[exp.size()], 0) << "off=" << off;
    }
    free_low32(buf);
}

// ---------------------------------------------------------------------------
// 候选 4 ④e：FUN_0043de40 = 整数绝对值 abs（众多桩的底层依赖，如 surface_span 的
// abs_int_de40(param_4-param_3)）。纯标量，签名安全（int32 参数，无指针/全局）。
// 证据 output/ghidra_all/FUN_0043de40.c（if(x<0) x=-x; return x;）。
// 注：INT_MIN 在二进制两补码下 -INT_MIN 回绕为 INT_MIN，与无符号等价算法一致。

namespace {
// 无 UB 的两补码绝对值（与二进制行为一致）。
int32_t abs_oracle(int32_t x) {
    uint32_t u = static_cast<uint32_t>(x);
    if (x < 0) u = static_cast<uint32_t>(-static_cast<int64_t>(x));
    return static_cast<int32_t>(u);
}
}  // namespace

TEST(ReplicaStubs, AbsIntDe40Sweep) {
    const int32_t vals[] = {0, 1, -1, 2, -2, 100, -100, 12345, -12345,
                            INT_MAX, INT_MIN, -INT_MAX};
    for (int32_t v : vals) {
        EXPECT_EQ(abs_int_de40(v), abs_oracle(v)) << "v=" << v;
    }
}

TEST(ReplicaStubs, AbsIntDe40Boundary) {
    EXPECT_EQ(abs_int_de40(0), 0);
    EXPECT_EQ(abs_int_de40(INT_MAX), INT_MAX);
    EXPECT_EQ(abs_int_de40(INT_MIN), INT_MIN);  // 两补码回绕
}

// ---------------------------------------------------------------------------
// 候选 4 ④f：FUN_00441f40 = 码号→字符集 switch 查表（纯查表，默认 0）。
// 映射：0x3a4→0x411, 0x3a8→0x804, 0x3b5→0x412, 0x3b6→0x404, 其余→0。
// 证据 output/ghidra_all/FUN_00441f40.c。

TEST(ReplicaStubs, CodepageToCharsetKnown) {
    EXPECT_EQ(codepage_to_charset(0x3a4), 0x411);
    EXPECT_EQ(codepage_to_charset(0x3a8), 0x804);
    EXPECT_EQ(codepage_to_charset(0x3b5), 0x412);
    EXPECT_EQ(codepage_to_charset(0x3b6), 0x404);
}

TEST(ReplicaStubs, CodepageToCharsetDefaultZero) {
    const int32_t unknowns[] = {0, 1, 0x3a0, 0x3a5, 0x3a9, 0x3b0, 0x3b7, 0xffff, -1};
    for (int32_t c : unknowns) {
        EXPECT_EQ(codepage_to_charset(c), 0) << "c=0x" << std::hex << c;
    }
}
