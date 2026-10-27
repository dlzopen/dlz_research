// test_dialogue_playback.cpp — 对话播放状态机端到端集成测试
//
// 铁律零依据：播放链路 start_dialogue → advance_dialogue → build_dialogue_display
// 已抽取至 logic/dialogue/dialogue_playback.{hpp,cpp}（原 main.cpp 匿名命名空间），
// 对应二进制 FUN_0043d454（驱动循环逐行播）+ FUN_0043cc96（单行解析，DAT_0098d810
// 游标逐行推进）+ FUN_0043d82f（整段一次性渲染，无逐字）。
//
// 本测试闭合此前唯一未覆盖的真逻辑：逐行解析的结果是否真在播放层逐条推进、
// 说话人名是否真前置进显示串、GG 令牌是否真逐字包 AA。

#include <gtest/gtest.h>

#include <cstring>
#include <string>

#include "core/data_model/game_data.hpp"
#include "core/data_model/types.hpp"
#include "logic/dialogue/dialogue_playback.hpp"

using namespace dlzr::logic;
using dlzr::core::GameData;
using dlzr::core::People;

namespace {

void set_people_name(GameData& gd, int idx, const char* s) {
    gd.people[idx].name.fill(0);
    std::strncpy(gd.people[idx].name.data(), s, gd.people[idx].name.size() - 1);
}

}  // namespace

// 一条多说话人条目（每行独立 2 位数字说话人码）应被展平为多行 Dialogue，
// 且各行说话人码 / 末行 xx 终止符被正确剥离。
TEST(DialoguePlayback, MultiSpeakerEntryFlattensToLines) {
    GameData gd;
    set_people_name(gd, 1, "Li");
    set_people_name(gd, 2, "Wang");
    set_people_name(gd, 3, "Zhao");

    DialogueState ds;
    start_dialogue(ds, {"01Li说\n02Wang说\n03Zhao说xx"}, GameMode::BATTLE);

    ASSERT_TRUE(ds.active);
    ASSERT_EQ(ds.parsed.size(), 3u);

    EXPECT_EQ(ds.parsed[0].speaker_id, 1);
    EXPECT_EQ(ds.parsed[0].text, "Li说");
    EXPECT_EQ(ds.parsed[1].speaker_id, 2);
    EXPECT_EQ(ds.parsed[1].text, "Wang说");
    EXPECT_EQ(ds.parsed[2].speaker_id, 3);
    EXPECT_EQ(ds.parsed[2].text, "Zhao说");  // xx 终止符已剥离，无残留
}

// 模拟真实播放：每条对话首次推进整页全显、再次推进跳下条；末条再次跳置结束。
TEST(DialoguePlayback, AdvanceWalksAllLinesThenEnds) {
    GameData gd;
    set_people_name(gd, 1, "Li");
    set_people_name(gd, 2, "Wang");
    set_people_name(gd, 3, "Zhao");

    DialogueState ds;
    start_dialogue(ds, {"01Li说\n02Wang说\n03Zhao说xx"}, GameMode::BATTLE);

    int advances = 0;
    while (ds.active && advances < 100) {
        advance_dialogue(ds, gd);
        ++advances;
    }
    EXPECT_FALSE(ds.active);
    // 3 行 × 2 次 advance（整页全显 + 跳下条）；末行跳下条即越界结束。
    EXPECT_EQ(advances, 6);
}

// 显示串必须把说话人名（People 表）前置 + 全角冒号，且 GG 令牌展开为
// 逐字包 AA 的主角名（着红 0xb7），无残留裸令牌。
TEST(DialoguePlayback, DisplayPrefixesSpeakerNameAndWrapsGG) {
    GameData gd;
    gd.protagonist_name = "Hero";  // 主角命名（GG 替换值）
    set_people_name(gd, 1, "Li");

    DialogueState ds;
    start_dialogue(ds, {"00GGhello"}, GameMode::BATTLE);  // 说话人 0 = 主角
    ASSERT_EQ(ds.parsed.size(), 1u);

    std::string disp = build_dialogue_display(ds.parsed[0], gd);
    EXPECT_EQ(disp.find("Hero："), 0u);                      // 主角名(people[0]→protagonist)前置
    // GG→主角 Hero 逐字包 AA（着红 0xb7）："Hero" 四 ASCII 字各前置 "AA" → "AAHAAeAArAAo"。
    EXPECT_NE(disp.find("AAH"), std::string::npos);         // 首个字已包 AA（逐字配色，与原版一致）
    EXPECT_EQ(disp.find("GG"), std::string::npos);          // 令牌已展开，无残留
}

// 单条对话：首次推进整页全显（current_index 不动、仍 active），再次推进跳下条越界结束。
TEST(DialoguePlayback, SingleLineFullRevealThenAdvanceEnds) {
    GameData gd;
    set_people_name(gd, 1, "X");

    DialogueState ds;
    start_dialogue(ds, {"01AB"}, GameMode::BATTLE);  // 说话人 1，正文 "AB"
    ASSERT_EQ(ds.parsed.size(), 1u);

    advance_dialogue(ds, gd);  // 首次：整页全显
    EXPECT_TRUE(ds.active);
    EXPECT_EQ(ds.current_index, 0);

    advance_dialogue(ds, gd);  // 再次：跳下条 → 越界 → 结束
    EXPECT_FALSE(ds.active);
}
