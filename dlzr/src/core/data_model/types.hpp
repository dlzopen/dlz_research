#pragma once

// 地雷战 (Landmine War, 1998) — 核心数据结构定义
// Core data structure definitions matching original binary layouts
//
// 所有结构体严格匹配原作内存布局，用于直接读写原始 DAT 文件。
// All structs strictly match original memory layouts for direct binary I/O.

#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace dlzr::core {

// ============================================================
// 人物属性结构体 / People attributes struct
// On-disk size: 0x84 bytes (PEOPLE.DAT: 40 records × 132 bytes)
// Base address in original: 0x7D0EF8
// ============================================================
struct People {
    std::array<char, 16> name;        // +0x00 角色名称 (GBK) / Character name
    int32_t face;                     // +0x10 头像索引 / Portrait index
    int32_t pad2;                     // +0x14 填充 / Padding (0)
    int32_t enemy;                    // +0x18 阵营 (0=我方,1=敌方,2=特殊) / Faction
    int32_t strength;                 // +0x1C 武力 / Martial force
    int32_t intelligence;             // +0x20 智力 / Intelligence
    int32_t level;                    // +0x24 等级 / Level
    int32_t experience;               // +0x28 经验值 / Experience
    int32_t class_type;               // +0x2C 兵种索引 / Class type index
    // 17个技能使用次数计数器 / 17 skill usage counters
    int32_t skill_gongji;             // +0x30 攻击 / Attack
    int32_t skill_buji;              // +0x34 补给 / Supply
    int32_t skill_jili;              // +0x38 激励 / Motivate
    int32_t skill_yiliao;            // +0x3C 医疗 / Medical
    int32_t skill_huogong;           // +0x40 火攻 / Fire attack
    int32_t skill_gunmu;             // +0x44 滚木 / Rolling logs
    int32_t skill_leishi;            // +0x48 擂石 / Rock throw
    int32_t skill_weishe;            // +0x4C 威慑 / Intimidation
    int32_t skill_saorao;            // +0x50 骚扰 / Harassment
    int32_t skill_pohuai;            // +0x54 破坏 / Sabotage
    int32_t skill_lueduo;            // +0x58 掠夺 / Plunder
    int32_t skill_fanjian;           // +0x5C 反间 / Counter-spy
    int32_t skill_yibing;            // +0x60 疑兵 / Decoy
    int32_t skill_duzhan;            // +0x64 督战 / Supervise
    int32_t skill_ansha;             // +0x68 暗杀 / Assassination
    int32_t skill_shidu;             // +0x6C 施毒 / Poison
    int32_t skill_kongxi;            // +0x70 空袭 / Air raid
    std::array<uint8_t, 8> items;     // +0x74 物品栏 (8格,0=空) / Item slots
    std::array<uint8_t, 8> mines;     // +0x7C 地雷栏 (8格,0=空) / Mine slots
};
static_assert(sizeof(People) == 0x84, "People must be 0x84 (132) bytes");

