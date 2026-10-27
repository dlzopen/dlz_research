// test_stateful_stubs.cpp — 状态桩受控 g_game_state 行为锁测试
//
// 目的：为"无 I/O、无图形、无 RNG、确定性"的状态桩建立受控 g_game_state 夹具，
// 注入已知字段初值，调用 stub，断言观测后状态逐位等于 Ghidra 证据派生的期望值。
//
// 与 test_cross_layer_diff 的区别（诚实保留）：
//   test_cross_layer_diff 是 clean(手写) 层 ↔ oracle(二进制 1:1) 对拍；
//   本文件是 replica stub 的"行为锁"——期望后状态直接来自 Ghidra 反编译逐指令语义，
//   而非独立的 clean 复刻。真正的 clean-vs-oracle 跨层差分需要 per-function 的
//   clean 实现（见 backlog）。本测试锁定 replica 在受控输入下的可观测行为，
//   一旦 replica 被改坏（返回值/写字段偏移），立即变红。
//
// 受控夹具：每个用例前保存并清零 g_game_state，用例后还原，杜绝跨测试全局污染
// （既有测试直接改全局、无复位，是真实脆弱点；本夹具消除之）。

#include <gtest/gtest.h>
#include <cstring>

#include "replica/replica_stubs.h"   // ::GameState g_game_state、状态桩 extern "C"

