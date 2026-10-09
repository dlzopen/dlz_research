// save_loader.cpp — 存档系统实现
// 严格匹配原作 28 数据块格式 / Strictly match original 28-block format

#include "app/resource/save_loader.hpp"

#include <fstream>
#include <cstring>

namespace dlzr::app {

// 28 数据块大小（字节）/ 28 data block sizes (bytes)
// 严格匹配原作 save_game@0x00427C94 的写入顺序
// Strictly match original save_game@0x00427C94 write order
static constexpr size_t BLOCK_SIZES[28] = {
    4, 4, 4, 4, 700, 4, 4, 4, 4, 4,       // blocks 0-9
    64, 76, 56, 380,                          // blocks 10-13
    8192, 8192, 16896, 16896,                 // blocks 14-17
    320, 320, 80, 40,                         // blocks 18-21
    32768, 8, 28, 12, 4, 4,                   // blocks 22-27
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

    // Block 0: game_mode (4 bytes)
    uint32_t game_mode = 2;  // STRATEGIC_MAP
    write_block(file, &game_mode, 4);

    // Block 1: current_fight_index (4 bytes)
    uint32_t fight_idx = static_cast<uint32_t>(game_data.current_fight_index);
    write_block(file, &fight_idx, 4);

    // Block 2: game_difficulty_or_mode (4 bytes)
    uint32_t difficulty = 1;  // Normal
    write_block(file, &difficulty, 4);

    // Block 3: terrain_attack_modifier (4 bytes)
    int32_t terrain_mod = 100;
    write_block(file, &terrain_mod, 4);

    // Block 4: battle_intro_config (700 bytes)
    // 当前战斗配置 / Current fight config (placeholder for now)
    std::vector<uint8_t> intro_config(700, 0);
    write_block(file, intro_config.data(), 700);

    // Block 5: loaded_map_width (4 bytes)
    uint32_t map_width = 128;
    write_block(file, &map_width, 4);

    // Block 6: current_town_index (4 bytes)
    uint32_t town_idx = static_cast<uint32_t>(game_data.current_town);
    write_block(file, &town_idx, 4);

    // Block 7: unknown_scalar_A (4 bytes)
    uint32_t scalar_a = 0;
    write_block(file, &scalar_a, 4);

    // Block 8: static_data_table_ptr (4 bytes)
    uint32_t data_ptr = 0;
    write_block(file, &data_ptr, 4);

    // Block 9: unknown_scalar_B (4 bytes)
    uint32_t scalar_b = 0;
    write_block(file, &scalar_b, 4);

    // Block 10: directdraw_surface_table (64 bytes = 16 × uint32)
    std::array<uint32_t, 16> surface_table{};
    write_block(file, surface_table.data(), 64);

    // Block 11: town_state_array (76 bytes = 19 × uint32)
    std::array<uint32_t, 19> town_states{};
    write_block(file, town_states.data(), 76);

    // Block 12: story_progress_array (56 bytes = 14 × uint32)
    std::array<uint32_t, 14> story_progress{};
    for (size_t i = 0; i < 14 && i < game_data.story_flags.size(); ++i) {
        story_progress[i] = static_cast<uint32_t>(game_data.story_flags[i]);
    }
    write_block(file, story_progress.data(), 56);

    // Block 13: class_state_and_flags (380 bytes)
    // 28 字节头部 + 88 × uint32 职业状态
    // 28-byte header + 88 × uint32 class states
    std::vector<uint8_t> class_state_block(380, 0);
    write_block(file, class_state_block.data(), 380);

    // Block 14: fighter_array (8192 bytes = 128 × 64 bytes)
    std::vector<uint8_t> fighter_array(8192, 0);
    write_block(file, fighter_array.data(), 8192);

    // Block 15: fighter_array_mirror (8192 bytes = 128 × 64 bytes)
    std::vector<uint8_t> fighter_mirror(8192, 0);
    write_block(file, fighter_mirror.data(), 8192);

    // Block 16: people_array (16896 bytes = 128 × 132 bytes)
    std::vector<uint8_t> people_array(16896, 0);
    // 序列化 People 数据 / Serialize People data
    for (size_t i = 0; i < core::MAX_PEOPLE && i < 128; ++i) {
        const auto& p = game_data.people[i];
        uint8_t* ptr = people_array.data() + i * 132;
        std::memcpy(ptr, p.name.data(), 16);
        *reinterpret_cast<uint32_t*>(ptr + 0x10) = p.face;
        *reinterpret_cast<uint32_t*>(ptr + 0x18) = p.enemy;
        *reinterpret_cast<uint32_t*>(ptr + 0x1C) = p.strength;
        *reinterpret_cast<uint32_t*>(ptr + 0x20) = p.intelligence;
        *reinterpret_cast<uint32_t*>(ptr + 0x24) = p.level;
        *reinterpret_cast<uint32_t*>(ptr + 0x28) = p.experience;
        *reinterpret_cast<uint32_t*>(ptr + 0x2C) = p.class_type;
        *reinterpret_cast<uint32_t*>(ptr + 0x30) = p.skill_gongji;
        *reinterpret_cast<uint32_t*>(ptr + 0x34) = p.skill_buji;
        *reinterpret_cast<uint32_t*>(ptr + 0x38) = p.skill_jili;
        *reinterpret_cast<uint32_t*>(ptr + 0x3C) = p.skill_yiliao;
        *reinterpret_cast<uint32_t*>(ptr + 0x40) = p.skill_huogong;
        *reinterpret_cast<uint32_t*>(ptr + 0x44) = p.skill_gunmu;
        *reinterpret_cast<uint32_t*>(ptr + 0x48) = p.skill_leishi;
        *reinterpret_cast<uint32_t*>(ptr + 0x4C) = p.skill_weishe;
        *reinterpret_cast<uint32_t*>(ptr + 0x50) = p.skill_saorao;
        *reinterpret_cast<uint32_t*>(ptr + 0x54) = p.skill_pohuai;
        *reinterpret_cast<uint32_t*>(ptr + 0x58) = p.skill_lueduo;
        *reinterpret_cast<uint32_t*>(ptr + 0x5C) = p.skill_fanjian;
        *reinterpret_cast<uint32_t*>(ptr + 0x60) = p.skill_yibing;
        *reinterpret_cast<uint32_t*>(ptr + 0x64) = p.skill_duzhan;
        *reinterpret_cast<uint32_t*>(ptr + 0x68) = p.skill_ansha;
        *reinterpret_cast<uint32_t*>(ptr + 0x6C) = p.skill_shidu;
        *reinterpret_cast<uint32_t*>(ptr + 0x70) = p.skill_kongxi;
        for (int w = 0; w < 8; ++w) {
            ptr[0x74 + w] = p.items[w];
        }
        for (int d = 0; d < 8; ++d) {
            ptr[0x7C + d] = p.mines[d];
        }
    }
    write_block(file, people_array.data(), 16896);

    // Block 17: people_array_mirror (16896 bytes = 128 × 132 bytes)
    std::vector<uint8_t> people_mirror(16896, 0);
    write_block(file, people_mirror.data(), 16896);

    // Block 18: warehouse_inventory (320 bytes = 160 × uint16)
    std::array<uint16_t, 160> warehouse_arr{};
    for (size_t i = 0; i < 160 && i < game_data.warehouse.size(); ++i) {
        warehouse_arr[i] = game_data.warehouse[i];
    }
    write_block(file, warehouse_arr.data(), 320);

    // Block 19: battle_warehouse_snapshot (320 bytes = 160 × uint16)
    std::array<uint16_t, 160> battle_warehouse{};
    write_block(file, battle_warehouse.data(), 320);

    // Block 20: character_state_array (80 bytes = 40 × uint16)
    std::array<uint16_t, 40> char_states{};
    write_block(file, char_states.data(), 80);

    // Block 21: battle_string_table_2 (40 bytes = 10 × uint32)
    std::array<uint32_t, 10> string_table{};
    write_block(file, string_table.data(), 40);

    // Block 22: loaded_map_grid (32768 bytes = 16384 × uint16, 128×128)
    std::vector<uint8_t> map_grid(32768, 0);
    write_block(file, map_grid.data(), 32768);

    // Block 23: grid_camera_offset (8 bytes = 2 × uint32)
    uint32_t camera_offset[2] = {0, 0};
    write_block(file, camera_offset, 8);

    // Block 24: battle_substate_struct (28 bytes)
    std::vector<uint8_t> box_state(28, 0);
    write_block(file, box_state.data(), 28);

    // Block 25: combat_counters (12 bytes = 3 × uint32)
    uint32_t combat_counters[3] = {0, 0, 0};
    write_block(file, combat_counters, 12);

    // Block 26: dynamic_value (4 bytes)
    uint32_t dynamic_val = 0;
    write_block(file, &dynamic_val, 4);

    // Block 27: game_tick_counter (4 bytes)
    uint32_t game_tick = 0;
    write_block(file, &game_tick, 4);

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

    // Block 2: game_difficulty_or_mode (4 bytes)
    uint32_t difficulty;
    if (!read_block(file, &difficulty, 4)) return false;

    // Block 3: terrain_attack_modifier (4 bytes)
    int32_t terrain_mod;
    if (!read_block(file, &terrain_mod, 4)) return false;

    // Block 4: battle_intro_config (700 bytes)
    std::vector<uint8_t> intro_config(700);
    if (!read_block(file, intro_config.data(), 700)) return false;

    // Block 5: loaded_map_width (4 bytes)
    uint32_t map_width;
    if (!read_block(file, &map_width, 4)) return false;

    // Block 6: current_town_index (4 bytes)
    uint32_t town_idx;
    if (!read_block(file, &town_idx, 4)) return false;
    game_data.current_town = static_cast<int32_t>(town_idx);

    // Block 7: unknown_scalar_A (4 bytes)
    uint32_t scalar_a;
    if (!read_block(file, &scalar_a, 4)) return false;

    // Block 8: static_data_table_ptr (4 bytes)
    uint32_t data_ptr;
    if (!read_block(file, &data_ptr, 4)) return false;

    // Block 9: unknown_scalar_B (4 bytes)
    uint32_t scalar_b;
    if (!read_block(file, &scalar_b, 4)) return false;

    // Block 10: directdraw_surface_table (64 bytes)
    std::array<uint32_t, 16> surface_table;
    if (!read_block(file, surface_table.data(), 64)) return false;

    // Block 11: town_state_array (76 bytes)
    std::array<uint32_t, 19> town_states;
    if (!read_block(file, town_states.data(), 76)) return false;

    // Block 12: story_progress_array (56 bytes = 14 × uint32)
    std::array<uint32_t, 14> story_progress;
    if (!read_block(file, story_progress.data(), 56)) return false;
    for (size_t i = 0; i < 14 && i < game_data.story_flags.size(); ++i) {
        game_data.story_flags[i] = static_cast<int32_t>(story_progress[i]);
    }

    // Block 13: class_state_and_flags (380 bytes)
    std::vector<uint8_t> class_state_block(380);
    if (!read_block(file, class_state_block.data(), 380)) return false;

    // Block 14: fighter_array (8192 bytes)
    std::vector<uint8_t> fighter_array(8192);
    if (!read_block(file, fighter_array.data(), 8192)) return false;

    // Block 15: fighter_array_mirror (8192 bytes)
    std::vector<uint8_t> fighter_mirror(8192);
    if (!read_block(file, fighter_mirror.data(), 8192)) return false;

    // Block 16: people_array (16896 bytes = 128 × 132 bytes)
    std::vector<uint8_t> people_array(16896);
    if (!read_block(file, people_array.data(), 16896)) return false;

    // 反序列化 People 数据 / Deserialize People data
    for (size_t i = 0; i < core::MAX_PEOPLE && i < 128; ++i) {
        auto& p = game_data.people[i];
        const uint8_t* ptr = people_array.data() + i * 132;
        std::memcpy(p.name.data(), ptr, 16);
        p.face = *reinterpret_cast<const uint32_t*>(ptr + 0x10);
        p.enemy = *reinterpret_cast<const uint32_t*>(ptr + 0x18);
        p.strength = *reinterpret_cast<const uint32_t*>(ptr + 0x1C);
        p.intelligence = *reinterpret_cast<const uint32_t*>(ptr + 0x20);
        p.level = *reinterpret_cast<const uint32_t*>(ptr + 0x24);
        p.experience = *reinterpret_cast<const uint32_t*>(ptr + 0x28);
        p.class_type = *reinterpret_cast<const uint32_t*>(ptr + 0x2C);
        p.skill_gongji = *reinterpret_cast<const uint32_t*>(ptr + 0x30);
        p.skill_buji = *reinterpret_cast<const uint32_t*>(ptr + 0x34);
        p.skill_jili = *reinterpret_cast<const uint32_t*>(ptr + 0x38);
        p.skill_yiliao = *reinterpret_cast<const uint32_t*>(ptr + 0x3C);
        p.skill_huogong = *reinterpret_cast<const uint32_t*>(ptr + 0x40);
        p.skill_gunmu = *reinterpret_cast<const uint32_t*>(ptr + 0x44);
        p.skill_leishi = *reinterpret_cast<const uint32_t*>(ptr + 0x48);
        p.skill_weishe = *reinterpret_cast<const uint32_t*>(ptr + 0x4C);
        p.skill_saorao = *reinterpret_cast<const uint32_t*>(ptr + 0x50);
        p.skill_pohuai = *reinterpret_cast<const uint32_t*>(ptr + 0x54);
        p.skill_lueduo = *reinterpret_cast<const uint32_t*>(ptr + 0x58);
        p.skill_fanjian = *reinterpret_cast<const uint32_t*>(ptr + 0x5C);
        p.skill_yibing = *reinterpret_cast<const uint32_t*>(ptr + 0x60);
        p.skill_duzhan = *reinterpret_cast<const uint32_t*>(ptr + 0x64);
        p.skill_ansha = *reinterpret_cast<const uint32_t*>(ptr + 0x68);
        p.skill_shidu = *reinterpret_cast<const uint32_t*>(ptr + 0x6C);
        p.skill_kongxi = *reinterpret_cast<const uint32_t*>(ptr + 0x70);
        for (int w = 0; w < 8; ++w) {
            p.items[w] = ptr[0x74 + w];
        }
        for (int d = 0; d < 8; ++d) {
            p.mines[d] = ptr[0x7C + d];
        }
    }

    // Block 17: people_array_mirror (16896 bytes)
    std::vector<uint8_t> people_mirror(16896);
    if (!read_block(file, people_mirror.data(), 16896)) return false;

    // Block 18: warehouse_inventory (320 bytes = 160 × uint16)
    std::array<uint16_t, 160> warehouse_arr;
    if (!read_block(file, warehouse_arr.data(), 320)) return false;
    for (size_t i = 0; i < 160 && i < game_data.warehouse.size(); ++i) {
        game_data.warehouse[i] = warehouse_arr[i];
    }

    // Block 19: battle_warehouse_snapshot (320 bytes)
    std::array<uint16_t, 160> battle_warehouse;
    if (!read_block(file, battle_warehouse.data(), 320)) return false;

    // Block 20: character_state_array (80 bytes)
    std::array<uint16_t, 40> char_states;
    if (!read_block(file, char_states.data(), 80)) return false;

    // Block 21: battle_string_table_2 (40 bytes)
    std::array<uint32_t, 10> string_table;
    if (!read_block(file, string_table.data(), 40)) return false;

    // Block 22: loaded_map_grid (32768 bytes)
    std::vector<uint8_t> map_grid(32768);
    if (!read_block(file, map_grid.data(), 32768)) return false;

    // Block 23: grid_camera_offset (8 bytes)
    uint32_t camera_offset[2];
    if (!read_block(file, camera_offset, 8)) return false;

    // Block 24: battle_substate_struct (28 bytes)
    std::vector<uint8_t> box_state(28);
    if (!read_block(file, box_state.data(), 28)) return false;

    // Block 25: combat_counters (12 bytes)
    uint32_t combat_counters[3];
    if (!read_block(file, combat_counters, 12)) return false;

    // Block 26: dynamic_value (4 bytes)
    uint32_t dynamic_val;
    if (!read_block(file, &dynamic_val, 4)) return false;

    // Block 27: game_tick_counter (4 bytes)
    uint32_t game_tick;
    if (!read_block(file, &game_tick, 4)) return false;

    return true;
}

bool SaveLoader::slot_exists(int32_t slot) const {
    if (slot < 0 || slot >= core::SAVE_SLOT_COUNT) {
        return false;
    }
    return fs::exists(get_save_path(slot));
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
