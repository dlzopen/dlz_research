// dat_loader.cpp — DAT 文件加载实现
// DAT file loading implementation
//
// 直接读取原始二进制文件，结构体布局严格匹配原作内存布局。
// Reads original binary files directly; struct layouts match original memory.

#include "app/resource/dat_loader.hpp"

#include "core/data_model/constants.hpp"

#include <cstring>
#include <stdexcept>

namespace dlzr::app {

core::GameData DatLoader::load_game_data() const {
    core::GameData gd;

    // PEOPLE.DAT: 40 records × 132 bytes
    {
        BinaryReader reader(resolve("PEOPLE.DAT"));
        static_assert(core::PEOPLE_RECORD_SIZE == 132);
        for (int i = 0; i < core::MAX_PEOPLE; ++i) {
            size_t offset = static_cast<size_t>(i) * core::PEOPLE_RECORD_SIZE;
            reader.read_bytes(offset, &gd.people[i], sizeof(core::People));
        }
    }

    // GAME.DAT: multiple sections
    // 兵种表(ClassType) 真实基址 = 0x1E0（已用 GAME.DAT 字节实证 + 逆向三重校验）：
    //   · 角色选择阵型→兵种表 DAT_00447b78 = {1,4,8,11}，须映射到
    //     {手枪队,大刀队,步枪队,骑兵队}；仅当基址=0x1E0 时索引 1=手枪队、4=大刀队、
    //     8=步枪队、11=骑兵队 成立（0x0320 下索引1=冲锋队，与玩家"手枪队"选择矛盾）。
    //   · FUN_0042e2d6.c:257 武器映射 class1→物品100→WuPin[99]=转轮式手枪，与用户确认一致。
    //   · PEOPLE.DAT 孙悟空 bingzhong=0 → 神仙队(0x1E0[0])、李向阳=1→手枪队(0x1E0[1])，
    //     与原作人物设定吻合。
    //   ⚠️ output/data_dictionary.md §3 把兵种表标在 0x0320 是误标（漏掉 0x1E0 起始的
    //     神仙队/手枪队两条记录，整体后移 320 字节），以本代码 + 字节实证为准。
    //   +0x0000: What0 (160 B)
    //   +0x00A0: DiXingModifier[16] (640 B)
    //   +0x1E0: BingZhong[96] (15360 B) — ClassType（真实基址，非 data_dictionary 的 0x0320）
    //   +0x3F20: BingZhongYiDong[96] (1536 B)           （地形移动消耗，本加载器暂未使用）
    //   +0x43E0: WuPin[160] (11520 B) — Items
    //   +0x7260: Material[10] (320 B)
    //   +0x73A0: DiLeiRecipe[10] (100 B)
    //   +0x7404: DiLei[10] (480 B)                      ← 此前误用 0x725C
    //   +0x75E4: JiNeng[16] (768 B)                     ← 此前误用 0x743C
    //   +0x78E4: SelectFanwei[16] (1296 B)
    //   +0x7DF0: ApplyFanwei[48] (9408 B)
    //   +0xA310: WuPinValid[160×96] (15360 B) — 兼容矩阵（本加载器按文件末尾 15360 字节读取）
    {
        BinaryReader reader(resolve("GAME.DAT"));

        // ClassType (BingZhong): offset 0x1E0（实证基址；data_dictionary.md 误标 0x0320）。
        // 证据：FUN_0042e2d6.c:257 阵型→兵种 DAT_00447b78={1,4,8,11}，须→
        //   {手枪队,大刀队,步枪队,骑兵队}，仅 0x1E0 成立（0x1E0[1]=手枪队 等）。
        //   PEOPLE.DAT 孙悟空 bingzhong=0→神仙队、李向阳=1→手枪队 亦吻合。
        constexpr size_t kClassTypeOffset = 0x1E0;
        for (int i = 0; i < core::MAX_CLASS_TYPES; ++i) {
            size_t offset = kClassTypeOffset + static_cast<size_t>(i) * sizeof(core::ClassType);
            if (offset + sizeof(core::ClassType) > reader.size()) break;
            reader.read_bytes(offset, &gd.class_types[i], sizeof(core::ClassType));
        }

        // WuPin (items): offset 0x43E0 = 17376
        constexpr size_t kWuPinOffset = 0x43E0;
        for (int i = 0; i < core::MAX_WUPIN; ++i) {
            size_t offset = kWuPinOffset + static_cast<size_t>(i) * sizeof(core::WuPin);
            if (offset + sizeof(core::WuPin) > reader.size()) break;
            reader.read_bytes(offset, &gd.items[i], sizeof(core::WuPin));
        }

        // DiLei (mines): 权威偏移 0x7404（证据 data_dictionary.md §3 + 真实抽取：
        //   0x7404 处 10 条均为合法地雷名 轰天雷/补给/激励/…）。
        // 注意：DiLei/DiLeiRecipe/JiNeng 记录的 name 字段实际位于记录 +0x08
        //   （data_dictionary.md 已注明模板偏移偏差），本结构体 name@+0x00 仅影响名称显示，
        //   数值字段（damage/effect_type 等）随本偏移修正后已对准正确记录。
        constexpr size_t kDiLeiOffset = 0x7404;
        for (int i = 0; i < core::MAX_DILEI; ++i) {
            size_t offset = kDiLeiOffset + static_cast<size_t>(i) * sizeof(core::DiLei);
            if (offset + sizeof(core::DiLei) > reader.size()) break;
            reader.read_bytes(offset, &gd.mines[i], sizeof(core::DiLei));
        }

        // Material[10]: 10 × 32 bytes at offset 0x7260 (29280)
        constexpr size_t kMaterialOffset = 0x7260;
        for (int i = 0; i < 10; ++i) {
            size_t offset = kMaterialOffset + static_cast<size_t>(i) * sizeof(core::Material);
            if (offset + sizeof(core::Material) > reader.size()) break;
            reader.read_bytes(offset, &gd.materials[i], sizeof(core::Material));
        }

        // DiLeiRecipe: 10×10 bytes at offset 0x73A0 (29600)
        constexpr size_t kRecipeOffset = 0x73A0;
        if (kRecipeOffset + sizeof(core::DiLeiRecipe) <= reader.size()) {
            reader.read_bytes(kRecipeOffset, &gd.mine_recipe, sizeof(core::DiLeiRecipe));
        }

        // JiNeng (skills): 权威偏移 0x75E4（证据 data_dictionary.md §3 + 真实抽取：
        //   0x75E4 处 16 条为合法技能名 掠夺/反间/疑兵/督战/暗杀/施毒/空袭）。
        // （name 字段同 DiLei 位于记录 +0x08，见上注。）
        constexpr size_t kJiNengOffset = 0x75E4;
        for (int i = 0; i < core::MAX_SKILLS; ++i) {
            size_t offset = kJiNengOffset + static_cast<size_t>(i) * sizeof(core::JiNeng);
            if (offset + sizeof(core::JiNeng) > reader.size()) break;
            reader.read_bytes(offset, &gd.skills[i], sizeof(core::JiNeng));
        }

        // 兵种×物品兼容性矩阵 battle_config_3 @ 0x95EB88 (= WuPinValid wpv)
        // GAME.DAT 末尾 (MAX_WUPIN × MAX_CLASS_TYPES) 字节 / last 15360 bytes
        // 每行 96 字节（每兵种 1 字节，非零=可装备）
        constexpr size_t kCompatSize =
            static_cast<size_t>(core::MAX_WUPIN) * core::MAX_CLASS_TYPES;  // 160 × 96 = 15360
        if (reader.size() >= kCompatSize) {
            size_t compat_offset = reader.size() - kCompatSize;
            std::vector<uint8_t> buf = reader.read_vector(compat_offset, kCompatSize);
            for (int item_idx = 0; item_idx < core::MAX_WUPIN; ++item_idx) {
                for (int cls = 0; cls < core::MAX_CLASS_TYPES; ++cls) {
                    gd.wupin_compat[item_idx][cls] =
                        buf[static_cast<size_t>(item_idx) * core::MAX_CLASS_TYPES + cls];
                }
            }
        }
    }

    return gd;
}

void DatLoader::load_towns(core::GameData& gd) const {
    BinaryReader reader(resolve("TOWN.DAT"));
    static_assert(core::TOWN_RECORD_SIZE == 346);
    for (int i = 0; i < core::MAX_TOWNS; ++i) {
        size_t offset = static_cast<size_t>(i) * core::TOWN_RECORD_SIZE;
        reader.read_bytes(offset, &gd.towns[i], sizeof(core::Town));
    }
}

core::FightData DatLoader::load_fight(int32_t fight_index) const {
    if (fight_index < 0 || fight_index >= core::MAX_FIGHTS) {
        throw std::out_of_range("fight_index out of range");
    }

    BinaryReader reader(resolve("FIGHT.DAT"));
    // 值初始化：people[0..39]（战役槽，含主角 people[0]）先清零，
    // 由 battle_engine::init() 用角色选择结果填充 people[0]；
    // people[40..127] 由下方磁盘名册覆盖。避免读到未初始化内存。
    core::FightData fd{};

    size_t base = static_cast<size_t>(fight_index) * core::FIGHT_RECORD_SIZE;

    // Intro: 700 bytes (name + difficulty + texts + boxes + grid_terrain[92])
    reader.read_bytes(base, &fd.intro, sizeof(core::FightIntro));

    // Fighter[128]: 128 × 64 = 8192 bytes, offset 700
    constexpr size_t kFighterOffset = 700;
    for (int i = 0; i < 128; ++i) {
        size_t off = base + kFighterOffset
                   + static_cast<size_t>(i) * sizeof(core::FightFighter);
        reader.read_bytes(off, &fd.fighters[i], sizeof(core::FightFighter));
    }

    // People[88]: 88 × 132 = 11616 bytes, offset 700 + 8192 = 8892
    //
    // ★ 关键映射（此前搞错，导致第0关敌人全是"孙悟空"）：
    //   磁盘的 88 条名册**不是** people[0..87]，而是装载到运行时 people 的
    //   **[40..127]** —— 即 people[40+k] = 磁盘[k]。
    //   证据 FUN_004292fe.c:15-18（FIGHT.DAT 装载器）：
    //     FUN_0043e6a0(file, fight_index * 0x501c, 0);      // 0x501c=20508
    //     FUN_0043e510(&DAT_007d01c8, 700, 1,   file);      // intro
    //     FUN_0043e510(&DAT_0096ab90, 0x40, 0x80, file);    // fighters 128×64
    //     FUN_0043e510(&DAT_007d2398, 0x84, 0x58, file);    // ★ people 88×132
    //   其中 DAT_007d2398 − people基址(0x7d0ef8) = 0x14A0 = 40 × 0x84(132)。
    //   旁证 FUN_004292fe.c:20-23：装载后立刻为 0x28(=40)..0x7f 加载头像
    //   （&DAT_007d0f08 + i*0x84，即 people[i].face），区间起点正是 40。
    //   实测 fight0：people[40]=伪军(face78/str70/步枪队)、[45]=碉堡，
    //   与原版截图面板"伪军 武力70 智力60 步枪队 等级2"逐项吻合。
    constexpr size_t kPeopleOffset = 700 + 8192;
    constexpr int kRuntimePeopleBase = 40;   // 磁盘名册在运行时数组中的起始下标
    for (int i = 0; i < 88; ++i) {
        size_t off = base + kPeopleOffset
                   + static_cast<size_t>(i) * sizeof(core::FightPeople);
        int dst = kRuntimePeopleBase + i;
        if (dst >= static_cast<int>(fd.people.size())) break;
        reader.read_bytes(off, &fd.people[dst], sizeof(core::FightPeople));
    }

    return fd;
}

// ============================================================
// MAP.DAT / BLK.DAT — 战斗地图瓦片数据
// 证据: output/map_dat_analysis.md §1/§2（格式）+ scripts/export_map.py（解析实现，
// 渲染结果与原版截图逐像素吻合）。MAP.DAT 与 FIGHT.DAT 按 1:1 索引对应（47 关）。
// ============================================================
core::FightMap DatLoader::load_map(int32_t map_index) const {
    if (map_index < 0 || map_index >= 47) {
        throw std::out_of_range("map_index out of range (0-46)");
    }

    BinaryReader reader(resolve("MAP.DAT"));
    // 512 字节索引表后，47 个地图记录顺序排列（索引表 offset 字段不做随机访问用，
    // 与 export_map.py 一致按顺序读取）。
    size_t off = 0x200;
    for (int32_t i = 0; i <= map_index; ++i) {
        const uint16_t w = reader.read_at<uint16_t>(off);
        const uint16_t h = reader.read_at<uint16_t>(off + 2);
        // +0x04 blk_file[12], +0x10 pal_file[12]
        char blk_file[13] = {};
        reader.read_bytes(off + 4, blk_file, 12);
        const size_t data_off = off + 0x1C;
        const size_t record_size = 0x1C + static_cast<size_t>(w) * h * 2;

        if (i == map_index) {
            core::FightMap fm;
            fm.width = w;
            fm.height = h;
            // "dlz-NN.blk" → NN（证据: map_dat_analysis.md §1.3，digit 在 [4][5]）
            if (blk_file[4] >= '0' && blk_file[4] <= '9') {
                fm.blk_group = (blk_file[4] - '0') * 10;
                if (blk_file[5] >= '0' && blk_file[5] <= '9') {
                    fm.blk_group += blk_file[5] - '0';
                }
            }
            fm.tiles.resize(static_cast<size_t>(w) * h);
            for (size_t t = 0; t < fm.tiles.size(); ++t) {
                fm.tiles[t] = reader.read_at<uint16_t>(data_off + t * 2);
            }
            return fm;
        }
        off += record_size;
    }
    throw std::runtime_error("MAP.DAT: record not found");
}

core::BlkGroupData DatLoader::load_blk_group(int32_t blk_group) const {
    if (blk_group < 0 || blk_group >= 0x40) {
        throw std::out_of_range("blk_group out of range");
    }

    BinaryReader reader(resolve("BLK.DAT"));
    // 64 × (offset u32, length u32) 索引；组数据顺序排列（与 export_map.py 一致）。
    // 简化：直接顺序扫到目标组（组内 tile = 48*48 字节调色板索引）。
    size_t off = 0x200;  // 索引表 64*8 = 512 字节
    for (int32_t g = 0; g < blk_group; ++g) {
        const uint32_t len = reader.read_at<uint32_t>(g * 8 + 4);
        off += len;
    }
    const uint32_t len = reader.read_at<uint32_t>(blk_group * 8 + 4);
    core::BlkGroupData bg;
    const size_t tile_bytes = 48 * 48;
    bg.tiles.reserve(len / tile_bytes);
    for (size_t t = 0; t + tile_bytes <= len; t += tile_bytes) {
        std::vector<uint8_t> tile(tile_bytes);
        reader.read_bytes(off + t, tile.data(), tile_bytes);
        bg.tiles.push_back(std::move(tile));
    }
    return bg;
}

std::vector<std::string> DatLoader::load_dialogues() const {
    BinaryReader reader(resolve("TXT.DAT"));
    std::vector<std::string> dialogues;

    // TXT.DAT: XOR 0x51 encrypted, variable-length records
    // After decryption: "NNN speaker_code text\r\n" format
    // Total size: ~110KB, 593 entries

    std::vector<uint8_t> decrypted(reader.size());
    for (size_t i = 0; i < reader.size(); ++i) {
        decrypted[i] = reader.data()[i] ^ core::TXT_XOR_KEY;
    }

    // Parse entries: look for pattern "NNN " (3-digit index + space)
    std::string text(reinterpret_cast<const char*>(decrypted.data()), decrypted.size());
    size_t pos = 0;

    while (pos < text.size()) {
        // Find entry start: 3 digits followed by space
        if (pos + 4 < text.size() &&
            std::isdigit(static_cast<unsigned char>(text[pos])) &&
            std::isdigit(static_cast<unsigned char>(text[pos+1])) &&
            std::isdigit(static_cast<unsigned char>(text[pos+2])) &&
            text[pos+3] == ' ') {

            // Find entry end: CR LF
            size_t end = text.find("\r\n", pos);
            if (end == std::string::npos) {
                end = text.size();
            }

            // Extract entry (skip "NNN " prefix)
            std::string entry = text.substr(pos + 4, end - pos - 4);
            dialogues.push_back(entry);

            pos = end + 2;  // Skip past CR LF
        } else {
            ++pos;
        }
    }

    return dialogues;
}

std::vector<uint8_t> DatLoader::load_wave_clip(int32_t wave_id) const {
    BinaryReader reader(resolve("WAVE.DAT"));

    // WAVE.DAT: 8KB index + raw PCM
    // 1024 index slots, 152 non-empty clips
    // mono 22050Hz 16-bit
    // Index at offset 0: 1024 entries × {offset(4), size(4)} = 8192 bytes
    // PCM data follows after the index (at file offset 0x2000)
    if (wave_id < 0 || wave_id >= 1024) {
        throw std::out_of_range("wave_id out of range");
    }

    size_t index_offset = static_cast<size_t>(wave_id) * 8;
    uint32_t pcm_offset = reader.read_at<uint32_t>(index_offset);
    uint32_t pcm_size = reader.read_at<uint32_t>(index_offset + 4);

    if (pcm_size == 0) {
        throw std::out_of_range("wave_id has no data");
    }

    // pcm_offset is relative to data section start (0x2000)
    constexpr size_t DATA_SECTION_OFFSET = 0x2000;
    return reader.read_vector(DATA_SECTION_OFFSET + pcm_offset, pcm_size);
}

}  // namespace dlzr::app
