// dialogue_manager.cpp — 对话系统管理器实现
// Dialogue system manager implementation

#include "logic/dialogue/dialogue_manager.hpp"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <filesystem>
#include <stdexcept>

namespace dlzr::logic {

void DialogueManager::load_dialogues(const std::vector<std::string>& dialogue_texts) {
    dialogues_ = dialogue_texts;
}

void DialogueManager::init_dialogue_groups(const std::string& engine_data_dir) {
    // 对话组表与映射表均来自原版 EXE 数据段（byte-级实测抽取），
    // 不再硬编码常量：原硬编码的 8 组表不完整（如 Group0 SubA 实为 12 项而非 8 项），
    // 会导致对话序列被截断、跳过条目。现统一从数据文件建索引。
    namespace fs = std::filesystem;

    // ---- 组表：dialogue_groups.bin（1024 字节 = 8 组 × [SubA 16u32][SubB 16u32]）----
    const fs::path grp_path = fs::path(engine_data_dir) / "dialogue_groups.bin";
    {
        std::ifstream f(grp_path, std::ios::binary);
        if (!f) {
            std::fprintf(stderr,
                "DialogueManager: 找不到组表 %s（构建需随 exe 部署 dialogue_groups.bin）\n",
                grp_path.string().c_str());
        } else {
            std::vector<uint8_t> buf(1024);
            f.read(reinterpret_cast<char*>(buf.data()), 1024);
            if (f.gcount() == 1024) {
                for (int g = 0; g < core::DIALOGUE_GROUP_COUNT; ++g) {
                    const uint8_t* base = buf.data() + g * 32 * 4;
                    for (int k = 0; k < 16; ++k) {
                        dialogue_groups_[g].sub_a[k] =
                            static_cast<int32_t>(*reinterpret_cast<const uint32_t*>(base + k * 4));
                        dialogue_groups_[g].sub_b[k] =
                            static_cast<int32_t>(*reinterpret_cast<const uint32_t*>(base + (16 + k) * 4));
                    }
                }
            } else {
                std::fprintf(stderr, "DialogueManager: 组表 %s 长度不足 1024 字节\n",
                             grp_path.string().c_str());
            }
        }
    }

    // ---- 映射表：dialogue_map.bin（64 字节 = 16×u32 fight_txt）----
    const fs::path map_path = fs::path(engine_data_dir) / "dialogue_map.bin";
    {
        std::ifstream f(map_path, std::ios::binary);
        if (!f) {
            std::fprintf(stderr,
                "DialogueManager: 找不到映射表 %s（构建需随 exe 部署 dialogue_map.bin）\n",
                map_path.string().c_str());
        } else {
            std::vector<uint8_t> buf(64);
            f.read(reinterpret_cast<char*>(buf.data()), 64);
            if (f.gcount() == 64) {
                for (int i = 0; i < 16; ++i) {
                    fight_txt_table_[i] =
                        static_cast<int32_t>(*reinterpret_cast<const uint32_t*>(buf.data() + i * 4));
                }
            } else {
                std::fprintf(stderr, "DialogueManager: 映射表 %s 长度不足 64 字节\n",
                             map_path.string().c_str());
            }
        }
    }
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
    // 搜索映射表（dialogue_map.bin，原版实测 0x46320）中的 fight_txt，
    // 命中后对话组 = (position + 2) % 8（证据：dispatch VA 0x43DCA0 搜索 16 项表，
    // 命中返回 position+2 存入 0x98D808；group 经 mod 8 取 0..7）。
    for (int i = 0; i < 16; ++i) {
        if (fight_txt_table_[i] == fight_txt) {
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
            // Skip entries that are empty or just the "xx" terminator
            core::Dialogue d = parse_dialogue(dialogues_[entry_id]);
            if (!d.text.empty()) {
                result.push_back(dialogues_[entry_id]);
            }
        }
    }

    return result;
}

std::vector<std::string> DialogueManager::resolve_fight_dialogue_b(int32_t fight_txt) const {
    std::vector<std::string> result;

    int32_t group = get_dialogue_group(fight_txt);
    const auto& sub_b = dialogue_groups_[group].sub_b;

    for (int32_t entry_id : sub_b) {
        if (entry_id == 0) break;
        if (entry_id > 0 && entry_id < static_cast<int32_t>(dialogues_.size())) {
            core::Dialogue d = parse_dialogue(dialogues_[entry_id]);
            if (!d.text.empty()) {
                result.push_back(dialogues_[entry_id]);
            }
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

std::vector<std::string> DialogueManager::resolve_conditional_dialogue(
    int32_t fight_txt,
    const std::array<int32_t, 14>& story_flags,
    int32_t level_phase) const {

    int32_t base_group = get_dialogue_group(fight_txt);
    int32_t resolved_group = base_group;

    // Check conditional branches (FUN_00406CE8 logic)
    // Priority: story_flags[2] > story_flags[0] > level_phase
    if (base_group >= 0 && base_group < 8) {
        // Check condition 1: story_flags[2] (DAT_0095eb00)
        if (kConditionalBranches[base_group][1] != base_group &&
            story_flags[2] > 0) {
            resolved_group = kConditionalBranches[base_group][1];
        }
        // Check condition 0: story_flags[0]
        else if (kConditionalBranches[base_group][0] != base_group &&
                 story_flags[0] > 0) {
            resolved_group = kConditionalBranches[base_group][0];
        }
        // Check condition 2: level_phase > 0 (for multi-phase dialogues)
        else if (kConditionalBranches[base_group][2] != base_group &&
                 level_phase > 0) {
            resolved_group = kConditionalBranches[base_group][2];
        }
    }

    // Resolve the selected group's SubA sequence
    std::vector<std::string> result;
    const auto& sub_a = dialogue_groups_[resolved_group].sub_a;

    for (int32_t entry_id : sub_a) {
        if (entry_id == 0) break;
        if (entry_id > 0 && entry_id < static_cast<int32_t>(dialogues_.size())) {
            result.push_back(dialogues_[entry_id]);
        }
    }

    return result;
}

int32_t DialogueManager::get_chain_continuation(
    int32_t current_group,
    const std::array<int32_t, 14>& story_flags) const {

    // Chain continuation logic (FUN_00406F96)
    // After completing SubA of current group, check if SubB should follow
    // or if chain should jump to another group

    if (current_group < 0 || current_group >= core::DIALOGUE_GROUP_COUNT) {
        return -1;
    }

    // Check if SubB is non-empty (chain continues within same group)
    const auto& sub_b = dialogue_groups_[current_group].sub_b;
    if (sub_b[0] != 0) {
        return current_group;  // SubB will be played after SubA
    }

    // Check for cross-group chains based on story flags
    // Group 1 → Group 7 if story_flags[1] set
    if (current_group == 1 && story_flags.size() > 1 && story_flags[1] > 0) {
        return 7;
    }

    // Group 2 → Group 6 if story_flags[2] set
    if (current_group == 2 && story_flags.size() > 2 && story_flags[2] > 0) {
        return 6;
    }

    return -1;  // Chain ends
}

core::Dialogue DialogueManager::parse_dialogue(const std::string& entry) {
    core::Dialogue dialogue;
    dialogue.speaker_id = -1;  // Default to narrator

    // TXT.DAT format (verified against decrypted binary):
    //   "speaker_code text" with the speaker code GLUED to the text
    //   (no separating space). speaker_code is "-1" (narration) or a
    //   2-digit value 00-99 (ally/enemy/named character id).
    // Token markers inside text: AA..AA=ally name, BB..BB=enemy name,
    //   CC..CC=item/tech, DD..DD=place, GG=protagonist (substituted before
    //   render), TT=battle/place token, and "xx"=entry terminator.
    if (entry.empty()) {
        dialogue.text = entry;
        return dialogue;
    }

    std::string body = entry;

    // Extract glued speaker code
    if (entry.size() >= 2 && entry[0] == '-' && entry[1] == '1') {
        dialogue.speaker_id = -1;
        body = entry.substr(2);
    } else if (entry.size() >= 2 && std::isdigit(static_cast<unsigned char>(entry[0])) &&
               std::isdigit(static_cast<unsigned char>(entry[1]))) {
        try {
            dialogue.speaker_id = std::stoi(entry.substr(0, 2));
        } catch (...) {
            dialogue.speaker_id = -1;
        }
        body = entry.substr(2);
    }
    // else: no speaker code (e.g. a bare "xx" terminator line) -> narrator

    // Strip the "xx" terminator marker (ASCII, may be glued to end of text)
    if (body.size() >= 2 && body[body.size() - 2] == 'x' && body[body.size() - 1] == 'x') {
        body = body.substr(0, body.size() - 2);
    }
    if (body == "xx") {
        body.clear();
    }

    dialogue.text = body;
    return dialogue;
}

}  // namespace dlzr::logic
