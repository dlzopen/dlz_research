#pragma once

// 地雷战 (Landmine War, 1998) — 全局常量定义
// Global constants from original binary analysis

#include <cstdint>

namespace dlzr::core {

// ============================================================
// 游戏模式 / Game modes (game_mode @ 0x7DB4E8)
// ============================================================
constexpr int32_t GAME_MODE_INIT = 0x00;
constexpr int32_t GAME_MODE_MAP = 0x01;
constexpr int32_t GAME_MODE_BATTLE = 0x96;

// ============================================================
// 战斗命令 / Battle commands (calc_behavior)
// ============================================================
constexpr int32_t BATTLE_CMD_END_VICTORY = 0x100;
constexpr int32_t BATTLE_CMD_END_RETREAT = 0x202;
constexpr int32_t BATTLE_CMD_ADVANCE_ROUND = 0x113;

// ============================================================
// 战斗常量 / Battle constants
// ============================================================
constexpr int32_t MAX_FIGHTER_SLOTS = 8;
constexpr int32_t MAX_FIGHTERS_PER_SIDE = 128;
constexpr int32_t MAX_PEOPLE = 40;
constexpr int32_t MAX_CLASS_TYPES = 96;
constexpr int32_t MAX_SKILLS = 16;
constexpr int32_t MAX_WUPIN = 160;
constexpr int32_t MAX_DILEI = 10;
constexpr int32_t MAX_FIGHTS = 47;
constexpr int32_t MAX_TOWNS = 19;

// ============================================================
// 战斗公式常量 / Combat formula constants
// ============================================================
constexpr int32_t MORALE_BASE = 100;
constexpr int32_t MORALE_DIVISOR = 200;
constexpr int32_t INTEGRITY_DIVISOR = 100;
constexpr int32_t SKILL_LEVEL_BASE = 59;
constexpr int32_t ATTACK_SCALE_DIV = 580;
constexpr int32_t ATTACK_DIVISOR = 60;
constexpr int32_t STRENGTH_OFFSET = 500;

// ============================================================
// 技能等级查找表 / Skill level lookup
// skill_exp → skill_level: {0,1,3,6,10,15,21,28,36,45,55}
// ============================================================
constexpr int32_t SKILL_LEVEL_COUNT = 11;
inline constexpr int32_t SKILL_LEVEL_THRESHOLDS[SKILL_LEVEL_COUNT] = {
    0, 1, 3, 6, 10, 15, 21, 28, 36, 45, 55
};

// ============================================================
// 存档常量 / Save system constants
// ============================================================
constexpr int32_t SAVE_SLOT_COUNT = 10;
constexpr int32_t SAVE_DATA_SIZE = 85072;
constexpr int32_t SAVE_DATA_BLOCK_COUNT = 28;

// ============================================================
// 显示常量 / Display constants
// ============================================================
constexpr int32_t SCREEN_WIDTH = 640;
constexpr int32_t SCREEN_HEIGHT = 480;
constexpr int32_t BATTLE_RENDER_WIDTH = 0x1E0;   // 480
constexpr int32_t BATTLE_RENDER_HEIGHT = 0xA0;   // 160

// ============================================================
// DAT 文件格式常量 / DAT file format constants
// ============================================================
constexpr int32_t IMG_HEADER_SIZE = 16;
constexpr int32_t IMG_PALETTE_SIZE = 768;         // 256 × 3 (RGB)
constexpr int32_t IMG_PALETTE_COMPONENT_SCALE = 4; // 6-bit → 8-bit

constexpr int32_t ANM_WIDTH = 320;
constexpr int32_t ANM_HEIGHT = 200;
constexpr int32_t ANM_FRAME_SIZE = ANM_WIDTH * ANM_HEIGHT;  // 64000
constexpr int32_t ANM_PALETTE_OFFSET = 0x100;
constexpr int32_t ANM_PALETTE_ENTRY_SIZE = 4;     // BGR0
constexpr int32_t ANM_PAGE_DATA_OFFSET = 0xB00;
constexpr int32_t ANM_PAGE_SIZE = 0x10000;

constexpr int32_t PEOPLE_RECORD_SIZE = 132;       // 0x84
constexpr int32_t TOWN_RECORD_SIZE = 346;
constexpr int32_t FIGHT_RECORD_SIZE = 20508;

constexpr int32_t TXT_XOR_KEY = 0x51;            // TXT.DAT 加密密钥
constexpr int32_t MAX_DIALOGUES = 640;            // TXT.DAT 最大条目数 / Max dialogue entries
constexpr int32_t DIALOGUE_GROUP_COUNT = 8;       // 对话组数量 / Number of dialogue groups

// ============================================================
// 隐藏关卡常量 / Hidden level constants
// ============================================================
constexpr int32_t HIDDEN_LEVEL_TRIGGER_FIGHT = 36;
constexpr int32_t HIDDEN_LEVEL_THRESHOLDS[4] = {2800, 3000, 3300, 3600};

}  // namespace dlzr::core
