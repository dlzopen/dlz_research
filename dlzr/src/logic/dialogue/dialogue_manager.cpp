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
            // 跳过纯空 / 仅 "xx" 的条目：逐行解析后任一行为非空即视为有内容。
            auto lines = parse_dialogue_entry(dialogues_[entry_id]);
            bool has_content = false;
            for (const auto& d : lines) {
                if (!d.text.empty()) { has_content = true; break; }
            }
            if (has_content) {
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
            auto lines = parse_dialogue_entry(dialogues_[entry_id]);
            bool has_content = false;
            for (const auto& d : lines) {
                if (!d.text.empty()) { has_content = true; break; }
            }
            if (has_content) {
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

namespace {

// 解析单行对话：抽前 2 字符作说话人代码（2 位 ASCII 数字 / "-1" 旁白），剥末位 "xx" 终止符。
// 实证：FUN_0043cc96.c:21-49 读前 2 字符作说话人码（FUN_0043e890 判数字，
//   (c1-0x30)*10+(c2-0x30)=说话人 ID；非数字 / '-' / 'a' / 'b' → 旁白 -1）；
// "xx" 终止符在二进制侧由 FUN_0043c9ec 在条目级消费，clean 模型（xx 保留在末行）显式剥离。
dlzr::core::Dialogue parse_dialogue_line(const std::string& line) {
    dlzr::core::Dialogue d;
    d.speaker_id = -1;
    if (line.empty()) return d;

    std::string body = line;

    // 抽说话人代码：必须 2 位 ASCII 数字，或 "-1"；其余（含全角起头、单数字）视为无代码旁白。
    if (line.size() >= 2 && line[0] == '-' && line[1] == '1') {
        d.speaker_id = -1;
        body = line.substr(2);
    } else if (line.size() >= 2 && std::isdigit(static_cast<unsigned char>(line[0])) &&
               std::isdigit(static_cast<unsigned char>(line[1]))) {
        d.speaker_id = (line[0] - '0') * 10 + (line[1] - '0');
        body = line.substr(2);
    }
    // else: 无代码 → 旁白，body 保持整行。

    // 剥 "xx" 终止符（出现在末行尾，粘连无空格）。
    if (body.size() >= 2 && body[body.size() - 2] == 'x' && body[body.size() - 1] == 'x') {
        body = body.substr(0, body.size() - 2);
    }
    if (body == "xx") body.clear();

    d.text = body;
    return d;
}

}  // namespace

std::vector<core::Dialogue> DialogueManager::parse_dialogue_entry(const std::string& raw) {
    std::vector<core::Dialogue> out;
    // 条目体可能含内嵌换行（视觉折行）；按 '\n' 拆行，每行独立解析
    // （匹配二进制 FUN_0043cc96 逐行模型：DAT_0098d810 游标逐行推进）。
    size_t start = 0;
    while (true) {
        size_t nl = raw.find('\n', start);
        std::string line = (nl == std::string::npos) ? raw.substr(start)
                                                    : raw.substr(start, nl - start);
        core::Dialogue d = parse_dialogue_line(line);
        if (!d.text.empty()) out.push_back(std::move(d));
        if (nl == std::string::npos) break;
        start = nl + 1;
    }
    return out;
}

core::Dialogue DialogueManager::parse_dialogue(const std::string& entry) {
    // 向后兼容薄包装：返回首条非空行 Dialogue（单行输入下与旧行为一致）。
    core::Dialogue fallback;
    fallback.speaker_id = -1;
    auto lines = parse_dialogue_entry(entry);
    for (const auto& d : lines) {
        if (!d.text.empty()) return d;
    }
    return fallback;
}

}  // namespace dlzr::logic
