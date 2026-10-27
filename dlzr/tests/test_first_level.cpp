// test_first_level.cpp — 第一关「数据装载保真」无界面测试
//
// ⚠️ 诚实范围声明（用户 2026-10-23 指正）：
// 本测试**不能**验证"真实的第一关"。真实第一关 = 原版 EXE 的完整链路
// （Mode4 角色选择 → FUN_0042e2d6 CONFIRM → PTR_FUN_004465d0[i] 关卡回调
// → FUN_0043dd47 开场脚本 → Mode150 战斗），其部署/AI/对话全由原版代码决定，
// 本 headless 引擎是**克隆重建**，不是原版。
//
// 本测试唯一可证伪、且有意义的范围：**克隆是否正确读入了原版的数据文件**
// （FIGHT.DAT / PEOPLE.DAT / TXT.DAT 的字节 → 引擎数据模型）。这是真实可
// 验证的：一旦磁盘→运行时映射再写错（如之前 people[40] 被当成 孙悟空），
// 此测试立即红。游戏性保真（单位怎么动、菜单在哪、敌人几个）必须由人类
// 对照原版截图/录屏判定，不在本测试职责内。
//
// 实现方式遵循新工作流程：每个断言都对应一条可查的原版数据事实（FIGHT.DAT
// 实测 / TXT.DAT 解码），而非我对"游戏应该长什么样"的想象。

#include "app/resource/dat_loader.hpp"
#include "app/resource/army_loader.hpp"
#include "app/resource/img_loader.hpp"
#include "logic/game_state_manager.hpp"
#include "logic/battle/battle_engine.hpp"
#include "logic/dialogue/dialogue_manager.hpp"
#include "logic/level/level_event_manager.hpp"
#include "core/data_model/game_data.hpp"

#include <gtest/gtest.h>
#include <filesystem>
#include <cstring>
#include <vector>

using namespace dlzr::logic;
using namespace dlzr::core;

namespace {

const char* kGameDataDir = "C:/seesaw/source/dlz_research/DLZFULL";

// 伪军（磁盘名册首条，FIGHT.DAT people[40]）的 GBK 名前缀与兵种索引
const char* kWeiJunPrefix = "\xCE\xB1\xA1\xA1\xBE\xFC";  // 伪军
const char* kSunWukong     = "\xCB\xEF\xCE\xF2\xBF\xD5";  // 孙悟空（占位，绝不可出现）

}  // namespace

// ---------------------------------------------------------------------------
// 数据保真：磁盘 88 条名册必须映射到运行时 people[40..127]
// （FUN_004292fe.c:18 DAT_007d2398 = people基址 + 40×0x84）。
// people[40] 应为磁盘[0] = 伪军（步枪队 class 50），绝不是占位 孙悟空。
// ---------------------------------------------------------------------------
TEST(FirstLevelTest, Fight0DiskToRuntimeMapping) {
    ASSERT_TRUE(std::filesystem::exists(kGameDataDir));
    dlzr::app::DatLoader loader(kGameDataDir);
    auto fight = loader.load_fight(0);

    // people[40] 必须落在名册首条（伪军），且长度为完整 GBK 名
    ASSERT_GE(fight.people[40].name.size(), 6);
    std::string first(fight.people[40].name.data(),
                      strnlen(fight.people[40].name.data(), 6));
    EXPECT_EQ(first, kWeiJunPrefix)
        << "people[40] 应为磁盘名册首条 伪军（磁盘→运行时 +40 偏移被改回？）";
    EXPECT_EQ(fight.people[40].class_type, 50)
        << "伪军应为步枪队(class 50)";

    // 关键不变式：任何**实际部署**（grid_x != 999）的战斗位，其映射到的名册
    // 条目绝不许是占位 孙悟空。引擎部署逻辑为 rt.people_idx = i（i<88 才部署，
    // 见 battle_engine.cpp::init_fighters），故上场单位即 people[i]。
    // 注意：名册尾部 123..127 在原版数据里本就名为 孙悟空（未使用的占位槽），
    // 其对应 fighters 槽位（>=88）引擎不部署，属正常，不在此断言范围内。
    for (int i = 0; i < static_cast<int>(fight.fighters.size()); ++i) {
        if (fight.fighters[i].grid_x == 999) continue;  // 未部署槽位跳过
        if (i >= static_cast<int>(fight.people.size())) continue;  // 引擎不部署 >=88
        std::string nm(fight.people[i].name.data(),
                       strnlen(fight.people[i].name.data(),
                               fight.people[i].name.size()));
        EXPECT_NE(nm, kSunWukong)
            << "部署槽位 " << i << " 映射到占位 孙悟空（槽位→名册映射错误）";
    }
}

