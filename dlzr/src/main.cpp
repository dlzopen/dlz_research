// dlzr — 抗日之地雷战 1:1 复刻
// Landmine War 1:1 Remake

#include "platform/platform.hpp"
#include "core/data_model/constants.hpp"
#include "app/resource/dat_loader.hpp"
#include "app/resource/save_loader.hpp"
#include "app/resource/face_loader.hpp"
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
#include "presentation/dialogue/dialogue_renderer.hpp"
#include "presentation/dialogue/dialogue_input.hpp"
#include "presentation/battle/battle_renderer.hpp"
#include "presentation/battle/battle_input_handler.hpp"
#include "presentation/battle/battle_intro_renderer.hpp"
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

namespace fs = std::filesystem;

namespace {

struct DialogueState {
    std::vector<std::string> entries;       // Raw entries (with speaker codes)
    std::vector<dlzr::core::Dialogue> parsed;  // Parsed dialogues
    int32_t current_index{0};
    int32_t visible_chars{0};
    bool active{false};
    dlzr::logic::GameMode return_mode{dlzr::logic::GameMode::STRATEGIC_MAP};
};

struct ChoiceState {
    bool active{false};
    std::string prompt;
    std::vector<std::string> options;
    int32_t selected{0};
    int32_t fight_index{-1};
    dlzr::logic::GameMode return_mode{dlzr::logic::GameMode::STRATEGIC_MAP};
};

void start_dialogue(DialogueState& ds,
                    std::vector<std::string> raw_entries,
                    dlzr::logic::GameMode return_mode) {
    ds.entries = raw_entries;
    ds.parsed.clear();
    for (const auto& raw : raw_entries) {
        ds.parsed.push_back(dlzr::logic::DialogueManager::parse_dialogue(raw));
    }
    ds.current_index = 0;
    ds.visible_chars = 0;
    ds.active = true;
    ds.return_mode = return_mode;
}

void advance_dialogue(DialogueState& ds) {
    if (!ds.active || ds.parsed.empty()) return;

    const auto& text = ds.parsed[ds.current_index].text;
    int32_t total_chars = 0;
    size_t pos = 0;
    while (pos < text.size()) {
        uint8_t b = static_cast<uint8_t>(text[pos]);
        pos += (b < 0x80) ? 1 : 2;
        total_chars++;
    }

    if (ds.visible_chars < total_chars) {
        ds.visible_chars += 2;
        if (ds.visible_chars > total_chars) ds.visible_chars = total_chars;
    } else {
        ds.current_index++;
        ds.visible_chars = 0;
        if (ds.current_index >= static_cast<int32_t>(ds.parsed.size())) {
            ds.active = false;
        }
    }
}

std::string get_speaker_name(int32_t speaker_id, const dlzr::core::GameData& gd) {
    if (speaker_id < 0 || speaker_id >= dlzr::core::MAX_PEOPLE) return "";
    const auto& name = gd.people[speaker_id].name;
    return std::string(name.data(), strnlen(name.data(), name.size()));
}

}  // namespace

