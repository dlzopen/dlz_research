// dat_loader.cpp — DAT 文件加载实现
// DAT file loading implementation
//
// 直接读取原始二进制文件，结构体布局严格匹配原作内存布局。
// Reads original binary files directly; struct layouts match original memory.
//
// ╔══════════════════════════════════════════════════════════════════════════╗
// ║ 铁律级纪律：所有从二进制文件读取的数据，必须参考 010 Editor 模板           ║
// ║ (template/*.bt) 作为权威 schema。模板（含 dlz_game.bt 的「扁平拼接偏移」） ║
// ║ 经验证是准确的：用模板真实 struct 尺寸重算 GAME.DAT 扁平布局，得到        ║
// ║   ClassType@0x1E0 · WuPin@0x43E0 · Material@0x70E0 · DiLeiRecipe@0x71F8 · ║
// ║   DiLei@0x725C · JiNeng@0x743C · ApplyFanWei@0x7C4C(=技能形状表 off 31820)║
// ║ 与真实二进制逐字节 decode 完全一致；ApplyFanWei@0x7C4C 亦与代码注释的     ║
// ║ 技能形状表偏移吻合。2026-10-28 修复的 bug 源自我方代码（JiNeng/DiLei 误加 ║
// ║ 8B phantom 头 + 装载器偏移误写成 0x75E4/0x7404），非模板之过。           ║
// ║   · 因此：禁止脱离模板手造结构体 / 偏移；改 / 新增文件读取须对照对应 .bt  ║
// ║     并用真实二进制 decode 复核（铁律零「望文生义」陷阱）。                 ║
// ║   · 注意：Material 在模板为 28B（name16+unit8+price4），本工程 C++ 曾误   ║
// ║     加成 32B 且偏移误置 0x7260；DiLeiRecipe 偏移曾误置 0x73A0。二者须按   ║
// ║     模板修正为 Material@0x70E0(28B) / DiLeiRecipe@0x71F8(10B)，否则与    ║
// ║     DiLei@0x725C 重叠读脏数据（见下方 load_game_data 修正）。            ║
// ║   各装载器下方均标注其对应模板文件名，新增 / 改动文件读取代码须同步标注。    ║
// ╚══════════════════════════════════════════════════════════════════════════╝

#include "app/resource/dat_loader.hpp"