// ============================================================
// 战斗人员结构体 / Fighter struct
// Size: 0x40 bytes, Base: 0x96AB90
// Indexed via shl edx, 6 (×64)
// ============================================================
struct Fighter {
    int32_t max_hp;            // +0x00 最大HP / Maximum HP
    int32_t current_hp;        // +0x04 当前HP / Current HP
    int32_t morale;            // +0x08 士气 / Morale
    int32_t equip_integrity;   // +0x0c 装备完整度 / Equipment integrity
    int32_t defense_modifier;  // +0x10 防御修正 / Defense modifier
    int32_t unknown_14;        // +0x14
    int32_t unknown_18;        // +0x18
    int32_t unknown_1c;        // +0x1c
        int32_t pos_x;             // +0x20 实时网格坐标X / runtime grid pos X (RE: FUN_0040124c)
        int32_t pos_y;             // +0x24 实时网格坐标Y / runtime grid pos Y (RE: FUN_0040144e)
    int32_t unknown_28;        // +0x28
    int32_t unknown_2c;        // +0x2c
    int32_t unknown_30;        // +0x30
    int32_t unknown_34;        // +0x34
    int32_t attacker_index;    // +0x38 攻击者索引 / Attacker index
    // +0x3c..+0x3f 四个 RE 实证字节（按字节语义命名）；同时为 C++ 侧提供整字视图：
    //   status_flag16 = 原 int16 视图（+0x3c..+0x3d），status_flags = 32 位整字视图。
    union {
        struct {
            uint8_t active_flag;   // +0x3c 部署/激活标志(byte) / deployed-active (g_battle_unit_active)
            uint8_t ai_action;     // +0x3d AI动作码(byte) / AI action code (g_battle_unit_action)
            uint8_t flag2;         // +0x3e 副标志(byte) / secondary flag (g_battle_unit_flag2)
            uint8_t unit_state;    // +0x3f 行动状态(byte) / unit state (g_battle_unit_state)
        };
        uint16_t status_flag16;    // +0x3c..+0x3d 原 int16 状态标志视图（battle_renderer 用）
        uint32_t status_flags;     // +0x3c..+0x3f 32 位整字视图
    };
};
static_assert(sizeof(Fighter) == 0x40, "Fighter must be 0x40 bytes");

// ============================================================
// 兵种数据结构体 / Class type (BinZhong) struct
// Size: 0xA0 bytes, Base: 0x96CBAC
// 96 entries in GAME.DAT (90 active + 6 unused)
// ============================================================
struct SkillFlags {
    uint8_t buji;             // +0x00 补给 / Supply
    uint8_t jili;             // +0x01 激励 / Motivate
    uint8_t yiliao;           // +0x02 医疗 / Medical
    uint8_t huogong;          // +0x03 火攻 / Fire attack
    uint8_t gunmu;            // +0x04 滚木 / Rolling logs
    uint8_t leishi;           // +0x05 擂石 / Rock throw
    uint8_t weishe;           // +0x06 威慑 / Intimidation
    uint8_t saorao;           // +0x07 骚扰 / Harassment
    uint8_t pohuai;           // +0x08 破坏 / Sabotage
    uint8_t lueduo;           // +0x09 掠夺 / Plunder
    uint8_t fanjian;          // +0x0A 反间 / Counter-spy
    uint8_t yibing;           // +0x0B 疑兵 / Decoy
    uint8_t duzhan;           // +0x0C 督战 / Supervise
    uint8_t ansha;            // +0x0D 暗杀 / Assassination
    uint8_t shidu;            // +0x0E 施毒 / Poison
    uint8_t kongxi;           // +0x0F 空袭 / Air raid
};

struct ClassType {
    std::array<char, 16> name;        // +0x00 兵种名称 (GBK) / Class name
    int32_t img_index;                // +0x10 图片索引 -> army图片 / Image index
    int32_t hp_base;                  // +0x14 体力基础 / Base HP
    int32_t hp_growth;                // +0x18 体力加成 / HP growth per level
    int32_t attack_growth;            // +0x1C 攻击加成 / Attack growth rate
    int32_t defense_growth;           // +0x20 防御加成 / Defense growth rate
    int32_t movement;                 // +0x24 移动力 / Movement points
    int32_t base_attack;              // +0x28 攻击力 / Base attack power
    int32_t base_defense;             // +0x2C 防御力 / Base defense power
    int32_t select_range;             // +0x30 选择范围 / Selection range
    int32_t apply_range;              // +0x34 应用范围 / Application range
    int32_t promotion_class;          // +0x38 转职兵种 (-1=不可) / Promotion target class
    int32_t promotion_item;           // +0x3C 转职道具 (-1=不可) / Required promotion item
    int32_t pad1;                     // +0x40 填充 / Padding
    int32_t flm_attack;               // +0x44 攻击动画 -> flm.dat / Attack animation
    int32_t audio_attack;             // +0x48 攻击音效 -> wave.dat / Attack sound
    int32_t audio_move;               // +0x4C 移动音效 -> wave.dat / Move sound
    std::array<int32_t, 16> reserved; // +0x50 保留字段 / Reserved (foo3)
    SkillFlags skill_flags;           // +0x90 技能标志 / Skill availability flags
};
static_assert(sizeof(ClassType) == 0xA0, "ClassType must be 0xA0 bytes");

