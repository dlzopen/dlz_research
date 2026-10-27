// test_dialogue_render.cpp — 对话整屏渲染 Tier A 保真（headless 管线）
//
// 验证对象（铁律零，逐指令实证）：
//   1) dialogue_token_palette_index() 的映射与 FUN_004061df.c:39-52 严格一致
//      （A=0xb7/B=0x85/C=0x89/D=0x4f，其余含 G/T→0x48 正文色）。
//   2) dialogue_build_segments() 的逐字配色分段：AA/BB/CC/DD 标记对不绘制自身、
//      仅着色紧随其后的 1 个字模（与 FUN_004061df.c:39-52,100 的调色板语义一致）。
//   3) 真实数据管线：DLZFULL TXT.DAT 解码→parse_dialogue→build_segments，
//      真实条目里出现的令牌后字模均得到正确的调色板 RGB（Tier A 玩家可感知保真）。
//
// 头less：不走 SDL/字体位图，直接校验渲染器产出的彩色分段（data-level 锁定）。

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "app/resource/dat_loader.hpp"
#include "core/data_model/palette.hpp"
#include "logic/dialogue/dialogue_manager.hpp"
#include "presentation/dialogue/dialogue_renderer.hpp"

namespace {

const char* kGameDataDir = "C:/seesaw/source/dlz_research/DLZFULL";

// 由调色板索引取 RGB（kOriginalPalette 已 RE，索引*4 偏移）。
std::array<uint8_t, 3> pal_rgb(uint8_t idx) {
    return {dlzr::core::kOriginalPalette[idx * 4 + 0],
            dlzr::core::kOriginalPalette[idx * 4 + 1],
            dlzr::core::kOriginalPalette[idx * 4 + 2]};
}

void ExpectSegRgb(const dlzr::presentation::DialogueColoredSegment& s, uint8_t idx) {
    const auto e = pal_rgb(idx);
    EXPECT_EQ(s.r, e[0]);
    EXPECT_EQ(s.g, e[1]);
    EXPECT_EQ(s.b, e[2]);
}

}  // namespace

// ---- 1) 令牌→调色板索引映射（FUN_004061df.c:39-52） ----
TEST(DialogueRender, TokenPaletteIndex) {
    EXPECT_EQ(dlzr::presentation::dialogue_token_palette_index('A'), 0xb7);
    EXPECT_EQ(dlzr::presentation::dialogue_token_palette_index('B'), 0x85);
    EXPECT_EQ(dlzr::presentation::dialogue_token_palette_index('C'), 0x89);
    EXPECT_EQ(dlzr::presentation::dialogue_token_palette_index('D'), 0x4f);
    // 解析层令牌（GG/TT）与任意 ASCII 一律正文色 0x48（FUN_004061df.c:51-52）。
    EXPECT_EQ(dlzr::presentation::dialogue_token_palette_index('G'), 0x48);
    EXPECT_EQ(dlzr::presentation::dialogue_token_palette_index('T'), 0x48);
    EXPECT_EQ(dlzr::presentation::dialogue_token_palette_index('x'), 0x48);
    EXPECT_EQ(dlzr::presentation::dialogue_token_palette_index('1'), 0x48);
    EXPECT_EQ(dlzr::presentation::dialogue_token_palette_index(' '), 0x48);
}

// ---- 2) ASCII 混合令牌分段（确定性，不依赖字体/文件） ----
TEST(DialogueRender, ColorSegmentsASCII) {
    // 'a'→友方红(0xb7) / 'b'→敌方绿(0x85) / 'c'→物品金(0x89) / 'd'→地名蓝(0x4f)
    // 'Ge' 非令牌→正文青(0x48)
    std::string text = "AAaBBbCCcDDdGe";
    auto segs = dlzr::presentation::dialogue_build_segments(text);

    ASSERT_EQ(segs.size(), 5u);
    EXPECT_EQ(segs[0].text, "a");
    ExpectSegRgb(segs[0], 0xb7);

    EXPECT_EQ(segs[1].text, "b");
    ExpectSegRgb(segs[1], 0x85);

    EXPECT_EQ(segs[2].text, "c");
    ExpectSegRgb(segs[2], 0x89);

    EXPECT_EQ(segs[3].text, "d");
    ExpectSegRgb(segs[3], 0x4f);

    EXPECT_EQ(segs[4].text, "Ge");
    ExpectSegRgb(segs[4], 0x48);
}

