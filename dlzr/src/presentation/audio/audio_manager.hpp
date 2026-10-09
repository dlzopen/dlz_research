#pragma once

// audio_manager.hpp — 音频管理器
// Audio manager
//
// 管理音效播放，支持从 WAVE.DAT 加载和播放音效。
// Manages sound playback, supports loading and playing from WAVE.DAT.

#include "app/resource/dat_loader.hpp"
#include "platform/platform.hpp"

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace dlzr::presentation {

class AudioManager {
public:
    void init(platform::Platform& platform, const app::DatLoader& loader);

    // 播放指定 ID 的音效 / Play sound effect by ID
    void play_sound(int32_t wave_id);

    // 停止指定音效 / Stop specific sound
    void stop_sound(platform::Platform::AudioHandle handle);

    // 检查音效是否正在播放 / Check if sound is playing
    bool is_playing(platform::Platform::AudioHandle handle) const;

    // 播放背景音乐（原作用 CD 音频，此处为接口预留）
    // Play background music (original used CD audio, this is interface placeholder)
    void play_bgm(int32_t track);

    // 停止背景音乐 / Stop background music
    void stop_bgm();

private:
    platform::Platform* platform_{nullptr};
    const app::DatLoader* loader_{nullptr};
    std::unordered_map<int32_t, std::vector<uint8_t>> cache_;
    int32_t current_bgm_track_{-1};
};

}  // namespace dlzr::presentation
