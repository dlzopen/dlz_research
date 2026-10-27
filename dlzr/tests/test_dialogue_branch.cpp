// test_dialogue_branch.cpp — Phase 6 对话系统 ②线（数据层组表对拍）保真闸门
//
// 铁律零定位：对话"分支"在二进制里由事件 FSM（FUN_0043c2dd 操作码大 switch +
// DAT_007d0cf0/DAT_007db4e8 跳转寄存器）驱动，属 Tier-B 内部调度，不在数据层。
// 干净层 DialogueManager 用**数据驱动分组模型**（fight_txt → group → SubA/SubB）
// 复现"播哪几句、顺序"这一可观测行为；组表与 fight_txt 映射表均有 PE 字节级实证
// （见下方各测试）。
//
// 重要勘误：早期版本曾在 DialogueManager 内放置 kConditionalBranches 条件分支表 +
// resolve_conditional_dialogue / get_chain_continuation，并引 FUN_00406CE8 /
// FUN_00406F96 为"依据"。经二进制复查，FUN_00406CE8 实为游戏流程初始化函数、
// FUN_00406F96 仅设置 DAT_0057bd68/_DAT_0095eb14 标志，二者均与对话分支无关；
// 该条件分支层无任何二进制依据，已按铁律零删除（不伪造）。故本测试只锁定真实
// 数据层不变量，不再为虚构行为盖绿章。
//
// 本测试锁定的可证不变量：
//   (A) FIGHT.DAT 真实出现的 fight_txt 全部落在"原版实测"16 项表内
//       {0x22,0x23,0x24,0x26,0x28,0x29,0x2A,0x2C,0x2E,0x31,0x32,0x33,0x35,
//        0x2B,0x00,0x63}（dialogue_manager.hpp:79-80 标注"原版实测 16 项"）。
//       这是数据层交叉校验：真实战斗内容 ⊆ 映射表，表若残缺会立刻暴露。
//   (B) 诚实记录 gap：dialogue_map.bin / dialogue_groups.bin 未随 DLZFULL 部署
//       → init_dialogue_groups 留空表 → get_dialogue_group 恒返 0 →
//       条件分支(kConditionalBranches)/链续逻辑当前为死代码，且 .hpp:86 自承
//       "早期分析推断，尚未逐字节与原版核对（TODO）"。此测试将其显式 FIXTURE 化，
//       不假绿、不掩盖。
//
// 资源目录硬指 DLZFULL（与 test_dialogue.cpp 一致）。

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <set>
#include <vector>
#include <array>
#include <algorithm>

#include "app/resource/dat_loader.hpp"
#include "logic/dialogue/dialogue_manager.hpp"
#include "core/data_model/types.hpp"

namespace {
const char* kGameDataDir = "C:/seesaw/source/dlz_research/DLZFULL";

// dialogue_manager.hpp:79-80 标注"原版实测 16 项"的 fight_txt 映射表。
const int32_t kDocumentedFightTxtTable[16] = {
    0x22, 0x23, 0x24, 0x26, 0x28, 0x29, 0x2A, 0x2C,
    0x2E, 0x31, 0x32, 0x33, 0x35, 0x2B, 0x00, 0x63};
}  // namespace

// (A) 真实战斗内容 ⊆ 实测映射表：任何 fight 引用的 fight_txt 必须在该 16 项内（或 0=无对话）。
TEST(DialogueBranch, FightTxtTableCoversAllFights) {
  ASSERT_TRUE(std::filesystem::exists(kGameDataDir));
  dlzr::app::DatLoader loader(kGameDataDir);

  std::set<int32_t> documented;
  for (int32_t v : kDocumentedFightTxtTable) documented.insert(v);

  std::set<int32_t> observed;
  for (int j = 0; j < dlzr::core::MAX_FIGHTS; ++j) {
    dlzr::core::FightData fight;
    try {
      fight = loader.load_fight(j);
    } catch (const std::exception&) {
      continue;  // 该关无数据
    }
    if (fight.intro.fight_txt != 0) observed.insert(fight.intro.fight_txt);
  }

  ASSERT_FALSE(observed.empty()) << "应从 FIGHT.DAT 观测到至少一个非零 fight_txt";

  for (int32_t v : observed) {
    EXPECT_TRUE(documented.count(v))
        << "fight_txt=0x" << std::hex << v << std::dec
        << " 出现在 FIGHT.DAT 但不在实测 16 项映射表内——表残缺或取值有误";
  }

  // get_dialogue_group 对所有观测值必须落在合法组 [0, DIALOGUE_GROUP_COUNT)。
  dlzr::logic::DialogueManager mgr;
  for (int32_t v : observed) {
    int32_t g = mgr.get_dialogue_group(v);
    EXPECT_GE(g, 0);
    EXPECT_LT(g, dlzr::core::DIALOGUE_GROUP_COUNT);
  }
}

