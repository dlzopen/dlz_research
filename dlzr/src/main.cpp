// dlzr — 抗日之地雷战 1:1 复刻
// Landmine War 1:1 Remake

#include "platform/platform.hpp"
#include "platform/crash_handler.hpp"
#include "core/data_model/constants.hpp"
#include "core/data_model/types.hpp"

#include "heartbeat.hpp"

#include <algorithm>
#include <string>
#include <thread>
#include "app/resource/dat_loader.hpp"
#include "app/resource/save_loader.hpp"
#include "app/resource/face_loader.hpp"
#include "app/resource/army_loader.hpp"
#include "app/resource/flm_loader.hpp"
#include "logic/game_state_manager.hpp"
#include "logic/battle/battle_engine.hpp"
#include "logic/battle/battle_settlement.hpp"
#include "presentation/font/font_renderer.hpp"
#include "presentation/image/image_cache.hpp"
#include "presentation/audio/audio_manager.hpp"
#include "presentation/animation/animation_player.hpp"
#include "presentation/title/title_renderer.hpp"
#include "presentation/title/title_input.hpp"
#include "presentation/character_select/character_select_renderer.hpp"
#include "presentation/character_select/character_select_input.hpp"
#include "presentation/dialogue/dialogue_renderer.hpp"
#include "presentation/dialogue/dialogue_input.hpp"
#include "presentation/battle/battle_renderer.hpp"
#include "presentation/battle/battle_input_handler.hpp"
// ↑ 自创的"战斗简报屏"已删除。RE 证据（2026-10-23 复核）：
//   原版确定链路 FUN_0042e2d6(确定)→FUN_0042e0f1(幻灯片)→FUN_0041b8e9(进战斗)
//   →FUN_004294be(部署)→PTR_FUN_004465d0[0]=FUN_0040be07(网格/实体放置)
//   全链路**没有任何独立简报屏**；第 0 关回调不弹对话。
#include "presentation/battle/battle_result_renderer.hpp"
#include "presentation/battle/battle_animation_overlay.hpp"
#include "presentation/battle/battle_animation_controller.hpp"
#include "presentation/strategic/strategic_map_renderer.hpp"
#include "presentation/strategic/strategic_map_input.hpp"
#include "presentation/strategic/town_renderer.hpp"
#include "presentation/strategic/town_input.hpp"
#include "presentation/strategic/shop_renderer.hpp"
#include "presentation/strategic/equipment_renderer.hpp"
#include "presentation/strategic/warehouse_renderer.hpp"
#include "presentation/strategic/save_load_renderer.hpp"
#include "presentation/strategic/level_select_renderer.hpp"
#include "presentation/strategic/level_select_input.hpp"
#include "presentation/forging/forging_renderer.hpp"
#include "presentation/forging/forging_input.hpp"
#include "presentation/promotion/promotion_renderer.hpp"
#include "presentation/promotion/promotion_input.hpp"
#include "presentation/ending/ending_renderer.hpp"
#include "presentation/opening/opening_renderer.hpp"
#include "logic/dialogue/dialogue_manager.hpp"
#include "logic/dialogue/dialogue_text_util.hpp"
#include "logic/dialogue/dialogue_playback.hpp"
#include "logic/forging/forging_system.hpp"
#include "logic/level/level_event_manager.hpp"
#include "logic/promotion/promotion_system.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>

// Convert UTF-8 string to GBK encoding
static std::string utf8_to_gbk(const std::string& utf8_str) {
    if (utf8_str.empty()) return "";
    
    // UTF-8 -> UTF-16 (wide char)
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, nullptr, 0);
    if (wlen <= 0) return utf8_str;
    
    std::wstring wstr(wlen - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, &wstr[0], wlen);
    
    // UTF-16 -> GBK
    int gbk_len = WideCharToMultiByte(936, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (gbk_len <= 0) return utf8_str;
    
    std::string gbk_str(gbk_len - 1, '\0');
    WideCharToMultiByte(936, 0, wstr.c_str(), -1, &gbk_str[0], gbk_len, nullptr, nullptr);
    
    return gbk_str;
}
#endif

namespace fs = std::filesystem;

namespace {

struct ChoiceState {
    bool active{false};
    std::string prompt;
    std::vector<std::string> options;
    int32_t selected{0};
    int32_t fight_index{-1};
    dlzr::logic::GameMode return_mode{dlzr::logic::GameMode::STRATEGIC_MAP};
};

}  // namespace

// 对话播放状态机已抽取至 logic/dialogue/dialogue_playback.{hpp,cpp}，此处复用。
using dlzr::logic::DialogueState;
using dlzr::logic::start_dialogue;
using dlzr::logic::advance_dialogue;
using dlzr::logic::build_dialogue_display;

// === 卡死诊断心跳（dlzr_heartbeat.log）===
// 卡死是无栈可取的挂起（多为堆损坏自旋 / SDL present 阻塞）。
// 细粒度子步骤标记 + 看门狗线程见 heartbeat.cpp。
static uint64_t g_frame_count = 0;

static const char* mode_name(dlzr::logic::GameMode m) {
    using GM = dlzr::logic::GameMode;
    switch (m) {
        case GM::TITLE: return "TITLE";
        case GM::CHARACTER_SELECT: return "CHARACTER_SELECT";
        case GM::BATTLE: return "BATTLE";
        case GM::CUTSCENE: return "CUTSCENE";
        case GM::LEVEL_SELECT: return "LEVEL_SELECT";
        case GM::STRATEGIC_MAP: return "STRATEGIC_MAP";
        case GM::TOWN: return "TOWN";
        case GM::SAVE_LOAD: return "SAVE_LOAD";
        case GM::PROMOTION: return "PROMOTION";
        case GM::EQUIPMENT: return "EQUIPMENT";
        case GM::FORGING: return "FORGING";
        case GM::ENDING: return "ENDING";
        case GM::HIDDEN_LEVEL_CHOICE: return "HIDDEN_LEVEL_CHOICE";
        default: return "UNKNOWN";
    }
}

// Modal dialog helper function
// Returns: 0 = first option, 1 = second option, -1 = cancel
static int32_t show_modal_dialog(dlzr::platform::Platform& platform,
                                  dlzr::presentation::FontRenderer& font,
                                  dlzr::presentation::ImageCache& images,
                                  const char* prompt,
                                  const char* option1,
                                  const char* option2) {
    // Dialog position and size
    // MENU03.IMG is 128x88, draw it centered
    constexpr int32_t DLG_X = 320;
    constexpr int32_t DLG_Y = 200;
    constexpr int32_t DLG_W = 320;
    constexpr int32_t DLG_H = 160;
    constexpr int32_t BG_W = 128;  // MENU03.IMG width
    constexpr int32_t BG_H = 88;   // MENU03.IMG height
    constexpr int32_t BG_X = DLG_X + (DLG_W - BG_W) / 2;  // Center background
    constexpr int32_t BG_Y = DLG_Y + (DLG_H - BG_H) / 2;
    
    // Main dialog loop
    int32_t hovered_option = -1;  // -1 = none, 0 = option1, 1 = option2
    
    while (true) {
        dlzr::platform::Event event;
        while (platform.poll_event(event)) {
            if (event.type == dlzr::platform::Event::Type::MouseMove) {
                int32_t mx = event.mouse_x;
                int32_t my = event.mouse_y;
                
                // Check which option is hovered
                // Original: option = ((mouse_y - dialog_y) - 32) / 24
                // Option 0: y in [BG_Y+32, BG_Y+56)
                // Option 1: y in [BG_Y+56, BG_Y+80)
                int32_t new_hovered = -1;
                if (mx >= BG_X && mx <= BG_X + BG_W) {
                    if (my >= BG_Y + 32 && my < BG_Y + 56) {
                        new_hovered = 0;  // 是
                    } else if (my >= BG_Y + 56 && my < BG_Y + 80) {
                        new_hovered = 1;  // 否
                    }
                }
                
                if (new_hovered != hovered_option) {
                    hovered_option = new_hovered;
                }
            } else if (event.type == dlzr::platform::Event::Type::MouseDown) {
                int32_t mx = event.mouse_x;
                int32_t my = event.mouse_y;
                
                // Check if clicked in dialog area
                if (mx >= BG_X && mx <= BG_X + BG_W &&
                    my >= BG_Y && my <= BG_Y + BG_H) {
                    if (my >= BG_Y + 32 && my < BG_Y + 56) {
                        return 0;  // option1
                    } else if (my >= BG_Y + 56 && my < BG_Y + 80) {
                        return 1;  // option2
                    }
                }
            }
        }
        
        // Render dialog background (MENU03.IMG at original 128x88 size, centered)
        images.draw_image("MENU03.IMG", BG_X, BG_Y);
        
        // Draw text ON TOP of the background image (inside the dialog panels)
        // Original positions from reverse engineering:
        // Prompt: centered at y=4
        // Option 1: (0x38, 0x24) = (56, 36)
        // Option 2: (0x38, 0x3c) = (56, 60)
        
        // Prompt text centered in top area
        font.draw_text(BG_X + 40, BG_Y + 4, prompt, 100, 200, 255);  // Cyan
        
        // Option1  - cyan default, orange when hovered
        if (hovered_option == 0) {
            font.draw_text(BG_X + 56, BG_Y + 36, option1, 255, 165, 0);  // Orange when hovered
        } else {
            font.draw_text(BG_X + 56, BG_Y + 36, option1, 100, 200, 255);  // Cyan normal
        }
        
        // Option2 - cyan default, orange when hovered
        if (hovered_option == 1) {
            font.draw_text(BG_X + 56, BG_Y + 60, option2, 255, 165, 0);  // Orange when hovered
        } else {
            font.draw_text(BG_X + 56, BG_Y + 60, option2, 100, 200, 255);  // Cyan normal
        }
        
        platform.present();
        platform.delay_ms(8);  // 避免 while(true) 模态循环 100% 占 CPU
    }
    
    return -1;
}

