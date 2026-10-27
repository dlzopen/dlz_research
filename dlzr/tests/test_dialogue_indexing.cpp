// test_dialogue_indexing.cpp — A 线收尾：闭合「497 token 池 vs 593 行」的索引关系
//
// 铁律零实证（全部指坐标，未靠猜）：
//   - 二进制装载器 FUN_0043c9ec（output/ghidra_all/FUN_0043c9ec.c）产 497 个
//     token-body，索引进 DAT_0047258c[0..496]（控制码之间的子文本：跳过 4 字节
//     控制码、遇 'x'(0x78) 终止，见 .c:52-65）。
//   - 干净层 DatLoader::load_dialogues()（dat_loader.cpp:310）产 593 行（NNN 行号
//     前缀聚合的整行，含控制码 AA/BB/CC/DD/GG/TT）。
//   - 两者是**不同粒度**的数据结构：二进制 token = 控制码间子文本；干净层行 =
//     含控制码的整行。并非同一索引空间的别名（实测 0/29 下标处文本相等）。
//   - dialogue_groups.bin / dialogue_map.bin（原版实测抽取，resource/data/）给出
//     sub_a/sub_b entry_id，实测区间 [1,72]，同时落在 497 与 593 内 → 干净层
//     dialogues_[entry_id] 无越界。
//
// 阻塞（铁律零：无证据即不落地）：二进制消费者（如何按 entry_id 索引
// DAT_0047258c[k] 拼接渲染）不在已反编译集合内——ghidra_all 与 ghidra_processed
// 全搜 DAT_0047258c 仅有 FUN_0043c9ec.c:55 一处写入，无任何读取者。故「entry_id →
// 二进制 token」的规范映射暂不可证，本测试显式化该 gap，不假绿。
//
// 既有 test_txt_loader.cpp 锁「497 vs 593 计数分叉」；本测试进一步锁「索引空间」：
// 既防 entry_id 越界（真实 PASS 闸门），又把粒度分叉固化为可证伪不变量。

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <set>
#include <string>
#include <algorithm>
#include <cstdint>
#include <cctype>

#include "app/resource/dat_loader.hpp"
#include "logic/dialogue/dialogue_manager.hpp"

