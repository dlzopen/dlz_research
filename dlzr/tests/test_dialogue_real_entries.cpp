// test_dialogue_real_entries.cpp — 真实 TXT.DAT 战前条目端到端保真测试
//
// 铁律零依据：前几轮对话测试均用「合成输入」（ASCII 主角名 + 单令牌），未覆盖
// clean 层实际播放的真实战前条目。本测试直接取原始 TXT.DAT（XOR 0x51 解码）中
// level_1 战前 TXT[27] 的真实字节（多说话人码 00/03/-1 + 混合令牌 AA友方红/BB敌方
// 绿/CC物品金/DD地名蓝 + xx 终止符），喂完整链路：
//   parse_dialogue_entry (FUN_0043cc96 逐行模型)
//   → format_dialogue_text (GG/TT 处理，FUN_0043cc96.c:71-93/83-93)
//   → dialogue_build_segments (逐字配色，FUN_004061df.c:39-52)
// 锁定「真实数据 → clean 层渲染」全链正确，闭合测试盲区。
//
// 真实条目来源：scripts 解码 DLZFULL/TXT.DAT，TXT[27] =
//   "00AA小AA嘎AA子，你在这里干什么？"        (说话人 00，AA=友方红)
//   "03报告队长，我在做地雷！"
//   "00你这个木头疙瘩，也叫地雷？"
//   "03赵虎大叔他们做地雷不要我插手，我就只有来做木头地雷嘛。…"
//   "00好！我给你这地雷取个名字叫CC伪CC装CC雷，多做一些到处都埋…"  (CC=物品金)
//   "-1取得CC伪CC装CC雷制作技术。xx"          (说话人 -1 旁白，xx 终止符)
// 本测试取第 0 行（00+AA）与第 5 行（-1+CC）覆盖两种令牌 + 说话人码剥离 + xx 剥离。

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <string>

#include "core/data_model/palette.hpp"
#include "core/data_model/types.hpp"
#include "logic/dialogue/dialogue_manager.hpp"
#include "logic/dialogue/dialogue_text_util.hpp"
#include "presentation/dialogue/dialogue_renderer.hpp"

namespace {

constexpr uint8_t kRed = 0xb7;     // 友方名（A 令牌）
constexpr uint8_t kGold = 0x89;    // 物品/技术名（C 令牌）
constexpr uint8_t kNormal = 0x48;  // 正文

std::array<uint8_t, 3> pal_rgb(uint8_t idx) {
    return {dlzr::core::kOriginalPalette[idx * 4 + 0],
            dlzr::core::kOriginalPalette[idx * 4 + 1],
            dlzr::core::kOriginalPalette[idx * 4 + 2]};
}

// 找分段中 text == want 的段，返回其 RGB；找不到返回 nullopt 语义（用 bool 标记）。
bool find_seg_color(const std::vector<dlzr::presentation::DialogueColoredSegment>& segs,
                    const std::string& want, std::array<uint8_t, 3>& out) {
    for (const auto& s : segs) {
        if (s.text == want) {
            out = {s.r, s.g, s.b};
            return true;
        }
    }
    return false;
}

}  // namespace

