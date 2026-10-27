// test_dialogue.cpp — Phase 6 对话系统：数据装载保真 + 令牌/语义解析
//
// 验证对象（铁律零，逐字节实证）：
//   1) DatLoader::load_dialogues() 从 TXT.DAT（XOR 0x51）解码的条目，
//      必须与「独立暴力抽取器」（按 "NNN " 索引行首聚合、保留内嵌换行）逐条一致
//      （条目数 + 每条内容）。这是数据层保真差分。
//   2) DialogueManager::parse_dialogue() 对说话人代码与令牌标记（AA/BB/CC/DD/GG/TT/xx）
//      的解析语义正确（Phase 6 Tier A 令牌语义的数据层，逐字配色属渲染层待续）。
//
// 资源目录硬性指向 DLZFULL（与 test_first_level.cpp 一致），保证读到原版 TXT.DAT。

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <cctype>
#include <string>
#include <vector>

#include "app/resource/dat_loader.hpp"
#include "logic/dialogue/dialogue_manager.hpp"

namespace {
const char* kGameDataDir = "C:/seesaw/source/dlz_research/DLZFULL";
}  // namespace

// 独立暴力抽取器：复刻 TXT.DAT 解码（XOR 0x51），按 "NNN " 索引行首聚合条目（保留内嵌换行）。
// 与 DatLoader::load_dialogues 是两份独立转录，互相校验转录正确性。
static std::vector<std::string> brute_extract_dialogues(const std::string& dir) {
    std::filesystem::path p = std::filesystem::path(dir) / "TXT.DAT";
    std::ifstream f(p, std::ios::binary);
    std::vector<std::string> out;
    if (!f) return out;
    std::vector<uint8_t> raw((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    std::string text(raw.size(), ' ');
    for (size_t i = 0; i < raw.size(); i++) text[i] = (char)(raw[i] ^ 0x51);  // TXT_XOR_KEY

    // 按行切分（CRLF→LF）
    std::vector<std::string> lines;
    {
        std::string cur;
        for (char c : text) {
            if (c == '\n') { lines.push_back(cur); cur.clear(); }
            else if (c != '\r') cur += c;
        }
        if (!cur.empty()) lines.push_back(cur);
    }
    auto is_entry_start = [](const std::string& s) {
        if (s.size() >= 4 && std::isdigit((unsigned char)s[0]) &&
            std::isdigit((unsigned char)s[1]) && std::isdigit((unsigned char)s[2]) && s[3] == ' ')
            return true;
        if (s.size() >= 3 && s[0] == '-' && s[1] == '1' && s[2] == ' ')
            return true;
        return false;
    };
    std::string entry;
    bool in_entry = false;
    for (const auto& ln : lines) {
        if (is_entry_start(ln)) {
            if (in_entry) out.push_back(entry);
            size_t skip = (ln.size() >= 3 && ln[0] == '-' && ln[1] == '1' && ln[2] == ' ') ? 3 : 4;
            entry = ln.substr(skip);
            in_entry = true;
        } else if (in_entry) {
            entry += '\n' + ln;
        }
    }
    if (in_entry) out.push_back(entry);
    return out;
}

TEST(DialogueLoad, MatchesBinaryBruteExtraction) {
    ASSERT_TRUE(std::filesystem::exists(kGameDataDir));
    dlzr::app::DatLoader loader(kGameDataDir);
    auto got = loader.load_dialogues();

    auto expected = brute_extract_dialogues(kGameDataDir);
    ASSERT_EQ(got.size(), expected.size())
        << "TXT.DAT 解码条目数应与独立暴力抽取一致";
    EXPECT_GT((int)got.size(), 100) << "条目数应处于合理量级（原注释称 ~593）";

    for (size_t i = 0; i < got.size(); i++) {
        ASSERT_EQ(got[i], expected[i]) << "条目 #" << i << " 内容与独立抽取分歧";
    }
}

TEST(DialogueLoad, FirstEntryIndex000Resolves) {
    // 索引 0 对应 "000 " 开头的条目；应非空且能被 parse_dialogue 解析。
    dlzr::app::DatLoader loader(kGameDataDir);
    auto got = loader.load_dialogues();
    ASSERT_GT((int)got.size(), 0);
    auto d = dlzr::logic::DialogueManager::parse_dialogue(got[0]);
    // 首条一般带说话人代码或旁白；至少 text 不应等于原始 "NNN " 前缀残留。
    EXPECT_TRUE(d.text.find("000 ") == std::string::npos)
        << "text 不应残留 'NNN ' 索引前缀";
}

TEST(DialogueParse, SpeakerCodeAndTokens) {
    using dlzr::logic::DialogueManager;
    // 旁白：'-1' 前缀 → speaker_id = -1，正文剥离前缀
    auto n = DialogueManager::parse_dialogue("-1我军已占领阵地");
    EXPECT_EQ(n.speaker_id, -1);
    EXPECT_EQ(n.text, "我军已占领阵地");

    // 数字说话人代码 "05..." → speaker_id = 5
    auto a = DialogueManager::parse_dialogue("05友军前来支援");
    EXPECT_EQ(a.speaker_id, 5);
    EXPECT_EQ(a.text, "友军前来支援");

    // 末尾 "xx" 终止符应被剥离
    auto t = DialogueManager::parse_dialogue("Some textxx");
    EXPECT_EQ(t.speaker_id, -1);  // 无数字前缀 → 旁白
    EXPECT_EQ(t.text, "Some text");

    // 令牌标记（AA/BB/CC/DD/GG/TT）应原样保留在 text 中，由渲染层负责逐字配色
    auto tok = DialogueManager::parse_dialogue("03AA李明BB见到CC宝剑DD长安");
    EXPECT_EQ(tok.speaker_id, 3);
    EXPECT_EQ(tok.text, "AA李明BB见到CC宝剑DD长安");
}

// 多说话人条目：逐行独立解析（铁律零实证 FUN_0043cc96 逐行模型）。
// 真实 TXT.DAT 条目 007 即 "43…\n44…\n42…\n42…xx" 形态：每行独立说话人，
// 末行 "xx" 终止符须剥离，且说话人代码不得残留为可见文本（修复前会保留 "44"/"42" 等数字串）。
TEST(DialogueParse, MultiLineEntryPerLineSpeakers) {
    using dlzr::logic::DialogueManager;
    std::string entry =
        "43好象是碰上八路了？\n"
        "44我的妈呀，快跑吧，逃命要紧！\n"
        "42慌什么慌！只有一个人，咱们仨还怕奈何不了他？\n"
        "42少罗嗦，冲啊！xx";
    auto lines = DialogueManager::parse_dialogue_entry(entry);
    ASSERT_EQ(lines.size(), 4u);
    EXPECT_EQ(lines[0].speaker_id, 43);
    EXPECT_EQ(lines[0].text, "好象是碰上八路了？");
    EXPECT_EQ(lines[1].speaker_id, 44);
    EXPECT_EQ(lines[1].text, "我的妈呀，快跑吧，逃命要紧！");
    EXPECT_EQ(lines[2].speaker_id, 42);
    EXPECT_EQ(lines[2].text, "慌什么慌！只有一个人，咱们仨还怕奈何不了他？");
    EXPECT_EQ(lines[3].speaker_id, 42);
    EXPECT_EQ(lines[3].text, "少罗嗦，冲啊！");  // "xx" 已剥离

    // 说话人代码不得残留为可见文本（multiline 修复前会保留 "44"/"42" 数字串）。
    for (const auto& d : lines) {
        EXPECT_EQ(d.text.find("44"), std::string::npos);
        EXPECT_EQ(d.text.find("42"), std::string::npos);
        EXPECT_EQ(d.text.find("xx"), std::string::npos);
    }
}

// 旁白多行（"-1 " 起头、全角空格续行）：逐行均为旁白，正文完整保留、xx 剥离。
TEST(DialogueParse, MultiLineNarration) {
    using dlzr::logic::DialogueManager;
    std::string entry = "-1　　出师未捷身先死，\n　　　常使英雄泪满襟。xx";
    auto lines = DialogueManager::parse_dialogue_entry(entry);
    ASSERT_GE(lines.size(), 2u);
    for (const auto& d : lines) {
        EXPECT_EQ(d.speaker_id, -1);
        EXPECT_EQ(d.text.find("xx"), std::string::npos);
    }
    EXPECT_NE(lines[0].text.find("出师未捷身先死"), std::string::npos);
    EXPECT_NE(lines[1].text.find("常使英雄泪满襟"), std::string::npos);
}

// 空条目（仅 "xx"）：解析为空序列，播放层应跳过。
TEST(DialogueParse, EmptyEntrySkipped) {
    using dlzr::logic::DialogueManager;
    EXPECT_TRUE(DialogueManager::parse_dialogue_entry("xx").empty());
    // 内嵌换行但每行都仅 "xx" 也应全空。
    EXPECT_TRUE(DialogueManager::parse_dialogue_entry("xx\nxx").empty());
}

// 条目 004 正文含内嵌折行（"...壮烈殉" + 换行 + "国，年仅２４岁。"）。
// 早期按 "\r\n" 切分的实现会截断续行，导致正文丢失；铁律零修复后必须保留内嵌换行。
TEST(DialogueLoad, EmbeddedNewlinePreserved) {
    ASSERT_TRUE(std::filesystem::exists(kGameDataDir));
    dlzr::app::DatLoader loader(kGameDataDir);
    auto got = loader.load_dialogues();
    ASSERT_GT((int)got.size(), 4);

    // 铁律零修复验证：条目 004 正文含内嵌折行（视觉换行），早期按 "\r\n" 切分的实现会在此处截断。
    // 以下断言均为编码无关（loaded 文本为 GBK 字节流，不依赖具体字符编码）：
    //   1) 条目总数应为 593（"NNN " 行首数）；
    //   2) 条目 004 必须含内嵌 '\n'（换行被保留，而非在首个 CRLF 处截断）；
    //   3) 条目 004 长度应远大于首行（证明续行文本未被丢弃）。
    EXPECT_EQ((int)got.size(), 593) << "TXT.DAT 解码条目数应为 593";

    EXPECT_NE(got[4].find("\n"), std::string::npos)
        << "条目 004 应保留内嵌换行符（早期 \\r\\n 切分实现会截断续行）";

    EXPECT_GT((int)got[4].size(), 120)
        << "条目 004 应保留完整续行文本（长度 >> 首行），证明未按 \\r\\n 截断";

    auto d = dlzr::logic::DialogueManager::parse_dialogue(got[4]);
    EXPECT_EQ(d.speaker_id, -1) << "条目 004 说话人代码应为旁白 -1";
    EXPECT_GT((int)d.text.size(), 100)
        << "parse_dialogue 后正文仍应保留完整续行文本";
}