namespace {
const char* kGameDataDir = "C:/seesaw/source/dlz_research/DLZFULL";
const char* kDataDir =
    "C:/seesaw/source/dlz_research/dlzr/src/app/resource/data";

// 从 DLZ.EXE 抽取字形属性表 PTR_DAT_004487c0（VA 0x4487c0 → 文件偏移 0x463c0，
// 经 PE 节解析）。FUN_0043e890(c) = table[c*2] & 4；bit-4 置位字符 = token 起点。
// 与 test_txt_loader.cpp 同源证据：返回集 = {0x01,0x03,'5'..'9',':',';','<','=','>'}。
bool extract_token_starts(std::set<int>& out) {
  std::ifstream f(std::string(kGameDataDir) + "/DLZ.exe", std::ios::binary);
  if (!f) return false;
  std::vector<uint8_t> pe((std::istreambuf_iterator<char>(f)),
                           std::istreambuf_iterator<char>());
  if (pe.size() < 0x40) return false;
  uint32_t e_lfanew = *reinterpret_cast<const uint32_t*>(&pe[0x3c]);
  uint16_t opt = *reinterpret_cast<const uint16_t*>(&pe[e_lfanew + 0x14]);
  uint16_t nsec = *reinterpret_cast<const uint16_t*>(&pe[e_lfanew + 0x06]);
  size_t sect = static_cast<size_t>(e_lfanew) + 0x18 + opt;
  uint32_t rva = 0x487c0;
  size_t fo = 0;
  bool found = false;
  for (uint16_t i = 0; i < nsec; ++i) {
    size_t h = sect + static_cast<size_t>(i) * 0x28;
    uint32_t vaddr = *reinterpret_cast<const uint32_t*>(&pe[h + 0x0c]);
    uint32_t vsize = *reinterpret_cast<const uint32_t*>(&pe[h + 0x08]);
    uint32_t raw = *reinterpret_cast<const uint32_t*>(&pe[h + 0x14]);
    if (rva >= vaddr && rva < vaddr + vsize) {
      fo = raw + (rva - vaddr);
      found = true;
      break;
    }
  }
  if (!found || fo + 512 > pe.size()) return false;
  for (int c = 0; c < 256; ++c)
    if (pe[fo + c * 2] & 4) out.insert(c);
  return true;
}

// 忠实重演 FUN_0043c9ec 扫描器，捕获每个 token 的字符串正文（.c:52-65 的
// DAT_004569c0 累积 + DAT_0047258c 指针写入）。
std::vector<std::string> reenact_tokens(const std::vector<uint8_t>& dat,
                                         const std::set<int>& toks) {
  std::vector<std::string> tokens;
  std::string cur;
  bool bVar2 = false;  // inside-token 标志
  size_t i = 0, n = dat.size();
  while (i < n) {
    uint8_t raw = dat[i++];
    if (raw == 0x4b) break;          // EOF 哨兵（XOR 前原始字节）
    int c = raw ^ 0x51;
    if (c == 0x0d || c == 0x0a) {    // 跳过 CR/LF，并吞掉紧随的一个空格
      if (i < n && (dat[i] ^ 0x51) == 0x20) i++;
      continue;
    }
    if (c == 0xff) continue;         // 流分隔（实测全文件无）
    bool ts = toks.count(c) != 0;
    if (!ts || bVar2) {
      if (bVar2) {
        if (c == 0x78) {             // 'x' token 终止符
          tokens.push_back(cur);
          cur.clear();
          bVar2 = false;
          i++;                       // FUN_0043e860 推进 1
          continue;
        } else {
          cur += static_cast<char>(c);
          i++;
          continue;
        }
      }
      // 非 token 起点且不在 token 内：丢弃（散文被跳过）
    } else {  // ts && !bVar2：token 起点，跳过 token 码(1)+载荷(2)
      bVar2 = true;
      i += 3;
      continue;
    }
  }
  return tokens;
}

// 镜像 DatLoader::load_dialogues 的 NNN 行聚合（dat_loader.cpp:340-374）。
std::vector<std::string> clean_lines_from(const std::vector<uint8_t>& dat) {
  std::string text(dat.size(), ' ');
  for (size_t i = 0; i < dat.size(); ++i) text[i] = static_cast<char>(dat[i] ^ 0x51);
  std::vector<std::string> raw_lines;
  {
    std::string cur;
    for (char ch : text) {
      if (ch == '\n') { raw_lines.push_back(cur); cur.clear(); }
      else if (ch != '\r') cur += ch;
    }
    if (!cur.empty()) raw_lines.push_back(cur);
  }
  auto is_start = [](const std::string& s) {
    if (s.size() >= 4 && std::isdigit(static_cast<unsigned char>(s[0])) &&
        std::isdigit(static_cast<unsigned char>(s[1])) &&
        std::isdigit(static_cast<unsigned char>(s[2])) && s[3] == ' ')
      return true;
    if (s.size() >= 3 && s[0] == '-' && s[1] == '1' && s[2] == ' ') return true;
    return false;
  };
  std::vector<std::string> dlg;
  std::string entry;
  bool in_entry = false;
  for (const auto& ln : raw_lines) {
    if (is_start(ln)) {
      if (in_entry) dlg.push_back(entry);
      size_t skip = (ln.size() >= 3 && ln[0] == '-' && ln[1] == '1' && ln[2] == ' ') ? 3 : 4;
      entry = ln.substr(skip);
      in_entry = true;
    } else if (in_entry) {
      entry += '\n' + ln;
    }
  }
  if (in_entry) dlg.push_back(entry);
  return dlg;
}

std::vector<uint8_t> read_file(const std::string& p) {
  std::ifstream f(p, std::ios::binary);
  return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)),
                              std::istreambuf_iterator<char>());
}

