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
#include "logic/promotion/promotion_system.hpp"

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

std::string get_speaker_name(int32_t speaker_id) {
    if (speaker_id < 0) return "";
    if (speaker_id == 0) return "\xCB\xEF\xCE\xF0\xBF\xD5";  // 孙悟空
    if (speaker_id == 7) return "\xC1\xF8\xE3\xD8\xB6";       // 柳忻
    if (speaker_id == 8) return "\xD0\xA4\xB7\xC9";            // 肖飞
    if (speaker_id >= 1 && speaker_id <= 18) return "";  // Allied NPCs: show as narrator
    if (speaker_id >= 40 && speaker_id <= 46) return "";  // Enemy NPCs: show as narrator
    return "";
}

}  // namespace

int main(int argc, char* argv[]) {
    fs::path game_data_dir = ".";
    if (argc > 1) {
        game_data_dir = argv[1];
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

    dlzr::presentation::AudioManager audio;
    audio.init(*platform, loader);

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

    // Save/Load
    dlzr::presentation::SaveLoadRenderer save_load_renderer;
    save_load_renderer.init(*platform, font, save_loader);

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

            switch (gsm.mode()) {
            case dlzr::logic::GameMode::TITLE: {
                audio.play_bgm(0);  // Title screen music
                auto action = title_input.process_event(event, title_selected);
                if (action == dlzr::presentation::TitleAction::NEW_GAME) {
                    opening_renderer.start(gsm);
                    gsm.set_mode(dlzr::logic::GameMode::CUTSCENE);
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
                if (event.type == dlzr::platform::Event::Type::MouseDown && event.mouse_button == 1) {
                    gsm.set_mode(dlzr::logic::GameMode::TOWN);
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
                } else if (event.type == dlzr::platform::Event::Type::MouseDown &&
                           event.mouse_button == 3) {
                    gsm.set_mode(dlzr::logic::GameMode::TOWN);
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
                if (event.type == dlzr::platform::Event::Type::MouseDown) {
                    if (event.mouse_button == 1) {
                        int32_t slot = save_load_renderer.slot_at_position(event.mouse_x, event.mouse_y);
                        if (slot >= 0) {
                            if (gsm.save_load_mode() == 0) {
                                save_loader.save_game(slot, gsm.game_data());
                            } else {
                                save_loader.load_game(slot, gsm.game_data());
                            }
                            gsm.set_mode(dlzr::logic::GameMode::TOWN);
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
                    int32_t fight_txt = fight_data.intro.fight_txt;
                    auto fight_dialogue = dialogue_mgr.resolve_fight_dialogue(fight_txt);
                    if (!fight_dialogue.empty()) {
                        start_dialogue(dialogue_state, std::move(fight_dialogue),
                                       dlzr::logic::GameMode::BATTLE);
                    } else {
                        gsm.set_mode(dlzr::logic::GameMode::BATTLE);
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
                    if (gsm.selected_fight() >= 46) {
                        gsm.set_mode(dlzr::logic::GameMode::ENDING);
                    } else {
                        int32_t fight_txt = fight_data.intro.fight_txt;
                        auto fight_dialogue = dialogue_mgr.resolve_fight_dialogue(fight_txt);
                        if (!fight_dialogue.empty()) {
                            start_dialogue(dialogue_state, std::move(fight_dialogue),
                                           dlzr::logic::GameMode::TOWN);
                        } else {
                            gsm.set_mode(dlzr::logic::GameMode::TOWN);
                        }
                    }
                }
                break;
            }

            case dlzr::logic::GameMode::ENDING: {
                if (event.type == dlzr::platform::Event::Type::KeyDown ||
                    (event.type == dlzr::platform::Event::Type::MouseDown && event.mouse_button == 1)) {
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
                } else if (ev.type == dlzr::logic::BattleEvent::MINE_TRIGGERED) {
                    int32_t mi = ev.value;
                    if (mi > 0 && mi < dlzr::core::MAX_DILEI) {
                        flm_idx = gsm.game_data().mines[mi].flm_index;
                        int32_t snd = gsm.game_data().mines[mi].audio_index;
                        if (snd >= 0 && snd < 152) audio.play_sound(snd);
                    }
                } else if (ev.type == dlzr::logic::BattleEvent::FIGHTER_DIED) {
                    audio.play_sound(3);
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
                }

                if (flm_idx >= 0 && flm_idx < dlzr::app::FlmLoader::MAX_ANIMATIONS
                    && tx >= 0 && ty >= 0) {
                    battle_anim_overlay.play(flm_idx, tx, ty, 40, 40, 40);
                }
            }
            battle_engine.clear_events();
        }

        battle_anim_overlay.update();

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
            if (dialogue_state.visible_chars < total_chars) {
                dialogue_state.visible_chars++;
            }
        }

        anim_player.update();
        opening_renderer.update(delta_ms);

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
            warehouse_renderer.render(gsm.game_data(), 0);
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
            save_load_renderer.render(mode, -1);
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

        case dlzr::logic::GameMode::ENDING: {
            int32_t ending_id = gsm.select_ending();
            std::string ending_text;
            if (ending_id >= 0 && ending_id < dialogue_mgr.dialogue_count()) {
                auto parsed = dlzr::logic::DialogueManager::parse_dialogue(
                    dialogue_mgr.get_dialogue_text(ending_id));
                ending_text = parsed.text;
            }
            ending_renderer.render(ending_id, ending_text);
            break;
        }

        default:
            break;
        }

        // Overlay: dialogue box
        if (dialogue_state.active && !dialogue_state.parsed.empty()) {
            const auto& d = dialogue_state.parsed[dialogue_state.current_index];
            std::string speaker_name = get_speaker_name(d.speaker_id);
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
