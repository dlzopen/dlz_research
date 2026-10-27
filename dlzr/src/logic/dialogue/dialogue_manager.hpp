#pragma once

// dialogue_manager.hpp — 对话系统管理器
// Dialogue system manager
//
// 管理对话序列播放，支持8个对话组，每组有SubA和SubB两个序列。
// Manages dialogue sequence playback, supports 8 dialogue groups with SubA/SubB sequences.

#include "core/data_model/types.hpp"
#include "core/data_model/constants.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace dlzr::logic {

class DialogueManager {
public:
    DialogueManager() = default;

    // 加载对话数据 / Load dialogue data
    void load_dialogues(const std::vector<std::string>& dialogue_texts);

    // 从数据文件建立对话组索引 / Build dialogue group index from data file.
    // 文件 dialogue_groups.bin：8 组 × [SubA 16×u32][SubB 16×u32]，零填充终止。
    //   原版 EXE 组表实测位置 = 文件偏移 0x43c30（1024 字节，非文档误标的 0x446030）。
    // 文件 dialogue_map.bin：16×u32 fight_txt 映射表（原版实测 0x46320，64 字节）。
    //   与文档 0x448720 处给出的值 {0x22..0x63} 一致，已 byte-级核对。
    // engine_data_dir：含上述两个 .bin 的目录（构建时随 exe 部署）。
    void init_dialogue_groups(const std::string& engine_data_dir);

    // 获取指定对话组的SubA序列 / Get SubA sequence for a dialogue group
    const std::array<int32_t, 16>& get_sub_a(int32_t group_index) const;

    // 获取指定对话组的SubB序列 / Get SubB sequence for a dialogue group
    const std::array<int32_t, 16>& get_sub_b(int32_t group_index) const;

    // 根据fight_txt获取对话组索引 / Get dialogue group index from fight_txt
    int32_t get_dialogue_group(int32_t fight_txt) const;

    // 获取对话文本 / Get dialogue text by entry index
    const std::string& get_dialogue_text(int32_t entry_index) const;

    // 解析对话条目为「逐行」对话序列（铁律零实证，匹配二进制 FUN_0043cc96）。
    // 一个 TXT.DAT 条目（raw，已剥 "NNN " 索引前缀、保留内嵌 '\n'）由多行组成，
    // 每行独立带说话人代码（2 位数字或 "-1" 旁白）并（末行）以 "xx" 终止符结尾。
    // 二进制 FUN_0043cc96 每次调用只解析「一行」（读前 2 字符作说话人码，
    // FUN_0043e890 判数字：(c1-0x30)*10+(c2-0x30)=说话人 ID，非数字→旁白 -1），
    // 上层用 DAT_0098d810 游标逐行推进；故原版是「条目=多行、每行独立说话人」。
    // 本函数逐行 parse_dialogue_line 后返回（跳过纯空行/仅 "xx" 行），使播放层
    // 每行作为独立 Dialogue 顺序推进——忠实于 binary 逐行模型。
    static std::vector<core::Dialogue> parse_dialogue_entry(const std::string& raw);

    // 向后兼容薄包装：返回 parse_dialogue_entry 的首条非空行 Dialogue。
    // 历史调用方（单测、结局文本取首行等）仍按「单 Dialogue」消费，单行输入下与旧行为一致。
    static core::Dialogue parse_dialogue(const std::string& entry);

    // 根据fight_txt获取对话序列 / Resolve dialogue sequence from fight_txt
    // Returns ordered list of TXT.DAT entry texts for the fight's dialogue group
    std::vector<std::string> resolve_fight_dialogue(int32_t fight_txt) const;

    // 根据fight_txt获取战后对话序列 / Resolve post-battle SubB sequence from fight_txt
    // SubB = post-battle/alternate dialogue sequence
    std::vector<std::string> resolve_fight_dialogue_b(int32_t fight_txt) const;

    // 获取对话总数 / Get total dialogue count
    int32_t dialogue_count() const { return static_cast<int32_t>(dialogues_.size()); }

private:
    std::vector<std::string> dialogues_;
    std::array<core::DialogueGroup, core::DIALOGUE_GROUP_COUNT> dialogue_groups_{};

    // fight_txt → dialogue group 映射表（运行时从 dialogue_map.bin 载入，
    // 原版实测 16 项：{0x22,0x23,0x24,0x26,0x28,0x29,0x2A,0x2C,
    // 0x2E,0x31,0x32,0x33,0x35,0x2B,0x00,0x63}）。搜索命中取 position+2。
    // 实证：FUN_0043c49f 搜 DAT_00448720[0..15]，命中 DAT_0098d808 = index+2。
    std::array<int32_t, 16> fight_txt_table_{};
};

}  // namespace dlzr::logic