// (B) 部署数据已就位（dlzr/src/app/resource/data/*.bin 由 PE 字节级抽取，与
//     DLZFULL/DLZ.exe @0x46320(映射表)/@0x43c30(组表) 逐字节一致）。本测试把原先
//     的"未部署 gap"升级为真实对拍闸门：加载真实组表后，get_dialogue_group 必须按
//     PE 映射表的 (index+2)%8 返回组号，且加载出的 8 组 SubA/SubB 必须与 PE 字节级一致。
//   诚实残留（铁律零）：kConditionalBranches 条件分支表仍为"早期分析推断，尚未逐字节
//     与原版核对（TODO，见 dialogue_manager.hpp:86）"，本测试不断言其具体映射值，
//     仅确认 resolve_conditional_dialogue 在真实组表下确定性可调用、不崩溃。
TEST(DialogueBranch, DialogueGroupTablesMatchPE) {
  ASSERT_TRUE(std::filesystem::exists(kGameDataDir));
  dlzr::logic::DialogueManager mgr;
  mgr.init_dialogue_groups(kGameDataDir);  // DLZFULL 现已随附 *.bin

  // (1) 加载出的组表非全空：至少组 0 有真实 SubA 序列（PE @0x43c30）。
  bool any_nonempty = false;
  for (int g = 0; g < dlzr::core::DIALOGUE_GROUP_COUNT; ++g) {
    const auto& sa = mgr.get_sub_a(g);
    if (std::any_of(sa.begin(), sa.end(), [](int32_t v) { return v != 0; })) {
      any_nonempty = true;
      break;
    }
  }
  ASSERT_TRUE(any_nonempty) << "组表应从 .bin 加载到真实对话序列（PE @0x43c30）";

  // (2) get_dialogue_group 与 PE 映射表一致：0x22 在 PE 表中 index=0 → (0+2)%8=2。
  EXPECT_EQ(mgr.get_dialogue_group(0x22), (0 + 2) % dlzr::core::DIALOGUE_GROUP_COUNT)
      << "fight_txt=0x22 在 PE 映射表 index=0 → 组 (0+2)%8=2";

  // (3) 字节级交叉校验：加载的 8 组 SubA/SubB 与 DLZ.exe @0x43c30 完全一致。
  const std::string pe_path = std::string(kGameDataDir) + "/DLZ.exe";
  std::ifstream f(pe_path, std::ios::binary);
  ASSERT_TRUE(f) << pe_path;
  std::vector<uint8_t> pe((std::istreambuf_iterator<char>(f)),
                          std::istreambuf_iterator<char>());
  const size_t goff = 0x43c30;
  for (int g = 0; g < dlzr::core::DIALOGUE_GROUP_COUNT; ++g) {
    const auto& sa = mgr.get_sub_a(g);
    const auto& sb = mgr.get_sub_b(g);
    for (int k = 0; k < 16; ++k) {
      int32_t pe_sa = *reinterpret_cast<const int32_t*>(&pe[goff + (g * 32 + k) * 4]);
      int32_t pe_sb = *reinterpret_cast<const int32_t*>(
          &pe[goff + (g * 32 + 16 + k) * 4]);
      EXPECT_EQ(sa[k], pe_sa) << "组" << g << " SubA[" << k << "] 与 PE 不符";
      EXPECT_EQ(sb[k], pe_sb) << "组" << g << " SubB[" << k << "] 与 PE 不符";
    }
  }

  // (4) 对话条件分支层（原 kConditionalBranches + resolve_conditional_dialogue +
  //     get_chain_continuation）经二进制复查确认无依据：原版对话分支由事件 FSM
  //     （FUN_0043c2dd 操作码大 switch + DAT_007d0cf0/DAT_007db4e8 跳转寄存器）驱动，
  //     而非数据层条件分支表；之前引为"依据"的 FUN_00406CE8（游戏流程初始化）/
  //     FUN_00406F96（设置 DAT_0057bd68/_DAT_0095eb14 标志）均与对话分支无关。
  //     铁律零要求不伪造：该层已从干净层删除，本测试不再为其盖"不崩溃"绿章。
  //     真实可观测行为（播哪几句、顺序）由 resolve_fight_dialogue / _b 保证，
  //     其组表已在本测试的 (2)(3) 段做字节级对拍。
  SUCCEED() << "条件分支伪造层已移除；真实组表对拍见 (2)(3)";
}