// ============================================================
// 物品数据结构体 / WuPin (item) struct
// Size: 72 bytes, 160 records in GAME.DAT
// Offset in GAME.DAT: 0x43E0
// ============================================================
struct WuPin {
    std::array<char, 16> name;        // +0x00 物品名称 (GBK) / Item name
    std::array<char, 8> unit;         // +0x10 计量单位 / Unit of measure
    int32_t goods_icon;               // +0x18 图标索引 -> GOODS.DAT / Icon index
    int32_t price;                    // +0x1C 价格 / Gold price
    int32_t para1;                    // +0x20 效果参数1 / Effect parameter 1
    int32_t para2;                    // +0x24 效果参数2 / Effect parameter 2
    int32_t para3;                    // +0x28 效果参数3 / Effect parameter 3
    int32_t para4;                    // +0x2C 效果参数4 / Effect parameter 4
    int32_t select_range;             // +0x30 选择范围 / Selection range index
    int32_t use_count;                // +0x34 使用次数 (0-5) / Number of uses
    int32_t pad1;                     // +0x38 保留 / Reserved (always 0)
    int32_t pad2;                     // +0x3C 保留 / Reserved (always 0)
    int32_t flm_index;                // +0x40 使用动画 -> flm.dat / Use animation
    int32_t audio_index;              // +0x44 使用音效 -> wave.dat / Use sound
};
static_assert(sizeof(WuPin) == 72, "WuPin must be 72 bytes");

// ============================================================
// 地雷数据结构体 / DiLei (mine) struct
// Size: 48 bytes, 10 records in GAME.DAT
// ============================================================
struct DiLei {
    // RE 实证（权威 010 模板 template/dlz_game.bt + DLZFULL/GAME.DAT @0x725C 逐字节抽取）：
    // 每条 48B，名称位于记录 +0x00（无前导文件头）。010 模板在 name 与 price 间误插了一个
    // pad0，真实文件无此字段；真实字段顺序 = name[16]/weili/price/effect_type/flm_index/
    // pad×3/audio_index。旧结构体误加 8B header 且把 name 推到 +0x08，致字段整体错位，
    // mines[].damage 读到 flm_index 等垃圾。已移除 phantom header，字段对齐 @0x725C 实证。
    std::array<char, 16> name;        // +0x00 地雷名称 (GBK) / Mine name
    int32_t weili;                    // +0x10 威力 / Damage (土雷60…轰天雷800)
    int32_t price;                    // +0x14 价格 / Price (石雷240…轰天雷800)
    int32_t effect_type;              // +0x18 效果类型 1=士气 0=HP / 1=morale 0=HP
    int32_t flm_index;                // +0x1C 小动画 -> flm.dat / Small animation
    int32_t pad0;                     // +0x20 保留 / Padding
    int32_t pad1;                     // +0x24 保留 / Padding
    int32_t pad2;                     // +0x28 保留 / Padding
    int32_t audio_index;              // +0x2C 音效 -> wave.dat / Sound (100..109)
};
static_assert(sizeof(DiLei) == 48, "DiLei must be 48 bytes");

