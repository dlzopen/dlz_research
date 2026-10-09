#pragma once

// face_loader.hpp — FACE.DAT 头像加载器
// FACE.DAT portrait loader
//
// 170 portraits, each 80×100 pixels, 8-bit palette-indexed.
// Total file size: 1,360,000 bytes.

#include <cstdint>
#include <string>
#include <vector>

namespace dlzr::app {

class FaceLoader {
public:
    static constexpr int32_t FACE_WIDTH = 80;
    static constexpr int32_t FACE_HEIGHT = 100;
    static constexpr int32_t FACE_SIZE = FACE_WIDTH * FACE_HEIGHT;
    static constexpr int32_t FACE_COUNT = 170;

    bool load(const std::string& path);

    const uint8_t* get_face_rgba(int32_t index) const;

    bool is_loaded() const { return !raw_data_.empty(); }

private:
    std::vector<uint8_t> raw_data_;
    std::vector<std::vector<uint8_t>> rgba_cache_;

    static const uint8_t* shared_palette();
    void build_rgba_cache();
};

}  // namespace dlzr::app