// ---- 3) 多字节 GBK 字模着色 + 令牌自身被消费 ----
TEST(DialogueRender, ColorSegmentsGBKAndMarkerConsumed) {
    // 构造：AA + 两字节 GBK 字(0xCE 0xCA) + 普通 ASCII 'X'
    std::string text;
    text += "AA";
    text += static_cast<char>(0xCE);
    text += static_cast<char>(0xCA);
    text += "X";

    auto segs = dlzr::presentation::dialogue_build_segments(text);
    // 期望两段：GBK 字(着友方红) + "X"(正文青)
    ASSERT_EQ(segs.size(), 2u);

    EXPECT_EQ(segs[0].text.size(), 2u);          // 两字节 GBK 字
    EXPECT_EQ(segs[0].text[0], static_cast<char>(0xCE));
    EXPECT_EQ(segs[0].text[1], static_cast<char>(0xCA));
    ExpectSegRgb(segs[0], 0xb7);

    EXPECT_EQ(segs[1].text, "X");
    ExpectSegRgb(segs[1], 0x48);

    // 令牌字符 'A' 本身不得出现在任何分段文本中（仅着色，不绘制）
    for (const auto& s : segs) {
        EXPECT_EQ(s.text.find('A'), std::string::npos);
    }
}

// ---- 4) 真实数据管线：DLZFULL 解码→解析→分段，令牌后字模着色正确 ----
TEST(DialogueRender, RealDataPipelineTokenColors) {
    ASSERT_TRUE(std::filesystem::exists(kGameDataDir));
    dlzr::app::DatLoader loader(kGameDataDir);
    auto entries = loader.load_dialogues();
    ASSERT_GT((int)entries.size(), 100);

    int markers_checked = 0;
    for (const auto& entry : entries) {
        auto d = dlzr::logic::DialogueManager::parse_dialogue(entry);
        const std::string& t = d.text;
        if (t.size() < 3) continue;

        // 找出所有 AA/BB/CC/DD 标记对，验证紧随其后的字模着色正确
        for (size_t i = 0; i + 1 < t.size(); ++i) {
            char c = t[i];
            if ((c == 'A' || c == 'B' || c == 'C' || c == 'D') && t[i + 1] == c) {
                uint8_t idx = dlzr::presentation::dialogue_token_palette_index(c);
                const auto expected = pal_rgb(idx);
                // 紧随标记对之后的字模（1-2 字节）
                size_t g = i + 2;
                int n = (g < t.size())
                            ? ((uint8_t)t[g] < 0x80 ? 1 : 2)
                            : 0;
                if (n == 0) continue;
                if (g + n > t.size()) n = 1;
                std::string glyph = t.substr(g, n);

                // 在分段结果中，该字模必须作为独立分段且 RGB == 对应调色板
                bool found = false;
                auto segs = dlzr::presentation::dialogue_build_segments(t);
                for (const auto& s : segs) {
                    if (s.text == glyph &&
                        s.r == expected[0] && s.g == expected[1] && s.b == expected[2]) {
                        found = true;
                        break;
                    }
                }
                EXPECT_TRUE(found) << "真实条目令牌 '" << c << c
                                   << "' 后字模着色未命中调色板索引 0x"
                                   << std::hex << (int)idx;
                ++markers_checked;
            }
        }
    }
    // 真实数据里应至少存在若干令牌（否则本测试未覆盖 Tier A 保真）
    EXPECT_GT(markers_checked, 0) << "DLZFULL 对话数据未找到任何 AA/BB/CC/DD 令牌";
}
