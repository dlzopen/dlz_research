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

    // 解析对话条目 / Parse dialogue entry to extract speaker and text
    static core::Dialogue parse_dialogue(const std::string& entry);

    // 根据fight_txt获取对话序列 / Resolve dialogue sequence from fight_txt
    // Returns ordered list of TXT.DAT entry texts for the fight's dialogue group
    std::vector<std::string> resolve_fight_dialogue(int32_t fight_txt) const;

    // 根据fight_txt获取战后对话序列 / Resolve post-battle SubB sequence from fight_txt
    // SubB = post-battle/alternate dialogue sequence
    std::vector<std::string> resolve_fight_dialogue_b(int32_t fight_txt) const;

    // 条件对话链解析 / Conditional dialogue chain resolution
    // Returns dialogue sequence based on game state (story_flags, level_phases)
    // Implements FUN_00406CE8 chain logic: checks conditions to select alternate groups
    std::vector<std::string> resolve_conditional_dialogue(
        int32_t fight_txt,
        const std::array<int32_t, 14>& story_flags,
        int32_t level_phase) const;

    // 检查对话链是否应该继续 / Check if dialogue chain should continue
    // Returns next dialogue group index if chain continues, -1 if chain ends
    int32_t get_chain_continuation(
        int32_t current_group,
        const std::array<int32_t, 14>& story_flags) const;

    // 获取对话总数 / Get total dialogue count
    int32_t dialogue_count() const { return static_cast<int32_t>(dialogues_.size()); }

private:
    std::vector<std::string> dialogues_;
    std::array<core::DialogueGroup, core::DIALOGUE_GROUP_COUNT> dialogue_groups_{};

    // fight_txt → dialogue group 映射表（运行时从 dialogue_map.bin 载入，
    // 原版实测 16 项：{0x22,0x23,0x24,0x26,0x28,0x29,0x2A,0x2C,
    // 0x2E,0x31,0x32,0x33,0x35,0x2B,0x00,0x63}）。搜索命中取 position+2。
    std::array<int32_t, 16> fight_txt_table_{};

    // 条件分支表 / Conditional branch table
    // [group][condition_type] = alternate_group
    // condition_type: 0=story_flags[0], 1=story_flags[2], 2=level_phase>0
    // 注：此表为早期分析推断，尚未逐字节与原版核对（TODO）。
    static constexpr int32_t kConditionalBranches[8][3] = {
        {0, 0, 0},  // Group 0: no alternate
        {1, 7, 1},  // Group 1: if story_flags[0] → group 7, if level_phase>0 → group 1 (self-loop for phase)
        {2, 6, 2},  // Group 2: if story_flags[0] → group 6, if level_phase>0 → group 2
        {3, 3, 3},  // Group 3: no alternate
        {4, 4, 4},  // Group 4: no alternate
        {5, 5, 5},  // Group 5: no alternate
        {2, 6, 2},  // Group 6: same as group 2
        {1, 7, 1},  // Group 7: same as group 1
    };
};

}  // namespace dlzr::logic