namespace {
// 最小 PE 解析：RVA → 文件偏移。SizeOfOptionalHeader 须从 PE 头动态读取（PE32=0xE0，非 0x60）。
bool rva_to_fileoffset(const std::vector<uint8_t>& pe, uint32_t rva, size_t& out_off) {
  if (pe.size() < 0x40) return false;
  uint32_t e_lfanew = *reinterpret_cast<const uint32_t*>(&pe[0x3c]);
  if (e_lfanew + 0x18 + 2 >= pe.size()) return false;
  uint16_t opt_size = *reinterpret_cast<const uint16_t*>(&pe[e_lfanew + 0x14]);
  uint16_t num_sections = *reinterpret_cast<const uint16_t*>(&pe[e_lfanew + 0x06]);
  size_t sect_hdr = e_lfanew + 0x18 + opt_size;
  for (uint16_t i = 0; i < num_sections; ++i) {
    size_t h = sect_hdr + static_cast<size_t>(i) * 0x28;
    if (h + 0x28 > pe.size()) return false;
    uint32_t vaddr = *reinterpret_cast<const uint32_t*>(&pe[h + 0x0c]);
    uint32_t vsize = *reinterpret_cast<const uint32_t*>(&pe[h + 0x08]);
    uint32_t rawoff = *reinterpret_cast<const uint32_t*>(&pe[h + 0x14]);
    if (rva >= vaddr && rva < vaddr + vsize) {
      out_off = rawoff + (rva - vaddr);
      return true;
    }
  }
  return false;
}

// 按内容字节模式（LE uint32）在 PE 中定位 16 项 fight_txt 表，返回文件偏移。
// 这是字节级实证的真值源：不依赖任何文档地址注释。
bool find_fight_txt_table(const std::vector<uint8_t>& pe, size_t& out_off) {
  const int32_t tbl[16] = {0x22, 0x23, 0x24, 0x26, 0x28, 0x29, 0x2A, 0x2C,
                           0x2E, 0x31, 0x32, 0x33, 0x35, 0x2B, 0x00, 0x63};
  std::vector<uint8_t> pat(64);
  for (int i = 0; i < 16; ++i) {
    pat[i * 4 + 0] = static_cast<uint8_t>(tbl[i] & 0xFF);
    pat[i * 4 + 1] = static_cast<uint8_t>((tbl[i] >> 8) & 0xFF);
    pat[i * 4 + 2] = static_cast<uint8_t>((tbl[i] >> 16) & 0xFF);
    pat[i * 4 + 3] = static_cast<uint8_t>((tbl[i] >> 24) & 0xFF);
  }
  auto it = std::search(pe.begin(), pe.end(), pat.begin(), pat.end());
  if (it == pe.end()) return false;
  out_off = static_cast<size_t>(it - pe.begin());
  return true;
}
}  // namespace