// ============================================================
// 技能数据结构体 / JiNeng (skill definition) struct
// Size: 40 bytes, 16 records in GAME.DAT
// ============================================================
struct JiNeng {
    // RE 实证（权威 010 模板 template/dlz_game.bt 的 JiNeng 结构 + DLZFULL/GAME.DAT @0x743C
    // 逐字节抽取，铁律零坐标）：每条 48B，名称位于记录 +0x00（无前导文件头）。
    // 010 模板字段顺序：name[16]/para1/para2/type/select_range/apply_range/anm_index/
    // flm_index/audio_index（共 1+8 个 int32 = 16+32 = 48B）。
    // 旧结构体误加 8B header 并把 name 推到 +0x08，致 para1/para2/type 整体错位 8B
    // （读取到名称尾字节，calc_skill_effect 用垃圾算技能效果）。已移除 phantom header。
    std::array<char, 16> name;        // +0x00 技能名称 (GBK) / Skill name
    int32_t para1;                    // +0x10 效果参数1 / Effect parameter 1
    int32_t para2;                    // +0x14 效果参数2 / Effect parameter 2
    int32_t type;                     // +0x18 技能类型 / Skill type (JiNengType 0..10)
    int32_t select_range;             // +0x1C 选择范围 / Selection range
    int32_t apply_range;              // +0x20 应用范围 / Application range
    int32_t anm_index;                // +0x24 大动画 -> anm.dat (-1=无) / Large animation
    int32_t flm_index;                // +0x28 小动画 -> flm.dat / Small animation
    int32_t audio_index;              // +0x2C 音效 -> wave.dat / Sound
};
static_assert(sizeof(JiNeng) == 48, "JiNeng must be 48 bytes");

// ============================================================
// 锻造材料结构体 / Material (crafting material) struct
// Size: 32 bytes, 10 records in GAME.DAT at offset 0x7260
// ============================================================
struct Material {
    std::array<char, 16> name;        // +0x00 材料名称 (GBK) / Material name
    std::array<char, 8> unit;         // +0x10 计量单位 / Unit of measure
    int32_t price;                    // +0x18 价格 / Price per unit
    int32_t pad;                      // +0x1C 填充 / Padding
};
static_assert(sizeof(Material) == 32, "Material must be 32 bytes");

// ============================================================
// 地雷配方表 / DiLei recipe matrix
// 10×10 bytes at GAME.DAT offset 0x73A0
// recipe[material_idx][item_idx] = quantity needed
// ============================================================
struct DiLeiRecipe {
    std::array<std::array<uint8_t, 10>, 10> matrix;
};

// ============================================================
// IMG 文件头 / IMG file header
// Size: 16 bytes
// ============================================================
struct ImgHeader {
    uint32_t magic;        // +0x00 魔数（游戏不读取）/ Magic (not read by game)
    uint16_t pad1;         // +0x04 填充 / Padding
    uint16_t width_m1;     // +0x06 宽度-1 / Width minus 1
    uint16_t height_m1;    // +0x08 高度-1 / Height minus 1
    uint16_t pad2;         // +0x0A 填充 / Padding
    uint32_t pad3;         // +0x0C 填充 / Padding
};
static_assert(sizeof(ImgHeader) == 16, "ImgHeader must be 16 bytes");

// ============================================================
// ANM/LPF 文件头 / ANM LPF container header
// ============================================================
struct LpfHeader {
    uint32_t magic;              // "LPF " = 0x2046504C
    uint32_t max_pages;          // 0x0100 (256)
    uint32_t pages_num;          // 实际使用页数 / Pages actually used
    uint32_t records_num;        // 总记录数 / Total records
    uint32_t max_records;        // 最大记录数 / Max records
    uint32_t page_table_offset;  // 页表偏移 / Page table offset
};

struct AnimHeader {
    uint32_t magic_anim;     // "ANIM" = 0x4D494E41
    uint16_t width;          // 帧宽度 / Frame width (320)
    uint16_t height;         // 帧高度 / Frame height (200)
    uint16_t zero;           // 零填充 / Zero padding
    uint16_t rate;           // 播放速率 / Playback rate
    uint16_t has_last;       // 非零则末记录特殊 / Non-zero = last record special
    uint16_t last_valid;     // 末帧有效性 / Last frame validity
    uint16_t skip;           // 跳过值 / Skip value
    std::array<uint32_t, 32> foo;  // 128字节辅助数据 / 128 bytes aux data
    uint16_t frames_num;     // 帧数 / Number of frames
    uint16_t info;           // 信息标志 / Info flags
    std::array<uint8_t, 58> foo2;  // 58字节辅助数据 / 58 bytes aux data
    std::array<uint8_t, 1152> extra;  // 1152字节（含调色板数据）/ Contains palette
};

