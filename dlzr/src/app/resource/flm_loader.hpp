#pragma once

// flm_loader.hpp — FLM.DAT 加载器
// FLM.DAT loader
//
// 32 个动画序列，每个 16 帧，48×48 调色板索引像素。
// 32 animation sequences, 16 frames each, 48x48 paletted pixels.

#include "core/data_model/constants.hpp"
#include "platform/platform.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace dlzr::app {

struct FlmFrame {
    std::vector<uint8_t> rgba;  // 48×48×4 RGBA
};

struct FlmAnimation {
    static constexpr int32_t WIDTH = 48;
    static constexpr int32_t HEIGHT = 48;
    static constexpr int32_t FRAME_COUNT = 16;
    std::array<FlmFrame, FRAME_COUNT> frames;
    bool valid{false};
};

class FlmLoader {
public:
    static constexpr int32_t MAX_ANIMATIONS = 32;

    bool load(const std::string& path);

    const FlmAnimation* get_animation(int32_t index) const {
        if (index < 0 || index >= MAX_ANIMATIONS) return nullptr;
        if (!animations_[index].valid) return nullptr;
        return &animations_[index];
    }

private:
    std::array<FlmAnimation, MAX_ANIMATIONS> animations_;

    static void apply_palette(const uint8_t* indexed, uint8_t* rgba_out);
};

}  // namespace dlzr::app