namespace {

class ControlledGameState : public ::testing::Test {
protected:
    GameState saved_;
    void SetUp() override {
        saved_ = g_game_state;                       // 保存原全局
        std::memset(&g_game_state, 0, sizeof(GameState));  // 清零，确定性基线
    }
    void TearDown() override {
        g_game_state = saved_;                       // 还原，避免污染后续用例
    }
};

// ------------------------------------------------------------------
// 全局标量回写：set_global_4fdb4 / get_global_4fdb4 往返
// RE: 004055b5 / 004055c2。语义：set 写 g_rtstate_fdb4；get 读回。
// ------------------------------------------------------------------
TEST_F(ControlledGameState, Global4fdb4RoundTrip) {
    ASSERT_EQ(set_global_4fdb4(12345), 0);
    int32_t v = -1;
    ASSERT_EQ(get_global_4fdb4(&v), 0);
    ASSERT_EQ(v, 12345);
}

TEST_F(ControlledGameState, Global4fdb4DefaultZero) {
    int32_t v = 0xdead;
    ASSERT_EQ(get_global_4fdb4(&v), 0);   // 夹具清零后默认值应为 0
    ASSERT_EQ(v, 0);
}

// ------------------------------------------------------------------
// 文本色写入：set_text_colors 写三色全局各 &0xff
// RE: 00405560。语义：fg=g_datatbl_6460, shadow=g_rtstate_fdbc/fdb8，均掩 &0xff。
// 仅写标量字段，无数组越界，可安全行为锁。
// ------------------------------------------------------------------
TEST_F(ControlledGameState, SetTextColorsMasksLowByte) {
    ASSERT_EQ(set_text_colors(0x1234, 0xabcd, 0xeeff), 0);
    EXPECT_EQ(g_game_state.g_datatbl_6460, 0x34);
    EXPECT_EQ(g_game_state.g_rtstate_fdbc, 0xcd);
    EXPECT_EQ(g_game_state.g_rtstate_fdb8, 0xff);
}

TEST_F(ControlledGameState, SetTextColorsZeroArgs) {
    ASSERT_EQ(set_text_colors(0, 0, 0), 0);
    EXPECT_EQ(g_game_state.g_datatbl_6460, 0);
    EXPECT_EQ(g_game_state.g_rtstate_fdbc, 0);
    EXPECT_EQ(g_game_state.g_rtstate_fdb8, 0);
}

// ------------------------------------------------------------------
// 全局指针返回：get_global_ptr_4fdc0 返 &g_rtstate_fdc0（标量地址）
// RE: 00406c1d。仅取标量字段地址，无越界。
// ------------------------------------------------------------------
TEST_F(ControlledGameState, GetGlobalPtr4fdc0ReturnsFieldAddress) {
    EXPECT_EQ(get_global_ptr_4fdc0(), &g_game_state.g_rtstate_fdc0);
}

// ------------------------------------------------------------------
// 音频停止：audio_stop_if_playing 当 g_flag_c!=0 时写 g_ui_blk_d80c=0。
// RE: 0043c76b。mci_seek 在 replica 为安全 no-op；仅标量条件写可观测。
// （注：同族 mode37_init_e407 因内嵌 play_mpeg_clip/scene_render_dispatch
//  解引用 null 图形指针，孤立态 SEH 崩溃，已撤除——铁律零：不提交会崩的测试。）
// ------------------------------------------------------------------
TEST_F(ControlledGameState, AudioStopIfPlayingClearsFlagWhenPlaying) {
    g_game_state.g_flag_c = 1;
    g_game_state.g_ui_blk_d80c = 0xaa;
    ASSERT_EQ(audio_stop_if_playing(), 0);
    EXPECT_EQ(g_game_state.g_ui_blk_d80c, 0);
}

TEST_F(ControlledGameState, AudioStopIfPlayingNoOpWhenNotPlaying) {
    g_game_state.g_flag_c = 0;
    g_game_state.g_ui_blk_d80c = 0xaa;
    ASSERT_EQ(audio_stop_if_playing(), 0);
    EXPECT_EQ(g_game_state.g_ui_blk_d80c, 0xaa);  // 未播放：不动
}

// ------------------------------------------------------------------
// 三元状态读：get_state_triple
// RE: 00405590。语义：*p1=g_datatbl_6460; *p2=g_rtstate_fdbc; *p3=g_rtstate_fdb8。
// ------------------------------------------------------------------
TEST_F(ControlledGameState, StateTripleReadsThreeGlobals) {
    g_game_state.g_datatbl_6460 = 11;
    g_game_state.g_rtstate_fdbc = 22;
    g_game_state.g_rtstate_fdb8 = 33;
    int32_t a = 0, b = 0, c = 0;
    ASSERT_EQ(get_state_triple(&a, &b, &c), 0);
    ASSERT_EQ(a, 11);
    ASSERT_EQ(b, 22);
    ASSERT_EQ(c, 33);
}

// ------------------------------------------------------------------
// 矩形状态：set_rect_state / get_rect_state 往返 + 边界拒绝
// RE: 004055d2 / 0040564b。
// 边界：a∈(-1, g_datatbl_6430], c∈(-1, g_datatbl_6430+1],
//       b∈(-1, g_datatbl_6434], d∈(-1, g_datatbl_6434+1]。
// 越界则整组不写（保留旧值）。
// ------------------------------------------------------------------
TEST_F(ControlledGameState, RectStateRoundTripInBounds) {
    g_game_state.g_datatbl_6430 = 100;
    g_game_state.g_datatbl_6434 = 100;
    ASSERT_EQ(set_rect_state(5, 6, 7, 8), 0);
    int32_t a = 0, b = 0, c = 0, d = 0;
    ASSERT_EQ(get_rect_state(&a, &b, &c, &d), 0);
    ASSERT_EQ(a, 5);
    ASSERT_EQ(b, 6);
    ASSERT_EQ(c, 7);
    ASSERT_EQ(d, 8);
}

TEST_F(ControlledGameState, RectStateRejectsOutOfBounds) {
    g_game_state.g_datatbl_6430 = 100;
    g_game_state.g_datatbl_6434 = 100;
    // 先写入一个合法基准值
    ASSERT_EQ(set_rect_state(5, 6, 7, 8), 0);
    // 越界：a=200 > 100 → 整组不应被改写
    ASSERT_EQ(set_rect_state(200, 6, 7, 8), 0);
    int32_t a = 0, b = 0, c = 0, d = 0;
    ASSERT_EQ(get_rect_state(&a, &b, &c, &d), 0);
    ASSERT_EQ(a, 5);   // 保持旧值
    ASSERT_EQ(b, 6);
    ASSERT_EQ(c, 7);
    ASSERT_EQ(d, 8);
}

TEST_F(ControlledGameState, RectStateRejectsNegative) {
    g_game_state.g_datatbl_6430 = 100;
    g_game_state.g_datatbl_6434 = 100;
    ASSERT_EQ(set_rect_state(5, 6, 7, 8), 0);
    // a=-1 不满足 -1<a → 拒绝
    ASSERT_EQ(set_rect_state(-1, 6, 7, 8), 0);
    int32_t a = 0, b = 0, c = 0, d = 0;
    ASSERT_EQ(get_rect_state(&a, &b, &c, &d), 0);
    ASSERT_EQ(a, 5);   // 保持旧值
}

// ------------------------------------------------------------------
// 寻路网格缓冲：clear_grid_square / mark_occupancy_diamond
// RE: 004192fb / 004193ba。
// 关键前置修复（铁律零实证）：原 replica 把 g_pathfind_grid 建模为单 int32 标量，
// 而二进制里它是 W×H 的 int32 数组（基址 0x00833418）。旧模型下
//   memset(&g_game_state.g_pathfind_grid,0/1,W*H) 与 (&g_state.g_pathfind_grid)[idx]
// 会越界写入相邻 GameState 内存（约 50 个寻路函数共用此基址）。
// 本轮已将字段改为 int32_t g_pathfind_grid[MAP_GRID_CAP]，并把所有
// (&g_state.g_pathfind_grid) 基址引用退化为数组形式。以下测试锁定其裁剪语义。
// ------------------------------------------------------------------
namespace {
constexpr int32_t kGridW = 40;
constexpr int32_t kGridH = 30;
constexpr int32_t kSentinel = 7;

void FillGrid(int32_t v) {
    for (int32_t i = 0; i < kGridW * kGridH; ++i) g_game_state.g_pathfind_grid[i] = v;
}
}  // namespace

TEST_F(ControlledGameState, ClearGridSquareClearsBoundedSquare) {
    g_game_state.g_map_grid_width = kGridW;
    g_game_state.g_map_grid_height = kGridH;
    FillGrid(kSentinel);
    // 以 (20,15) 为中心、半径 3 → 清 7×7 方块 [17..23]×[12..18]
    ASSERT_EQ(clear_grid_square(20, 15, 3), 0);
    for (int32_t y = 12; y <= 18; ++y) {
        for (int32_t x = 17; x <= 23; ++x) {
            ASSERT_EQ(g_game_state.g_pathfind_grid[y * kGridW + x], 0)
                << "cell (" << x << "," << y << ") should be cleared";
        }
    }
    // 方块外保持哨兵值
    ASSERT_EQ(g_game_state.g_pathfind_grid[10 * kGridW + 10], kSentinel);
}

TEST_F(ControlledGameState, ClearGridSquareClipsToBounds) {
    g_game_state.g_map_grid_width = kGridW;
    g_game_state.g_map_grid_height = kGridH;
    FillGrid(kSentinel);
    // 中心 (1,1) 半径 5 → 名义方 [-4..6]×[-4..6]，裁剪到 [0..6]×[0..6]
    ASSERT_EQ(clear_grid_square(1, 1, 5), 0);
    for (int32_t y = 0; y <= 6; ++y) {
        for (int32_t x = 0; x <= 6; ++x) {
            ASSERT_EQ(g_game_state.g_pathfind_grid[y * kGridW + x], 0)
                << "cell (" << x << "," << y << ") should be cleared (clipped)";
        }
    }
    // 超出裁剪范围（7,7）保持哨兵
    ASSERT_EQ(g_game_state.g_pathfind_grid[7 * kGridW + 7], kSentinel);
}

TEST_F(ControlledGameState, MarkOccupancyDiamondMarksCenterAndDiamond) {
    g_game_state.g_map_grid_width = kGridW;
    g_game_state.g_map_grid_height = kGridH;
    FillGrid(kSentinel);
    // 中心 (20,15) 半径 3：曼哈顿距离<=3 的方块内单元清 0，中心再置 1
    ASSERT_EQ(mark_occupancy_diamond(20, 15, 3), 0);
    // 中心单元 = 1（最后一步赋值覆盖）
    ASSERT_EQ(g_game_state.g_pathfind_grid[15 * kGridW + 20], 1);
    // 曼哈顿距离 1 的邻格 ∈ 菱形 → 0
    ASSERT_EQ(g_game_state.g_pathfind_grid[15 * kGridW + 19], 0);
    ASSERT_EQ(g_game_state.g_pathfind_grid[14 * kGridW + 20], 0);
    // 方块内但菱形外（|dx|+|dy|=6 > 3）→ 保持哨兵
    ASSERT_EQ(g_game_state.g_pathfind_grid[12 * kGridW + 17], kSentinel);
    // 方块外 → 保持哨兵
    ASSERT_EQ(g_game_state.g_pathfind_grid[10 * kGridW + 10], kSentinel);
}

// ------------------------------------------------------------------
// 纯访问器：mru_queue4_head(GET g_ui_blk_d3f8) / store_global_448790(SET g_pow2_1_448790)
// RE: 004251bc / 0043e300。
// ------------------------------------------------------------------
TEST_F(ControlledGameState, MruQueue4HeadReadsField) {
    g_game_state.g_ui_blk_d3f8 = 0x1234;
    ASSERT_EQ(mru_queue4_head(), static_cast<int16_t>(0x1234));
}

TEST_F(ControlledGameState, StoreGlobal448790WritesField) {
    ASSERT_EQ(store_global_448790(0x55), 0);
    ASSERT_EQ(g_game_state.g_pow2_1_448790, 0x55);
}

// ------------------------------------------------------------------
// 纯确定性工具（无 I/O / 无全局写入 / 无 RNG）：行为锁来自 Ghidra 逐指令语义
// RE: 0042254f swap_ints / 00440490 ptr_read_u32 / 004404b0 ptr_read_u64
//     00426810 rect_overlap / 00441f40 codepage_to_charset
// ------------------------------------------------------------------
TEST_F(ControlledGameState, SwapIntsExchangesValues) {
    int32_t a = 1, b = 2;
    ASSERT_EQ(swap_ints(&a, &b), 0);
    ASSERT_EQ(a, 2);
    ASSERT_EQ(b, 1);
    // 幂等：再交换一次恢复
    swap_ints(&a, &b);
    ASSERT_EQ(a, 1);
    ASSERT_EQ(b, 2);
}

TEST_F(ControlledGameState, RectOverlapHalfOpenContainment) {
    // Ghidra FUN_00426810: 返回 1 当且仅当 !(x<rx || rx+rw<=x || y<ry || ry+rh<=y)
    // 即点 (x,y) 落在半开矩形 [rx,rx+rw) × [ry,ry+rh)
    ASSERT_EQ(rect_overlap(5, 5, 0, 0, 10, 10), 1);    // 内部
    ASSERT_EQ(rect_overlap(10, 5, 0, 0, 10, 10), 0);   // x==rx+rw 右开边界外
    ASSERT_EQ(rect_overlap(-1, 5, 0, 0, 10, 10), 0);   // x<rx 左外
    ASSERT_EQ(rect_overlap(5, 10, 0, 0, 10, 10), 0);   // y==ry+rh 下开边界外
    ASSERT_EQ(rect_overlap(5, -1, 0, 0, 10, 10), 0);   // y<ry 上外
}

TEST_F(ControlledGameState, CodepageToCharsetSwitch) {
    // Ghidra FUN_00441f40 switch: 0x3a4->0x411, 0x3a8->0x804, 0x3b5->0x412, default->0
    ASSERT_EQ(codepage_to_charset(0x3a4), 0x411);
    ASSERT_EQ(codepage_to_charset(0x3a8), 0x804);
    ASSERT_EQ(codepage_to_charset(0x3b5), 0x412);
    ASSERT_EQ(codepage_to_charset(0), 0);              // default
    ASSERT_EQ(codepage_to_charset(0x999), 0);          // default
}

// ------------------------------------------------------------------
// 纯标量工具：abs_int_de40
// RE: 0043de40。语义：if (param_1<0) param_1=-param_1; return param_1。
// ------------------------------------------------------------------
TEST_F(ControlledGameState, AbsIntDe40ReturnsAbsoluteValue) {
    ASSERT_EQ(abs_int_de40(7), 7);
    ASSERT_EQ(abs_int_de40(-7), 7);
    ASSERT_EQ(abs_int_de40(0), 0);
    ASSERT_EQ(abs_int_de40(-2147483647), 2147483647);
}

// ------------------------------------------------------------------
// 游标读取原语：ptr_read_u32 / ptr_read_u64 / ptr_read_u16（铁律零·64位修正验证）
// RE: 00440490 / 004404b0 / 004404d0。
// 关键：param_1 指向"游标"，游标本身是 64 位指针；64 位构建下必须以 intptr_t
// 全宽读写游标，否则 *param_1=(int32_t)(...) 会截断高 32 位导致游标错乱。
// 这三个测试同时锁定：(a) 读取的单元值正确；(b) 游标前进量正确（u32:+4/u64:+8/u16:+4）。
// ------------------------------------------------------------------
TEST_F(ControlledGameState, PtrReadU32AdvancesCursorBy4) {
    int32_t buf[4] = {0x11223344, 0x55667788, 0x99aabbcc, 0};
    intptr_t cursor = (intptr_t)&buf[0];
    int32_t v0 = ptr_read_u32(&cursor);
    ASSERT_EQ(v0, 0x11223344);
    ASSERT_EQ(cursor, (intptr_t)&buf[1]);           // 前进 1 个 int32 = 4 字节
    int32_t v1 = ptr_read_u32(&cursor);
    ASSERT_EQ(v1, 0x55667788);
    ASSERT_EQ(cursor, (intptr_t)&buf[2]);
}

TEST_F(ControlledGameState, PtrReadU64AdvancesCursorBy8) {
    uint64_t buf[3] = {0x1122334455667788ULL, 0x99aabbccddeeff00ULL, 0};
    intptr_t cursor = (intptr_t)&buf[0];
    uint64_t v0 = ptr_read_u64(&cursor);
    ASSERT_EQ(v0, 0x1122334455667788ULL);
    ASSERT_EQ(cursor, (intptr_t)&buf[1]);           // 前进 1 个 uint64 = 8 字节
}

TEST_F(ControlledGameState, PtrReadU16AdvancesCursorBy4) {
    // 证据 FUN_004404d0.c:16 —— CONCAT22(lo,hi) 宏 = (uint16_t)lo | ((uint16_t)hi<<16)。
    // 此处 lo=(int16_t)((uint32_t)puVar2>>0x10)（取指针 puVar2 的地址高16位，Ghidra 伪影，
    // 1:1 保真保留，值依赖地址、非确定），hi=*puVar1（读取的 u16 数据）。
    // 故：有效数据 *puVar1=0x5566 落在**高16位**；低16位为地址伪影、不锁。
    // 游标 puVar2=puVar1+2 即 uint16_t*+2 = 前进 4 字节（确定）。
    int32_t buf[2] = {0x33445566, 0x778899aa};
    intptr_t cursor = (intptr_t)&buf[0];
    int32_t v = ptr_read_u16(&cursor);
    ASSERT_EQ((int)((v >> 16) & 0xFFFF), (int)0x5566);  // 高16位=有效读取数据
    ASSERT_EQ(cursor, (intptr_t)&buf[0] + 4);            // uint16_t* + 2 = 前进 4 字节
}

// ------------------------------------------------------------------
// 阶段结束标志阈值族：set_endflag_over{20,30,40,50}（铁律零·确定性标量锁）
// RE: 0040f441 / 0040f429 / 0040f56c / 0040fc7e；Ghidra 实证 FUN_0040f429.c:10-11
//   `if (0x1e < DAT_00475aa4) { DAT_007d50f8 = 0xffffffff; }`。
// 语义：读 g_battle_frame，若 > 阈值(0x14/0x1e/0x28/0x32) 则写
//       g_battle_end_flag=0xffffffff，否则不动。边界(等于阈值)不触发，越过(+1)触发。返回恒 0。
// 受控夹具已保存/清零/还原 g_game_state，故直接置字段即可。
// ------------------------------------------------------------------
TEST_F(ControlledGameState, SetEndflagOver20Threshold) {
    g_game_state.g_battle_end_flag = 0;
    g_game_state.g_battle_frame = 0x14;            // 边界：等于阈值不触发
    ASSERT_EQ(set_endflag_over20(), 0);
    ASSERT_EQ((uint32_t)g_game_state.g_battle_end_flag, 0u);
    g_game_state.g_battle_frame = 0x15;            // 越过阈值触发
    ASSERT_EQ(set_endflag_over20(), 0);
    ASSERT_EQ((uint32_t)g_game_state.g_battle_end_flag, 0xffffffffu);
}

TEST_F(ControlledGameState, SetEndflagOver30Threshold) {
    g_game_state.g_battle_end_flag = 0;
    g_game_state.g_battle_frame = 0x1e;
    ASSERT_EQ(set_endflag_over30(), 0);
    ASSERT_EQ((uint32_t)g_game_state.g_battle_end_flag, 0u);
    g_game_state.g_battle_frame = 0x1f;
    ASSERT_EQ(set_endflag_over30(), 0);
    ASSERT_EQ((uint32_t)g_game_state.g_battle_end_flag, 0xffffffffu);
}

TEST_F(ControlledGameState, SetEndflagOver40Threshold) {
    g_game_state.g_battle_end_flag = 0;
    g_game_state.g_battle_frame = 0x28;
    ASSERT_EQ(set_endflag_over40(), 0);
    ASSERT_EQ((uint32_t)g_game_state.g_battle_end_flag, 0u);
    g_game_state.g_battle_frame = 0x29;
    ASSERT_EQ(set_endflag_over40(), 0);
    ASSERT_EQ((uint32_t)g_game_state.g_battle_end_flag, 0xffffffffu);
}

TEST_F(ControlledGameState, SetEndflagOver50Threshold) {
    g_game_state.g_battle_end_flag = 0;
    g_game_state.g_battle_frame = 0x32;
    ASSERT_EQ(set_endflag_over50(), 0);
    ASSERT_EQ((uint32_t)g_game_state.g_battle_end_flag, 0u);
    g_game_state.g_battle_frame = 0x33;
    ASSERT_EQ(set_endflag_over50(), 0);
    ASSERT_EQ((uint32_t)g_game_state.g_battle_end_flag, 0xffffffffu);
}

// ------------------------------------------------------------------
// 人员技能条件族：people_cond_a / people_cond_b（铁律零·确定性标量锁）
// RE: 00401697 / 004016ce；Ghidra 实证 FUN_00401697.c:12-22 / FUN_004016ce.c:12-22。
// 地址核定：people_array[0] 基址 = 0x007d0ef8；故 Ghidra 的
//   DAT_007d0f2c = 基址+0x34 = skill_buji（people_cond_a 首字段）
//   DAT_007d0f34 = 基址+0x3c = skill_yiliao（两函数次字段）
//   DAT_007d0f30 = 基址+0x38 = skill_jili（people_cond_b 首字段）
// 与 replica 字段选取一致，replica 正确。
// 真值表：
//   people_cond_a: buji==0 ? (yiliao==0 ? -1 : 2) : 0
//   people_cond_b: jili==0 ? (yiliao==0 ? -1 : 2) : 1
// 受控夹具清零 g_game_state，直接置 people_array[0] 字段即可。
// ------------------------------------------------------------------
TEST_F(ControlledGameState, PeopleCondAThreeBranches) {
    // 分支1：buji==0 且 yiliao==0 -> -1
    g_game_state.people_array[0].skill_buji = 0;
    g_game_state.people_array[0].skill_yiliao = 0;
    ASSERT_EQ(people_cond_a(0), -1);
    // 分支2：buji==0 且 yiliao!=0 -> 2
    g_game_state.people_array[0].skill_buji = 0;
    g_game_state.people_array[0].skill_yiliao = 7;
    ASSERT_EQ(people_cond_a(0), 2);
    // 分支3：buji!=0 -> 0（yiliao 任意）
    g_game_state.people_array[0].skill_buji = 3;
    g_game_state.people_array[0].skill_yiliao = 0;
    ASSERT_EQ(people_cond_a(0), 0);
}

TEST_F(ControlledGameState, PeopleCondBThreeBranches) {
    // 分支1：jili==0 且 yiliao==0 -> -1
    g_game_state.people_array[0].skill_jili = 0;
    g_game_state.people_array[0].skill_yiliao = 0;
    ASSERT_EQ(people_cond_b(0), -1);
    // 分支2：jili==0 且 yiliao!=0 -> 2
    g_game_state.people_array[0].skill_jili = 0;
    g_game_state.people_array[0].skill_yiliao = 9;
    ASSERT_EQ(people_cond_b(0), 2);
    // 分支3：jili!=0 -> 1（yiliao 任意）
    g_game_state.people_array[0].skill_jili = 4;
    g_game_state.people_array[0].skill_yiliao = 0;
    ASSERT_EQ(people_cond_b(0), 1);
}

// ------------------------------------------------------------------
// 单位装载标志族：any_unit_flag_loaded / any_unit_flag_unloaded / count_units_flag_loaded
// （铁律零·确定性数组扫描锁）
// RE: 00406c30 / 00406c8c / 0040b40e；Ghidra 实证：
//   FUN_00406c30.c:13-21 扫 0..0x7f，遇 (active_flag!=0 && enemy!=0) 命中返回 0，越界返 1
//   FUN_00406c8c.c:13-21 同上但条件 enemy==0 才命中
//   FUN_0040b40e.c:14-20 扫 0x28..0x80 统计 (active_flag!=0 && enemy!=0) 个数
// 字段：active_flag @ Fighter+0xcc (DAT_0096abcc)；enemy @ People+0x18 (DAT_007d0f10)。
// 受控夹具清零 g_game_state，默认全 0；置指定下标即可。
// ------------------------------------------------------------------
TEST_F(ControlledGameState, AnyUnitFlagLoaded) {
    // 默认全 0 -> 无 active 且 enemy 均非 0 -> 越界返回 1
    ASSERT_EQ(any_unit_flag_loaded(), 1);
    // 下标 5 置 active!=0 且 enemy!=0 -> 命中 -> 返回 0
    g_game_state.fighter_array[5].active_flag = 1;
    g_game_state.people_array[5].enemy = 1;
    ASSERT_EQ(any_unit_flag_loaded(), 0);
    // active!=0 但 enemy==0 -> 不命中 -> 返回 1
    g_game_state.people_array[5].enemy = 0;
    ASSERT_EQ(any_unit_flag_loaded(), 1);
}

TEST_F(ControlledGameState, AnyUnitFlagUnloaded) {
    // 默认全 0 -> 无 active!=0 项 -> 返回 1
    ASSERT_EQ(any_unit_flag_unloaded(), 1);
    // 下标 3 置 active!=0 且 enemy==0 -> 命中 -> 返回 0
    g_game_state.fighter_array[3].active_flag = 1;
    g_game_state.people_array[3].enemy = 0;
    ASSERT_EQ(any_unit_flag_unloaded(), 0);
    // 同一下标改为 enemy!=0 -> 不再命中 -> 返回 1
    g_game_state.people_array[3].enemy = 1;
    ASSERT_EQ(any_unit_flag_unloaded(), 1);
}

TEST_F(ControlledGameState, CountUnitsFlagLoaded) {
    // 默认全 0 -> 计数 0
    ASSERT_EQ(count_units_flag_loaded(), 0);
    // 下限下标 0x28(40) 置 active!=0 且 enemy!=0 -> 计数 1
    g_game_state.fighter_array[0x28].active_flag = 1;
    g_game_state.people_array[0x28].enemy = 1;
    ASSERT_EQ(count_units_flag_loaded(), 1);
    // 下标 50 再置 -> 计数 2
    g_game_state.fighter_array[50].active_flag = 1;
    g_game_state.people_array[50].enemy = 1;
    ASSERT_EQ(count_units_flag_loaded(), 2);
    // 下界下一位 0x27(39) 置同条件 -> 循环从 0x28 起，不计 -> 仍 2
    g_game_state.fighter_array[0x27].active_flag = 1;
    g_game_state.people_array[0x27].enemy = 1;
    ASSERT_EQ(count_units_flag_loaded(), 2);
    // 上界下标 127 置 -> 计数 3（循环 <0x80 含 127）
    g_game_state.fighter_array[127].active_flag = 1;
    g_game_state.people_array[127].enemy = 1;
    ASSERT_EQ(count_units_flag_loaded(), 3);
}

// ------------------------------------------------------------------
// 掩码写入族：stamp_preset_movemask_8a70
// （铁律零·跨记录步长 + 字节缓冲双重锁）
// RE: 00418a70；Ghidra 实证 FUN_00418a70.c:14,20：
//   iVar1 = *(int*)(&DAT_0057ba5c + param_1*0x30)        // skill_array 字节步长 0x30
//   *(char*)((y*W + x) + 0x833414) = 1 - (&DAT_00475be8)[local_10 + iVar1*0x51 + local_14*9]
//   DAT_00475be8 为字节缓冲（[idx] 字节索引）；DAT_00833414 为真实网格缓冲。
// 此测试同时锁定：
//   ① skill_array[param_1].movement_index 在 param_1>0 时读对元素（步长 0x30 修复）
//   ② g_misc_blk_5be8 作为字节缓冲（非 int32 标量）索引正确
// 设局：skill_array[1].movement_index=1（被测），skill_array[0].movement_index=0（区分步长 bug）
//   正确：iVar1=1 -> 字节 idx=lo+0x51+li*9 置 1 -> 写入 1-1=0
//   步长 bug：iVar1=0 -> 字节 idx=lo+li*9 为 0 -> 写入 1-0=1（捕获）
//   缓冲 int32 bug：(int32*)&buf[idx] 读 idx*4 处=0 -> 写入 1-0=1（捕获）
// ------------------------------------------------------------------
TEST_F(ControlledGameState, StampPresetMovemaskReadsSkillIndex) {
    const int W = 20, H = 20;
    g_game_state.g_map_grid_width = W;
    g_game_state.g_map_grid_height = H;
    for (int i = 0; i < MAP_GRID_CAP; i++) g_game_state.g_grid_833414[i] = 0;
    for (int i = 0; i < 0x510; i++) g_game_state.g_misc_blk_5be8[i] = 0;

    g_game_state.skill_array[1].movement_index = 1;
    g_game_state.skill_array[0].movement_index = 0;
    for (int lo = 0; lo < 9; lo++)
        for (int li = 0; li < 9; li++)
            g_game_state.g_misc_blk_5be8[lo + 1 * 0x51 + li * 9] = 1;

    stamp_preset_movemask_8a70(1, 10, 10);

    // 9x9 区域：x = 10+lo (10..18), y = 6+li (6..14)
    for (int li = 0; li < 9; li++) {
        for (int lo = 0; lo < 9; lo++) {
            int x = 10 + lo;
            int y = 6 + li;
            ASSERT_EQ((int)g_game_state.g_grid_833414[y * W + x], 0)
                << "cell (" << x << "," << y << ") should be 0 (correct stride + byte buffer)";
        }
    }
}

// ------------------------------------------------------------------
// 纯标量 / 状态机族（确定性，无 I/O、无跨函数调用）
// ------------------------------------------------------------------

// rand_impl — Borland LCG：seed = seed*0x343fd+0x269ec3（32 位回绕），返回 (seed>>16)&0x7fff
// RE: 0043e310；Ghidra 实证 FUN_0043e310.c:10-11（DAT_00448790 = g_pow2_1_448790）
TEST_F(ControlledGameState, RandImplBorlandLCG) {
    auto lcg = [](uint32_t s) -> uint32_t {
        s = s * 0x343fdu + 0x269ec3u;          // 32-bit wrap
        return (s >> 16) & 0x7fffu;            // 取中间 15 位
    };
    // 主 oracle：seed=1 -> 0x29e2c0 -> (>>16)&0x7fff = 0x29 = 41
    g_game_state.g_pow2_1_448790 = 1;
    ASSERT_EQ((uint32_t)rand_impl(), 41u);
    ASSERT_EQ((uint32_t)g_game_state.g_pow2_1_448790, (1u * 0x343fdu + 0x269ec3u));
    // 差分锁：对齐 Ghidra 公式，覆盖多组种子（含负数位模式）防回归
    for (uint32_t s : {12345u, 0xdeadbeefu, 0x80000000u, 7u, 0xffffffffu}) {
        g_game_state.g_pow2_1_448790 = (int32_t)s;
        ASSERT_EQ((uint32_t)rand_impl(), lcg(s)) << "seed=" << s;
    }
}

// tick_count_inc — g_level_step_count++
// RE: 0041b9a1；Ghidra 实证 FUN_0041b9a1.c:10（DAT_006223a8 = g_level_step_count）
TEST_F(ControlledGameState, TickCountIncIncrements) {
    g_game_state.g_level_step_count = 5;
    ASSERT_EQ(tick_count_inc(), 0);
    ASSERT_EQ(g_game_state.g_level_step_count, 6);
    ASSERT_EQ(tick_count_inc(), 0);
    ASSERT_EQ(g_game_state.g_level_step_count, 7);
}

// select_endflag_value — 分支状态机：
//   (fighter[40].active_flag==0 && end_flag==0) -> end_flag=-1
//   else (fighter[56].active_flag==0 && end_flag==0) -> end_flag=1
//   else 不变
// RE: 00411672；Ghidra 实证 FUN_00411672.c:10-15
//   DAT_0096b5cc = fighter[40].active_flag (0x0096ab90+0x1000+0x3c)
//   DAT_0096b9cc = fighter[56].active_flag (0x0096ab90+0x1600+0x3c)
TEST_F(ControlledGameState, SelectEndflagValueBranches) {
    // 分支 A：fighter[40] 未激活 -> end_flag=-1
    g_game_state.g_battle_end_flag = 0;
    g_game_state.fighter_array[40].active_flag = 0;
    g_game_state.fighter_array[56].active_flag = 1;
    select_endflag_value();
    ASSERT_EQ(g_game_state.g_battle_end_flag, -1);

    // 分支 B：fighter[40] 激活但 fighter[56] 未激活 -> end_flag=1
    g_game_state.g_battle_end_flag = 0;
    g_game_state.fighter_array[40].active_flag = 1;
    g_game_state.fighter_array[56].active_flag = 0;
    select_endflag_value();
    ASSERT_EQ(g_game_state.g_battle_end_flag, 1);

    // 两分支均不满足 -> 不变（仍为 0）
    g_game_state.g_battle_end_flag = 0;
    g_game_state.fighter_array[40].active_flag = 1;
    g_game_state.fighter_array[56].active_flag = 1;
    select_endflag_value();
    ASSERT_EQ(g_game_state.g_battle_end_flag, 0);

    // 守卫：end_flag 已非 0 时不改写（两分支条件均要求 end_flag==0）
    g_game_state.g_battle_end_flag = -5;
    g_game_state.fighter_array[40].active_flag = 0;
    select_endflag_value();
    ASSERT_EQ(g_game_state.g_battle_end_flag, -5);
}

// set_stage2_flag — if (g_battle_frame==2) fighter[80].flag2 = 1
// RE: 0041199c；Ghidra 实证 FUN_0041199c.c:10-11
//   DAT_00475aa4 = g_battle_frame；DAT_0096bfce = fighter[80].flag2 (0x0096ab90+0x1400+0x3e)
TEST_F(ControlledGameState, SetStage2FlagWhenFrameIs2) {
    g_game_state.fighter_array[80].flag2 = 0;
    // frame != 2 -> 不置位
    g_game_state.g_battle_frame = 1;
    set_stage2_flag();
    ASSERT_EQ((int)g_game_state.fighter_array[80].flag2, 0);
    // frame == 2 -> 置位
    g_game_state.g_battle_frame = 2;
    set_stage2_flag();
    ASSERT_EQ((int)g_game_state.fighter_array[80].flag2, 1);
    // 再设：幂等（仅赋值 1）
    g_game_state.g_battle_frame = 5;
    set_stage2_flag();
    ASSERT_EQ((int)g_game_state.fighter_array[80].flag2, 1);
}

// ------------------------------------------------------------------
// 经验 / 资源确定性计算族（无 RNG，纯 g_game_state 读取/回写）
// 铁律零：期望后状态直接来自 Ghidra FUN_*.c 逐指令语义，与 battle_calc oracle 独立核对。
// 这四个函数均为"已语义化转发"：replica 体只有一行 `return battle_calc_*(...)`，
// 但锁定它们等于锁定 RF 包装层不被改坏，且期望值是按 Ghidra 公式手算（非调 oracle）。
// ------------------------------------------------------------------

// battle_exp_from_kill_ab23 — 击杀经验 (FUN_0042ab23)
// 转发 battle_calc_kill_exp(defender, attacker):
//   diff = people[def].level - people[atk].level
//   <-6→1, <-3→2, <-1→4, <0→8, ==0→0xc, else diff*0x10; cap 0x50(80)
TEST_F(ControlledGameState, BattleExpFromKillAb23Formula) {
    auto kill_exp = [&](int dl, int al) -> int32_t {
        g_game_state.people_array[0].level = dl;  // defender idx 0
        g_game_state.people_array[1].level = al;  // attacker idx 1
        return battle_exp_from_kill_ab23(0, 1);    // (defender, attacker)
    };
    ASSERT_EQ(kill_exp(1, 10), 1);     // diff=-9 -> 1
    ASSERT_EQ(kill_exp(1, 5), 2);      // diff=-4 -> 2
    ASSERT_EQ(kill_exp(1, 3), 4);      // diff=-2 -> 4
    ASSERT_EQ(kill_exp(1, 2), 8);      // diff=-1 -> 8
    ASSERT_EQ(kill_exp(5, 5), 0xc);    // diff=0  -> 12
    ASSERT_EQ(kill_exp(3, 1), 32);     // diff=2  -> 2*16
    ASSERT_EQ(kill_exp(10, 1), (int32_t)0x50);  // diff=9  -> 144 cap 80
    ASSERT_EQ(kill_exp(8, 1), (int32_t)0x50);   // diff=7  -> 112 cap 80
}

// battle_exp_from_attack_aa92 — 攻击经验 (FUN_0042aa92)
// 转发 battle_calc_attack_exp(attacker, defender):
//   diff = people[def].level - people[atk].level
//   <-6→1, <-3→2, <-1→4, <0→6, ==0→8, else diff*8+8; cap 0x20(32)
TEST_F(ControlledGameState, BattleExpFromAttackAa92Formula) {
    auto atk_exp = [&](int al, int dl) -> int32_t {
        g_game_state.people_array[0].level = al;  // attacker idx 0
        g_game_state.people_array[1].level = dl;  // defender idx 1
        return battle_exp_from_attack_aa92(0, 1);  // (attacker, defender)
    };
    ASSERT_EQ(atk_exp(10, 1), 1);    // diff=1-10=-9 -> 1
    ASSERT_EQ(atk_exp(5, 1), 2);     // diff=-4     -> 2
    ASSERT_EQ(atk_exp(3, 1), 4);     // diff=-2     -> 4
    ASSERT_EQ(atk_exp(2, 1), 6);     // diff=-1     -> 6
    ASSERT_EQ(atk_exp(5, 5), 8);     // diff=0      -> 8
    ASSERT_EQ(atk_exp(1, 3), 24);    // diff=3-1=2  -> 2*8+8
    ASSERT_EQ(atk_exp(1, 4), 32);    // diff=3      -> 32 (==cap 不截断)
    ASSERT_EQ(atk_exp(1, 5), 32);    // diff=4      -> 40 cap 32
    ASSERT_EQ(atk_exp(1, 10), 32);   // diff=9      -> 80 cap 32
}

// clamp_stat_delta — 资源增减夹取 (FUN_00427830)
// 转发 battle_calc_clamp_resource(base, delta, cap) = (uint)(base+delta)<=cap ? base+delta : cap
// 注意无符号比较：base+delta 为负时回绕为大 uint，必 > cap，返回 cap。
TEST_F(ControlledGameState, ClampStatDeltaMinCapBaseDelta) {
    ASSERT_EQ(clamp_stat_delta(50, 10, 100), 60);    // 60 <= 100
    ASSERT_EQ(clamp_stat_delta(90, 10, 100), 100);   // 100 <= 100
    ASSERT_EQ(clamp_stat_delta(95, 10, 100), 100);   // 105 > 100 -> cap
    ASSERT_EQ(clamp_stat_delta(-5, 10, 100), 5);     // -5+10 = 5
    ASSERT_EQ(clamp_stat_delta(50, -60, 100), 100);  // -10 回绕 -> cap
    ASSERT_EQ(clamp_stat_delta(0, 0, 0), 0);         // 0 <= 0
}

// unit_gain_exp_9e92 — 经验累计 + 升级 (FUN_00429e92)
// 转发 battle_calc_gain_exp(unit, gained):
//   total = people[unit].experience + gained
//   leveled = total/100 != 0; exp = total%100; level += total/100 (cap 99); return leveled
TEST_F(ControlledGameState, UnitGainExp9e92AccumulatesAndLevels) {
    // 无升级：exp 0+50 -> exp=50, level 不变, 返回 0
    g_game_state.people_array[2].experience = 0;
    g_game_state.people_array[2].level = 1;
    ASSERT_EQ(unit_gain_exp_9e92(2, 50), 0);
    ASSERT_EQ(g_game_state.people_array[2].experience, 50);
    ASSERT_EQ(g_game_state.people_array[2].level, 1);

    // 跨级：exp 80+50=130 -> exp=30, level 1->2, 返回 1
    g_game_state.people_array[2].experience = 80;
    g_game_state.people_array[2].level = 1;
    ASSERT_EQ(unit_gain_exp_9e92(2, 50), 1);
    ASSERT_EQ(g_game_state.people_array[2].experience, 30);
    ASSERT_EQ(g_game_state.people_array[2].level, 2);

    // 升级溢出封顶 99：level 98 + 500/100=5 -> 103 封 99；exp=500%100=0
    g_game_state.people_array[2].experience = 0;
    g_game_state.people_array[2].level = 98;
    ASSERT_EQ(unit_gain_exp_9e92(2, 500), 1);
    ASSERT_EQ(g_game_state.people_array[2].experience, 0);
    ASSERT_EQ(g_game_state.people_array[2].level, 99);
}

// ------------------------------------------------------------------
// 续十二：参数化确定性计算转发器（2 锁）
// 铁律零证据：
//   battle_damage_resolve_2784b -> output/ghidra_all/FUN_0042784b.c:7-14
//     逻辑 (param_2<param_1 && param_3<=param_1-param_2) -> param_3=param_1-param_2
//     replica 转发 battle_calc_damage (battle_calc.cpp:121-126) 1:1
//   calc_level_bonus_a199        -> output/ghidra_all/FUN_0042a199.c:7-48
//     g_fight_index<0x24 时遍历 0x20 单位，对未装备(enemy==0)且已部署(active_flag!=0)
//     求和 level、计数，bonus=avg-(fight_index*2+1)；<2->0；按 g_flow_phase(=DAT_007db578) 缩放 0/÷2/×4÷5
// ------------------------------------------------------------------

// battle_damage_resolve_2784b 纯参数函数，无全局副作用；锁定与 Ghidra 公式逐位一致
// （钳位上抬 / 维持高位 incoming / defense>=attack 原样返回 / 边界相等）
TEST_F(ControlledGameState, BattleDamageResolve2784b) {
    ASSERT_EQ(battle_damage_resolve_2784b(100, 30,  0u),  70u);  // 30<100, 0<=70 -> 100-30
    ASSERT_EQ(battle_damage_resolve_2784b(100, 30, 50u),  70u);  // 钳位上抬 50->70
    ASSERT_EQ(battle_damage_resolve_2784b(100, 30, 80u),  80u);  // 80>70 维持 incoming
    ASSERT_EQ(battle_damage_resolve_2784b(100, 120, 0u),  0u);   // defense>=attack -> 原样 0
    ASSERT_EQ(battle_damage_resolve_2784b(100, 120, 50u), 50u);  // defense>=attack -> 原样 50
    ASSERT_EQ(battle_damage_resolve_2784b(100, 100, 5u),  5u);   // 相等非小于 -> 返回 5
    ASSERT_EQ(battle_damage_resolve_2784b(50,   0,   0u), 50u);   // 干净正差
}

// calc_level_bonus_a199 读取 g_fight_index / people_array[].enemy,level / fighter_array[].active_flag / g_flow_phase
TEST_F(ControlledGameState, CalcLevelBonusA199) {
    // 部署 4 个未装备单位，level [10,20,30,40] -> sum=100, n=4, avg=25
    const int32_t lv[4] = {10, 20, 30, 40};
    for (int i = 0; i < 4; ++i) {
        *(int32_t*)((uint8_t*)&g_game_state.people_array[0].enemy + i * 0x84) = 0;  // 未装备
        (&g_game_state.people_array[0].level)[i * 0x21] = lv[i];                    // level
        (&g_game_state.fighter_array[0].active_flag)[i * 0x40] = 1;                 // 已部署
    }
    g_game_state.g_fight_index = 0;   // fight_index*2+1 = 1 -> iVar1 = 25 - 1 = 24

    // g_flow_phase 缩放分支（0->0, 1->÷2, 2->×4÷5）
    g_game_state.g_flow_phase = 0;
    ASSERT_EQ(calc_level_bonus_a199(), 0);
    g_game_state.g_flow_phase = 1;
    ASSERT_EQ(calc_level_bonus_a199(), 12);   // 24/2
    g_game_state.g_flow_phase = 2;
    ASSERT_EQ(calc_level_bonus_a199(), 19);   // (24*4)/5 = 19

    // 无已部署单位 -> 0
    for (int i = 0; i < 4; ++i) (&g_game_state.fighter_array[0].active_flag)[i * 0x40] = 0;
    g_game_state.g_flow_phase = 2;
    ASSERT_EQ(calc_level_bonus_a199(), 0);

    // fight_index >= 0x24 -> 直接返回 0（跳过整段）
    (&g_game_state.fighter_array[0].active_flag)[0] = 1;
    g_game_state.g_fight_index = 0x30;
    ASSERT_EQ(calc_level_bonus_a199(), 0);

    // avg-(fight_index*2+1) < 2 -> 0（即便 flow=2）
    g_game_state.g_fight_index = 0x10;  // 16*2+1=33; 仅 unit0 活跃 level=10 -> avg=10; 10-33=-23<2 -> 0
    g_game_state.g_flow_phase = 2;
    ASSERT_EQ(calc_level_bonus_a199(), 0);
}

// ------------------------------------------------------------------
// 续十三：backlog ③ 裸地址 → GameState 建模（铁律零：PE DLZFULL/DLZ.exe 实证）
//   · 0x448b7c  MSVC 运行时错误消息串表（18 条 R6002…/DOMAIN/SING/TLOSS/"runtime error "），
//     原版 *(char**)(iVar8*8+0x448b7c) 在 replica 非法解引用；改为内嵌 PE 字符串数组 k_rt_err_msgs。
//   · 0x4744a0/0x474fa0  surface 指针全局(g_state_blk_b4f4/g_buf_blk_7418)，原版指向固定 VRAM；
//     改为指向 GameState 内 g_surf_b4f4/g_surf_7418 缓冲（surface_blit_22889 确实解引用源指针）。
// ------------------------------------------------------------------

// 0x448b7c：用有效 key(0x02 -> "R6002...") 触发索引路径，验证函数不再解引用裸地址 0x448b7c
// （若回退为 *(char**)(iVar8*8+0x448b7c) 会段错误，被本测试捕获）。
TEST_F(ControlledGameState, GetModuleFilename2230NoNakedDeref) {
    g_game_state.g_ui_blk_d820 = 1;  // 走 WriteFile 分支, 不触 GetModuleFileNameA
    // param_1 在函数内仅作值比较(==key), 不解析为指针, 故 0x02 安全
    get_module_filename_2230((int32_t *)(uintptr_t)0x02);
    SUCCEED();  // 到达此处即未崩溃 = 裸地址 0x448b7c 已消除
}

// 0x4744a0/0x474fa0：调用生产函数 asset_mine_img() 验证 surface 指针全局确实指向 GameState 内缓冲，
// 而非裸地址（否则解引用崩溃/读非法内存）。
TEST_F(ControlledGameState, AssetMineImgRepointsSurfacePtrsToGameState) {
    asset_mine_img();  // 资源文件在测试环境可能缺失, file_read_at 优雅失败; 指针赋值(行18880-18882)独立执行
    ASSERT_EQ((void *)g_game_state.g_state_blk_b4f4, (void *)&g_game_state.g_surf_b4f4);
    ASSERT_EQ((void *)g_game_state.g_buf_blk_7418, (void *)&g_game_state.g_surf_7418);
    // 确证不再是裸地址常量
    ASSERT_NE((uintptr_t)g_game_state.g_state_blk_b4f4, (uintptr_t)0x4744a0);
    ASSERT_NE((uintptr_t)g_game_state.g_buf_blk_7418, (uintptr_t)0x474fa0);
    // 解引用合法内存(验证 buffer 真实可达)
    uint8_t probe = *(uint8_t *)g_game_state.g_state_blk_b4f4;
    (void)probe;
}

}  // namespace