struct AnmPageEntry {
    uint16_t base_record;  // 首页记录索引 / First record index
    uint16_t nb_records;   // 本页记录数 / Records in this page
    uint16_t size;         // 页数据大小 / Page data size
};

// ============================================================
// 解码后的图像 / Decoded image
// ============================================================
struct Palette {
    std::array<std::array<uint8_t, 3>, 256> colors;  // RGB 8-bit per component
};

struct DecodedImage {
    uint16_t width{0};
    uint16_t height{0};
    std::vector<uint8_t> pixels;   // 调色板索引 / Palette indices
    Palette palette;
};

// ============================================================
// 解码后的动画 / Decoded animation
// ============================================================
struct DecodedFrame {
    std::vector<uint8_t> pixels;  // 320×200 调色板索引 / Palette indices
};

struct DecodedAnimation {
    uint16_t width{320};
    uint16_t height{200};
    Palette palette;
    std::vector<DecodedFrame> frames;
    uint16_t rate{0};
};

// ============================================================
// FIGHT.DAT 磁盘结构体 / FIGHT.DAT on-disk structs
// 每关 20508 字节 = Intro(700) + Fighter[128](8192) + People[88](11616)
// ============================================================

struct FightBox {
    int32_t x;              // +0x00 宝箱网格X (999=未使用)
    int32_t y;              // +0x04 宝箱网格Y (999=未使用)
    int32_t appearance;     // +0x08 宝箱外观 (0-2)
    int32_t wupin;          // +0x0C 物品 (-1=已取)
    int32_t wupin0;         // +0x10 随机物品1 (50%)
    int32_t wupin1;         // +0x14 随机物品2 (30%)
    int32_t wupin2;         // +0x18 随机物品3 (20%)
};
static_assert(sizeof(FightBox) == 28, "FightBox must be 28 bytes");

struct FightIntro {
    std::array<char, 32> name;          // +0x000 关卡名称 (GBK)
    int32_t difficulty;                 // +0x020 难度系数
    int32_t fight_txt;                  // +0x024 开场对话索引
    std::array<char, 64> success_text;  // +0x028 胜利条件文本 (GBK)
    std::array<char, 64> failure_text;  // +0x068 失败条件文本 (GBK)
    std::array<FightBox, 4> boxes;      // +0x0A8 4个宝箱
    int32_t people_count;               // +0x118 NPC数量
    std::array<uint8_t, 32> people_indices; // +0x11C 角色索引 (0xFF=空)
    int32_t grid_param_x;               // +0x13C 网格参数X
    int32_t grid_param_y;               // +0x140 网格参数Y
    std::array<int32_t, 92> grid_terrain; // +0x144 地形网格 (0=可行,99=障碍)
    int32_t grid_sentinel;              // +0x2B4 固定99
    int32_t pad_final;                  // +0x2B8 固定0
};
static_assert(sizeof(FightIntro) == 700, "FightIntro must be 700 bytes");

struct FightFighter {
    int32_t base_hp;          // +0x00 基础HP (通常0)
    int32_t stamina;          // +0x04 体力
    int32_t morale;           // +0x08 士气 (通常100)
    int32_t stat3;            // +0x0C 未知 (通常100)
    int32_t movement;         // +0x10 移动力 (通常100)
    int32_t grid_x;           // +0x14 网格X (999=未部署, 冗余拷贝+0x20)
    int32_t grid_y;           // +0x18 网格Y (冗余拷贝+0x24)
    int32_t facing;           // +0x1C 朝向 0=N 1=S 2=W 3=E (冗余拷贝+0x28)
    int32_t pos_x;            // +0x20 X坐标
    int32_t pos_y;            // +0x24 Y坐标
    int32_t direction;        // +0x28 方向
    int32_t target_people;    // +0x2C 跟踪目标人物索引
    int32_t target_pos_x;     // +0x30 巡逻/目标X
    int32_t target_pos_y;     // +0x34 巡逻/目标Y
    int32_t reserved;         // +0x38 固定0xFFFFFFFF
    uint8_t stage;            // +0x3C 0=预备,1=登场
    uint8_t behavior;         // +0x3D 行为类型 (0=idle,1=patrol,4=chase,5=guard,6=assist,7=ambush)
    uint8_t flag;             // +0x3E 未知标志
    uint8_t moveable;         // +0x3F 行动状态 (0=已行动,1=未行动)
};
static_assert(sizeof(FightFighter) == 64, "FightFighter must be 64 bytes");

