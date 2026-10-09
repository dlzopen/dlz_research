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

    // 初始化对话组表 / Initialize dialogue group table (from VA 0x446030)
    void init_dialogue_groups();

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

    // 获取对话总数 / Get total dialogue count
    int32_t dialogue_count() const { return static_cast<int32_t>(dialogues_.size()); }

private:
    std::vector<std::string> dialogues_;
    std::array<core::DialogueGroup, core::DIALOGUE_GROUP_COUNT> dialogue_groups_{};

    // fight_txt → dialogue mapping table (from VA 0x448720)
    // 16 entries: search for match, position determines dialogue group
    static constexpr int32_t kFightTxtTable[16] = {
        0x22, 0x23, 0x24, 0x26, 0x28, 0x29, 0x2A, 0x2C,
        0x2E, 0x31, 0x32, 0x33, 0x35, 0x2B, 0x00, 0x63
    };
};

}  // namespace dlzr::logic