TEST(DialogueRealEntries, PreBattleTXT27_AAandCC_RenderedWithCorrectColors) {
    // TXT[27] 第 0 行真实字节（去 \r）：说话人码 00 + AA小AA嘎AA子…，你在这里干什么？
    std::string line0 =
        "\x30\x30\x41\x41\xD0\xA1\x41\x41\xB8\xC2\x41\x41\xD7\xD3\xA3\xAC"
        "\xC4\xE3\xD4\xDA\xD5\xE2\xC0\xEF\xB8\xC9\xCA\xB2\xC3\xB4\xA3\xBF";
    // TXT[27] 第 5 行真实字节（去 \r）：说话人 -1 + 取得CC伪CC装CC雷制作技术。xx
    std::string line5 =
        "\x2D\x31\xC8\xA1\xB5\xC3\x43\x43\xCE\xB1\x43\x43\xD7\xB0\x43\x43"
        "\xC0\xD7\xD6\xC6\xD7\xF7\xBC\xBC\xCA\xF5\xA1\xA3\x78\x78";

    std::string raw = line0 + "\n" + line5;

    // 1) 逐行解析（FUN_0043cc96 逐行模型）
    auto parsed = dlzr::logic::DialogueManager::parse_dialogue_entry(raw);
    ASSERT_EQ(parsed.size(), 2u) << "TXT[27] 两行应解析为 2 条 Dialogue";
    EXPECT_EQ(parsed[0].speaker_id, 0) << "第 0 行说话人码 00 → id 0";
    EXPECT_EQ(parsed[1].speaker_id, -1) << "第 5 行说话人码 -1 → 旁白 -1";
    // 说话人码应被剥离，不残留为可见乱码
    EXPECT_EQ(parsed[0].text.find("\x30\x30"), std::string::npos)
        << "说话人码 00 必须剥离，不得残留为可见 '00'";
    EXPECT_EQ(parsed[1].text.find("xx"), std::string::npos)
        << "xx 终止符必须剥离";

    // 2) 格式化（本行不含 GG/TT，AA/CC 标记原样保留交由渲染层着色）
    std::string disp0 = dlzr::logic::format_dialogue_text(parsed[0].text, "Hero", "");
    std::string disp1 = dlzr::logic::format_dialogue_text(parsed[1].text, "Hero", "");

    // 3) 逐字配色（FUN_004061df.c:39-52）
    auto segs0 = dlzr::presentation::dialogue_build_segments(disp0);
    auto segs1 = dlzr::presentation::dialogue_build_segments(disp1);

    // AA 令牌：小/嘎/子 各自着红(0xb7)
    std::array<uint8_t, 3> c;
    ASSERT_TRUE(find_seg_color(segs0, "\xD0\xA1", c)) << "『小』应作为独立红色分段出现";
    EXPECT_EQ(c, pal_rgb(kRed)) << "AA 友方名 → 红 0xb7";
    ASSERT_TRUE(find_seg_color(segs0, "\xD7\xD3", c)) << "『子』应作为独立红色分段出现";
    EXPECT_EQ(c, pal_rgb(kRed)) << "AA 友方名 → 红 0xb7";

    // 分段 text 不含 AA 标记对本身
    for (const auto& s : segs0) {
        EXPECT_EQ(s.text.find("AA"), std::string::npos)
            << "分段 text 不应含 AA 标记对";
    }

    // CC 令牌：伪/装/雷 各自着金(0x89)
    ASSERT_TRUE(find_seg_color(segs1, "\xCE\xB1", c)) << "『伪』应作为独立金色分段出现";
    EXPECT_EQ(c, pal_rgb(kGold)) << "CC 物品/技术名 → 金 0x89";
    ASSERT_TRUE(find_seg_color(segs1, "\xC0\xD7", c)) << "『雷』应作为独立金色分段出现";
    EXPECT_EQ(c, pal_rgb(kGold)) << "CC 物品/技术名 → 金 0x89";

    for (const auto& s : segs1) {
        EXPECT_EQ(s.text.find("CC"), std::string::npos)
            << "分段 text 不应含 CC 标记对";
    }

    // 普通正文（如 "，"/"。"/"取得"）应为正文色(0x48)，非红非金
    bool saw_normal = false;
    for (const auto& s : segs1) {
        if (s.text == "\xC8\xA1\xB5\xC3") {  // 取得
            EXPECT_EQ(s.r, pal_rgb(kNormal)[0]);
            EXPECT_EQ(s.g, pal_rgb(kNormal)[1]);
            EXPECT_EQ(s.b, pal_rgb(kNormal)[2]);
            saw_normal = true;
        }
    }
    EXPECT_TRUE(saw_normal) << "『取得』应为正文色(0x48)";
}