// ============================================================
// MAP.DAT / BLK.DAT 运行时结构（战斗地图瓦片）
// 证据: output/map_dat_analysis.md (格式) + scripts/export_map.py (解析实现，
//       其渲染结果 export/map/00_*.png 与原版截图逐像素吻合)。
// MAP.DAT: 64×8B 索引 + 47 个顺序地图记录
//   记录 = width(u16) height(u16) blk_file[12] pal_file[12] tiles[u16 * w*h]
//   tile 低 12 位 = BLK 组内瓦片号，高 4 位 = 地形旗标
// BLK.DAT: 64×(offset,length) 索引 + 顺序组数据；每组 = N 个 48×48 调色板索引瓦片
// ============================================================

struct FightMap {
    int32_t width{0};
    int32_t height{0};
    int32_t blk_group{0};              // blk_file "dlz-NN.blk" → NN
    std::vector<uint16_t> tiles;       // w*h 项 (低12位瓦片号, 高4位旗标)
};

struct BlkGroupData {
    static constexpr int32_t kTileSize = 48;
    std::vector<std::vector<uint8_t>> tiles;  // 每项 48*48 调色板索引
};

struct FightPeople {
    std::array<char, 16> name;    // +0x00 角色名称 (GBK)
    int32_t face;                 // +0x10 头像索引
    int32_t pad2;                 // +0x14 填充(0)
    int32_t enemy;                // +0x18 阵营 (0=我方,1=敌方)
    int32_t strength;             // +0x1C 武力
    int32_t intelligence;         // +0x20 智力
    int32_t level;                // +0x24 等级
    int32_t experience;           // +0x28 经验
    int32_t class_type;           // +0x2C 兵种索引
    int32_t skill_attack;         // +0x30 攻击技能
    int32_t skill_supply;         // +0x34 补给
    int32_t skill_motivate;       // +0x38 激励
    int32_t skill_medical;        // +0x3C 医疗
    int32_t skill_fire;           // +0x40 火攻
    int32_t skill_log;            // +0x44 滚木
    int32_t skill_rock;           // +0x48 擂石
    int32_t skill_intimidate;     // +0x4C 威慑
    int32_t skill_harass;         // +0x50 骚扰
    int32_t skill_sabotage;       // +0x54 破坏
    int32_t skill_plunder;        // +0x58 掠夺
    int32_t skill_counterspy;     // +0x5C 反间
    int32_t skill_decoy;          // +0x60 疑兵
    int32_t skill_supervise;      // +0x64 督战
    int32_t skill_assassinate;    // +0x68 暗杀
    int32_t skill_poison;         // +0x6C 施毒
    int32_t skill_airraid;        // +0x70 空袭
    std::array<uint8_t, 8> items; // +0x74 物品栏 (8格,0=空)
    std::array<uint8_t, 8> mines; // +0x7C 地雷栏 (8格,0=空)
};
static_assert(sizeof(FightPeople) == 0x84, "FightPeople must be 132 bytes");

