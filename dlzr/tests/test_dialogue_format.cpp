// test_dialogue_format.cpp — 对话文本令牌格式化层保真（铁律零）
//
// 验证对象（逐指令实证）：
//   format_dialogue_text() 是解析层→渲染层的桥接（已抽离为 dlzr::logic 可测模块）：
//   1) AA/BB/CC/DD 配色标记原样保留，交由 dialogue_build_segments 着色
//      （FUN_004061df.c:39-52：A=0xb7/B=0x85/C=0x89/D=0x4f，其余=0x48 正文色）。
//   2) GG（主角名令牌）→ 按原版 FUN_0043cc96.c:83-93 包成 "AA"+逐字，
//      使渲染层 dialogue_build_segments 将名字着红(0xb7)。早期 clean 实现把 GG 直接替换成裸名
//      （被 dialogue_build_segments 当作默认色 0x48），与原版分叉——本测试锁定修复后的行为。
//   3) TT（战斗/地名令牌）→ 原版 FUN_0043cc96.c:71-82 展开 DAT_007d01c8，与 GG 同构
//      （逐字包 "AA" 着红 0xb7）。t_name 为空（对应原版空缓冲→:74 循环 0 次）时退化为剔除，
//      与非空时按 "AA"+t_name 展开均已实证；确切运行时 T 名值待游戏流程 RE。
//   4) 集成：format → build_segments，GG 展开后的名字必须为红(0xb7)。
//
// 编码说明：wrapping 逻辑按字节（GBK：≥0x80 视为双字节）工作，与具体编码无关。
//   本测试主角名用 ASCII（避免 UTF-8/GBK 源文件歧义），并额外加一条显式 GBK 字节
//   （"王"=0xD5C5）用例，坐实真实 GBK 姓名也能被正确逐字包 AA 并着红。

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <string>

#include "core/data_model/palette.hpp"
#include "logic/dialogue/dialogue_text_util.hpp"
#include "presentation/dialogue/dialogue_renderer.hpp"

namespace {

constexpr uint8_t kRed = 0xb7;

std::array<uint8_t, 3> pal_rgb(uint8_t idx) {
    return {dlzr::core::kOriginalPalette[idx * 4 + 0],
            dlzr::core::kOriginalPalette[idx * 4 + 1],
            dlzr::core::kOriginalPalette[idx * 4 + 2]};
}

}  // namespace

// ---- 1) 配色标记原样保留 ----
TEST(DialogueFormat, ColorMarkersPreserved) {
    EXPECT_EQ(dlzr::logic::format_dialogue_text("AA李AA正BB鬼BB子", "Wang"),
              "AA李AA正BB鬼BB子");
    EXPECT_EQ(dlzr::logic::format_dialogue_text("CC土CC雷DD白DD洋DD淀", "Wang"),
              "CC土CC雷DD白DD洋DD淀");
}

// ---- 2) GG 按原版包成 "AA"+逐字（每字前置 "AA"）----
TEST(DialogueFormat, GgWrappedPerCharInAA) {
    // ASCII 主角名避免源文件编码歧义；wrapping 按字节，与编码无关。
    std::string out = dlzr::logic::format_dialogue_text("我是GG，来", "Zhao");
    EXPECT_NE(out.find("AAZ"), std::string::npos);
    EXPECT_NE(out.find("AAh"), std::string::npos);
    EXPECT_NE(out.find("AAa"), std::string::npos);
    EXPECT_NE(out.find("AAo"), std::string::npos);
    EXPECT_EQ(out.find("GG"), std::string::npos)
        << "GG 应被展开为 \"AA\"+主角名，不应残留裸令牌";
}

// ---- 3) TT 空 t_name → 剔除（对应原版 DAT_007d01c8 空缓冲，FUN_0043cc96.c:74 循环 0 次）----
TEST(DialogueFormat, TtStrippedWhenEmpty) {
    // 不传 t_name（默认 ""）→ 退化为剔除，与原版空缓冲行为一致。
    EXPECT_EQ(dlzr::logic::format_dialogue_text("在TT中殉国", "x"), "在中殉国");
    // 显式传空串同样剔除。
    EXPECT_EQ(dlzr::logic::format_dialogue_text("在TT中殉国", "x", ""), "在中殉国");
}

