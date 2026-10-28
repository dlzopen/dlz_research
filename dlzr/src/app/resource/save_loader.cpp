// save_loader.cpp — 存档系统实现
// 严格匹配原作 28 数据块格式 / Strictly match original 28-block format

#include "app/resource/save_loader.hpp"

#include <fstream>
#include <cstring>

namespace dlzr::app {

static constexpr size_t BLOCK_SIZES[28] = {
    4, 4, 4, 4, 700, 4, 4, 4, 4, 4,
    64, 76, 56, 380,
    8192, 8192, 16896, 16896,
    320, 320, 80, 40,
    32768, 8, 28, 12, 4, 4,
};

bool SaveLoader::save_game(int32_t slot, const core::GameData& game_data) const {
    if (slot < 0 || slot >= core::SAVE_SLOT_COUNT) {
        return false;
    }

    std::string path = get_save_path(slot);
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    // Block 0: game_mode (4 bytes) @ 0x7DB4E8
    uint32_t game_mode = 2;  // STRATEGIC_MAP
    write_block(file, &game_mode, 4);

    // Block 1: current_fight_index (4 bytes) @ 0x7D06A0
    uint32_t fight_idx = static_cast<uint32_t>(game_data.current_fight_index);
    write_block(file, &fight_idx, 4);

    // Block 2: game_difficulty (4 bytes) @ 0x7DB578
    uint32_t difficulty = static_cast<uint32_t>(game_data.difficulty);
    write_block(file, &difficulty, 4);

    // Block 3: terrain_attack_modifier (4 bytes) @ 0x475AA4
    int32_t terrain_mod = 100;
    write_block(file, &terrain_mod, 4);

    // Block 4: battle_intro_config (700 bytes) @ 0x7D01C8
    write_block(file, &game_data.cached_fight_intro, 700);

    // Block 5: loaded_map_width (4 bytes) @ 0x962788
    uint32_t map_width = static_cast<uint32_t>(game_data.map_width);
    write_block(file, &map_width, 4);

    // Block 6: current_town_index (4 bytes) @ 0x7D0484
    uint32_t town_idx = static_cast<uint32_t>(game_data.current_town);
    write_block(file, &town_idx, 4);

    // Block 7: hidden_level_score (4 bytes) @ 0x6223A8
    uint32_t hidden_score = static_cast<uint32_t>(game_data.hidden_level_score);
    write_block(file, &hidden_score, 4);

    // Block 8: static_data_table_ptr (4 bytes) @ 0x472580
    uint32_t data_ptr = 0;
    write_block(file, &data_ptr, 4);

    // Block 9: battle_result_scalar (4 bytes) @ 0x7D069C
    uint32_t battle_result = static_cast<uint32_t>(game_data.battle_result_scalar);
    write_block(file, &battle_result, 4);

    // Block 10: directdraw_surface_table (64 bytes) @ 0x583F18
    std::array<uint32_t, 16> surface_table{};
    write_block(file, surface_table.data(), 64);

    // Block 11: town_state_array (76 bytes = 19 × uint32) @ 0x7CDCB0
    write_block(file, game_data.town_states.data(), 76);

    // Block 12: story_progress_array (56 bytes = 14 × uint32) @ 0x95EAF8
    std::array<uint32_t, 14> story_progress{};
    for (size_t i = 0; i < 14 && i < game_data.story_flags.size(); ++i) {
        story_progress[i] = static_cast<uint32_t>(game_data.story_flags[i]);
    }
    write_block(file, story_progress.data(), 56);

    // Block 13: class_state composite (380 bytes) @ 0x7D06C8
    // Layout: [0-11] header(3×uint32), [12] ending_flag_protagonist, [16] ending_flag_xiaofei,
    //         [20] ally_mode, [24] battle_phase, [28] class_states[0], [32] class_states[1],
    //         [36] ending_flag_third, [40-379] class_states[2..86]
    std::array<uint8_t, 380> class_block{};
    auto* cb = class_block.data();
    // header[3] at offset 0: zeros (unknown array metadata)
    *reinterpret_cast<int32_t*>(cb + 0x0C) = game_data.ending_flag_protagonist;
    *reinterpret_cast<int32_t*>(cb + 0x10) = game_data.ending_flag_xiaofei;
    *reinterpret_cast<int32_t*>(cb + 0x14) = game_data.ally_mode_flag;
    *reinterpret_cast<int32_t*>(cb + 0x18) = game_data.battle_phase_2;
    // class_states[0] at 0x1C, class_states[1] at 0x20
    std::memcpy(cb + 0x1C, &game_data.class_states[0], 2 * 4);
    // ending_flag_third at 0x24
    *reinterpret_cast<int32_t*>(cb + 0x24) = game_data.ending_flag_third;
    // class_states[2..86] at 0x28
    std::memcpy(cb + 0x28, &game_data.class_states[2], 85 * 4);
    write_block(file, cb, 380);

    // Block 14: fighter_array (8192 bytes = 128 × 64 bytes) @ 0x96AB90
    write_block(file, game_data.fighter_array.data(), 8192);

    // Block 15: fighter_array_mirror (8192 bytes = 128 × 64 bytes) @ 0x7D93A8
    write_block(file, game_data.fighter_mirror.data(), 8192);

    // Block 16: people_array (16896 bytes = 128 × 132 bytes) @ 0x7D0EF8
    for (size_t i = 0; i < core::RUNTIME_PEOPLE_MAX; ++i) {
        write_block(file, &game_data.people[i], sizeof(core::People));
    }

    // Block 17: people_array_mirror (16896 bytes = 128 × 132 bytes) @ 0x7D51A8
    for (size_t i = 0; i < core::RUNTIME_PEOPLE_MAX; ++i) {
        write_block(file, &game_data.people[i], sizeof(core::People));
    }

    // Block 18: warehouse_inventory (320 bytes = 160 × uint16) @ 0x57BD50
    write_block(file, game_data.warehouse.data(), 320);

    // Block 19: battle_warehouse_snapshot (320 bytes = 160 × uint16) @ 0x7DB3A8
    write_block(file, game_data.battle_warehouse.data(), 320);

    // Block 20: character_state_array (80 bytes = 40 × uint16) @ 0x95EB30
    write_block(file, game_data.character_states.data(), 80);

    // Block 21: battle_string_table_2 (40 bytes = 10 × uint32) @ 0x473018
    std::array<uint32_t, 10> string_table{};
    write_block(file, string_table.data(), 40);

    // Block 22: loaded_map_grid (32768 bytes = 16384 × uint16) @ 0x962790
    write_block(file, game_data.map_grid.data(), 32768);

    // Block 23: grid_camera_offset (8 bytes = 2 × uint32) @ 0x7D0C48
    uint32_t camera_offset[2] = {
        static_cast<uint32_t>(game_data.grid_camera_col),
        static_cast<uint32_t>(game_data.grid_camera_row),
    };
    write_block(file, camera_offset, 8);

    // Block 24: battle_substate_struct (28 bytes = 7 × uint32) @ 0x7D06A8
    write_block(file, game_data.box_state.data(), 28);

    // Block 25: combat_counters (12 bytes = 3 × uint32) @ 0x8331F0
    write_block(file, game_data.combat_counters.data(), 12);

    // Block 26: dynamic_value (4 bytes) — runtime computed
    uint32_t dynamic_val = 0;
    write_block(file, &dynamic_val, 4);

    // Block 27: game_tick_counter (4 bytes) @ 0x584388
    write_block(file, &game_data.game_tick, 4);

    return true;
}

bool SaveLoader::load_game(int32_t slot, core::GameData& game_data) const {
    if (slot < 0 || slot >= core::SAVE_SLOT_COUNT) {
        return false;
    }

    std::string path = get_save_path(slot);
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    // Block 0: game_mode (4 bytes)
    uint32_t game_mode;
    if (!read_block(file, &game_mode, 4)) return false;

    // Block 1: current_fight_index (4 bytes)
    uint32_t fight_idx;
    if (!read_block(file, &fight_idx, 4)) return false;
    game_data.current_fight_index = static_cast<int32_t>(fight_idx);

    // Block 2: game_difficulty (4 bytes)
    uint32_t difficulty;
    if (!read_block(file, &difficulty, 4)) return false;
    game_data.difficulty = static_cast<int32_t>(difficulty);

    // Block 3: terrain_attack_modifier (4 bytes)
    int32_t terrain_mod;
    if (!read_block(file, &terrain_mod, 4)) return false;

    // Block 4: battle_intro_config (700 bytes)
    if (!read_block(file, &game_data.cached_fight_intro, 700)) return false;

    // Block 5: loaded_map_width (4 bytes)
    uint32_t map_width;
    if (!read_block(file, &map_width, 4)) return false;
    game_data.map_width = static_cast<int32_t>(map_width);

    // Block 6: current_town_index (4 bytes)
    uint32_t town_idx;
    if (!read_block(file, &town_idx, 4)) return false;
    game_data.current_town = static_cast<int32_t>(town_idx);

    // Block 7: hidden_level_score (4 bytes)
    uint32_t hidden_score;
    if (!read_block(file, &hidden_score, 4)) return false;
    game_data.hidden_level_score = static_cast<int32_t>(hidden_score);

    // Block 8: static_data_table_ptr (4 bytes)
    uint32_t data_ptr;
    if (!read_block(file, &data_ptr, 4)) return false;

    // Block 9: battle_result_scalar (4 bytes)
    uint32_t battle_result;
    if (!read_block(file, &battle_result, 4)) return false;
    game_data.battle_result_scalar = static_cast<int32_t>(battle_result);

    // Block 10: directdraw_surface_table (64 bytes)
    std::array<uint32_t, 16> surface_table;
    if (!read_block(file, surface_table.data(), 64)) return false;

    // Block 11: town_state_array (76 bytes = 19 × uint32)
    if (!read_block(file, game_data.town_states.data(), 76)) return false;

    // Block 12: story_progress_array (56 bytes = 14 × uint32)
    std::array<uint32_t, 14> story_progress;
    if (!read_block(file, story_progress.data(), 56)) return false;
    for (size_t i = 0; i < 14 && i < game_data.story_flags.size(); ++i) {
        game_data.story_flags[i] = static_cast<int32_t>(story_progress[i]);
    }

    // Block 13: class_state composite (380 bytes)
    std::array<uint8_t, 380> class_block;
    if (!read_block(file, class_block.data(), 380)) return false;
    {
        const auto* cb = class_block.data();
        game_data.ending_flag_protagonist = *reinterpret_cast<const int32_t*>(cb + 0x0C);
        game_data.ending_flag_xiaofei = *reinterpret_cast<const int32_t*>(cb + 0x10);
        game_data.ally_mode_flag = *reinterpret_cast<const int32_t*>(cb + 0x14);
        game_data.battle_phase_2 = *reinterpret_cast<const int32_t*>(cb + 0x18);
        // class_states[0] at 0x1C, class_states[1] at 0x20
        std::memcpy(&game_data.class_states[0], cb + 0x1C, 2 * 4);
        // ending_flag_third at 0x24
        game_data.ending_flag_third = *reinterpret_cast<const int32_t*>(cb + 0x24);
        // class_states[2..86] at 0x28
        std::memcpy(&game_data.class_states[2], cb + 0x28, 85 * 4);
    }

    // Block 14: fighter_array (8192 bytes = 128 × 64 bytes)
    if (!read_block(file, game_data.fighter_array.data(), 8192)) return false;

    // Block 15: fighter_array_mirror (8192 bytes = 128 × 64 bytes)
    if (!read_block(file, game_data.fighter_mirror.data(), 8192)) return false;

    // Block 16: people_array (16896 bytes = 128 × 132 bytes) @ 0x7D0EF8
    // 必须读满 128 条（与写侧一致）；否则后续块整体错位，map_grid 等会被读偏。
    for (size_t i = 0; i < core::RUNTIME_PEOPLE_MAX; ++i) {
        if (!read_block(file, &game_data.people[i], sizeof(core::People))) return false;
    }

    // Block 17: people_array_mirror (16896 bytes = 128 × 132 bytes)
    {
        uint8_t skip_buf[sizeof(core::People)];
        for (size_t i = 0; i < 128; ++i) {
            if (!read_block(file, skip_buf, sizeof(core::People))) return false;
        }
    }

    // Block 18: warehouse_inventory (320 bytes = 160 × uint16)
    if (!read_block(file, game_data.warehouse.data(), 320)) return false;

    // Block 19: battle_warehouse_snapshot (320 bytes = 160 × uint16)
    if (!read_block(file, game_data.battle_warehouse.data(), 320)) return false;

    // Block 20: character_state_array (80 bytes = 40 × uint16)
    if (!read_block(file, game_data.character_states.data(), 80)) return false;

    // Block 21: battle_string_table_2 (40 bytes = 10 × uint32)
    std::array<uint32_t, 10> string_table;
    if (!read_block(file, string_table.data(), 40)) return false;

    // Block 22: loaded_map_grid (32768 bytes = 16384 × uint16)
    if (!read_block(file, game_data.map_grid.data(), 32768)) return false;

    // Block 23: grid_camera_offset (8 bytes = 2 × uint32)
    uint32_t camera_offset[2];
    if (!read_block(file, camera_offset, 8)) return false;
    game_data.grid_camera_col = static_cast<int32_t>(camera_offset[0]);
    game_data.grid_camera_row = static_cast<int32_t>(camera_offset[1]);

    // Block 24: battle_substate_struct (28 bytes = 7 × uint32)
    if (!read_block(file, game_data.box_state.data(), 28)) return false;

    // Block 25: combat_counters (12 bytes = 3 × uint32)
    if (!read_block(file, game_data.combat_counters.data(), 12)) return false;

    // Block 26: dynamic_value (4 bytes)
    uint32_t dynamic_val;
    if (!read_block(file, &dynamic_val, 4)) return false;

    // Block 27: game_tick_counter (4 bytes)
    if (!read_block(file, &game_data.game_tick, 4)) return false;

    return true;
}

bool SaveLoader::slot_exists(int32_t slot) const {
    if (slot < 0 || slot >= core::SAVE_SLOT_COUNT) {
        return false;
    }
    return fs::exists(get_save_path(slot));
}

SaveInfo SaveLoader::peek_save_info(int32_t slot) const {
    SaveInfo info;
    if (slot < 0 || slot >= core::SAVE_SLOT_COUNT) return info;

    std::string path = get_save_path(slot);
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return info;
    info.exists = true;

    // Block 1 at offset 4: fight_index
    file.seekg(4);
    uint32_t fight_idx = 0;
    read_block(file, &fight_idx, 4);
    info.fight_index = static_cast<int32_t>(fight_idx);

    // Block 6 at offset 720: current_town
    // Blocks 0-5: 4+4+4+4+700+4 = 720
    file.seekg(720);
    uint32_t town_idx = 0;
    read_block(file, &town_idx, 4);
    info.current_town = static_cast<int32_t>(town_idx);

    // Block 18 at offset 51488: warehouse[0] = gold
    // Sum blocks 0-17: 4+4+4+4+700+4+4+4+4+4+64+76+56+380+8192+8192+16896+16896 = 51488
    file.seekg(51488);
    uint16_t gold = 0;
    read_block(file, &gold, 2);
    info.gold = static_cast<int32_t>(gold);

    return info;
}

std::string SaveLoader::get_save_path(int32_t slot) const {
    return (save_dir_ / ("save" + std::to_string(slot) + ".dat")).string();
}

bool SaveLoader::write_block(std::ofstream& file, const void* data, size_t size) const {
    file.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(size));
    return file.good();
}

bool SaveLoader::read_block(std::ifstream& file, void* data, size_t size) const {
    file.read(reinterpret_cast<char*>(data), static_cast<std::streamsize>(size));
    return file.good();
}

}  // namespace dlzr::app
