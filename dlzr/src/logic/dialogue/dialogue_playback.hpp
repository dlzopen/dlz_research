#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "core/data_model/game_data.hpp"        // GameData (protagonist_name, battle_name, people)
#include "core/data_model/types.hpp"             // Dialogue, People, MAX_PEOPLE
#include "logic/game_state_manager.hpp"          // GameMode
#include "logic/dialogue/dialogue_manager.hpp"   // DialogueManager::parse_dialogue_entry
#include "logic/dialogue/dialogue_text_util.hpp"  // format_dialogue_text

namespace dlzr::logic {

// 对话播放状态机（从 main.cpp 匿名命名空间抽取，便于单测）。
//
// 忠实 binary 模型：一条 TXT.DAT 条目 → 多行 → 每行独立 Dialogue
// （FUN_0043cc96 逐行 + DAT_0098d810 游标推进；逐行解析见
// DialogueManager::parse_dialogue_entry）。start_dialogue 把多条目的
// 逐行 Dialogue 展平为顺序序列，播放层每行各自显示正确说话人。
struct DialogueState {
    std::vector<std::string> entries;        // 原始条目（含说话人代码，未解析）
    std::vector<dlzr::core::Dialogue> parsed; // 逐行解析后的 Dialogue 序列
    int32_t current_index{0};
    int32_t visible_chars{0};
    bool active{false};
    // 对话弹出时刻（毫秒）。用于屏蔽"刚弹出就被上一次点击立即推掉"。
    // 起因：19 屏开场幻灯片靠连点跳过，幻灯片结束的同帧/次帧弹出战前对话，
    //   残留的一次 MouseDown 会被当作 ADVANCE/DISMISS，单次点击即整段推完且立即关闭，
    //   观感等同于"进入关卡对话没有触发"。仅屏蔽极短窗口，稳态操作不受影响。
    uint32_t started_ms{0};
    GameMode return_mode{GameMode::STRATEGIC_MAP};
};

// 取 People 表说话人名（id 0 = 主角，由 GameData::protagonist_name 提供）。
// 原版 FUN_0043cc96.c:44-49 把说话人名（raw，默认色）写入文本缓冲。
std::string get_speaker_name(int32_t speaker_id, const dlzr::core::GameData& gd);

// 启动一段对话：展平所有条目的逐行 Dialogue，重置播放游标。
void start_dialogue(DialogueState& ds,
                    std::vector<std::string> raw_entries,
                    GameMode return_mode);

// 构建整条显示串：说话人名（默认色）+ 全角冒号 + 正文（GG/TT 已包 "AA"）。
// 与二进制 FUN_0043cc96:44-49 + FUN_0043d82f 一致（说话人名内联绘制；
// 二进制后缀 DAT_00448780 在 clean 层以全角冒号替代，Tier-B 实现自由）。
std::string build_dialogue_display(const dlzr::core::Dialogue& d,
                                   const dlzr::core::GameData& gd);

// 推进对话：原版一次整页渲染（FUN_0043d82f），故首次推进即全显；
// 再次推进跳到下一条，全部播完置 active=false。
void advance_dialogue(DialogueState& ds, const dlzr::core::GameData& gd);

}  // namespace dlzr::logic