int main(int argc, char* argv[]) {
    dlzr::platform::install_crash_handler();

    dlzr::hb_init();
    // 看门狗：主线程若 3 秒未推进帧号，落盘 "WATCHDOG STALL: mode=.. step=.."，
    // 精确锁定卡死所在 Mode 与帧内子步骤。detach 即可（进程退出随之结束）。
    std::thread(dlzr::hb_watchdog).detach();

    fs::path game_data_dir = ".";
    std::string mod_dir;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--mod" && i + 1 < argc) {
            mod_dir = argv[++i];
        } else if (game_data_dir == ".") {
            game_data_dir = arg;
        }
    }

    // 引擎自带数据文件（dialogue_groups.bin / dialogue_map.bin 等）随 exe 部署，
    // 故以其所在目录为 engine_data_dir。
    fs::path exe_path = fs::path(argv[0]).lexically_normal();
    fs::path engine_data_dir = exe_path.parent_path();
    if (engine_data_dir.empty()) engine_data_dir = ".";

    auto platform = dlzr::platform::create_sdl3_platform();

    dlzr::platform::PlatformConfig config;
    config.title = "dlzr - Landmine War Remake";
    config.width = dlzr::core::SCREEN_WIDTH;
    config.height = dlzr::core::SCREEN_HEIGHT;

    if (!platform->init(config)) {
        std::fprintf(stderr, "Platform init failed\n");
        return 1;
    }

    dlzr::app::DatLoader loader(game_data_dir.string());
    dlzr::app::SaveLoader save_loader(game_data_dir.string());
    dlzr::app::FaceLoader face_loader;
    face_loader.load((game_data_dir / "FACE.DAT").string());

    dlzr::app::ArmyLoader army_loader;
    army_loader.load((game_data_dir / "ARMY.DAT").string());

    dlzr::app::FlmLoader flm_loader;
    flm_loader.load((game_data_dir / "FLM.DAT").string());

    dlzr::logic::DialogueManager dialogue_mgr;
    {
        auto texts = loader.load_dialogues();
        dialogue_mgr.load_dialogues(texts);
        dialogue_mgr.init_dialogue_groups(engine_data_dir.string());
    }

    dlzr::logic::LevelEventManager level_events;

    dlzr::logic::GameStateManager gsm;
    gsm.init(loader);

    dlzr::logic::ForgingSystem forging_system;
    forging_system.init(gsm.game_data());

    dlzr::logic::PromotionSystem promotion_system;
    promotion_system.init(gsm.game_data());

    // Presentation subsystems
    dlzr::presentation::FontRenderer font;
    font.init(*platform, game_data_dir.string());

    dlzr::presentation::ImageCache images;
    images.init(*platform, game_data_dir.string());
    if (!mod_dir.empty()) {
        images.set_mod_dir(mod_dir);
    }

    dlzr::presentation::AudioManager audio;
    audio.init(*platform, loader, game_data_dir.string());
    if (!mod_dir.empty()) {
        audio.set_mod_dir(mod_dir);
    }

    dlzr::presentation::AnimationPlayer anim_player;
    anim_player.init(*platform);

    // Title screen
    dlzr::presentation::TitleRenderer title_renderer;
    title_renderer.init(*platform, font, images, game_data_dir.string());
    dlzr::presentation::TitleInput title_input;
    int32_t title_selected = 0;
    int32_t prev_title_selected = 0;

    // Character selection screen
    dlzr::presentation::CharacterSelectRenderer char_select_renderer;
    char_select_renderer.init(*platform, font, images, face_loader, army_loader);
    dlzr::presentation::CharacterSelectInput char_select_input;
    int32_t char_selected_portrait = 0;
    int32_t char_selected_formation = 0;
    int32_t char_slider_value = 80;
    int32_t char_difficulty = 1;
    std::string char_player_name = "\xD6\xF7\xBD\xC7";  // 主角 (GBK)
    int32_t char_hovered_button = -1;
    int32_t char_pressed_button = -1;
    bool char_slider_dragging = false;
    bool char_naming_mode = false;

    // Dialogue
    dlzr::presentation::DialogueRenderer dialogue_renderer;
    dialogue_renderer.init(*platform, font, images, game_data_dir.string());
    dlzr::presentation::DialogueInput dialogue_input;
    DialogueState dialogue_state;
    ChoiceState choice_state;
    int32_t loaded_fight_index{-1};   // 当前 setup_battle 载入的关卡索引（供开场对话路由）

    // Battle
    dlzr::logic::BattleEngine battle_engine;
    dlzr::logic::BattleEngine::ScoreBreakdown last_score_breakdown;
    dlzr::presentation::BattleRenderer battle_renderer;
    battle_renderer.init(*platform, font, images, army_loader, face_loader);
    dlzr::presentation::BattleInputHandler battle_input;
    dlzr::presentation::BattleResultRenderer battle_result;
    battle_result.init(*platform, font);
    dlzr::presentation::BattleAnimationOverlay battle_anim_overlay;
    battle_anim_overlay.init(*platform, flm_loader);
    dlzr::presentation::BattleAnimationController battle_anim_controller;
    battle_anim_controller.init(40, 40, 40);
    dlzr::core::FightData fight_data;
    dlzr::core::FightMap current_map;     // 当前战斗地图（持久，供 set_map 持有指针）
    dlzr::core::BlkGroupData current_blk; // 当前地图瓦片组

    // ------------------------------------------------------------------
    // 战斗开场对话（证据坐标：
    //   - 关卡回调 PTR_FUN_004465d0[0]=FUN_0040be07 → FUN_0043dd47(开场事件脚本)；
    //   - screen/walk19/walk19.log: MAP 出现 → after dialog dismiss → after select，
    //     即进地图后先弹对话、dismiss 后才选兵——对话叠在战斗地图上，非独立屏；
    //   - 该链路上没有任何"战斗简报屏"（自创 BATTLE_INTRO 已删除）。
    // 首关对话内容取 FIGHT.DAT fight_txt（fight0 实测 @0x24 = 40）。
    // ------------------------------------------------------------------
    bool pending_opening_dialogue = false;   // 战后对话结束后补播下一关开场对话
    auto start_battle_opening_dialogue = [&]() {
        // RE 证据（铁律零：用逐关逆向结论，禁止用被误识别的表推断）：
        //   各关战前对话 = LevelEventManager::get_pre_battle_events(fight_index).dialogue_entries
        //   （TXT.DAT 条目索引，逐关对应原版关卡回调 PTR_FUN_004465d0[i] 的开场事件脚本，
        //    见 level_event_manager.cpp 各 level_N_pre_battle 注释，全部标注 FUN_xxxx 起源）。
        //   例：level_0_pre_battle → {21} = TXT[21]"01大队长，赵村离这儿不远…"（说话人码 01）。
        //   旧路径 dialogue_mgr.resolve_fight_dialogue(fight_data.intro.fight_txt) 依赖两张
        //   被误识别的表（0x446030=战斗编排脚本码、0x448720=BGM 音轨），会把事件码当 TXT
        //   行号取出乱序/错误文本，已废弃。
        //   说话人码(00/01/03/06/36…) 直接索引 FACE.DAT 头像（main.cpp:1819），头像随正确文本一并修复。
        auto events = level_events.get_pre_battle_events(loaded_fight_index, gsm.game_data());
        std::vector<std::string> lines;
        for (int32_t idx : events.dialogue_entries) {
            if (idx >= 0 && idx < dialogue_mgr.dialogue_count()) {
                lines.push_back(dialogue_mgr.get_dialogue_text(idx));
            }
        }
        // 可观测性：此前"战前对话到底有没有触发"无法证伪（代码里还并存着
        // "第0关没有战前对话"的矛盾注释），只能靠猜/靠改外围参数碰运气。
        // 这里把事件表与取文结果打到 stderr，实机一眼可判定是哪个环节空了。
        std::fprintf(stderr,
                     "[DIALOGUE] fight=%d pre-battle: entries=%zu lines=%zu "
                     "dialogue_count=%d\n",
                     loaded_fight_index, events.dialogue_entries.size(),
                     lines.size(), dialogue_mgr.dialogue_count());
        if (lines.empty() && !events.dialogue_entries.empty()) {
            std::fprintf(stderr,
                         "[DIALOGUE][WARN] 事件表有条目却取不到文本 —— "
                         "索引越界或 TXT.DAT 未装载\n");
        }
        if (!lines.empty()) {
            start_dialogue(dialogue_state, std::move(lines),
                           dlzr::logic::GameMode::BATTLE);
            // 记录弹出时刻，用于屏蔽跳过幻灯片时的残留点击（见 DialogueState::started_ms）。
            dialogue_state.started_ms = platform->ticks_ms();
            std::fprintf(stderr, "[DIALOGUE] shown: %zu line(s), first=%.40s\n",
                         dialogue_state.parsed.size(),
                         dialogue_state.parsed.empty()
                             ? ""
                             : dialogue_state.parsed[0].text.c_str());
        }
    };

    // Strategic map
    dlzr::presentation::StrategicMapRenderer map_renderer;
    map_renderer.init(*platform, font, images, face_loader);
    dlzr::presentation::StrategicMapInputHandler map_input;

    // Town
    dlzr::presentation::TownRenderer town_renderer;
    town_renderer.init(*platform, font, images);
    dlzr::presentation::TownInputHandler town_input;

    // Shop
    dlzr::presentation::ShopRenderer shop_renderer;
    shop_renderer.init(*platform, font);

    dlzr::presentation::EquipmentRenderer equipment_renderer;
    equipment_renderer.init(*platform, font);

    dlzr::presentation::WarehouseRenderer warehouse_renderer;
    warehouse_renderer.init(*platform, font);

    // Forging
    dlzr::presentation::ForgingRenderer forging_renderer;
    forging_renderer.init(*platform, font, images);
    dlzr::presentation::ForgingInput forging_input;
    int32_t forging_tab = 0;
    int32_t forging_item = 0;
    int32_t forging_quantity = 1;

    // Level select
    dlzr::presentation::LevelSelectRenderer level_select_renderer;
    level_select_renderer.init(*platform, font);
    dlzr::presentation::LevelSelectInput level_select_input;
    int32_t level_select_scroll = 0;

    // Promotion
    dlzr::presentation::PromotionRenderer promotion_renderer;
    promotion_renderer.init(*platform, font);
    dlzr::presentation::PromotionInput promotion_input;
    int32_t promotion_selected_char = 0;

    // Warehouse scroll
    int32_t warehouse_scroll = 0;

    // Save/Load
    dlzr::presentation::SaveLoadRenderer save_load_renderer;
    save_load_renderer.init(*platform, font, save_loader);
    int32_t save_load_selected_slot = -1;
    bool save_load_confirm_pending = false;

    // Ending
    dlzr::presentation::EndingRenderer ending_renderer;
    ending_renderer.init(*platform, font, images);

    // Opening
    dlzr::presentation::OpeningRenderer opening_renderer;
    opening_renderer.init(*platform, font, images);

    constexpr uint32_t FRAME_MS = 1000 / 60;
    bool running = true;
    uint32_t last_frame_time = platform->ticks_ms();

    // 进入一场战斗的统一装配（加载 FIGHT/MAP/BLK + 注入地图 + 初始化引擎 + 装载瓦片）。
    // MAP.DAT 尺寸 = 战斗网格尺寸（证据 map_dat_analysis.md §4：equipment_stride=MAP.DAT宽,
    // grid_rows=MAP.DAT高），故必须在 init 前 set_map，使 init_grid 按真实地图钳制相机。
    auto setup_battle = [&](int32_t fight_idx) {
        loaded_fight_index = fight_idx;
        fight_data = loader.load_fight(fight_idx);
        current_map = loader.load_map(fight_idx);
        current_blk = loader.load_blk_group(current_map.blk_group);
        battle_engine.set_map(&current_map);          // 在 init 前注入：真实地图尺寸
        battle_engine.init(fight_data, gsm.game_data(), gsm.difficulty_level());
        battle_renderer.set_battle_map(current_map, current_blk);
    };

    while (running) {
        uint32_t frame_start = platform->ticks_ms();
        uint32_t delta_ms = frame_start - last_frame_time;
        last_frame_time = frame_start;

        dlzr::platform::Event event;
        while (platform->poll_event(event)) {
            if (event.type == dlzr::platform::Event::Type::Quit) {
                running = false;
            }

            // Dialogue takes priority when active
            if (dialogue_state.active) {
                // 弹出后 400ms 内忽略输入：防止"跳过 19 屏开场幻灯片"的连点/长按
                // 余波在对话弹出的次帧被当作推进或关闭，导致战前对话一闪即灭
                // （观感=进入关卡没有对话）。400ms 远小于一次有意识操作的间隔，
                // 稳态下不影响正常推进。
                if (platform->ticks_ms() - dialogue_state.started_ms < 400) {
                    continue;
                }
                auto da = dialogue_input.process_event(event);
                if (da == dlzr::presentation::DialogueAction::ADVANCE) {
                    advance_dialogue(dialogue_state, gsm.game_data());
                    if (!dialogue_state.active) {
                        gsm.set_mode(dialogue_state.return_mode);
                        if (pending_opening_dialogue &&
                            gsm.mode() == dlzr::logic::GameMode::BATTLE) {
                            // 战后对话播完 → 补播新关卡的开场对话（关卡回调行为）
                            pending_opening_dialogue = false;
                            start_battle_opening_dialogue();
                        }
                    }
                } else if (da == dlzr::presentation::DialogueAction::DISMISS) {
                    dialogue_state.active = false;
                    gsm.set_mode(dialogue_state.return_mode);
                    if (pending_opening_dialogue &&
                        gsm.mode() == dlzr::logic::GameMode::BATTLE) {
                        pending_opening_dialogue = false;
                        start_battle_opening_dialogue();
                    }
                }
                continue;
            }

            // Choice dialog takes priority when active
            if (choice_state.active) {
                if (event.type == dlzr::platform::Event::Type::KeyDown) {
                    if (event.key == '1' && !choice_state.options.empty()) {
                        choice_state.selected = 0;
                    } else if (event.key == '2' && choice_state.options.size() > 1) {
                        choice_state.selected = 1;
                    } else if (event.key == '3' && choice_state.options.size() > 2) {
                        choice_state.selected = 2;
                    } else if (event.key == 0x0D || event.key == ' ') {
                        // Enter/Space: confirm selection
                        auto choice_result = level_events.process_choice(
                            choice_state.fight_index, choice_state.selected,
                            gsm.game_data());
                        // Apply choice consequences to game data
                        if (choice_result.set_story_flag &&
                            choice_result.story_flag_index >= 0 &&
                            choice_result.story_flag_index < 14) {
                            gsm.game_data().story_flags[choice_result.story_flag_index] =
                                choice_result.story_flag_value;
                        }
                        if (choice_result.advance_level_phase &&
                            choice_state.fight_index >= 0 &&
                            choice_state.fight_index < 47) {
                            gsm.game_data().level_phases[choice_state.fight_index] +=
                                choice_result.phase_increment;
                        }
                        choice_state.active = false;
                        // Check if choice triggers follow-up dialogue
                        if (choice_result.trigger_dialogue &&
                            choice_result.dialogue_group >= 0) {
                            auto chain_texts = dialogue_mgr.resolve_fight_dialogue(
                                choice_result.dialogue_group);
                            if (!chain_texts.empty()) {
                                start_dialogue(dialogue_state, std::move(chain_texts),
                                    choice_result.proceed_to_battle
                                        ? dlzr::logic::GameMode::BATTLE
                                        : choice_state.return_mode);
                                break;
                            }
                        }
                        if (choice_result.proceed_to_battle) {
                            gsm.set_mode(dlzr::logic::GameMode::BATTLE);
                        } else {
                            gsm.set_mode(choice_state.return_mode);
                        }
                    } else if (event.key == 0x1B) {
                        choice_state.active = false;
                        gsm.set_mode(choice_state.return_mode);
                    }
                } else if (event.type == dlzr::platform::Event::Type::MouseDown &&
                           event.mouse_button == 1) {
                    int32_t mx = event.mouse_x;
                    int32_t my = event.mouse_y;
                    for (size_t i = 0; i < choice_state.options.size(); ++i) {
                        int32_t opt_y = 220 + static_cast<int32_t>(i) * 30;
                        if (mx >= 160 && mx <= 480 && my >= opt_y && my <= opt_y + 25) {
                            auto choice_result = level_events.process_choice(
                                choice_state.fight_index, static_cast<int32_t>(i),
                                gsm.game_data());
                            // Apply choice consequences
                            if (choice_result.set_story_flag &&
                                choice_result.story_flag_index >= 0 &&
                                choice_result.story_flag_index < 14) {
                                gsm.game_data().story_flags[choice_result.story_flag_index] =
                                    choice_result.story_flag_value;
                            }
                            if (choice_result.advance_level_phase &&
                                choice_state.fight_index >= 0 &&
                                choice_state.fight_index < 47) {
                                gsm.game_data().level_phases[choice_state.fight_index] +=
                                    choice_result.phase_increment;
                            }
                            choice_state.active = false;
                            if (choice_result.trigger_dialogue &&
                                choice_result.dialogue_group >= 0) {
                                auto chain_texts = dialogue_mgr.resolve_fight_dialogue(
                                    choice_result.dialogue_group);
                                if (!chain_texts.empty()) {
                                    start_dialogue(dialogue_state, std::move(chain_texts),
                                        choice_result.proceed_to_battle
                                            ? dlzr::logic::GameMode::BATTLE
                                            : choice_state.return_mode);
                                    break;
                                }
                            }
                            if (choice_result.proceed_to_battle) {
                                gsm.set_mode(dlzr::logic::GameMode::BATTLE);
                            } else {
                                gsm.set_mode(choice_state.return_mode);
                            }
                            break;
                        }
                    }
                }
                continue;
            }

            switch (gsm.mode()) {
            case dlzr::logic::GameMode::TITLE: {
                audio.play_bgm(0);  // Title screen music
                auto action = title_input.process_event(event, title_selected);
                // Sound effect on menu selection confirmation, not on hover/change
                if (action == dlzr::presentation::TitleAction::NEW_GAME) {
                    audio.play_sound(5);  // Confirm sound
                    // 初出茅庐 → Mode 4 (CHARACTER_SELECT)
                    gsm.set_mode(dlzr::logic::GameMode::CHARACTER_SELECT);
                } else if (action == dlzr::presentation::TitleAction::CONTINUE) {
                    audio.play_sound(5);  // Confirm sound
                    // 继往开来 → Mode 5 (CONTINUE_GAME)
                    gsm.set_mode(dlzr::logic::GameMode::CONTINUE_GAME);
                } else if (action == dlzr::presentation::TitleAction::BATTLE_HERO) {
                    audio.play_sound(5);  // Confirm sound
                    // 战斗英雄 → Mode 2 (TITLE_RANKING)
                    gsm.set_mode(dlzr::logic::GameMode::TITLE_RANKING);
                } else if (action == dlzr::presentation::TitleAction::SUPPORT_MODEL) {
                    audio.play_sound(5);  // Confirm sound
                    // 支前模范 → Mode 6 (VICTORY_ANIMATION/ranking)
                    gsm.set_mode(dlzr::logic::GameMode::VICTORY_ANIMATION);
                } else if (action == dlzr::presentation::TitleAction::REST_TROOPS) {
                    audio.play_sound(5);  // Confirm sound
                    // 部队休整 → Mode 3 (TITLE_REST)
                    gsm.set_mode(dlzr::logic::GameMode::TITLE_REST);
                } else if (action == dlzr::presentation::TitleAction::EXIT) {
                    running = false;
                }
                break;
            }

            case dlzr::logic::GameMode::TITLE_ANIMATION: {
                // Mode 0: FLC opening animation (not yet implemented)
                // For now, skip directly to title menu
                gsm.set_mode(dlzr::logic::GameMode::TITLE);
                break;
            }

            case dlzr::logic::GameMode::TITLE_RANKING: {
                // Mode 2: 战斗英雄 ranking — any key/click returns to title
                if (event.type == dlzr::platform::Event::Type::MouseDown ||
                    event.type == dlzr::platform::Event::Type::KeyDown) {
                    gsm.set_mode(dlzr::logic::GameMode::TITLE);
                }
                break;
            }

            case dlzr::logic::GameMode::TITLE_REST: {
                // Mode 3: 部队休整 — "退出吗?" dialog
                // Show modal dialog with 是/否 options
                int32_t result = show_modal_dialog(*platform, font, images,
                    "\xCD\xCB\xB3\xF6\xC2\xF0?",  // 退出吗?
                    "\xCA\xC7",                      // 是
                    "\xB7\xF1");                     // 否
                
                // 是 (Yes, result=0) = exit game
                // 否 (No, result=1) = return to title
                if (result == 0) {
                    running = false;  // Exit game
                } else {
                    gsm.set_mode(dlzr::logic::GameMode::TITLE);  // Return to title
                }
                break;
            }

            case dlzr::logic::GameMode::CONTINUE_GAME: {
                // Mode 5: 继往开来 — load game transition
                gsm.set_save_load_mode(1);  // Load mode
                gsm.set_mode(dlzr::logic::GameMode::SAVE_LOAD);
                break;
            }

            case dlzr::logic::GameMode::VICTORY_ANIMATION: {
                // Mode 6: 支前模范 ranking / victory scroll — any key returns to title
                if (event.type == dlzr::platform::Event::Type::MouseDown ||
                    event.type == dlzr::platform::Event::Type::KeyDown) {
                    gsm.set_mode(dlzr::logic::GameMode::TITLE);
                }
                break;
            }

            case dlzr::logic::GameMode::CHARACTER_SELECT: {
                // Handle naming mode
                if (char_naming_mode) {
                    if (event.type == dlzr::platform::Event::Type::TextInput) {
                        // Append text to player name (convert UTF-8 to GBK)
#ifdef _WIN32
                        std::string gbk_text = utf8_to_gbk(event.text);
                        size_t current_len = char_player_name.length();
                        size_t text_len = gbk_text.length();
                        // Limit name length to 20 bytes (about 10 Chinese chars)
                        if (current_len + text_len <= 20) {
                            char_player_name += gbk_text;
                        }
#else
                        // Fallback: assume UTF-8 (non-Windows)
                        size_t current_len = char_player_name.length();
                        size_t text_len = std::strlen(event.text);
                        if (current_len + text_len <= 20) {
                            char_player_name += event.text;
                        }
#endif
                    } else if (event.type == dlzr::platform::Event::Type::KeyDown) {
                        if (event.key == 0x08) {  // Backspace
                            // Delete last character (handle UTF-8)
                            if (!char_player_name.empty()) {
                                size_t len = char_player_name.length();
                                // Find start of last UTF-8 character
                                size_t pos = len - 1;
                                while (pos > 0 && (char_player_name[pos] & 0xC0) == 0x80) {
                                    pos--;
                                }
                                char_player_name.erase(pos);
                            }
                        } else if (event.key == 0x0D || event.key == ' ') {  // Enter or Space
                            // Confirm name
                            char_naming_mode = false;
                            platform->stop_text_input();
                        } else if (event.key == 0x1B) {  // Escape
                            // Cancel naming
                            char_naming_mode = false;
                            platform->stop_text_input();
                        }
                    } else if (event.type == dlzr::platform::Event::Type::MouseDown &&
                               event.mouse_button == 1) {
                        // C4 改名体验：点击名字输入区域“外”直接确认，不必回车。
                        // 名字显示区在 (108,374) 附近（左对齐、GBK 字体），
                        // 以此为中心给出判定矩形；区域内点击保持输入，区域外点击确认。
                        constexpr int32_t NAME_IN_X0 = 90,  NAME_IN_Y0 = 358;
                        constexpr int32_t NAME_IN_X1 = 360, NAME_IN_Y1 = 398;
                        bool inside = event.mouse_x >= NAME_IN_X0 && event.mouse_x <= NAME_IN_X1 &&
                                      event.mouse_y >= NAME_IN_Y0 && event.mouse_y <= NAME_IN_Y1;
                        if (!inside) {
                            char_naming_mode = false;
                            platform->stop_text_input();
                        }
                    }
                } else {
                    // Normal mode
                    auto action = char_select_input.process_event(
                        event, char_selected_portrait, char_selected_formation,
                        char_slider_value, char_difficulty, char_player_name,
                        char_hovered_button, char_pressed_button,
                        char_slider_dragging);
                    if (action == dlzr::presentation::CharacterSelectAction::CONFIRM) {
                        audio.play_sound(6);
                        // 原版 Mode 4(战前准备) 的"进入战斗"按钮(Button 1) 直接
                        // → FUN_0042e0f1(开场幻灯片) → 设置战斗 → Mode 150(战斗)。
                        // 记录玩家修改的主角名与头像，播放开场幻灯片后再进入战斗开场。
                        dlzr::hb_step("CS_CONFIRM:set_data");
                        gsm.game_data().protagonist_name = char_player_name;
                        gsm.game_data().protagonist_face = 103 + char_selected_portrait;
                        // 阵型→主角兵种、滑块→主角武力/智力（原版确认路径写入 People[0]）
                        static constexpr int32_t FORMATION_CLASS_IDX[4] = {1, 4, 8, 11};
                        gsm.game_data().protagonist_class_type =
                            FORMATION_CLASS_IDX[char_selected_formation % 4];
                        gsm.game_data().protagonist_strength = char_slider_value;
                        gsm.game_data().protagonist_intelligence = 160 - char_slider_value;
                        gsm.set_difficulty_level(char_difficulty);
                        gsm.set_selected_fight(0);
                        dlzr::hb_step("CS_CONFIRM:setup_battle");
                        setup_battle(0);
                        battle_input.init(14, 32, 48);  // 视口原点(14,32)+48px格（FUN_0041be02: 0xe,0x20,0x30*0x30? 640/432判定）
                        dlzr::hb_step("CS_CONFIRM:opening_start");
                        opening_renderer.start(gsm);
                        dlzr::hb_step("CS_CONFIRM:set_mode_CUTSCENE");
                        gsm.set_mode(dlzr::logic::GameMode::CUTSCENE);
                    } else if (action == dlzr::presentation::CharacterSelectAction::CANCEL) {
                        audio.play_sound(7);
                        gsm.set_mode(dlzr::logic::GameMode::TITLE);
                    } else if (action == dlzr::presentation::CharacterSelectAction::CHANGE) {
                        audio.play_sound(6);
                        char_naming_mode = true;
                        platform->start_text_input();
                    } else if (action == dlzr::presentation::CharacterSelectAction::SELECT_FORMATION) {
                        // Trigger attack east animation and play attack sound
                        char_select_renderer.trigger_formation_anim(char_selected_formation);
                        // Play attack sound for this formation's class type
                        static constexpr int32_t FORMATION_CLASS_IDX[4] = {1, 4, 8, 11};
                        int32_t class_idx = FORMATION_CLASS_IDX[char_selected_formation];
                        int32_t attack_snd = gsm.game_data().class_types[class_idx].audio_attack;
                        std::fprintf(stderr, "[DEBUG] SELECT_FORMATION: formation=%d class_idx=%d audio_attack=%d\n",
                                     char_selected_formation, class_idx, attack_snd);
                        if (attack_snd >= 0 && attack_snd < 1024) {
                            std::fprintf(stderr, "[DEBUG] Playing attack sound %d\n", attack_snd);
                            audio.play_sound(attack_snd);
                        } else {
                            std::fprintf(stderr, "[DEBUG] Invalid sound ID, not playing\n");
                        }
                    }
                }
                break;
            }

            case dlzr::logic::GameMode::STRATEGIC_MAP: {
                audio.play_bgm(1);  // Strategic map music
                auto result = map_input.process_event(event, gsm.game_data(),
                                                       gsm.game_data().current_town,
                                                       map_renderer);
                if (result.action == dlzr::presentation::MapAction::SELECT_TOWN) {
                    gsm.set_selected_town(result.town_index);
                    gsm.game_data().current_town = result.town_index;
                } else if (result.action == dlzr::presentation::MapAction::ENTER_TOWN) {
                    gsm.set_mode(dlzr::logic::GameMode::TOWN);
                }
                break;
            }

            case dlzr::logic::GameMode::TOWN: {
                audio.play_bgm(2);  // Town music
                const auto& town = gsm.game_data().towns[gsm.game_data().current_town];
                auto result = town_input.process_event(event, town, town_renderer);
                if (result.action == dlzr::presentation::TownAction::EXIT_TOWN) {
                    gsm.set_mode(dlzr::logic::GameMode::STRATEGIC_MAP);
                } else if (result.action == dlzr::presentation::TownAction::ENTER_BUILDING) {
                    if (result.building_index == 0) {
                        gsm.set_save_load_mode(0);
                        gsm.set_mode(dlzr::logic::GameMode::SAVE_LOAD);
                    } else if (result.building_index == 1) {
                        gsm.set_selected_equipment_char(0);
                        gsm.set_mode(dlzr::logic::GameMode::EQUIPMENT);
                    } else if (result.building_index == 2) {
                        gsm.set_current_shop_type(0);
                        gsm.set_mode(dlzr::logic::GameMode::SHOP);
                    } else if (result.building_index == 3) {
                        gsm.set_current_shop_type(1);
                        gsm.set_mode(dlzr::logic::GameMode::SHOP);
                    } else if (result.building_index == 4) {
                        gsm.set_level_select_return_mode(
                            dlzr::logic::GameMode::TOWN);
                        gsm.set_mode(dlzr::logic::GameMode::LEVEL_SELECT);
                    } else if (result.building_index == 5) {
                        gsm.set_mode(dlzr::logic::GameMode::WAREHOUSE);
                    }
                }
                if (event.type == dlzr::platform::Event::Type::KeyDown &&
                    (event.key == 'p' || event.key == 'P')) {
                    gsm.set_mode(dlzr::logic::GameMode::PROMOTION);
                }
                break;
            }

            case dlzr::logic::GameMode::SHOP: {
                auto shop_type = static_cast<dlzr::presentation::ShopType>(gsm.current_shop_type());
                if (event.type == dlzr::platform::Event::Type::MouseDown) {
                    if (event.mouse_button == 1) {
                        const auto& town = gsm.game_data().towns[gsm.game_data().current_town];
                        int32_t item_idx = shop_renderer.item_at_position(
                            event.mouse_x, event.mouse_y, town, shop_type);
                        if (item_idx >= 0) {
                            bool bought = false;
                            if (shop_type == dlzr::presentation::ShopType::MATERIAL) {
                                bought = gsm.buy_mine(item_idx);
                            } else {
                                bought = gsm.buy_wupin(item_idx);
                            }
                            if (bought) {
                                audio.play_sound(15);
                            }
                        }
                    } else if (event.mouse_button == 3) {
                        gsm.set_mode(dlzr::logic::GameMode::TOWN);
                    }
                } else if (event.type == dlzr::platform::Event::Type::KeyDown &&
                           event.key == 0x1B) {
                    gsm.set_mode(dlzr::logic::GameMode::TOWN);
                }
                break;
            }

            case dlzr::logic::GameMode::EQUIPMENT: {
                if (event.type == dlzr::platform::Event::Type::MouseDown) {
                    if (event.mouse_button == 1) {
                        int32_t char_idx = equipment_renderer.char_at_position(
                            event.mouse_x, event.mouse_y, gsm.game_data());
                        if (char_idx >= 0) {
                            gsm.set_selected_equipment_char(char_idx);
                        } else {
                            int32_t sel = gsm.selected_equipment_char();
                            int32_t item_slot = equipment_renderer.item_slot_at_position(
                                event.mouse_x, event.mouse_y);
                            if (item_slot >= 0 && sel >= 0 && sel < dlzr::core::MAX_PEOPLE) {
                                uint8_t equipped = gsm.game_data().people[sel].items[item_slot];
                                if (equipped != 0) {
                                    gsm.unequip_item(sel, item_slot);
                                    audio.play_sound(15);
                                } else {
                                    for (int32_t i = 1; i < dlzr::core::MAX_WUPIN; ++i) {
                                        if (gsm.game_data().warehouse[i] > 0) {
                                            if (gsm.equip_item(sel, item_slot, i)) {
                                                audio.play_sound(15);
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                            int32_t mine_slot = equipment_renderer.mine_slot_at_position(
                                event.mouse_x, event.mouse_y);
                            if (mine_slot >= 0 && sel >= 0 && sel < dlzr::core::MAX_PEOPLE) {
                                uint8_t equipped = gsm.game_data().people[sel].mines[mine_slot];
                                if (equipped != 0) {
                                    gsm.unequip_mine(sel, mine_slot);
                                    audio.play_sound(15);
                                } else {
                                    for (int32_t i = 1; i < dlzr::core::MAX_DILEI; ++i) {
                                        int32_t wh = dlzr::core::MAX_WUPIN - dlzr::core::MAX_DILEI + i;
                                        if (gsm.game_data().warehouse[wh] > 0) {
                                            if (gsm.equip_mine(sel, mine_slot, i)) {
                                                audio.play_sound(15);
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    } else if (event.mouse_button == 3) {
                        gsm.set_mode(dlzr::logic::GameMode::TOWN);
                    }
                } else if (event.type == dlzr::platform::Event::Type::KeyDown &&
                           event.key == 0x1B) {
                    gsm.set_mode(dlzr::logic::GameMode::TOWN);
                }
                break;
            }

            case dlzr::logic::GameMode::WAREHOUSE: {
                if (event.type == dlzr::platform::Event::Type::KeyDown &&
                    event.key == 0x1B) {
                    gsm.set_mode(dlzr::logic::GameMode::TOWN);
                } else if (event.type == dlzr::platform::Event::Type::MouseDown) {
                    if (event.mouse_button == 1) {
                        int32_t item_idx = warehouse_renderer.item_at_position(
                            event.mouse_x, event.mouse_y, gsm.game_data(),
                            warehouse_scroll);
                        if (item_idx >= 0) {
                            audio.play_sound(15);
                        }
                    } else if (event.mouse_button == 3) {
                        gsm.set_mode(dlzr::logic::GameMode::TOWN);
                    }
                } else if (event.type == dlzr::platform::Event::Type::MouseWheel) {
                    int32_t non_zero = 0;
                    for (int32_t i = 0; i < dlzr::core::MAX_WUPIN; ++i) {
                        if (gsm.game_data().warehouse[i] > 0) non_zero++;
                    }
                    int32_t max_scroll = std::max(0, non_zero -
                        dlzr::presentation::WarehouseRenderer::VISIBLE_ROWS);
                    warehouse_scroll -= event.wheel_y;
                    if (warehouse_scroll < 0) warehouse_scroll = 0;
                    if (warehouse_scroll > max_scroll) warehouse_scroll = max_scroll;
                }
                break;
            }

            case dlzr::logic::GameMode::FORGING: {
                auto action = forging_input.process_event(event, forging_tab,
                                                           forging_item, forging_quantity);
                if (action == dlzr::presentation::ForgingAction::BACK) {
                    gsm.set_mode(dlzr::logic::GameMode::TOWN);
                } else if (action == dlzr::presentation::ForgingAction::CONFIRM) {
                    if (forging_system.can_craft(forging_tab, forging_item,
                                                  forging_quantity,
                                                  static_cast<int32_t>(gsm.game_data().warehouse[0]))) {
                        forging_system.execute_craft(forging_tab, forging_item,
                                                      forging_quantity);
                    }
                }
                break;
            }

            case dlzr::logic::GameMode::LEVEL_SELECT: {
                int32_t selected = gsm.selected_fight();
                auto action = level_select_input.process_event(event, selected,
                                                                level_select_scroll);
                gsm.set_selected_fight(selected);
                if (action == dlzr::presentation::LevelSelectAction::BACK) {
                    gsm.set_mode(gsm.level_select_return_mode());
                } else if (action == dlzr::presentation::LevelSelectAction::SELECT) {
                    bool unlocked = (selected == 0) ||
                                    gsm.is_fight_completed(selected - 1);
                    if (unlocked && selected < dlzr::core::MAX_FIGHTS) {
                        setup_battle(selected);
                        battle_input.init(14, 32, 48);
                        // RE 证据: 选关进战斗 = FUN_0041b8e9 直接进 Mode150，
                        // 无独立"战斗开场"屏（FUN_0042e2d6/FUN_0041b8e9 指令流）。
                        gsm.set_mode(dlzr::logic::GameMode::BATTLE);
                    }
                }
                break;
            }

            case dlzr::logic::GameMode::PROMOTION: {
                auto action = promotion_input.process_event(event,
                                                             promotion_selected_char);
                if (action == dlzr::presentation::PromotionAction::BACK) {
                    gsm.set_mode(dlzr::logic::GameMode::TOWN);
                } else if (action == dlzr::presentation::PromotionAction::PROMOTE) {
                    if (promotion_system.can_promote(promotion_selected_char)) {
                        promotion_system.execute_promotion(promotion_selected_char);
                    }
                } else if (action == dlzr::presentation::PromotionAction::SELECT) {
                    if (event.type == dlzr::platform::Event::Type::MouseDown &&
                        event.mouse_button == 1) {
                        int32_t idx = promotion_renderer.char_at_position(
                            event.mouse_x, event.mouse_y, gsm.game_data());
                        if (idx >= 0) {
                            promotion_selected_char = idx;
                        }
                    }
                }
                break;
            }

            case dlzr::logic::GameMode::SAVE_LOAD: {
                if (save_load_confirm_pending) {
                    if (event.type == dlzr::platform::Event::Type::KeyDown) {
                        if (event.key == 'y' || event.key == 'Y') {
                            auto mode = static_cast<dlzr::presentation::SaveLoadMode>(
                                gsm.save_load_mode());
                            if (mode == dlzr::presentation::SaveLoadMode::SAVE) {
                                save_loader.save_game(save_load_selected_slot, gsm.game_data());
                            } else {
                                save_loader.load_game(save_load_selected_slot, gsm.game_data());
                            }
                            save_load_confirm_pending = false;
                            save_load_selected_slot = -1;
                            gsm.set_mode(dlzr::logic::GameMode::TOWN);
                        } else if (event.key == 'n' || event.key == 'N' ||
                                   event.key == 0x1B) {
                            save_load_confirm_pending = false;
                            save_load_selected_slot = -1;
                        }
                    }
                    break;
                }
                if (event.type == dlzr::platform::Event::Type::MouseDown) {
                    if (event.mouse_button == 1) {
                        int32_t slot = save_load_renderer.slot_at_position(
                            event.mouse_x, event.mouse_y);
                        if (slot >= 0) {
                            save_load_selected_slot = slot;
                            save_load_confirm_pending = true;
                        }
                    } else if (event.mouse_button == 3) {
                        gsm.set_mode(dlzr::logic::GameMode::TOWN);
                    }
                } else if (event.type == dlzr::platform::Event::Type::KeyDown &&
                           event.key == 0x1B) {
                    gsm.set_mode(dlzr::logic::GameMode::TOWN);
                }
                break;
            }

            case dlzr::logic::GameMode::CUTSCENE: {
                // 任意按键/点击可快进幻灯片；播放结束的自动跳转在帧更新里处理
                opening_renderer.process_input(event);
                break;
            }

            // ⚠️ 此处曾并存一条已被证伪的注释："…也没有战前对话（第 0 关）"。
            //   它导致"第0关到底有没有战前对话"长期自相矛盾，实现时反复被删掉。
            //   **已裁决（以数据 + 装载器为准）**：第 0 关有战前对话，
            //   由 LevelEventManager::get_pre_battle_events(0) → {21} → TXT[21]
            //   "01大队长，赵村离这儿不远，大半个时辰就到了。"（说话人码 01）。
            //   该对话**叠加在战斗地图上**弹出（FUN_0043d82f 对话框覆盖层），
            //   不是独立简报屏 —— 故"没有简报屏"是对的，"没有战前对话"是错的。
            //   自创的独立 BATTLE_INTRO 屏幕确已删除（原版无此屏）。

            case dlzr::logic::GameMode::BATTLE: {
                audio.play_bgm(3);  // Battle music
                static int32_t last_sel = -1;
                if (!battle_engine.is_over()) {
                    const auto& state = battle_engine.state();
                    int32_t cur = state.current_fighter_idx;
                    if (cur >= 0 && state.fighters[cur].team == 0 && !state.fighters[cur].has_acted) {
                        auto cmd = battle_input.process_event(event, state);
                        // 选中单位即亮出可达区域（原版 FUN_0041f424 -> FUN_00418e29）
                        int32_t sel = battle_input.selected_fighter();
                        if (sel != last_sel) {
                            battle_engine.set_selected_fighter(sel);
                            last_sel = sel;
                        }
                        if (cmd.type != dlzr::logic::BattleCommand::NONE) {
                            battle_engine.execute_command(cmd);
                            battle_engine.advance_turn();
                        }
                    } else {
                        if (last_sel != -1) { battle_engine.set_selected_fighter(-1); last_sel = -1; }
                    }
                }
                break;
            }

            case dlzr::logic::GameMode::BATTLE_RESULT: {
                if (event.type == dlzr::platform::Event::Type::KeyDown ||
                    (event.type == dlzr::platform::Event::Type::MouseDown &&
                     event.mouse_button == 1)) {
                    // 检查隐藏关卡触发 / Check hidden level trigger
                    if (gsm.selected_fight() == 36 &&
                        gsm.check_hidden_level_trigger()) {
                        gsm.set_mode(dlzr::logic::GameMode::HIDDEN_LEVEL_CHOICE);
                    } else if (gsm.selected_fight() >= 46) {
                        int32_t ending_id = gsm.select_ending();
                        std::string ending_text;
                        if (ending_id >= 0 && ending_id < dialogue_mgr.dialogue_count()) {
                            // 逐行解析后拼接（结局为单条目，可能多行；匹配二进制逐行模型）。
                            auto ending_lines = dlzr::logic::DialogueManager::parse_dialogue_entry(
                                dialogue_mgr.get_dialogue_text(ending_id));
                            for (const auto& dl : ending_lines) {
                                if (!ending_text.empty()) ending_text += "\n";
                                ending_text += dl.text;
                            }
                        }
                        ending_renderer.start(ending_id, ending_text);
                        gsm.set_mode(dlzr::logic::GameMode::ENDING);
                        audio.play_bgm(4);
                    } else {
                        int32_t fight_idx = gsm.selected_fight();
                        bool victory = battle_engine.is_victory();

                        // Apply post-battle level events (feature flags, counters)
                        auto post_events = level_events.get_post_battle_events(
                            fight_idx, victory, gsm.game_data());
                        level_events.apply_post_battle(post_events, gsm.game_data());

                        // Apply strategic map event (Table 4 callbacks)
                        auto strat_event = level_events.get_strategic_event(
                            fight_idx, gsm.game_data());
                        level_events.apply_strategic_effects(strat_event, gsm.game_data());

                            // ============================================================
                            // 战后推进（原版 Mode150 的 DAT_007d50f8>=1 分支）：
                            //   显示战后对话 → fight_index++ → FUN_004292fe → 关卡回调
                            // 原版本关结束后**直接进入下一关**，中间没有城镇/战略地图中转屏。
                            // 克隆的 TOWN→STRATEGIC_MAP 城市节点图是自创（原版 Mode150 就是
                            // 战术战场本尊），属 Tier A 违规，已从新游戏链路彻底移除。
                            // C5 关卡回调表(PTR_FUN_0044668c)未实现，此处以"推进下一关"占位。
                            // ============================================================
                            int32_t target = fight_idx;   // 默认重打本关
                            bool redirected = false;

                            // Strategic score check → level branching
                            if (strat_event.check_score) {
                                int32_t score = gsm.score();
                                int32_t lvl = (score >= strat_event.score_threshold)
                                                  ? strat_event.high_score_level
                                                  : strat_event.low_score_level;
                                if (lvl >= 0) { target = lvl; redirected = true; }
                            }

                            // Strategic level transition
                            if (strat_event.do_transition && strat_event.target_level >= 0) {
                                target = strat_event.target_level;
                                redirected = true;
                            }

                            if (!redirected) {
                                if (victory) {
                                    int32_t next_fight = fight_idx + 1;
                                    if (next_fight < dlzr::core::MAX_FIGHTS) {
                                        target = next_fight;   // 原版 fight_index++
                                    }
                                }
                                // 战败：原版由关卡回调决定（C5 未实现）→ 重打本关占位
                            }
                            gsm.set_selected_fight(target);

                            // 用**已完成关卡**的 fight_txt 解析战后对话（必须在重载下一关之前取）
                            std::vector<std::string> strat_texts;
                            for (int32_t idx : strat_event.dialogues) {
                                if (idx >= 0 && idx < dialogue_mgr.dialogue_count()) {
                                    strat_texts.push_back(
                                        dialogue_mgr.get_dialogue_text(idx));
                                }
                            }
                            const int32_t cur_fight_txt = fight_data.intro.fight_txt;
                            auto post_dialogue =
                                dialogue_mgr.resolve_fight_dialogue_b(cur_fight_txt);

                            // 载入目标关卡：战后对话结束后直接进入下一关的战斗开场
                            setup_battle(target);
                            battle_input.init(14, 32, 48);  // 视口原点(14,32)+48px格（FUN_0041be02: 0xe,0x20,0x30*0x30? 640/432判定）

                            // Resolve dialogue: strategic event dialogues take priority
                            if (!strat_texts.empty()) {
                                pending_opening_dialogue = true;
                                start_dialogue(dialogue_state, std::move(strat_texts),
                                               dlzr::logic::GameMode::BATTLE);
                                break;
                            }

                            // Resolve SubB (post-battle) dialogue sequence
                            if (!post_dialogue.empty()) {
                                pending_opening_dialogue = true;
                                start_dialogue(dialogue_state, std::move(post_dialogue),
                                               dlzr::logic::GameMode::BATTLE);
                            } else {
                                gsm.set_mode(dlzr::logic::GameMode::BATTLE);
                                start_battle_opening_dialogue();
                            }
                    }
                }
                break;
            }

            case dlzr::logic::GameMode::HIDDEN_LEVEL_CHOICE: {
                // 模拟=0 (tactical replay), 虚构=1 (narrative → fight 40), 跳过=2
                if (event.type == dlzr::platform::Event::Type::KeyDown) {
                    if (event.key == '1') {
                        // 模拟模式: replay fight 36 as tactical battle
                        setup_battle(36);
                        battle_input.init(14, 32, 48);  // 视口原点(14,32)+48px格（FUN_0041be02: 0xe,0x20,0x30*0x30? 640/432判定）
                        gsm.set_mode(dlzr::logic::GameMode::BATTLE);
                    } else if (event.key == '2' && gsm.is_narrative_mode_unlocked()) {
                        // 虚构模式: skip to fight 40 narrative
                        gsm.set_selected_fight(40);
                        setup_battle(40);
                        battle_input.init(14, 32, 48);  // 视口原点(14,32)+48px格（FUN_0041be02: 0xe,0x20,0x30*0x30? 640/432判定）
                        gsm.set_mode(dlzr::logic::GameMode::BATTLE);
                    } else if (event.key == '3' || event.key == 0x1B) {
                        gsm.set_mode(dlzr::logic::GameMode::TOWN);
                    }
                } else if (event.type == dlzr::platform::Event::Type::MouseDown &&
                           event.mouse_button == 1) {
                    int32_t mx = event.mouse_x;
                    int32_t my = event.mouse_y;
                    // 3 choices centered: y=200,240,280
                    if (mx >= 200 && mx <= 440) {
                        if (my >= 190 && my <= 215) {
                            setup_battle(36);
                            battle_input.init(14, 32, 48);  // 视口原点(14,32)+48px格（FUN_0041be02: 0xe,0x20,0x30*0x30? 640/432判定）
                            gsm.set_mode(dlzr::logic::GameMode::BATTLE);
                        } else if (my >= 230 && my <= 255 && gsm.is_narrative_mode_unlocked()) {
                            gsm.set_selected_fight(40);
                            setup_battle(40);
                            battle_input.init(14, 32, 48);  // 视口原点(14,32)+48px格（FUN_0041be02: 0xe,0x20,0x30*0x30? 640/432判定）
                            gsm.set_mode(dlzr::logic::GameMode::BATTLE);
                        } else if (my >= 270 && my <= 295) {
                            gsm.set_mode(dlzr::logic::GameMode::TOWN);
                        }
                    }
                }
                break;
            }

            case dlzr::logic::GameMode::ENDING: {
                ending_renderer.process_input(event);
                if (ending_renderer.is_finished()) {
                    gsm.set_mode(dlzr::logic::GameMode::TITLE);
                }
                break;
            }

            // Town sub-menus (original modes 51-58) — stubs returning to TOWN
            case dlzr::logic::GameMode::ARMY_INFO:
            case dlzr::logic::GameMode::RECRUIT:
            case dlzr::logic::GameMode::RECRUIT_DETAIL:
            case dlzr::logic::GameMode::MINE_PLACEMENT:
            case dlzr::logic::GameMode::CONFIRM_DIALOG: {
                // Any key/click returns to town hub
                if (event.type == dlzr::platform::Event::Type::MouseDown ||
                    event.type == dlzr::platform::Event::Type::KeyDown) {
                    gsm.set_mode(dlzr::logic::GameMode::TOWN);
                }
                break;
            }

            case dlzr::logic::GameMode::RESET: {
                // Mode 999: reset to title
                gsm.set_mode(dlzr::logic::GameMode::TITLE);
                break;
            }

            default:
                break;
            }
        }

        // Update
        if (gsm.mode() == dlzr::logic::GameMode::BATTLE) {
            if (!battle_engine.is_over()) {
                if (battle_engine.needs_ai_input()) {
                    battle_engine.execute_ai_turn();
                    platform->delay_ms(200);
                }
            }

            const auto& bstate = battle_engine.state();
            for (int32_t ei = 0; ei < bstate.event_count; ++ei) {
                const auto& ev = bstate.events[ei];
                int32_t flm_idx = -1;
                int32_t tx = ev.target_x;
                int32_t ty = ev.target_y;

                if (ev.type == dlzr::logic::BattleEvent::DAMAGE_DEALT) {
                    battle_anim_controller.queue_impact(tx, ty);
                    battle_anim_controller.queue_damage_number(tx, ty, ev.value);
                    if (ev.source_idx >= 0) {
                        const auto& atk = bstate.fighters[ev.source_idx];
                        int32_t pidx = atk.people_idx;
                        if (pidx >= 0 && pidx < 88) {
                            int32_t cidx = fight_data.people[pidx].class_type;
                            if (cidx >= 0 && cidx < dlzr::core::MAX_CLASS_TYPES) {
                                flm_idx = gsm.game_data().class_types[cidx].flm_attack;
                                int32_t snd = gsm.game_data().class_types[cidx].audio_attack;
                                if (snd >= 0 && snd < 1024) audio.play_sound(snd);
                            }
                        }
                    }
                } else if (ev.type == dlzr::logic::BattleEvent::SKILL_USED) {
                    int32_t sk = ev.value;
                    if (sk >= 0 && sk < dlzr::core::MAX_SKILLS) {
                        // ⚠️ flm_index/audio_index 在原版 GAME.DAT 技能记录中不存在
                        // （RE 实证：48B 记录 = [8B 头][16B 名][para1..anm_index 共 6 int32]，无动画/音效字段）。
                        // Tier-B 默认 -1（无专属动画/音效），由 battle_anim_controller 按 skill_type 播放通用演出。
                        flm_idx = -1;
                        int32_t snd = -1;
                        if (snd >= 0 && snd < 1024) audio.play_sound(snd);
                    }
                    if (ev.source_idx >= 0 && tx >= 0 && ty >= 0) {
                        int32_t skill_type = (sk >= 0 && sk < dlzr::core::MAX_SKILLS)
                            ? gsm.game_data().skills[sk].type : -1;
                        battle_anim_controller.queue_skill_cast(
                            ev.source_idx, tx, ty, skill_type);
                    }
                } else if (ev.type == dlzr::logic::BattleEvent::MINE_TRIGGERED) {
                    int32_t mi = ev.value;
                    if (mi > 0 && mi < dlzr::core::MAX_DILEI) {
                        // ⚠️ 同技能：flm_index/audio_index 不在地雷 48B 记录中（Tier-B 默认 -1）。
                        flm_idx = -1;
                        int32_t snd = -1;
                        if (snd >= 0 && snd < 1024) audio.play_sound(snd);
                        if (tx >= 0 && ty >= 0) {
                            battle_anim_controller.queue_impact(tx, ty);
                            battle_anim_controller.queue_damage_number(
                                tx, ty, gsm.game_data().mines[mi].weili);
                        }
                    }
                } else if (ev.type == dlzr::logic::BattleEvent::FIGHTER_DIED) {
                    audio.play_sound(3);
                    if (ev.target_idx >= 0 && ev.target_idx < dlzr::logic::BattleState::MAX_FIGHTERS) {
                        battle_anim_controller.queue_death(
                            ev.target_idx,
                            bstate.fighters[ev.target_idx].grid_x,
                            bstate.fighters[ev.target_idx].grid_y);
                    } else if (tx >= 0 && ty >= 0) {
                        battle_anim_controller.queue_death(ev.target_idx, tx, ty);
                    }
                } else if (ev.type == dlzr::logic::BattleEvent::FIGHTER_MOVED) {
                    if (ev.source_idx >= 0) {
                        const auto& mover = bstate.fighters[ev.source_idx];
                        int32_t pidx = mover.people_idx;
                        if (pidx >= 0 && pidx < 88) {
                            int32_t cidx = fight_data.people[pidx].class_type;
                            if (cidx >= 0 && cidx < dlzr::core::MAX_CLASS_TYPES) {
                                int32_t snd = gsm.game_data().class_types[cidx].audio_move;
                                if (snd >= 0 && snd < 1024) audio.play_sound(snd);
                            }
                        }
                    }
                } else if (ev.type == dlzr::logic::BattleEvent::ITEM_USED) {
                    audio.play_sound(15);
                } else if (ev.type == dlzr::logic::BattleEvent::MINE_PLACED) {
                    audio.play_sound(15);
                }

                if (flm_idx >= 0 && flm_idx < dlzr::app::FlmLoader::MAX_ANIMATIONS
                    && tx >= 0 && ty >= 0) {
                    battle_anim_overlay.play(flm_idx, tx, ty, 40, 40, 40);
                }
            }
            battle_engine.clear_events();
        }

        battle_anim_overlay.update();
        battle_anim_controller.update(delta_ms);

        if (dialogue_state.active) {
            // 原版 FUN_0043d82f.c 一次性渲染整段文本（FUN_004061df 整段 buffer），
            // 无逐字打字机。这里直接整页显示：每帧把 visible_chars 设为全量。
            const auto& d = dialogue_state.parsed[dialogue_state.current_index];
            std::string display = build_dialogue_display(d, gsm.game_data());
            int32_t total_chars = 0;
            size_t pos = 0;
            while (pos < display.size()) {
                uint8_t b = static_cast<uint8_t>(display[pos]);
                pos += (b < 0x80) ? 1 : 2;
                total_chars++;
            }
            dialogue_state.visible_chars = total_chars;  // 整页显示，不自创逐字动画
        }

        anim_player.update();
        char_select_renderer.update_anim(delta_ms);
        opening_renderer.update(delta_ms);
        dlzr::hb_step("UPDATE_DONE");
        // 确定链路（RE 证据坐标）：
        //   FUN_0042e2d6.c:244-299(确定分支) → FUN_0042e0f1(幻灯片模态,仅左键结束)
        //   → FUN_0041b8e9(进战斗) → FUN_004294be(部署) → BGM
        //   → PTR_FUN_004465d0[0]=FUN_0040be07(第0关回调: 网格/实体放置
        //     + FUN_0043dd47 开场事件脚本)。
        // 链路上没有任何独立"战斗简报屏"（自创 BATTLE_INTRO 已删除）。
        // walk19.log 实测时序: MAP 出现 → dialog dismiss → select，
        // 故幻灯片结束后直接进 BATTLE，开场对话以覆盖层叠在地图上弹出。
        if (gsm.mode() == dlzr::logic::GameMode::CUTSCENE &&
            !opening_renderer.is_playing()) {
            gsm.set_mode(dlzr::logic::GameMode::BATTLE);
            start_battle_opening_dialogue();
        }
        ending_renderer.update(delta_ms);
        audio.update(delta_ms);

        // Render
        platform->clear(0, 0, 0);

        switch (gsm.mode()) {
        case dlzr::logic::GameMode::TITLE:
            title_renderer.render(title_selected, gsm.difficulty_level());
            break;

        case dlzr::logic::GameMode::TITLE_ANIMATION:
            // TODO: FLC animation — for now just show title bg
            title_renderer.render(title_selected, gsm.difficulty_level());
            break;

        case dlzr::logic::GameMode::TITLE_RANKING: {
            // Mode 2: 战斗英雄排行榜 — 英雄战绩排名（数据驱动）
            platform->clear(10, 14, 30);
            font.draw_text(214, 36, "\xD5\xBD\xB6\xD3\xD3\xA2\xC4\xCF\xC5\xC5\xD0\xD0\xB0\xE5",
                           255, 215, 0);  // 战斗英雄排行榜

            const auto& gd = gsm.game_data();
            std::vector<std::pair<std::string, int32_t>> heroes;
            for (int32_t i = 0; i < dlzr::core::MAX_PEOPLE; ++i) {
                const auto& p = gd.people[i];
                if (p.enemy != 0) continue;  // 仅我方
                std::string nm;
                for (char c : p.name) { if (!c) break; nm += c; }
                if (nm.empty()) continue;
                // 战绩 = 等级*1000 + 经验（仅作排名展示）
                heroes.emplace_back(nm, p.level * 1000 + (p.experience % 1000));
            }
            std::sort(heroes.begin(), heroes.end(),
                      [](const auto& a, const auto& b) { return a.second > b.second; });

            int32_t y = 92;
            char line[80];
            for (size_t k = 0; k < heroes.size() && k < 16; ++k) {
                std::snprintf(line, sizeof(line), "%2d. %-14s  等级 %2d   战绩 %4d",
                              (int)(k + 1), heroes[k].first.c_str(),
                              heroes[k].second / 1000, heroes[k].second % 1000);
                font.draw_text(150, y, line, 232, 232, 220);
                y += 23;
            }
            font.draw_text(150, 466, "\xB5\xE7\xBB\xF7\xC8\xB7\xB7\xF1\xB5\xBD\xB1\xEA\xCC\xF5\xC4\xBF\xC2\xB7",
                           140, 140, 140);  // 点击任意处返回标题
            break;
        }

        case dlzr::logic::GameMode::VICTORY_ANIMATION: {
            // Mode 6: 胜利动画 / 支前模范 — 显示胜利画面（点击/按键返回标题）
            platform->clear(12, 10, 8);
            font.draw_text(280, 180, "\xCA\xFA\xC1\xBF", 255, 230, 80);  // 胜利
            font.draw_text(150, 250,
                "\xBF\xCD\xC8\xCB\xC3\xD7\xB5\xC4\xBF\xD5\xB5\xD8\xB2\xBB\xB6\xC8\xC0\xEB\xB4\xF3",
                220, 220, 220);  // 敌人的根据地不断被扩大（近似）
            font.draw_text(150, 300,
                "\xD5\xBD\xB6\xD3\xBD\xF1\xBD\xE1\xCA\xC7\xC4\xFA\xD3\xC5\xC4\xE3\xC3\xF1\xB5\xC4\xCA\xB5\xC3\xFB",
                200, 200, 200);  // 战斗结束是你们人民的胜利
            font.draw_text(150, 466, "\xB5\xE7\xBB\xF7\xC8\xB7\xB7\xF1\xB5\xBD\xB1\xEA\xCC\xF5\xC4\xBF\xC2\xB7",
                           140, 140, 140);  // 点击任意处返回标题
            break;
        }

        case dlzr::logic::GameMode::CONTINUE_GAME:
            // Transition mode — immediately goes to SAVE_LOAD
            title_renderer.render(title_selected, gsm.difficulty_level());
            break;

        case dlzr::logic::GameMode::CHARACTER_SELECT: {
            // Formation class names from class_types[indices 1,4,8,11]
            static constexpr int32_t FORMATION_CLASS_IDX[4] = {1, 4, 8, 11};
            const char* formation_names[4];
            for (int i = 0; i < 4; ++i) {
                formation_names[i] = gsm.game_data().class_types[FORMATION_CLASS_IDX[i]].name.data();
            }
            dlzr::hb_step("CS_RENDER_BEGIN");
            char_select_renderer.render(char_selected_portrait,
                                        char_selected_formation,
                                        char_slider_value,
                                        char_difficulty,
                                        char_player_name,
                                        formation_names,
                                        char_hovered_button,
                                        char_pressed_button,
                                        char_naming_mode);
            dlzr::hb_step("CS_RENDER_END");
            break;
        }

        case dlzr::logic::GameMode::STRATEGIC_MAP:
            map_renderer.render(gsm.game_data(), gsm.game_data().current_town);
            break;

        case dlzr::logic::GameMode::TOWN:
            town_renderer.render(gsm.game_data().towns[gsm.game_data().current_town],
                                 gsm.game_data().current_town);
            break;

        case dlzr::logic::GameMode::SHOP: {
            auto shop_type = static_cast<dlzr::presentation::ShopType>(gsm.current_shop_type());
            shop_renderer.render(gsm.game_data().towns[gsm.game_data().current_town], shop_type, gsm.game_data());
            break;
        }

        case dlzr::logic::GameMode::EQUIPMENT:
            equipment_renderer.render(gsm.game_data(), gsm.selected_equipment_char());
            break;

        case dlzr::logic::GameMode::WAREHOUSE:
            warehouse_renderer.render(gsm.game_data(), warehouse_scroll);
            break;

        case dlzr::logic::GameMode::FORGING:
            forging_renderer.render(forging_tab, forging_item, forging_quantity,
                                     forging_system);
            break;

        case dlzr::logic::GameMode::LEVEL_SELECT:
            level_select_renderer.render(gsm, gsm.selected_fight(),
                                          level_select_scroll);
            break;

        case dlzr::logic::GameMode::PROMOTION:
            promotion_renderer.render(gsm, promotion_system,
                                       promotion_selected_char);
            break;

        case dlzr::logic::GameMode::SAVE_LOAD: {
            auto mode = static_cast<dlzr::presentation::SaveLoadMode>(gsm.save_load_mode());
            save_load_renderer.render(mode, save_load_selected_slot);
            if (save_load_selected_slot >= 0) {
                auto info = save_loader.peek_save_info(save_load_selected_slot);
                save_load_renderer.render_slot_preview(save_load_selected_slot, info);
            }
            if (save_load_confirm_pending) {
                save_load_renderer.render_confirm_dialog(
                    mode, save_load_selected_slot);
            }
            break;
        }

        case dlzr::logic::GameMode::BATTLE:
            battle_renderer.render(battle_engine.state(), fight_data, gsm.game_data());
            {
                auto input_state = battle_input.state();
                int32_t sel = battle_input.selected_fighter();
                if (sel >= 0 && (input_state == dlzr::presentation::BattleInputHandler::State::FIGHTER_SELECTED ||
                                 input_state == dlzr::presentation::BattleInputHandler::State::COMMAND_MENU ||
                                 input_state == dlzr::presentation::BattleInputHandler::State::MOVE_TARGET ||
                                 input_state == dlzr::presentation::BattleInputHandler::State::ATTACK_TARGET ||
                                 input_state == dlzr::presentation::BattleInputHandler::State::SKILL_TARGET ||
                                 input_state == dlzr::presentation::BattleInputHandler::State::MINE_TARGET)) {
                    const auto& sf = battle_engine.state().fighters[sel];
                    battle_renderer.render_selected_highlight(sf.grid_x, sf.grid_y);
                }
                if (input_state == dlzr::presentation::BattleInputHandler::State::COMMAND_MENU) {
                    battle_renderer.render_command_menu(0, 0, battle_input.menu_hover_item());
                }
                if (input_state == dlzr::presentation::BattleInputHandler::State::MOVE_TARGET) {
                    battle_renderer.render_status_text("\xD1\xA1\xD4\xF1\xD2\xC6\xB6\xAF\xC4\xBF\xB1\xEA \xD3\xD2\xBC\xFC\xC8\xA1\xCF\xFB");
                } else if (input_state == dlzr::presentation::BattleInputHandler::State::ATTACK_TARGET) {
                    battle_renderer.render_status_text("\xD1\xA1\xD4\xF1\xB9\xA5\xBB\xF7\xC4\xBF\xB1\xEA \xD3\xD2\xBC\xFC\xC8\xA1\xCF\xFB");
                } else if (input_state == dlzr::presentation::BattleInputHandler::State::SKILL_TARGET) {
                    battle_renderer.render_status_text("\xD1\xA1\xD4\xF1\xBC\xBC\xC4\xDC\xC4\xBF\xB1\xEA \xD3\xD2\xBC\xFC\xC8\xA1\xCF\xFB");
                } else if (input_state == dlzr::presentation::BattleInputHandler::State::MINE_TARGET) {
                    battle_renderer.render_status_text("\xD1\xA1\xD4\xF1\xB5\xD8\xC0\xD7\xCE\xBB\xD6\xC3 \xD3\xD2\xBC\xFC\xC8\xA1\xCF\xFB");
                }
            }
            battle_anim_overlay.render();
            battle_anim_controller.render(*platform, font);
            if (battle_engine.is_over()) {
                dlzr::logic::finalize_battle(
                    battle_engine.state(),
                    fight_data,
                    gsm.game_data());
                gsm.complete_fight(gsm.selected_fight());
                last_score_breakdown = battle_engine.compute_score_breakdown();
                gsm.add_score(last_score_breakdown.total);
                gsm.set_mode(dlzr::logic::GameMode::BATTLE_RESULT);
            }
            break;

        case dlzr::logic::GameMode::BATTLE_RESULT:
            battle_result.render(battle_engine.is_victory(),
                                 gsm.score(), last_score_breakdown);
            break;

        case dlzr::logic::GameMode::DIALOGUE:
            break;

        case dlzr::logic::GameMode::CUTSCENE:
            opening_renderer.render();
            break;

        case dlzr::logic::GameMode::ENDING:
            ending_renderer.render();
            break;

        case dlzr::logic::GameMode::HIDDEN_LEVEL_CHOICE: {
            // 隐藏关卡选择画面 / Hidden level choice screen
            int32_t threshold = gsm.get_hidden_level_threshold();
            char title_buf[128];
            std::snprintf(title_buf, sizeof(title_buf),
                          "\xD2\xFE\xB2\xD8\xB9\xD8\xBF\xA8\xBD\xE2\xCB\xF8 "
                          "\xB7\xD6\xCA\xFD: %d  \xB5\xC8\xBC\xB6: %d",
                          gsm.score(), threshold);
            font.draw_text(160, 140, title_buf, 255, 255, 0);

            // 选项 1: 模拟模式 / Option 1: Simulation
            font.draw_text(200, 200,
                           "[1] \xC4\xA3\xC4\xE2\xC4\xA3\xCA\xBD - "
                           "\xD6\xD8\xD0\xC2\xD5\xBD\xB6\xB7 36",
                           255, 255, 255);

            // 选项 2: 虚构模式 (需要3300分) / Option 2: Narrative (needs 3300)
            if (gsm.is_narrative_mode_unlocked()) {
                font.draw_text(200, 240,
                               "[2] \xD0\xe9\xB9\xB9\xC4\xA3\xCA\xBD - "
                               "\xBD\xF8\xC8\xEB\xD5\xBD\xB6\xB7 40",
                               255, 255, 255);
            } else {
                font.draw_text(200, 240,
                               "[2] \xD0\xe9\xB9\xB9\xC4\xA3\xCA\xBD - "
                               "\xD0\xe8\xD2\xAA 3300 \xB7\xD6",
                               128, 128, 128);
            }

            // 选项 3: 跳过 / Option 3: Skip
            font.draw_text(200, 280,
                           "[3] \xCC\xF8\xB9\xFD",
                           255, 255, 255);
            break;
        }

        case dlzr::logic::GameMode::ARMY_INFO: {
            // Mode 51: 军队信息 — 我方人员名册
            platform->clear(14, 12, 10);
            font.draw_text(280, 30, "\xBE\xFC\xB6\xD3\xD0\xC2\xCF\xA2", 255, 215, 0);  // 军队信息

            const auto& gd = gsm.game_data();
            int32_t y = 76;
            char line[96];
            std::snprintf(line, sizeof(line), "%-14s %-8s %4s %4s %4s %6s",
                          "姓名", "兵种", "等级", "武力", "智力", "经验");
            font.draw_text(60, y, line, 180, 200, 255);
            y += 26;
            for (int32_t i = 0; i < dlzr::core::MAX_PEOPLE && y < 456; ++i) {
                const auto& p = gd.people[i];
                if (p.enemy != 0) continue;
                std::string nm;
                for (char c : p.name) { if (!c) break; nm += c; }
                if (nm.empty()) continue;
                const char* cls = (p.class_type >= 0 &&
                    p.class_type < dlzr::core::MAX_CLASS_TYPES)
                    ? gd.class_types[p.class_type].name.data() : "";
                std::string cls_nm;
                for (char c : std::string(cls)) { if (!c) break; cls_nm += c; }
                std::snprintf(line, sizeof(line), "%-14s %-8s %4d %4d %4d %6d",
                              nm.c_str(), cls_nm.c_str(), p.level,
                              p.strength, p.intelligence, p.experience);
                font.draw_text(60, y, line, 230, 230, 220);
                y += 22;
            }
            font.draw_text(540, 40, "\xB7\xB5\xBB\xD8", 255, 255, 255);  // 返回
            break;
        }

        case dlzr::logic::GameMode::CONFIRM_DIALOG: {
            // Mode 58: 确认对话框
            platform->clear(10, 10, 10);
            platform->fill_rect(180, 180, 280, 120, 40, 40, 60);
            platform->draw_rect(180, 180, 280, 120, 255, 215, 0);
            font.draw_text(250, 205, "\xC8\xB7\xC8\xCF\xC2\xF0\xA3\xBF", 255, 255, 255);  // 确认吗？
            font.draw_text(230, 250, "\xCA\xC7", 120, 255, 120);   // 是
            font.draw_text(360, 250, "\xB7\xF1", 255, 120, 120);   // 否
            break;
        }

        case dlzr::logic::GameMode::RECRUIT:
        case dlzr::logic::GameMode::RECRUIT_DETAIL:
        case dlzr::logic::GameMode::MINE_PLACEMENT:
            // TODO: dedicated renderers for recruit / mine-placement
            town_renderer.render(gsm.game_data().towns[gsm.game_data().current_town],
                                 gsm.game_data().current_town);
            break;

        case dlzr::logic::GameMode::RESET:
            // Transition mode — no render needed
            break;

        default:
            break;
        }

        // Overlay: choice dialog
        if (choice_state.active) {
            int32_t panel_x = 140;
            int32_t panel_y = 160;
            int32_t panel_w = 360;
            int32_t panel_h = 40 + 30 * static_cast<int32_t>(choice_state.options.size()) + 20;
            platform->fill_rect(panel_x, panel_y, panel_w, panel_h, 0, 0, 0);
            platform->draw_rect(panel_x, panel_y, panel_w, panel_h, 180, 180, 180);

            font.draw_text(panel_x + 20, panel_y + 10, choice_state.prompt.c_str(),
                           255, 255, 0);

            for (size_t i = 0; i < choice_state.options.size(); ++i) {
                int32_t opt_y = 220 + static_cast<int32_t>(i) * 30;
                uint8_t r = (static_cast<int32_t>(i) == choice_state.selected) ? 255 : 200;
                uint8_t g = (static_cast<int32_t>(i) == choice_state.selected) ? 255 : 200;
                uint8_t b = (static_cast<int32_t>(i) == choice_state.selected) ? 0 : 200;
                if (static_cast<int32_t>(i) == choice_state.selected) {
                    platform->fill_rect(160, opt_y, 320, 25, 40, 40, 0);
                }
                char num_buf[4];
                std::snprintf(num_buf, sizeof(num_buf), "[%d] ",
                              static_cast<int>(i) + 1);
                std::string line = num_buf + choice_state.options[i];
                font.draw_text(170, opt_y + 4, line.c_str(), r, g, b);
            }
        }

        // Overlay: dialogue box
        if (dialogue_state.active && !dialogue_state.parsed.empty()) {
            const auto& d = dialogue_state.parsed[dialogue_state.current_index];
            // 原版 FUN_0043cc96:44-49 把说话人名（raw，默认色）写入文本缓冲，
            // FUN_0043d82f 内联绘制；故此处把说话人名前置进 display（默认色），
            // 与二进制一致。DAT_00448780 后缀在 clean 层以全角冒号替代（Tier-B）。
            std::string display = build_dialogue_display(d, gsm.game_data());
            int32_t face_idx = -1;
            // 头像按说话人码直接索引 FACE 缓冲（原版 FUN_0043d82f.c:41：
            //   FUN_00422f68(0x82,0x134,0x50,100, &DAT_004761f8 + param_1*8000)，
            //   param_1 即说话人码；FACE.DAT 共 170 张，码 0-99 均合法）。
            // 敌人(40-99)此前走 people[].face 越界而永不显示，现统一按码索引。
            if (d.speaker_id >= 0 &&
                d.speaker_id < dlzr::app::FaceLoader::FACE_COUNT) {
                face_idx = d.speaker_id;
            }
            const uint8_t* face_rgba = nullptr;
            if (face_idx >= 0) {
                face_rgba = face_loader.get_face_rgba(face_idx);
            }
            dialogue_renderer.render(display,
                                     dialogue_state.visible_chars, face_rgba);
        }

        // Overlay: animation
        if (anim_player.is_playing()) {
            anim_player.render();
        }

        platform->present();
        dlzr::hb_step("POST_PRESENT");

        // 心跳埋点：Mode 切换必有记录；每 30 帧记录一次。卡死时最后一行
        // （含 WATCHDOG STALL）即卡住的屏/帧/子步骤。
        g_frame_count++;
        {
            dlzr::logic::GameMode cur = gsm.mode();
            static dlzr::logic::GameMode s_last_mode =
                static_cast<dlzr::logic::GameMode>(-1);
            if (cur != s_last_mode) {
                s_last_mode = cur;
                dlzr::hb_mode(mode_name(cur));
            }
            dlzr::hb_frame(g_frame_count);
        }

        uint32_t frame_elapsed = platform->ticks_ms() - frame_start;
        if (frame_elapsed < FRAME_MS) {
            platform->delay_ms(FRAME_MS - frame_elapsed);
        }
    }

    platform->shutdown();
    return 0;
}
