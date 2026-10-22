#pragma once

// dat_loader.hpp — 统一 DAT 文件读取接口
// Unified binary DAT file reading interface
//
// 运行时直接读取原始二进制文件，不依赖中间格式。
// Reads original binary files at runtime, no intermediate formats.

#include "core/data_model/types.hpp"
#include "core/data_model/constants.hpp"
#include "core/data_model/game_data.hpp"

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace dlzr::app {

namespace fs = std::filesystem;

// ============================================================
// 二进制文件读取辅助 / Binary file read helper
// ============================================================
class BinaryReader {
public:
    explicit BinaryReader(const fs::path& path) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open: " + path.string());
        }
        auto size = file.tellg();
        data_.resize(static_cast<size_t>(size));
        file.seekg(0);
        file.read(reinterpret_cast<char*>(data_.data()),
                  static_cast<std::streamsize>(size));
    }

    const uint8_t* data() const { return data_.data(); }
    size_t size() const { return data_.size(); }

    template <typename T>
    T read_at(size_t offset) const {
        if (offset + sizeof(T) > data_.size()) {
            throw std::out_of_range("BinaryReader::read_at out of bounds");
        }
        T value;
        std::memcpy(&value, data_.data() + offset, sizeof(T));
        return value;
    }

    void read_bytes(size_t offset, void* dst, size_t count) const {
        if (offset + count > data_.size()) {
            throw std::out_of_range("BinaryReader::read_bytes out of bounds");
        }
        std::memcpy(dst, data_.data() + offset, count);
    }

    std::vector<uint8_t> read_vector(size_t offset, size_t count) const {
        if (offset + count > data_.size()) {
            throw std::out_of_range("BinaryReader::read_vector out of bounds");
        }
        return {data_.begin() + static_cast<long>(offset),
                data_.begin() + static_cast<long>(offset + count)};
    }

private:
    std::vector<uint8_t> data_;
};

// ============================================================
// DAT 加载器 / DAT file loader
// ============================================================
class DatLoader {
public:
    explicit DatLoader(fs::path game_data_dir)
        : game_dir_(std::move(game_data_dir)) {}

    core::GameData load_game_data() const;
    core::FightData load_fight(int32_t fight_index) const;
    core::FightMap load_map(int32_t map_index) const;
    core::BlkGroupData load_blk_group(int32_t blk_group) const;
    std::vector<std::string> load_dialogues() const;
    std::vector<uint8_t> load_wave_clip(int32_t wave_id) const;
    void load_towns(core::GameData& gd) const;

    const fs::path& game_dir() const { return game_dir_; }

private:
    fs::path resolve(const std::string& filename) const {
        return game_dir_ / filename;
    }

    fs::path game_dir_;
};

}  // namespace dlzr::app