// ---- 3b) TT 非空 t_name → 按原版逐字包 "AA"（FUN_0043cc96.c:71-82），着红 0xb7 ----
TEST(DialogueFormat, TtExpandedPerCharInAA) {
    // 战斗/地名用 GBK 字节（与真实数据一致）；"平阳"=0xC6BD 0xD1F3。
    // 注意：原版 FUN_0043cc96.c:71-82 对 T 名「每个 GBK 字」独立包 "AA"（"AA平"+"AA阳"），
    //   而非整串单 "AA"。环绕正文用 ASCII，避免 UTF-8 字面量与 GBK 名混排时
    //   dialogue_build_segments 误判 "AA" 标记对（真实对话串为纯 GBK，无此问题）。
    std::string pingyang(std::string("\xC6\xBD\xD1\xF3", 4));  // 平阳 (GBK)
    std::string ping(std::string("\xC6\xBD", 2));               // 平
    std::string yang(std::string("\xD1\xF3", 2));               // 阳
    std::string out = dlzr::logic::format_dialogue_text("Say TT end", "Wang", pingyang);
    // 期望 "Say " + "AA平" + "AA阳" + " end" —— 每个 GBK 字独立包 "AA"。
    std::string expect = std::string("Say ") + "AA" + ping + "AA" + yang + " end";
    EXPECT_EQ(out, expect);
    EXPECT_EQ(out.find("TT"), std::string::npos)
        << "TT 应被展开为 \"AA\"+战斗/地名，不应残留裸令牌";

    // 集成：渲染层 dialogue_build_segments 应把 "平"/"阳" 各自着红(0xb7)。
    auto segs = dlzr::presentation::dialogue_build_segments(out);
    const auto e = pal_rgb(kRed);
    bool red_ping = false, red_yang = false;
    for (const auto& s : segs) {
        if (s.text == ping) {
            EXPECT_EQ(s.r, e[0]); EXPECT_EQ(s.g, e[1]); EXPECT_EQ(s.b, e[2]);
            red_ping = true;
        }
        if (s.text == yang) {
            EXPECT_EQ(s.r, e[0]); EXPECT_EQ(s.g, e[1]); EXPECT_EQ(s.b, e[2]);
            red_yang = true;
        }
    }
    EXPECT_TRUE(red_ping) << "展开的战斗名首字『平』应作为红分段出现";
    EXPECT_TRUE(red_yang) << "展开的战斗名次字『阳』应作为红分段出现";
}

// ---- 4) 集成：GG 展开后首字（ASCII）经渲染层着红(0xb7) ----
TEST(DialogueFormat, GgExpandedNameRendersRed) {
    std::string out = dlzr::logic::format_dialogue_text("GG加油", "Zhao");
    auto segs = dlzr::presentation::dialogue_build_segments(out);
    ASSERT_GE(segs.size(), 1u);
    EXPECT_EQ(segs[0].text, "Z");  // 首字 'Z' 着红
    const auto e = pal_rgb(kRed);
    EXPECT_EQ(segs[0].r, e[0]);
    EXPECT_EQ(segs[0].g, e[1]);
    EXPECT_EQ(segs[0].b, e[2]);
}

// ---- 5) GBK 字节级保真：主角名以 GBK 存储（"王"=0xD5C5），逐字包 AA 并着红 ----
TEST(DialogueFormat, GgWrappedGbkNameRendersRed) {
    std::string wang(std::string("\xD5\xC5", 2));  // 王 (GBK, 双字节)
    std::string out = dlzr::logic::format_dialogue_text("GG", wang);
    // 期望含 "AA" + 王(0xD5C5) 作为整体
    EXPECT_NE(out.find(std::string("AA", 2) + wang), std::string::npos);
    auto segs = dlzr::presentation::dialogue_build_segments(out);
    ASSERT_GE(segs.size(), 1u);
    EXPECT_EQ(segs[0].text, wang);  // 整字 "王" 作为独立分段着红
    const auto e = pal_rgb(kRed);
    EXPECT_EQ(segs[0].r, e[0]);
    EXPECT_EQ(segs[0].g, e[1]);
    EXPECT_EQ(segs[0].b, e[2]);
}
