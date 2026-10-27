// test_txt_bulk.cpp — Phase 6 A 路：TXT.DAT 批量结构与二进制解码器分叉闸门
//
// 铁律零实证：FUN_0043c9ec（唯一 TXT.DAT 解析器，FUN_004246ab.c:59 单点调用）逐字节逻辑：
//   - 读字节，若 == 0x4b（原始，XOR 前）即 LAB_0043cc84 收尾 return（FUN_0043c9ec.c:43）
//   - 否则 XOR 0x51 解码；遇 0x0d/0x0a 处理换行；抽 AA/BB/CC/DD/GG/TT 令牌
//     写入 DAT_00472588[0x280] = 640 项指针表（replica_stubs.c:27374）
//   → 二进制把 TXT.DAT 当作「单条令牌流」（0x4b 终止、640 令牌指针），
//     而非干净层 load_dialogues 的「593 条 NNN 前缀」模型。
//   RAW 实测：TXT.DAT 110900B 中原始 0x4b 仅 2 处 → 二进制无 593 分隔语义。
//
// 干净层（Tier-B 重构）：XOR 0x51 + 「NNN 」/「-1 」行首聚合 = 593 条
//   （test_dialogue.cpp 已锁：MatchesBinaryBruteExtraction / EmbeddedNewlinePreserved）。
// 本测试把「二进制单流」与「干净层 593」的结构性分叉显式化，不假绿。

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <cstdint>

#include "app/resource/dat_loader.hpp"
#include "core/data_model/constants.hpp"

namespace {
const char* kGameDataDir = "C:/seesaw/source/dlz_research/DLZFULL";

std::vector<uint8_t> read_file(const std::string& path) {
  std::ifstream f(path, std::ios::binary);
  return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)),
                              std::istreambuf_iterator<char>());
}
}  // namespace

// 干净层 load_dialogues 应产出 593 条（NNN 前缀聚合，XOR 0x51 解码后）。
TEST(TxtBulk, CleanLayerProduces593ViaNnnPrefix) {
  ASSERT_TRUE(std::filesystem::exists(kGameDataDir));
  dlzr::app::DatLoader loader(kGameDataDir);
  auto dlg = loader.load_dialogues();
  EXPECT_EQ(dlg.size(), 593u)
      << "干净层 load_dialogues 应产出 593 条（NNN 前缀聚合）";
}

// 二进制解码器是「单条令牌流」而非「593 条分隔模型」：
// 原始 0x4b（FUN_0043c9ec.c:43 的 XOR 前终止符）在 TXT.DAT 中仅 2 处，
// 远小于 592（593 条需 592 分隔），证明二进制无 593 分隔语义。
TEST(TxtBulk, BinaryDecoderIsSingleTokenStreamNot593Model) {
  auto buf = read_file(std::string(kGameDataDir) + "/TXT.DAT");
  ASSERT_GT(buf.size(), 0u);
  int cnt_4b = 0;
  for (uint8_t b : buf)
    if (b == 0x4b) ++cnt_4b;
  // 二进制无 593 分隔：0x4b 计数应远小于 592，且绝不等于 592。
  EXPECT_LT(cnt_4b, 100) << "原始 0x4b 计数应很小（二进制非 593 分隔模型）";
  EXPECT_NE(cnt_4b, 592) << "二进制 FUN_0043c9ec 无 593 分隔语义，与干净层 593 模型分叉";
}

// 干净层 XOR 密钥必须 == 0x51（FUN_0043c9ec.c:44 local_28 = local_28 ^ 0x51）。
TEST(TxtBulk, XorKeyMatchesBinary) {
  EXPECT_EQ(dlzr::core::TXT_XOR_KEY, 0x51u);
}