// ---------------------------------------------------------------------------
// 数据保真：FIGHT.DAT fight0 部署位 40/41 实测 grid_x!=999 且坐标 = (6,17)/(7,17)，
// stage=1（= 第一回合登场单位）。这里钉死"克隆解析出的 fighters 与文件字节一致"。
// （注：真实第一回合只显示这 2 个敌人由原版逐回合激活逻辑决定，属游戏性，
//   由人类对照原版判定；本测试只保证数据被正确读入。）
// ---------------------------------------------------------------------------
TEST(FirstLevelTest, Fight0FighterDeployDataMatchesFile) {
    ASSERT_TRUE(std::filesystem::exists(kGameDataDir));
    dlzr::app::DatLoader loader(kGameDataDir);
    auto fight = loader.load_fight(0);

    ASSERT_LT(40, static_cast<int>(fight.fighters.size()));
    ASSERT_LT(41, static_cast<int>(fight.fighters.size()));

    EXPECT_NE(fight.fighters[40].grid_x, 999)
        << "槽位 40 应是已部署（grid_x != 999）";
    EXPECT_NE(fight.fighters[41].grid_x, 999)
        << "槽位 41 应是已部署（grid_x != 999）";

    // 实测坐标（来自 FIGHT.DAT fight0 字节级 dump）
    EXPECT_EQ(fight.fighters[40].grid_x, 6);
    EXPECT_EQ(fight.fighters[40].grid_y, 17);
    EXPECT_EQ(fight.fighters[41].grid_x, 7);
    EXPECT_EQ(fight.fighters[41].grid_y, 17);
    EXPECT_EQ(fight.fighters[40].stage, 1);
    EXPECT_EQ(fight.fighters[41].stage, 1);
}

// ---------------------------------------------------------------------------
// 数据保真：战前对话事件表（level_0_pre_battle）必须返回 TXT[21]，且
// TXT[21] 解码后说话人码 = 01（大队长），正文以"大队长"开头。
// 这钉死"战前对话链路没被静默吞掉"，且文本/说话人取自真实 TXT.DAT。
// ---------------------------------------------------------------------------
TEST(FirstLevelTest, Fight0PreBattleDialogueData) {
    ASSERT_TRUE(std::filesystem::exists(kGameDataDir));
    dlzr::app::DatLoader loader(kGameDataDir);
    GameStateManager gsm;
    gsm.init(loader);

    dlzr::logic::DialogueManager dm;
    dm.load_dialogues(loader.load_dialogues());
    ASSERT_GT(dm.dialogue_count(), 0);

    dlzr::logic::LevelEventManager level_events;
    auto events = level_events.get_pre_battle_events(0, gsm.game_data());
    ASSERT_FALSE(events.dialogue_entries.empty())
        << "第 0 关战前对话条目不应为空";

    // 主验证：事件表首条必须是 TXT[21]（大队长开场白，唯一经数据核实的战前线）
    EXPECT_EQ(events.dialogue_entries[0], 21)
        << "战前对话首条应为 TXT[21]（大队长开场白）";

    const std::string raw = dm.get_dialogue_text(21);
    auto parsed = dlzr::logic::DialogueManager::parse_dialogue(raw);
    EXPECT_EQ(parsed.speaker_id, 1)
        << "TXT[21] 说话人码应为 01（大队长）";
    EXPECT_NE(parsed.text.find("\xB4\xF3\xB6\xD3\xB3\xA4"), std::string::npos)  // 大队长
        << "TXT[21] 正文应以「大队长」开头，实际=" << parsed.text;
}

// ---------------------------------------------------------------------------
// 重建冒烟：用真实数据跑 BattleEngine::init 不崩溃、有部署单位。
// 标签明确：这是"克隆重建能跑通"，**不是**原版游戏性保真。游戏性需人工对照。
// ---------------------------------------------------------------------------
TEST(FirstLevelTest, Fight0EngineConstructsWithoutCrash) {
    ASSERT_TRUE(std::filesystem::exists(kGameDataDir));
    dlzr::app::DatLoader loader(kGameDataDir);
    GameStateManager gsm;
    gsm.init(loader);

    auto fight = loader.load_fight(0);
    auto map = loader.load_map(0);
    BattleEngine engine;
    engine.set_map(&map);
    engine.init(fight, gsm.game_data(), 1);  // 不崩溃即通过

    const auto& st = engine.state();
    EXPECT_GT(st.fighter_count, 0) << "应有部署单位（重建冒烟）";
    EXPECT_FALSE(engine.is_over());
}

