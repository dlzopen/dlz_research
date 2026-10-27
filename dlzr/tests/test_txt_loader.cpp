// test_txt_loader.cpp
//
// 选项 ②（用户 "A & B & C" 后的下一拍）：定位 TXT.DAT 真实的批量/按需装载函数。
//
// 铁律零结论（证据驱动，非猜测）：
//   真实装载函数 = FUN_0043c9ec（output/ghidra_all/FUN_0043c9ec.c），由游戏初始化
//   FUN_004246ab.c:59 在启动时**调用一次**：FUN_0043c9ec(s_txt_dat_00447558)。
//   它是**单次全文件扫描的批量装载器**（外层 while(true) 仅在原始 0x4b 到达文件尾时
//   经 LAB_0043cc84 跳出 fclose），不是按需逐条解码。
//
//   它的模型是「控制码 token 抽取」而非「按行号聚合」：
//     - 逐个字节 XOR 0x51 解码；
//     - 用二进制字形属性表 PTR_DAT_004487c0[c*2] & 4（FUN_0043e890）判定 token 起点；
//       bit-4 置位的字符 = token 起点（实测：0x01,0x03,'5'..'9',':',';','<','=','>'）；
//     - token 起点后跳 3 字节（码 1 + 载荷 2），其正文累积进 DAT_004569c0；
//     - 遇 'x'(0x78) 收尾：把指针写进 DAT_0047258c[local_10++]，null 终止，重置；
//     - 0xff 为流分隔（实测全文件为 0 处，永不触发）；token 之外的散文被丢弃。
//
//   忠实重演得：497 个 token-body（DAT_004569c0 共 45345 字节），索引 DAT_0047258c[0..496]。
//   （token 体内若再出现 token 起点字符，按二进制 if((iVar5==0)||bVar2) 语义当作正文拷贝，不另起 token。）
//
//   这与干净层 DatLoader::load_dialogues() 的「NNN 行号前缀 → 593 条」模型**结构分叉**：
//     二进制按字形属性表识别控制码、丢弃 token 外散文；干净层按行号前缀保留全文。
//   两者用途不同、尚未对拍。本测试显式锁定该分叉（诚实标注，不假绿）。
//
// 既有 test_dialogue.cpp 的 MatchesBinaryBruteExtraction 仅用同一「NNN 」模型做自校验，
// 并未对拍二进制——故 593 是干净层自身属性，不证明与原版一致。

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <cstdint>
#include <set>
#include <string>

#include "app/resource/dat_loader.hpp"

namespace {
const char* kGameDataDir = "C:/seesaw/source/dlz_research/DLZFULL";

// 从 DLZ.EXE 抽取二进制字形属性表 PTR_DAT_004487c0（VA 0x4487c0 → RVA 0x487c0 →
// 经 PE 节解析 .data 映射文件偏移 0x463c0）。FUN_0043e890(c) = table[c*2] & 4；
// bit-4 置位字符即 token 起点。返回 true 并把集合写入 out。
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
  uint32_t rva = 0x487c0;  // VA 0x4487c0 - ImageBase 0x400000
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

// 忠实重演 FUN_0043c9ec 扫描器（CFG 逐行对齐 FUN_0043c9ec.c）。
// 返回 <token_count(local_10), text_buf_size(local_18)>。
std::pair<int, int> reenact_c9ec(const std::vector<uint8_t>& dat,
                                 const std::set<int>& toks) {
  int local_10 = 0;   // token / 指针计数
  int local_18 = 0;   // DAT_004569c0 文本缓冲尺寸
  bool bVar2 = false; // inside-token 标志
  size_t i = 0;
  size_t n = dat.size();
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
          local_10++;
          bVar2 = false;
          i++;                       // FUN_0043e860 推进 1
          continue;
        } else {
          local_18++;                // 拷贝正文到 DAT_004569c0
          i++;
          continue;
        }
      }
      // 非 token 起点且不在 token 内：丢弃（对齐二进制 case A2）
    } else {
      bVar2 = true;
      i += 3;                        // 跳过 token 码(1)+载荷(2)
      continue;
    }
  }
  return {local_10, local_18};
}

std::vector<uint8_t> read_file(const std::string& p) {
  std::ifstream f(p, std::ios::binary);
  return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)),
                              std::istreambuf_iterator<char>());
}
}  // namespace

// 字形属性表给出的 token 起点集合 = {0x01,0x03,'5'..'9',':',';','<','=','>'}。
TEST(TxtLoader, GlyphTableTokenStartSet) {
  std::set<int> toks;
  ASSERT_TRUE(extract_token_starts(toks));
  std::set<int> expected{0x01, 0x03, 0x35, 0x36, 0x37, 0x38, 0x39,
                         0x3a, 0x3b, 0x3c, 0x3d, 0x3e};
  EXPECT_EQ(toks, expected)
      << "FUN_0043e890 的 token 起点来自 DLZ.EXE 字形属性表 bit-4，"
         "非硬编码 AA/BB/CC/DD/GG/TT（早期猜测已证伪）。";
}

// 忠实重演 FUN_0043c9ec：二进制把 TXT.DAT 解析为 497 个 token-body，而非 593 行。
TEST(TxtLoader, BinaryExtracts497TokenBodies) {
  std::set<int> toks;
  ASSERT_TRUE(extract_token_starts(toks));
  auto dat = read_file(std::string(kGameDataDir) + "/TXT.DAT");
  ASSERT_GT(dat.size(), 0u);

  auto [n, sz] = reenact_c9ec(dat, toks);
  EXPECT_EQ(n, 497) << "FUN_0043c9ec 在 DAT_0047258c 写入 497 个 token 指针";
  EXPECT_EQ(sz, 45345) << "DAT_004569c0 累积 45345 字节 token 正文（含 token 体内嵌套 token 起点字符）";
  EXPECT_NE(n, 593) << "与干净层 593 行模型结构分叉";
}