#include "core/data_model/constants.hpp"
#include "logic/combat/combat_formulas.hpp"
#include "replica/battle_calc.hpp"
#include "replica/battle_sim.hpp"

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
    //   +0x70E0: Material[10] (280 B)                   （010 模板 dlz_game.bt Material 结构：name16+unit8+price4=28B）
    //   +0x71F8: DiLeiRecipe[10] (100 B)                （010 模板 DiLeiRecipe：10×10 配方矩阵）
    //   +0x725C: DiLei[10] (480 B)                      （010 模板结构 + GAME.DAT 实证；旧装载器误写 0x7404）
    //   +0x743C: JiNeng[16] (768 B)                     （010 模板结构 + GAME.DAT 实证；旧装载器误写 0x75E4）
    //   +0x773C: SelectFanwei[16] (1296 B)
    //   +0x7C4C: ApplyFanWei[48] (9408 B)               （= 技能形状表 off 31820，FUN_00424566 块#11）
    //   +0xA41C: WuPinValid[160×96] (15360 B) — 兼容矩阵（文件末 15360 字节）
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

        // DiLei (mines): 权威偏移 0x725C（证据：template/dlz_game.bt DiLei 结构 +
        //   DLZFULL/GAME.DAT @0x725C 逐字节抽取，名称位于 +0x00 无前导头；旧 0x7404 错位 0x1A8）。
        //   地雷名 土雷/石雷/铁雷/伪装雷/蝎子雷/瘴气雷/磁电雷/风火雷/彻地雷/轰天雷。
        constexpr size_t kDiLeiOffset = 0x725C;
        for (int i = 0; i < core::MAX_DILEI; ++i) {
            size_t offset = kDiLeiOffset + static_cast<size_t>(i) * sizeof(core::DiLei);
            if (offset + sizeof(core::DiLei) > reader.size()) break;
            reader.read_bytes(offset, &gd.mines[i], sizeof(core::DiLei));
        }

        // Material[10]: 10 × 28 bytes at offset 0x70E0 (29024)。
        // 证据：template/dlz_game.bt Material 结构（name16+unit8+price4=28B）+ 真实
        //   GAME.DAT @0x70E0 逐字节 decode：粘土/石头/木料/铁块/火硝/硫磺/木炭粉/蝎钩/
        //   毒瘴菇/电池（单位 斤/块/两/只/个，价格 1/1/2/4/2/2/2/10/10/80）。
        //   ⚠️ 旧装载器误置 0x7260 且 C++ 误加成 32B，落入 DiLei 块读脏数据。
        constexpr size_t kMaterialOffset = 0x70E0;
        for (int i = 0; i < 10; ++i) {
            size_t offset = kMaterialOffset + static_cast<size_t>(i) * sizeof(core::Material);
            if (offset + sizeof(core::Material) > reader.size()) break;
            reader.read_bytes(offset, &gd.materials[i], sizeof(core::Material));
        }

        // DiLeiRecipe: 10×10 bytes at offset 0x71F8 (29176)。
        // 证据：template/dlz_game.bt DiLeiRecipe（char recipe[10]）+ 真实 GAME.DAT
        //   @0x71F8 逐字节 decode：row0=[10,0,0,0,5,10,15,0,0,0] …（地雷配方矩阵）。
        //   ⚠️ 旧装载器误置 0x73A0，落入 DiLei 块读脏数据。
        constexpr size_t kRecipeOffset = 0x71F8;
        if (kRecipeOffset + sizeof(core::DiLeiRecipe) <= reader.size()) {
            reader.read_bytes(kRecipeOffset, &gd.mine_recipe, sizeof(core::DiLeiRecipe));
        }

        // JiNeng (skills): 权威偏移 0x743C（证据：template/dlz_game.bt JiNeng 结构 +
        //   DLZFULL/GAME.DAT @0x743C 逐字节抽取，名称位于 +0x00 无前导头；旧 0x75E4 错位 0x1A8）。
        //   技能名 补给/激励/医疗/火攻/滚木/擂石/威慑/骚扰/破坏/掠夺/反间/疑兵/督战/暗杀/施毒/空袭。
        constexpr size_t kJiNengOffset = 0x743C;
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

        // 兵种地形移动消耗矩阵 BingZhongYiDong @ GAME.DAT +0x3F20
        // 96 兵种 × 16 地形 × uint8（二进制实测：class[0]=[1×14,99,99]、
        // class[1] 地形3/4=2）。值=移动消耗，99=不可通行。
        constexpr size_t kMoveCostOffset = 0x3F20;
        for (int i = 0; i < core::MAX_CLASS_TYPES; ++i) {
            size_t offset = kMoveCostOffset + static_cast<size_t>(i) * 16;
            if (offset + 16 > reader.size()) break;
            reader.read_bytes(offset, &gd.class_move_cost[i], 16);
        }

        // 地形修正表 DiXingModifier @ GAME.DAT +0x00A0
        // 二进制实测（FUN_00424566.c:15 装载 16×0x14=320B；FUN_0042a98e.c:21 /
        // FUN_004138ea.c:262/310 按 20B(=5×int32) 记录索引）：
        //   int0=mod_a(case3/4/5) int1=mod_b(case6) int2=防御加成%(case def) int3/int4 保留。
        // ⚠️ 旧实现读 40B/地形是读穿进相邻块的 bug；以 20B/记录（5 int）为准。
        constexpr size_t kTerrainModOffset = 0x00A0;
        int32_t terrain_def_bonus[16];
        for (int t = 0; t < 16; ++t) {
            size_t offset = kTerrainModOffset + static_cast<size_t>(t) * 20;
            if (offset + 20 > reader.size()) break;
            int32_t rec[5];
            reader.read_bytes(offset, rec, 20);
            for (int j = 0; j < 5; ++j) gd.terrain_modifiers[t][j] = rec[j];
            terrain_def_bonus[t] = rec[2];   // int2 = 防御加成%
        }
        dlzr::logic::set_terrain_defense_bonus(terrain_def_bonus);

        // ★ 真实数据运行时装载接线（residual #1 接 DatLoader 统一装载点）：
        //   FUN_00424566 装载 GAME.DAT 块#1(地形修正) → replica 三层视图（防御加成 / mod_a / mod_b）；
        //   块#11(技能形状表, off 31820=0x7C4C, 9408B=0x24C0) → g_battlecalc_skill_shape_raw(=DAT_007cdd00)。
        //   证据 FUN_00424566.c:15(地形) / .c:24(技能形状表, game.dat 顺序块#11)。
        battle_calc_install_terrain_table(&gd.terrain_modifiers[0][0]);
        battle_calc_install_terrain_mod_ab(&gd.terrain_modifiers[0][0]);
        {
            constexpr size_t kSkillShapeOff  = 31820;  // 0x7C4C：前 10 块 (param_2*param_3) 累加
            constexpr size_t kSkillShapeSize = 9408;   // 0x24C0：48 条目 × 196B
            if (reader.size() >= kSkillShapeOff + kSkillShapeSize) {
                std::vector<uint8_t> skill_raw = reader.read_vector(kSkillShapeOff, kSkillShapeSize);
                battle_calc_install_skill_shape_table(skill_raw.data());
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

            // ★ 真实地形网格接线（residual #1 接 DatLoader 统一装载点）：
            //   FUN_00416c70 按 fight_index fread MAP.DAT 瓦片 → 0x584390 地形网格全局。
            //   oracle battle_calc_equip_defense(0x0042a95b) 现读真实地形而非恒为 0。
            battle_calc_install_terrain_grid(fm.tiles.data(), fm.tiles.size(), fm.width);
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

    // TXT.DAT 解码（铁律零实证）：
    //   FUN_004246ab.c:59 在启动时调一次 FUN_0043c9ec(s_txt_dat_00447558) 打开 TXT.DAT
    //   （PE 静态全局 = 文件名）；PE 数据实测 XOR 0x51 解码后为可读 GBK 文本。
    //
    //   ⚠️ 注意（诚实分叉，非猜测）：本 clean 层采用「3 位十进制索引 + 空格（"NNN "）
    //   行首聚合 → 593 条」模型，是 Tier-B 重构选择，并**未**与原版对拍：
    //     - 原版 FUN_0043c9ec 实为「控制码 token 抽取器」：按字形属性表
    //       PTR_DAT_004487c0[c*2] & 4（FUN_0043e890）识别 token 起点
    //       {0x01,0x03,'5'..'9',':',';','<','=','>'}，抽取 497 个 token-body 进
    //       DAT_004569c0、索引进 DAT_0047258c；token 之外的散文被丢弃（运行时按
    //       原始缓冲 + 控制码渲染）。见 test_txt_loader.cpp 的忠实重演（497 vs 593）。
    //     - 原版从不按 "NNN " 行号前缀切分；本层 593 条数来自自身模型，不证明与原版一致。
    //   既有 test_dialogue.cpp::MatchesBinaryBruteExtraction 仅用同一「NNN 」模型自校验。
    //
    //   条目可含**内嵌换行**（正文视觉折行），故不能按 "\r\n" 切分，
    //   必须按「索引行首（"NNN " 或 "-1 "）」聚合为条目（与 parse_dialogue 的粘连语义一致）。
    //   "-1" 续行（诗词旁白）用全角空格分隔，属同一 NNN 条目内的续行，不另起条目。
    //   共 593 条（clean 层自身「NNN 」行首聚合实测值）。

    std::vector<uint8_t> raw(reader.size());
    for (size_t i = 0; i < reader.size(); ++i) {
        raw[i] = reader.data()[i] ^ core::TXT_XOR_KEY;   // XOR 0x51
    }
    std::string text(raw.size(), ' ');
    for (size_t i = 0; i < raw.size(); ++i) text[i] = static_cast<char>(raw[i]);

    // 按行切分（CRLF→LF，丢弃裸 \r）
    std::vector<std::string> lines;
    {
        std::string cur;
        for (char c : text) {
            if (c == '\n') { lines.push_back(cur); cur.clear(); }
            else if (c != '\r') cur += c;
        }
        if (!cur.empty()) lines.push_back(cur);
    }

    auto is_entry_start = [](const std::string& s) {
        if (s.size() >= 4 && std::isdigit(static_cast<unsigned char>(s[0])) &&
            std::isdigit(static_cast<unsigned char>(s[1])) &&
            std::isdigit(static_cast<unsigned char>(s[2])) && s[3] == ' ')
            return true;
        // "-1 " 行首（ASCII 空格）亦可起头；全角空格分隔的 "-1" 续行不算。
        if (s.size() >= 3 && s[0] == '-' && s[1] == '1' && s[2] == ' ')
            return true;
        return false;
    };

    std::string entry;
    bool in_entry = false;
    for (const auto& ln : lines) {
        if (is_entry_start(ln)) {
            if (in_entry) dialogues.push_back(entry);
            size_t skip = (ln.size() >= 3 && ln[0] == '-' && ln[1] == '1' && ln[2] == ' ') ? 3 : 4;
            entry = ln.substr(skip);   // 跳过 "NNN "（4字节）或 "-1 "（3字节）索引前缀
            in_entry = true;
        } else if (in_entry) {
            entry += '\n' + ln;        // 保留内嵌换行
        }
    }
    if (in_entry) dialogues.push_back(entry);

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
