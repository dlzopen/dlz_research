#pragma once

// 存档系统 / Save system
// 10 存档位 × 28 数据块 × 85072 字节
// 10 slots × 28 blocks × 85072 bytes
// 与原作存档格式兼容 / Compatible with original save format

#include "core/data_model/game_data.hpp"
#include "core/data_model/constants.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>

namespace dlzr::app {

namespace fs = std::filesystem;

struct SaveInfo {
    bool exists{false};
    int32_t fight_index{0};
    int32_t current_town{0};
    int32_t gold{0};
};

class SaveLoader {
public:
    explicit SaveLoader(fs::path save_dir) : save_dir_(std::move(save_dir)) {}

    bool save_game(int32_t slot, const core::GameData& game_data) const;
    bool load_game(int32_t slot, core::GameData& game_data) const;

    bool slot_exists(int32_t slot) const;
    std::string get_save_path(int32_t slot) const;
    SaveInfo peek_save_info(int32_t slot) const;

private:
    fs::path save_dir_;

    bool write_block(std::ofstream& file, const void* data, size_t size) const;
    bool read_block(std::ifstream& file, void* data, size_t size) const;
};

}  // namespace dlzr::app