// ---------------------------------------------------------------------------
// 数据保真：首关战斗部署必须"恰好 1 个我方单位（主角）+ 敌人数 = FIGHT.DAT stage==1 数"。
// 证据（铁律零，逐字节实证，见 battle_engine.cpp:238-251 + FIGHT.DAT fight0 字节级 dump）：
//   · FIGHT.DAT fight0 全部 88 名 people[] 均为 enemy=1（敌军名册），玩家单位不在 FIGHT.DAT 内；
//     主角来自战役大军 game.people[0]，由 CONFIRM 分支写入，运行时覆盖 people[0] 的"伪军"占位。
//   · 敌方登场单位 = 磁盘 fighters[].stage==1；我方仅主角 1 员（战役招募体系尚未建模，不臆造同伴）。
//   · 主角部署坐标 = intro.grid_param（fight0 实测 = (9,25)），即相机中心/主角锚点。
// 本测试把这段注释级断言变成可证闸门：若 load_fight(0) 实际产出我方单位、或部署循环
// 错配阵营，立即红。
// ---------------------------------------------------------------------------
TEST(FirstLevelTest, Fight0DeploysExactlyOnePlayerPlusEnemies) {
    ASSERT_TRUE(std::filesystem::exists(kGameDataDir));
    dlzr::app::DatLoader loader(kGameDataDir);
    GameStateManager gsm;
    gsm.init(loader);

    auto fight = loader.load_fight(0);
    auto map = loader.load_map(0);

    // 磁盘登场敌人数 = stage==1 的 fighters 数（干净层 init 按此部署敌方）。
    int32_t disk_stage1 = 0;
    for (int i = 0; i < 128; ++i) {
        if (fight.fighters[i].stage == 1) ++disk_stage1;
    }
    ASSERT_GT(disk_stage1, 0) << "fight0 应有登场敌军（否则测试无意义）";

    BattleEngine engine;
    engine.set_map(&map);
    engine.init(fight, gsm.game_data(), 1);

    const auto& st = engine.state();
    int32_t player_units = 0, enemy_units = 0;
    int32_t hero_idx = -1;
    for (int i = 0; i < BattleState::MAX_FIGHTERS; ++i) {
        const auto& f = st.fighters[i];
        if (!f.is_active) continue;
        if (f.team == 0) { ++player_units; hero_idx = i; }
        else if (f.team == 1) { ++enemy_units; }
    }

    // ★ 核心不变量：首关恰好 1 个我方单位（主角），绝不臆造同伴。
    EXPECT_EQ(player_units, 1)
        << "fight0 应恰好部署 1 个我方单位（主角）；若 >1 说明错误地把 FIGHT.DAT 名册当玩家单位";
    EXPECT_EQ(enemy_units, disk_stage1)
        << "登场敌人数应 == 磁盘 stage==1 数（部署循环阵营错配？）";

    // 主角部署坐标 = intro.grid_param（相机中心/主角锚点）。
    ASSERT_GE(hero_idx, 0) << "应存在主角";
    EXPECT_EQ(st.fighters[hero_idx].grid_x, fight.intro.grid_param_x)
        << "主角 grid_x 应 == intro.grid_param_x（见 battle_engine.cpp:244）";
    EXPECT_EQ(st.fighters[hero_idx].grid_y, fight.intro.grid_param_y)
        << "主角 grid_y 应 == intro.grid_param_y";
}