int main(int argc, char* argv[]) {
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

    dlzr::app::FlmLoader flm_loader;
    flm_loader.load((game_data_dir / "FLM.DAT").string());

    dlzr::logic::DialogueManager dialogue_mgr;
    {
        auto texts = loader.load_dialogues();
        dialogue_mgr.load_dialogues(texts);
        dialogue_mgr.init_dialogue_groups();
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
    title_renderer.init(*platform, font, images);
    dlzr::presentation::TitleInput title_input;
    int32_t title_selected = 0;

    // Dialogue
    dlzr::presentation::DialogueRenderer dialogue_renderer;
    dialogue_renderer.init(*platform, font, images);
    dlzr::presentation::DialogueInput dialogue_input;
    DialogueState dialogue_state;
    ChoiceState choice_state;

    // Battle
    dlzr::logic::BattleEngine battle_engine;
    dlzr::logic::BattleEngine::ScoreBreakdown last_score_breakdown;
    dlzr::presentation::BattleRenderer battle_renderer;
    battle_renderer.init(*platform, font, images);
    dlzr::presentation::BattleInputHandler battle_input;
    dlzr::presentation::BattleIntroRenderer battle_intro;
    battle_intro.init(*platform, font);
    dlzr::presentation::BattleResultRenderer battle_result;
    battle_result.init(*platform, font);
    dlzr::presentation::BattleAnimationOverlay battle_anim_overlay;
    battle_anim_overlay.init(*platform, flm_loader);
    dlzr::presentation::BattleAnimationController battle_anim_controller;
    battle_anim_controller.init(40, 40, 40);
    dlzr::core::FightData fight_data;

    // Strategic map
    dlzr::presentation::StrategicMapRenderer map_renderer;
    map_renderer.init(*platform, font, images);
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
                auto da = dialogue_input.process_event(event);
                if (da == dlzr::presentation::DialogueAction::ADVANCE) {
                    advance_dialogue(dialogue_state);
                    if (!dialogue_state.active) {
                        gsm.set_mode(dialogue_state.return_mode);
                    }
                } else if (da == dlzr::presentation::DialogueAction::DISMISS) {
                    dialogue_state.active = false;
                    gsm.set_mode(dialogue_state.return_mode);
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
                if (action == dlzr::presentation::TitleAction::NEW_GAME) {
                    opening_renderer.start(gsm);
                    gsm.set_mode(dlzr::logic::GameMode::CUTSCENE);
                    audio.play_bgm(5);
                } else if (action == dlzr::presentation::TitleAction::LOAD_GAME) {
                    gsm.set_save_load_mode(1);  // Load mode
                    gsm.set_mode(dlzr::logic::GameMode::SAVE_LOAD);
                } else if (action == dlzr::presentation::TitleAction::CYCLE_DIFFICULTY) {
                    gsm.set_difficulty_level((gsm.difficulty_level() + 1) % 3);
                } else if (action == dlzr::presentation::TitleAction::EXIT) {
                    running = false;
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
                    gsm.set_mode(dlzr::logic::GameMode::TOWN);
                } else if (action == dlzr::presentation::LevelSelectAction::SELECT) {
                    bool unlocked = (selected == 0) ||
                                    gsm.is_fight_completed(selected - 1);
                    if (unlocked && selected < dlzr::core::MAX_FIGHTS) {
                        fight_data = loader.load_fight(selected);
                        battle_engine.init(fight_data, gsm.game_data(),
                                           gsm.difficulty_level());
                        battle_input.init(40, 40, 40);
                        gsm.set_mode(dlzr::logic::GameMode::BATTLE_INTRO);
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
                opening_renderer.process_input(event);
                if (!opening_renderer.is_playing()) {
                    gsm.set_mode(dlzr::logic::GameMode::STRATEGIC_MAP);
                }
                break;
            }

            case dlzr::logic::GameMode::BATTLE_INTRO: {
                if (event.type == dlzr::platform::Event::Type::KeyDown ||
                    (event.type == dlzr::platform::Event::Type::MouseDown &&
                     event.mouse_button == 1)) {
                    int32_t fight_idx = gsm.selected_fight();
                    auto pre_events = level_events.get_pre_battle_events(
                        fight_idx, gsm.game_data());

                    if (pre_events.has_choice) {
                        // Show choice dialog before battle
                        choice_state.active = true;
                        choice_state.prompt = pre_events.choice_prompt;
                        choice_state.options = pre_events.choice_options;
                        choice_state.selected = 0;
                        choice_state.fight_index = fight_idx;
                        choice_state.return_mode = dlzr::logic::GameMode::TOWN;
                    } else {
                        int32_t fight_txt = fight_data.intro.fight_txt;
                        auto fight_dialogue = dialogue_mgr.resolve_fight_dialogue(fight_txt);
                        if (!fight_dialogue.empty()) {
                            start_dialogue(dialogue_state, std::move(fight_dialogue),
                                           dlzr::logic::GameMode::BATTLE);
                        } else {
                            gsm.set_mode(dlzr::logic::GameMode::BATTLE);
                        }
                    }
                }
                break;
            }

            case dlzr::logic::GameMode::BATTLE: {
                audio.play_bgm(3);  // Battle music
                if (!battle_engine.is_over()) {
                    const auto& state = battle_engine.state();
                    int32_t cur = state.current_fighter_idx;
                    if (cur >= 0 && state.fighters[cur].team == 0 && !state.fighters[cur].has_acted) {
                        auto cmd = battle_input.process_event(event, state);
                        if (cmd.type != dlzr::logic::BattleCommand::NONE) {
                            battle_engine.execute_command(cmd);
                            battle_engine.advance_turn();
                        }
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
                            auto parsed = dlzr::logic::DialogueManager::parse_dialogue(
                                dialogue_mgr.get_dialogue_text(ending_id));
                            ending_text = parsed.text;
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

                        // Strategic score check → level branching
                        if (strat_event.check_score) {
                            int32_t score = gsm.score();
                            if (score >= strat_event.score_threshold) {
                                if (strat_event.high_score_level >= 0) {
                                    gsm.set_selected_fight(strat_event.high_score_level);
                                }
                            } else {
                                if (strat_event.low_score_level >= 0) {
                                    gsm.set_selected_fight(strat_event.low_score_level);
                                }
                            }
                        }

                        // Strategic level transition
                        if (strat_event.do_transition && strat_event.target_level >= 0) {
                            gsm.set_selected_fight(strat_event.target_level);
                        }

                        // Resolve dialogue: strategic event dialogues take priority
                        if (!strat_event.dialogues.empty()) {
                            std::vector<std::string> strat_texts;
                            for (int32_t idx : strat_event.dialogues) {
                                if (idx >= 0 && idx < dialogue_mgr.dialogue_count()) {
                                    strat_texts.push_back(
                                        dialogue_mgr.get_dialogue_text(idx));
                                }
                            }
                            if (!strat_texts.empty()) {
                                start_dialogue(dialogue_state, std::move(strat_texts),
                                               dlzr::logic::GameMode::TOWN);
                                break;
                            }
                        }

                        // Resolve SubB (post-battle) dialogue sequence
                        int32_t fight_txt = fight_data.intro.fight_txt;
                        auto post_dialogue = dialogue_mgr.resolve_fight_dialogue_b(fight_txt);
                        if (!post_dialogue.empty()) {
                            start_dialogue(dialogue_state, std::move(post_dialogue),
                                           dlzr::logic::GameMode::TOWN);
                        } else {
                            gsm.set_mode(dlzr::logic::GameMode::TOWN);
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
                        fight_data = loader.load_fight(36);
                        battle_engine.init(fight_data, gsm.game_data(),
                                           gsm.difficulty_level());
                        battle_input.init(40, 40, 40);
                        gsm.set_mode(dlzr::logic::GameMode::BATTLE_INTRO);
                    } else if (event.key == '2' && gsm.is_narrative_mode_unlocked()) {
                        // 虚构模式: skip to fight 40 narrative
                        gsm.set_selected_fight(40);
                        fight_data = loader.load_fight(40);
                        battle_engine.init(fight_data, gsm.game_data(),
                                           gsm.difficulty_level());
                        battle_input.init(40, 40, 40);
                        gsm.set_mode(dlzr::logic::GameMode::BATTLE_INTRO);
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
                            fight_data = loader.load_fight(36);
                            battle_engine.init(fight_data, gsm.game_data(),
                                               gsm.difficulty_level());
                            battle_input.init(40, 40, 40);
                            gsm.set_mode(dlzr::logic::GameMode::BATTLE_INTRO);
                        } else if (my >= 230 && my <= 255 && gsm.is_narrative_mode_unlocked()) {
                            gsm.set_selected_fight(40);
                            fight_data = loader.load_fight(40);
                            battle_engine.init(fight_data, gsm.game_data(),
                                               gsm.difficulty_level());
                            battle_input.init(40, 40, 40);
                            gsm.set_mode(dlzr::logic::GameMode::BATTLE_INTRO);
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
                                if (snd >= 0 && snd < 152) audio.play_sound(snd);
                            }
                        }
                    }
                } else if (ev.type == dlzr::logic::BattleEvent::SKILL_USED) {
                    int32_t sk = ev.value;
                    if (sk >= 0 && sk < dlzr::core::MAX_SKILLS) {
                        flm_idx = gsm.game_data().skills[sk].flm_index;
                        int32_t snd = gsm.game_data().skills[sk].audio_index;
                        if (snd >= 0 && snd < 152) audio.play_sound(snd);
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
                        flm_idx = gsm.game_data().mines[mi].flm_index;
                        int32_t snd = gsm.game_data().mines[mi].audio_index;
                        if (snd >= 0 && snd < 152) audio.play_sound(snd);
                        if (tx >= 0 && ty >= 0) {
                            battle_anim_controller.queue_impact(tx, ty);
                            battle_anim_controller.queue_damage_number(
                                tx, ty, gsm.game_data().mines[mi].damage);
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
                                if (snd >= 0 && snd < 152) audio.play_sound(snd);
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
            // Auto-advance typewriter effect
            const auto& text = dialogue_state.parsed[dialogue_state.current_index].text;
            int32_t total_chars = 0;
            size_t pos = 0;
            while (pos < text.size()) {
                uint8_t b = static_cast<uint8_t>(text[pos]);
                pos += (b < 0x80) ? 1 : 2;
                total_chars++;
            }
            bool text_scrolling = (dialogue_state.visible_chars < total_chars);
            if (text_scrolling) {
                dialogue_state.visible_chars++;
            }
            // Update portrait animation
            dialogue_renderer.update_animation(delta_ms, text_scrolling);
        }

        anim_player.update();
        opening_renderer.update(delta_ms);
        ending_renderer.update(delta_ms);
        audio.update(delta_ms);

        // Render
        platform->clear(0, 0, 0);

        switch (gsm.mode()) {
        case dlzr::logic::GameMode::TITLE:
            title_renderer.render(title_selected, gsm.difficulty_level());
            break;

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

        case dlzr::logic::GameMode::BATTLE_INTRO:
            battle_intro.render(fight_data.intro);
            break;

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
            std::string speaker_name = get_speaker_name(d.speaker_id, gsm.game_data());
            const uint8_t* face_rgba = nullptr;
            if (d.speaker_id >= 0 && d.speaker_id < dlzr::core::MAX_PEOPLE) {
                int32_t face_idx = gsm.game_data().people[d.speaker_id].face;
                face_rgba = face_loader.get_face_rgba(face_idx);
            }
            dialogue_renderer.render(speaker_name, d.text,
                                     dialogue_state.visible_chars, face_rgba);
        }

        // Overlay: animation
        if (anim_player.is_playing()) {
            anim_player.render();
        }

        platform->present();

        uint32_t frame_elapsed = platform->ticks_ms() - frame_start;
        if (frame_elapsed < FRAME_MS) {
            platform->delay_ms(FRAME_MS - frame_elapsed);
        }
    }

    platform->shutdown();
    return 0;
}
