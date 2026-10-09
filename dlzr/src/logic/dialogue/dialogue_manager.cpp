// dialogue_manager.cpp — 对话系统管理器实现
// Dialogue system manager implementation

#include "logic/dialogue/dialogue_manager.hpp"

#include <algorithm>
#include <stdexcept>

namespace dlzr::logic {

void DialogueManager::load_dialogues(const std::vector<std::string>& dialogue_texts) {
    dialogues_ = dialogue_texts;
}

void DialogueManager::init_dialogue_groups() {
    // Dialogue group table from VA 0x446030
    // 8 groups, each with SubA and SubB sequences (zero-terminated)
    // Data from dialogue_branch_analysis.md section 2.1

    // Group 0: SubA = {2,12,21,31,41,51,61,71}, SubB = {3,13,8,18}
    dialogue_groups_[0].sub_a = {2,12,21,31,41,51,61,71, 0,0,0,0,0,0,0,0};
    dialogue_groups_[0].sub_b = {3,13,8,18, 0,0,0,0,0,0,0,0,0,0,0,0};

    // Group 1: SubA = {1,11,22,42,52,72,61,31}, SubB = {6,16,7,17}
    dialogue_groups_[1].sub_a = {1,11,22,42,52,72,61,31, 0,0,0,0,0,0,0,0};
    dialogue_groups_[1].sub_b = {6,16,7,17, 0,0,0,0,0,0,0,0,0,0,0,0};

    // Group 2: SubA = {1,11,23,22,42,52,72,61}, SubB = {31,5,15,33}
    dialogue_groups_[2].sub_a = {1,11,23,22,42,52,72,61, 0,0,0,0,0,0,0,0};
    dialogue_groups_[2].sub_b = {31,5,15,33, 0,0,0,0,0,0,0,0,0,0,0,0};

    // Group 3: SubA = {23,5,15,33}, SubB = empty
    dialogue_groups_[3].sub_a = {23,5,15,33, 0,0,0,0,0,0,0,0,0,0,0,0};
    dialogue_groups_[3].sub_b = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

    // Group 4: SubA = {1,11,22,42,52,72,31,4}, SubB = {14,32}
    dialogue_groups_[4].sub_a = {1,11,22,42,52,72,31,4, 0,0,0,0,0,0,0,0};
    dialogue_groups_[4].sub_b = {14,32, 0,0,0,0,0,0,0,0,0,0,0,0,0,0};

    // Group 5: SubA = {4,14,32,31,41,71}, SubB = empty
    dialogue_groups_[5].sub_a = {4,14,32,31,41,71, 0,0,0,0,0,0,0,0,0,0};
    dialogue_groups_[5].sub_b = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

    // Group 6: Same as Group 2
    dialogue_groups_[6] = dialogue_groups_[2];

    // Group 7: Same as Group 1
    dialogue_groups_[7] = dialogue_groups_[1];
}

const std::array<int32_t, 16>& DialogueManager::get_sub_a(int32_t group_index) const {
    if (group_index < 0 || group_index >= core::DIALOGUE_GROUP_COUNT) {
        throw std::out_of_range("dialogue group index out of range");
    }
    return dialogue_groups_[group_index].sub_a;
}

const std::array<int32_t, 16>& DialogueManager::get_sub_b(int32_t group_index) const {
    if (group_index < 0 || group_index >= core::DIALOGUE_GROUP_COUNT) {
        throw std::out_of_range("dialogue group index out of range");
    }
    return dialogue_groups_[group_index].sub_b;
}

int32_t DialogueManager::get_dialogue_group(int32_t fight_txt) const {
    // Search mapping table for fight_txt value (VA 0x448720)
    // On match, dialogue index = position + 2, mapped to group via mod 8
    for (int i = 0; i < 16; ++i) {
        if (kFightTxtTable[i] == fight_txt) {
            return (i + 2) % core::DIALOGUE_GROUP_COUNT;
        }
    }
    return 0;
}

std::vector<std::string> DialogueManager::resolve_fight_dialogue(int32_t fight_txt) const {
    std::vector<std::string> result;

    int32_t group = get_dialogue_group(fight_txt);
    const auto& sub_a = dialogue_groups_[group].sub_a;

    for (int32_t entry_id : sub_a) {
        if (entry_id == 0) break;
        if (entry_id > 0 && entry_id < static_cast<int32_t>(dialogues_.size())) {
            result.push_back(dialogues_[entry_id]);
        }
    }

    return result;
}

const std::string& DialogueManager::get_dialogue_text(int32_t entry_index) const {
    if (entry_index < 0 || entry_index >= static_cast<int32_t>(dialogues_.size())) {
        throw std::out_of_range("dialogue entry index out of range");
    }
    return dialogues_[entry_index];
}

core::Dialogue DialogueManager::parse_dialogue(const std::string& entry) {
    core::Dialogue dialogue;
    dialogue.speaker_id = -1;  // Default to narrator

    // Parse format: "speaker_code text"
    // Speaker code can be: -1, 00-39, 40-99
    if (entry.empty()) {
        dialogue.text = entry;
        return dialogue;
    }

    // Find first space to separate speaker code from text
    size_t space_pos = entry.find(' ');
    if (space_pos == std::string::npos) {
        // No space found, entire entry is text
        dialogue.text = entry;
        return dialogue;
    }

    // Extract speaker code
    std::string speaker_str = entry.substr(0, space_pos);

    // Parse speaker code
    if (speaker_str == "-1") {
        dialogue.speaker_id = -1;
    } else {
        try {
            dialogue.speaker_id = std::stoi(speaker_str);
        } catch (...) {
            dialogue.speaker_id = -1;  // Default to narrator on parse error
        }
    }

    // Extract text (everything after first space)
    dialogue.text = entry.substr(space_pos + 1);

    return dialogue;
}

}  // namespace dlzr::logic