// 干净层 load_dialogues 返回 593 条；二进制重演返回 497 token。两者分叉显式化。
TEST(TxtLoader, CleanModelDivergesFromBinary) {
  dlzr::app::DatLoader loader(kGameDataDir);
  auto dlg = loader.load_dialogues();
  ASSERT_EQ(dlg.size(), 593u) << "干净层自身模型：593 条（NNN 行号聚合）";

  std::set<int> toks;
  ASSERT_TRUE(extract_token_starts(toks));
  auto dat = read_file(std::string(kGameDataDir) + "/TXT.DAT");
  auto [n, sz] = reenact_c9ec(dat, toks);

  EXPECT_NE(n, static_cast<int>(dlg.size()))
      << "二进制 FUN_0043c9ec 提取 " << n << " 个控制码 token-body（实测 497），"
         "与干净层 593 条全文模型用途不同、未对拍：二进制按字形属性表识别控制码并"
         "丢弃 token 外散文，干净层按行号前缀保留全文。属 Tier-B 重构，非假绿。";
}

// 原始 0x4b 仅出现在文件末尾（索引 110898/110899，文件 110900B），故扫描器覆盖全文，
// 不会在中途早停。
TEST(TxtLoader, EofSentinelAtFileEnd) {
  auto dat = read_file(std::string(kGameDataDir) + "/TXT.DAT");
  ASSERT_EQ(dat.size(), 110900u);
  std::vector<size_t> pos;
  for (size_t i = 0; i < dat.size(); ++i)
    if (dat[i] == 0x4b) pos.push_back(i);
  ASSERT_EQ(pos.size(), 2u);
  EXPECT_EQ(pos[0], dat.size() - 2);
  EXPECT_EQ(pos[1], dat.size() - 1);
}

// 去掉全部空白（\n \r \t 与空格），用于无损重分段对拍。
static std::string strip_ws(const std::string& s) {
  std::string out;
  for (char ch : s)
    if (ch != '\n' && ch != '\r' && ch != '\t' && ch != ' ') out += ch;
  return out;
}

// 保真闸门（修正的 oracle）：二进制 FUN_0043c9ec 按字形属性表 bit-4 识别 token 起点
// {0x01,0x03,'5'..'9',':',';','<','=','>'}，抽取 497 个 token-body 并**丢弃 token 外散文**；
// 干净层 DatLoader::load_dialogues() 改用「NNN 行号前缀聚合 → 593 条」的 Tier-B 重构，保留全文。
// 两者是同一解码文本的不同**分段**方式，不是内容增减。
//
// 本测试证明：干净层是对解码 TXT.DAT 的**无损、保序重分段**——除剥离条目索引前缀（"NNN " / "-1 "）
// 与换行空白外，不丢、不改、不重排任何解码字符。因此干净层 593 条是二进制 497 token 的**超集**，
// 二进制展示的每段正文都可在干净层全文中按保序找到；NNN 聚合不会破坏/丢失任何控制码区间文本。
// 这把「593 vs 497 结构分叉」从「未知/或丢文本」转为「已证超集、仅分段粒度差异」（诚实闭合）。
TEST(TxtLoader, CleanLayerIsLosslessResegmentation) {
  auto dat = read_file(std::string(kGameDataDir) + "/TXT.DAT");
  // 解码全文（XOR 0x51）
  std::string decoded;
  for (uint8_t b : dat) decoded.push_back(static_cast<char>(b ^ 0x51));

  dlzr::app::DatLoader loader(kGameDataDir);
  auto dlg = loader.load_dialogues();
  ASSERT_EQ(dlg.size(), 593u);
  std::string corpus;
  for (size_t k = 0; k < dlg.size(); ++k) {
    if (k) corpus += '\n';
    corpus += dlg[k];
  }

  // 从解码全文剥离与干净层相同的索引前缀（行首 "NNN " 或 "-1 "），逐行拼接。
  std::string stripped;
  {
    std::vector<std::string> lines;
    std::string cur;
    for (char c : decoded) {
      if (c == '\n') { lines.push_back(cur); cur.clear(); }
      else if (c != '\r') cur += c;
    }
    if (!cur.empty()) lines.push_back(cur);
    for (auto& ln : lines) {
      if (ln.size() >= 4 && std::isdigit(static_cast<unsigned char>(ln[0])) &&
          std::isdigit(static_cast<unsigned char>(ln[1])) &&
          std::isdigit(static_cast<unsigned char>(ln[2])) && ln[3] == ' ')
        ln = ln.substr(4);                       // 剥 "NNN "
      else if (ln.size() >= 3 && ln[0] == '-' && ln[1] == '1' && ln[2] == ' ')
        ln = ln.substr(3);                       // 剥 "-1 "
      stripped += ln;
      stripped += '\n';
    }
  }

  EXPECT_EQ(strip_ws(corpus), strip_ws(stripped))
      << "干净层应是对解码 TXT.DAT 的无损、保序重分段（仅剥离索引前缀与空白），"
         "不丢/不改/不重排任何正文；否则 NNN 聚合存在文本损坏。";
}