// ============================================================
// 战役招募名册 / Campaign recruitment roster
// Tier-B 接缝（招募"事件"由事件 FSM 写入，本结构仅被消费）：
// 战斗部署按**同一槽位索引**读 fight.people[slot] / fight.fighters[slot]
// （证据 FUN_0041b53c.c:19-29：遍历部署位 slot，读 people[slot].class_type/face
//  → 滑稽绘制；face > -1 视为有效单元）。故"招募一名同伴"= 把 game.people[k]
// （k∈[1,39] 战役同伴槽）复制到 fight.people[k] 并标 fight.fighters[k].grid_x/y(登场)，
// 原版同样用 slot k 既作战役名册也作战斗部署位（FightData 注释已钉定 [0..39]=战役槽）。
// 事件系统（FUN_0043c2dd 等）负责决定"哪些 k 被招募"——本结构是其写回目标，
// 克隆不臆造招募事件逻辑（铁律零：不伪造游戏流程）。
// ============================================================
struct RecruitedCompanion {
    int32_t people_index{0};  // 源名册索引（指向 game.people[k]，k∈[1,39]）
    int32_t grid_x{999};      // 部署网格 X（999=未部署，与 FIGHT.DAT 约定一致）
    int32_t grid_y{999};      // 部署网格 Y
    int32_t facing{1};        // 朝向 0=N 1=S 2=W 3=E（FUN_0042e2d6 派生自方向选择）
};

struct CampaignRoster {
    std::vector<RecruitedCompanion> companions;  // 已招募并参与本关的同伴
};

// ============================================================
// TOWN.DAT 磁盘结构体 / TOWN.DAT on-disk struct
// 19 城镇 × 346 字节 / 19 towns × 346 bytes
// Loader: VA 0x00429286, buffer 0x7D0D08, stride 346 (0x15A)
// ============================================================
#pragma pack(push, 1)
struct Town {
    int32_t reserved0;                  // +0x00 固定0 / Always 0
    int32_t reserved1;                  // +0x04 固定-1 / Always -1

    // 环境音1 / Ambient sound 1
    int32_t sound1_id;                  // +0x08 音频资源ID / Audio resource ID
    int32_t sound1_interval;            // +0x0C 播放间隔 / Playback interval
    int32_t sound1_duration;            // +0x10 播放时长 / Playback duration

    // 环境音2 / Ambient sound 2
    int32_t sound2_id;                  // +0x14
    int32_t sound2_interval;            // +0x18
    int32_t sound2_duration;            // +0x1C

    // 环境音3 / Ambient sound 3 (-1 = disabled)
    int32_t sound3_id;                  // +0x20
    int32_t sound3_interval;            // +0x24
    int32_t sound3_duration;            // +0x28

    // 建筑标签位置 / Building label positions (6 buildings)
    // 0=民房 1=指挥部 2=军械部 3=杂货铺 4=出口 5=作坊
    // (0,0) or 999 = absent
    std::array<int32_t, 6> name_x;      // +0x2C
    std::array<int32_t, 6> name_y;      // +0x44
    std::array<int32_t, 6> click_x;     // +0x5C
    std::array<int32_t, 6> click_y;     // +0x74

    // 未使用商店栏 / Unused shop slots (all -1)
    std::array<int32_t, 10> goods_unused;  // +0x8C

    // 军械部商店 / Arsenal shop (WuPin indices into GAME.DAT, -1=end)
    std::array<int32_t, 12> goods_arsenal;  // +0xB4

    // 杂货铺 / General store (WuPin indices, -1=end)
    std::array<int32_t, 12> goods_general;  // +0xE4

    // 材料店 / Material shop (Material indices 0-9, -1=end)
    std::array<int32_t, 12> goods_material; // +0x114

    // 标志 / Flags (only flags[1] used: animated background)
    std::array<uint16_t, 11> flags;     // +0x144
};
#pragma pack(pop)
static_assert(sizeof(Town) == 346, "Town must be 346 bytes");

// ============================================================
// 对话系统运行时结构 / Dialogue system runtime structures
// TXT.DAT: 593 entries, XOR 0x51 encrypted, GBK encoded
// ============================================================
struct Dialogue {
    int32_t speaker_id;           // 说话人代码 / Speaker code (-1=旁白, 0-39=盟友, 40-99=敌人)
    std::string text;             // 对话文本 (GBK) / Dialogue text (GBK encoded)
};

// 对话组 / Dialogue group (8 groups, each with SubA and SubB sequences)
struct DialogueGroup {
    std::array<int32_t, 16> sub_a;  // 主序列 / Primary sequence (zero-terminated)
    std::array<int32_t, 16> sub_b;  // 副序列 / Secondary sequence (zero-terminated)
};

}  // namespace dlzr::core