// ---------------------------------------------------------------------------
// 数据保真：GAME.DAT 兵种表（@0x1E0，stride 0xA0=160B）磁盘字节 → 引擎 ClassType。
// 二进制实测（Python 抽取 DLZFULL/GAME.DAT，GBK 解码）：
//   [0]=神仙队 [1]=手枪队 [4]=大刀队 [8]=步枪队 [11]=骑兵队。
// 钉死"loader 的兵种表基址/步长/字段偏移"与二进制一致 —— 此前 data_dictionary.md
// 误标 0x0320（差 320 字节，整表后移），以本测试 + 字节实证为准（铁律零）。
// （注：兵种名位于记录 +0x00，无 DiLei/JiNeng 那种 +0x08 偏移怪癖。）
// ---------------------------------------------------------------------------
TEST(FirstLevelTest, GameDataClassTableMatchesBinary) {
    ASSERT_TRUE(std::filesystem::exists(kGameDataDir));
    dlzr::app::DatLoader loader(kGameDataDir);
    auto gd = loader.load_game_data();

    auto name_eq = [](const std::array<char, 16>& nm,
                      const std::vector<uint8_t>& exp) -> bool {
        const auto* p = reinterpret_cast<const uint8_t*>(nm.data());
        for (size_t i = 0; i < exp.size(); ++i)
            if (p[i] != exp[i]) return false;
        return p[exp.size()] == 0;  // 名后紧跟 '\0'（GBK 名 6 字节）
    };

    EXPECT_TRUE(name_eq(gd.class_types[0].name,  {0xC9,0xF1,0xCF,0xC9,0xB6,0xD3})) << "class[0] 应为 神仙队";
    EXPECT_TRUE(name_eq(gd.class_types[1].name,  {0xCA,0xD6,0xC7,0xB9,0xB6,0xD3})) << "class[1] 应为 手枪队";
    EXPECT_TRUE(name_eq(gd.class_types[4].name,  {0xB4,0xF3,0xB5,0xB6,0xB6,0xD3})) << "class[4] 应为 大刀队";
    EXPECT_TRUE(name_eq(gd.class_types[8].name,  {0xB2,0xBD,0xC7,0xB9,0xB6,0xD3})) << "class[8] 应为 步枪队";
    EXPECT_TRUE(name_eq(gd.class_types[11].name, {0xC6,0xEF,0xB1,0xF8,0xB6,0xD3})) << "class[11] 应为 骑兵队";
}

// ---------------------------------------------------------------------------
// 数据保真：PEOPLE.DAT（40 条 × 132B，name@+0x00 / class_type@+0x2C）磁盘字节
// → 引擎 People。二进制实测：people[0]=孙悟空(bingzhong=0→神仙队)、
// people[4]=李向阳(bingzhong=1→手枪队)。钉死"loader 的 PEOPLE.DAT 基址/步长/
// 字段偏移"与二进制一致。
// ---------------------------------------------------------------------------
TEST(FirstLevelTest, PeopleDataMatchesBinary) {
    ASSERT_TRUE(std::filesystem::exists(kGameDataDir));
    dlzr::app::DatLoader loader(kGameDataDir);
    auto gd = loader.load_game_data();

    auto name_eq = [](const std::array<char, 16>& nm,
                      const std::vector<uint8_t>& exp) -> bool {
        const auto* p = reinterpret_cast<const uint8_t*>(nm.data());
        for (size_t i = 0; i < exp.size(); ++i)
            if (p[i] != exp[i]) return false;
        return p[exp.size()] == 0;
    };

    EXPECT_TRUE(name_eq(gd.people[0].name, {0xCB,0xEF,0xCE,0xF2,0xBF,0xD5})) << "people[0] 应为 孙悟空";
    EXPECT_EQ(gd.people[0].class_type, 0) << "孙悟空 bingzhong=0 → 神仙队";
    EXPECT_TRUE(name_eq(gd.people[4].name, {0xC0,0xEE,0xCF,0xF2,0xD1,0xF4})) << "people[4] 应为 李向阳";
    EXPECT_EQ(gd.people[4].class_type, 1) << "李向阳 bingzhong=1 → 手枪队";
}

