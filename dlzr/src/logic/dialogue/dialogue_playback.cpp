#include "logic/dialogue/dialogue_playback.hpp"

#include <cstring>

namespace dlzr::logic {

std::string get_speaker_name(int32_t speaker_id, const dlzr::core::GameData& gd) {
    if (speaker_id < 0 || speaker_id >= dlzr::core::MAX_PEOPLE) return "";
    const auto& name = gd.people[speaker_id].name;
    return std::string(name.data(), strnlen(name.data(), name.size()));
}

// 解析说话人码为显示名（id 0 = 主角，使用玩家命名；其余取 People 表名）。
// 原版 FUN_0043cc96.c:44-49 把说话人名（raw，默认色）写入文本缓冲，
// FUN_0043d82f 内联绘制；故 clean 层在 build_dialogue_display 里把说话人名前置进文本。
static std::string resolve_speaker_name(int32_t speaker_id, const dlzr::core::GameData& gd) {
    if (speaker_id == 0) return gd.protagonist_name;
    return get_speaker_name(speaker_id, gd);
}

std::string build_dialogue_display(const dlzr::core::Dialogue& d,
                                   const dlzr::core::GameData& gd) {
    std::string body = dlzr::logic::format_dialogue_text(d.text, gd.protagonist_name, gd.battle_name);
    std::string speaker = resolve_speaker_name(d.speaker_id, gd);
    if (!speaker.empty()) return speaker + "：" + body;
    return body;
}

void start_dialogue(DialogueState& ds,
                    std::vector<std::string> raw_entries,
                    GameMode return_mode) {
    ds.entries = std::move(raw_entries);
    ds.parsed.clear();
    // 逐行解析（铁律零实证 FUN_0043cc96 逐行模型）：一条 TXT.DAT 条目内每行独立说话人，
    // 展平为顺序 Dialogue 序列，使播放层每行各自显示正确说话人（修复多说话人条目丢失说话人/
    // 说话人代码残留为可见乱码的问题）。
    for (const auto& raw : ds.entries) {
        for (auto& d : dlzr::logic::DialogueManager::parse_dialogue_entry(raw)) {
            if (!d.text.empty()) ds.parsed.push_back(std::move(d));
        }
    }
    ds.current_index = 0;
    ds.visible_chars = 0;
    ds.active = true;
    ds.return_mode = return_mode;
}

void advance_dialogue(DialogueState& ds, const dlzr::core::GameData& gd) {
    if (!ds.active || ds.parsed.empty()) return;

    // 计数显示字符须与渲染串一致（含说话人名），故统一走 build_dialogue_display。
    std::string display = build_dialogue_display(ds.parsed[ds.current_index], gd);
    int32_t total_chars = 0;
    size_t pos = 0;
    while (pos < display.size()) {
        uint8_t b = static_cast<uint8_t>(display[pos]);
        pos += (b < 0x80) ? 1 : 2;
        total_chars++;
    }

    if (ds.visible_chars < total_chars) {
        // 整页显示（原版 FUN_0043d82f 一次渲染整段，无逐字）：首次推进即全显。
        ds.visible_chars = total_chars;
    } else {
        ds.current_index++;
        ds.visible_chars = 0;
        if (ds.current_index >= static_cast<int32_t>(ds.parsed.size())) {
            ds.active = false;
        }
    }
}

}  // namespace dlzr::logic
