#pragma once

// audio_manager.hpp — 音频管理器
// Audio manager
//
// 管理音效和背景音乐播放。
// Manages sound effects and background music playback.

#include "app/resource/dat_loader.hpp"
#include "platform/platform.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace dlzr::presentation {

class AudioManager {
public:
    void init(platform::Platform& platform, const app::DatLoader& loader,
              const std::string& game_data_dir = ".");

    void set_mod_dir(const std::string& mod_dir);

    // 播放指定 ID 的音效 / Play sound effect by ID
    void play_sound(int32_t wave_id);

    // 停止指定音效 / Stop specific sound
    void stop_sound(platform::Platform::AudioHandle handle);

    // 检查音效是否正在播放 / Check if sound is playing
    bool is_playing(platform::Platform::AudioHandle handle) const;

    // 播放背景音乐 / Play background music
    // 会在 game_data_dir/music/ 目录下查找 track{N}.mp3 或 track{N}.ogg
    // Looks for track{N}.mp3 or track{N}.ogg in game_data_dir/music/
    void play_bgm(int32_t track);

    // 停止背景音乐 / Stop background music
    void stop_bgm();

    // 设置音乐音量 (0-128) / Set music volume (0-128)
    void set_music_volume(int32_t volume);

    // BGM 淡入淡出 / BGM fade transition
    void fade_bgm(int32_t target_volume, int32_t duration_ms);

    // 每帧更新（处理淡入淡出）/ Per-frame update (handles fading)
    void update(uint32_t delta_ms);

private:
    platform::Platform* platform_{nullptr};
    const app::DatLoader* loader_{nullptr};
    std::string game_data_dir_;
    std::string mod_dir_;
    std::unordered_map<int32_t, std::vector<uint8_t>> cache_;
    int32_t current_bgm_track_{-1};

    int32_t fade_from_volume_{-1};
    int32_t fade_target_volume_{-1};
    int32_t fade_duration_ms_{0};
    int32_t fade_elapsed_ms_{0};
    int32_t base_volume_{128};
};

}  // namespace dlzr::presentation