// ---------------------------------------------------------------------------
// 数据保真：ARMY.DAT（91 条 × 19 帧 × 48×48 8-bit 调色板精灵）。
// 二进制实测：DLZFULL/ARMY.DAT 大小 = 3983616 = 91 × 0xAB00（每条 19×2304B）。
// 差分断言：loader 读出的「帧字节索引」须与磁盘字节一致，且经权威调色板解码的
// RGBA 与二进制同址字节吻合（证明 entry/frame/pixel → 字节偏移布局 1:1）。
// ---------------------------------------------------------------------------
TEST(AssetLoaderDiffTest, ArmyDatStructureMatchesBinary) {
    const std::string path = std::string(kGameDataDir) + "/ARMY.DAT";
    ASSERT_TRUE(std::filesystem::exists(path)) << path;

    // 1) 文件大小 = 91 × 0xAB00（二进制实测钉死）
    const int64_t expected_size =
        static_cast<int64_t>(dlzr::app::ArmyLoader::ENTRY_COUNT) *
        dlzr::app::ArmyLoader::ENTRY_SIZE;
    EXPECT_EQ(std::filesystem::file_size(path), expected_size)
        << "ARMY.DAT 大小须 = 91 × 0xAB00";

    // 2) 全量加载 + 结构非 null（每 entry × frame 均可取 RGBA）
    dlzr::app::ArmyLoader loader;
    ASSERT_TRUE(loader.load(path));
    ASSERT_TRUE(loader.is_loaded());

    const uint8_t* pal = dlzr::app::ArmyLoader::palette();
    ASSERT_NE(pal, nullptr);

    // 直接读磁盘字节用于差分比对
    std::ifstream bin(path, std::ios::binary);
    ASSERT_TRUE(bin);
    std::vector<uint8_t> raw(expected_size);
    bin.read(reinterpret_cast<char*>(raw.data()), expected_size);
    ASSERT_EQ(bin.gcount(), expected_size);

    // 3) 抽样 entry/frame/pixel：loader 解码 RGBA == 权威调色板[磁盘索引]
    //    覆盖边界 entry(0/31/90)、边界帧(0/18)、边界像素(0/中线/末像素)。
    const int ICON = dlzr::app::ArmyLoader::ICON_SIZE;   // 2304
    const int ENTRY = dlzr::app::ArmyLoader::ENTRY_SIZE; // 0xAB00
    int samples = 0;
    for (int e : {0, 31, 90}) {
        for (int f : {0, 18}) {
            const uint8_t* frgba = loader.get_frame_rgba(e, f);
            ASSERT_NE(frgba, nullptr) << "entry " << e << " frame " << f;
            for (int p : {0, ICON / 2, ICON - 1}) {
                const size_t off =
                    static_cast<size_t>(e) * ENTRY +
                    static_cast<size_t>(f) * ICON + p;
                const uint8_t idx = raw[off];
                EXPECT_EQ(frgba[p * 4 + 0], pal[idx * 4 + 0]) << "e" << e << "f" << f << "p" << p;
                EXPECT_EQ(frgba[p * 4 + 1], pal[idx * 4 + 1]) << "e" << e << "f" << f << "p" << p;
                EXPECT_EQ(frgba[p * 4 + 2], pal[idx * 4 + 2]) << "e" << e << "f" << f << "p" << p;
                EXPECT_EQ(frgba[p * 4 + 3], (idx == 0 ? 0 : 0xFF)) << "e" << e << "f" << f << "p" << p;
                ++samples;
            }
        }
    }
    EXPECT_GE(samples, 18);
}

// ---------------------------------------------------------------------------
// 数据保真：BIGMAP.IMG（战略/战斗地图背景，IMG 8-bit 调色板索引格式）。
// 差分断言：loader 解出的 width/height/pixel 索引/调色板 须与磁盘字节一致
// （header width_m1@+0x06、height_m1@+0x08、pixels@+0x10、palette@+0x10+W*H，
// 6-bit 分量 ×4 → 8-bit）。
// ---------------------------------------------------------------------------
TEST(AssetLoaderDiffTest, BigmapImgMatchesBinary) {
    const std::string path = std::string(kGameDataDir) + "/BIGMAP.IMG";
    ASSERT_TRUE(std::filesystem::exists(path)) << path;

    auto img = dlzr::app::ImgLoader::load(path);

    // 直接读磁盘字节用于差分比对
    std::ifstream bin(path, std::ios::binary | std::ios::ate);
    ASSERT_TRUE(bin);
    const auto fsize = bin.tellg();
    std::vector<uint8_t> raw(static_cast<size_t>(fsize));
    bin.seekg(0);
    bin.read(reinterpret_cast<char*>(raw.data()), fsize);

    // header: width_m1@+0x06, height_m1@+0x08
    uint16_t wm1 = 0, hm1 = 0;
    std::memcpy(&wm1, raw.data() + 0x06, 2);
    std::memcpy(&hm1, raw.data() + 0x08, 2);
    EXPECT_EQ(img.width, static_cast<uint16_t>(wm1 + 1)) << "BIGMAP width";
    EXPECT_EQ(img.height, static_cast<uint16_t>(hm1 + 1)) << "BIGMAP height";

    const size_t pixcount = static_cast<size_t>(img.width) * img.height;
    EXPECT_EQ(img.pixels.size(), pixcount) << "pixel count";

    // pixels@+0x10：逐字节比对（抽样首尾 + 中线）
    for (size_t off : {size_t{0}, pixcount / 2, pixcount - 1}) {
        ASSERT_LT(off, pixcount);
        EXPECT_EQ(img.pixels[off], raw[0x10 + off]) << "pixel@" << off;
    }

    // palette@+0x10+pixcount：256×3，6-bit 分量 ×4 → 8-bit
    const size_t pal_off = 0x10 + pixcount;
    ASSERT_LE(pal_off + 256 * 3, raw.size());
    for (int i = 0; i < 256; ++i) {
        for (int c = 0; c < 3; ++c) {
            const uint8_t expected =
                static_cast<uint8_t>(raw[pal_off + i * 3 + c] * 4);
            EXPECT_EQ(img.palette.colors[i][c], expected)
                << "palette[" << i << "][" << c << "]";
        }
    }
}