// 铁律零实测：干净层 dialogue_manager.hpp:79-80 标注「原版实测 16 项」的 fight_txt 映射表，
// 确实以 uint32 LE 形式存在于 DLZ.EXE 的 RVA 0x46320（PE 文件层取证，非注释臆测）。
TEST(DialogueBranch, FightTxtTableBackedByPE) {
  const std::string pe_path = std::string(kGameDataDir) + "/DLZ.exe";
  ASSERT_TRUE(std::filesystem::exists(pe_path)) << pe_path;
  std::ifstream f(pe_path, std::ios::binary);
  std::vector<uint8_t> pe((std::istreambuf_iterator<char>(f)),
                          std::istreambuf_iterator<char>());
  ASSERT_GT(pe.size(), 0u);

  // 字节级实证：按内容定位 16 项表（PE 真值源，不依赖注释地址）。
  size_t foff;
  ASSERT_TRUE(find_fight_txt_table(pe, foff)) << "16 项 fight_txt 表未在 DLZ.EXE 中找到";
  // 干净层 dialogue_manager.hpp:79-80 标注"原版实测 0x46320"——作为文件偏移精确命中。
  EXPECT_EQ(foff, 0x46320u)
      << "表应位于文件偏移 0x46320（与干净层文档地址一致）；其对应 RVA=0x48720"
         "（PE32 .data 节 vaddr=0x46000/raw=0x43C00，SizeOfOptionalHeader=0xE0）。"
         "若注释把 0x46320 当作 RVA 则偏差 0x2400，属文档 erratum，数据本身正确。";
  ASSERT_GE(pe.size(), foff + 16 * 4);

  std::array<int32_t, 16> pe_table{};
  for (int i = 0; i < 16; ++i)
    pe_table[i] = *reinterpret_cast<const int32_t*>(&pe[foff + i * 4]);

  for (int i = 0; i < 16; ++i)
    EXPECT_EQ(pe_table[i], kDocumentedFightTxtTable[i])
        << "PE 第" << i << "项与干净层文档表不符";

  // 交叉核验：转换器应能把真实 RVA 0x48720 映射回文件 0x46320（自洽）。
  size_t back;
  ASSERT_TRUE(rva_to_fileoffset(pe, 0x48720, back));
  EXPECT_EQ(back, foff);
}

// 用 PE 实测表驱动：47 关真实 fight_txt 全部落于 PE 表内，且 (index+2)%8 恒为合法组 [0,7]。
// 这把 A 路 FightTxtTableCoversAllFights 的数据层校验升级为「PE 字节级实证」。
TEST(DialogueBranch, FightTxtToGroupFromPEValid) {
  const std::string pe_path = std::string(kGameDataDir) + "/DLZ.exe";
  std::ifstream f(pe_path, std::ios::binary);
  std::vector<uint8_t> pe((std::istreambuf_iterator<char>(f)),
                          std::istreambuf_iterator<char>());
  size_t off;
  ASSERT_TRUE(find_fight_txt_table(pe, off));
  std::array<int32_t, 16> pe_table{};
  for (int i = 0; i < 16; ++i)
    pe_table[i] = *reinterpret_cast<const int32_t*>(&pe[off + i * 4]);

  dlzr::app::DatLoader loader(kGameDataDir);
  int checked = 0;
  for (int j = 0; j < dlzr::core::MAX_FIGHTS; ++j) {
    dlzr::core::FightData fight;
    try {
      fight = loader.load_fight(j);
    } catch (const std::exception&) {
      continue;
    }
    int32_t ft = fight.intro.fight_txt;
    if (ft == 0) continue;
    auto it = std::find(pe_table.begin(), pe_table.end(), ft);
    ASSERT_NE(it, pe_table.end())
        << "fight#" << j << " fight_txt=0x" << std::hex << ft
        << " 不在 PE 0x46320 表内";
    int idx = static_cast<int>(it - pe_table.begin());
    int grp = (idx + 2) % 8;
    EXPECT_GE(grp, 0);
    EXPECT_LT(grp, dlzr::core::DIALOGUE_GROUP_COUNT);
    ++checked;
  }
  EXPECT_GT(checked, 0) << "应至少校验一个非零 fight_txt";
}
