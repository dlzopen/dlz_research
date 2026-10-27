#pragma once

#include <string>

namespace dlzr::logic {

// 逐 GBK 字给每个字前置 marker（如 "AA"），用于把名字包成原版逐字配色格式。
// 原版 FUN_0043cc96.c:83-93：G/T 令牌展开为 "AA"+name（每 GBK 字前置 "AA"），
//   后续 FUN_004061df.c:39-42 将 "AA" 前缀字着红色(0xb7)。
std::string wrap_per_gbk_char(const std::string& s, const std::string& marker);

// 解析层→渲染层桥接的令牌替换（铁律零，逐指令实证）：
//   · AA/BB/CC/DD 配色标记原样保留，交由 dialogue_build_segments 着色
//     （FUN_004061df.c:39-52：A=0xb7/B=0x85/C=0x89/D=0x4f，其余=0x48 正文色）。
//   · GG = 主角名令牌：按原版 FUN_0043cc96.c:83-93 包成 "AA"+逐字，
//       使渲染层 dialogue_build_segments 将其着红(0xb7)；裸 GG 不会到达渲染层。
//   · TT = 战斗/地名令牌：原版 FUN_0043cc96.c:69-82 的 T 分支展开 DAT_007d01c8
//     （T 名表，运行时由场景装载，PE 抽不到），展开方式 = wrap_per_gbk_char(t_name,"AA")
//     ——与 GG 完全同构（FUN_0043cc96.c:71-82 与 :83-93 结构一致）。
//     t_name 为空（运行时尚未装载，对应原版 DAT_007d01c8 空缓冲 → :74 循环 0 次）
//     时退化为剔除，与原版行为一致；非空时按原版逐字包 AA 着红(0xb7)。
//     诚实残留：确切的运行时 T 名值（平阳战役等）需游戏流程 RE 方可落定，勿臆造。
std::string format_dialogue_text(const std::string& raw,
                                 const std::string& protagonist_name,
                                 const std::string& t_name = "");

}  // namespace dlzr::logic