// 读 dialogue_groups.bin（8 组 × [SubA 16u32][SubB 16u32] = 1024B），收集所有
// 非零 entry_id（这些即干净层 resolve_*_dialogue 用作 dialogues_ 下标的引用）。
std::vector<int32_t> load_all_entry_ids(const std::string& path) {
  std::ifstream f(path, std::ios::binary);
  std::vector<uint8_t> buf(1024);
  if (!f || f.read(reinterpret_cast<char*>(buf.data()), 1024).gcount() != 1024)
    return {};
  std::vector<int32_t> ids;
  for (int g = 0; g < 8; ++g) {
    const uint8_t* base = buf.data() + g * 32 * 4;
    for (int k = 0; k < 32; ++k) {
      int32_t v = *reinterpret_cast<const int32_t*>(base + k * 4);
      if (v != 0) ids.push_back(v);
    }
  }
  return ids;
}
}  // namespace

// 闸门 1（真实 PASS）：entry_id 区间同时落在二进制 497 token 池与干净层 593 行数组内，
// 干净层 dialogues_[entry_id] 索引无越界；若将来 .bin 出现 >=497 的 id，立即暴露。
TEST(DialogueIndexing, EntryIdsFitBothPools) {
  auto ids = load_all_entry_ids(std::string(kDataDir) + "/dialogue_groups.bin");
  ASSERT_FALSE(ids.empty()) << "dialogue_groups.bin 应含非零 entry_id";
  int32_t mn = *std::min_element(ids.begin(), ids.end());
  int32_t mx = *std::max_element(ids.begin(), ids.end());
  EXPECT_GE(mn, 1);
  EXPECT_LE(mx, 497) << "entry_id 必须 <= 二进制 token 池大小(497)，否则索引 DAT_0047258c 越界";
  EXPECT_LE(mx, 593) << "entry_id 必须 <= 干净层行数(593)，否则索引 dialogues_ 越界";
}

// 闸门 2（防回归 tie-together）：二进制 497 token 与干净层 593 行共存且计数分叉。
TEST(DialogueIndexing, Binary497VsClean593) {
  std::set<int> toks;
  ASSERT_TRUE(extract_token_starts(toks));
  auto dat = read_file(std::string(kGameDataDir) + "/TXT.DAT");
  ASSERT_GT(dat.size(), 0u);
  auto bt = reenact_tokens(dat, toks);
  EXPECT_EQ(bt.size(), 497u) << "FUN_0043c9ec 写入 DAT_0047258c 497 个 token 指针";

  dlzr::app::DatLoader loader(kGameDataDir);
  auto cl = loader.load_dialogues();
  EXPECT_EQ(cl.size(), 593u) << "干净层 NNN 行号聚合 593 条";
  EXPECT_NE(bt.size(), cl.size());
}

// 闸门 3（诚实分叉，非假绿）：二进制 token 池与干净层行是不同粒度的数据结构。
// 二进制 token = 控制码间子文本（FUN_0043c9ec 跳过 4 字节控制码、'x' 终止）；
// 干净层行 = NNN 整行（含控制码）。同一下标 e 处文本不同（实测 0/29 精确匹配）。
// 二进制消费者（按 entry_id 索引 DAT_0047258c 拼接渲染）不在已反编译集合内 → 阻塞；
// 此测试把"两模型粒度分叉"固化为可证伪不变量，若将来变相等则提示需重审消费者 RE。
TEST(DialogueIndexing, IndexSpacesDifferByGranularity) {
  std::set<int> toks;
  ASSERT_TRUE(extract_token_starts(toks));
  auto dat = read_file(std::string(kGameDataDir) + "/TXT.DAT");
  auto bt = reenact_tokens(dat, toks);
  dlzr::app::DatLoader loader(kGameDataDir);
  auto cl = loader.load_dialogues();

  auto ids = load_all_entry_ids(std::string(kDataDir) + "/dialogue_groups.bin");
  int checked = 0, same = 0;
  for (int32_t e : ids) {
    if (static_cast<size_t>(e) < bt.size() && static_cast<size_t>(e) < cl.size()) {
      ++checked;
      if (bt[e] == cl[e]) ++same;
    }
  }
  EXPECT_GT(checked, 0) << "应至少校验一个被引用 entry_id";
  EXPECT_EQ(same, 0) << "二进制 token[e] 与干净层整行[e] 应是不同粒度文本（实测 0 处相等）；"
                        "若此处变非零，说明两模型粒度已对齐，需重新审视二进制消费者 RE。";
}
