#include "presentation/audio/audio_manager.hpp"

namespace dlzr::presentation {

void AudioManager::init(platform::Platform& platform, const app::DatLoader& loader) {
    platform_ = &platform;
    loader_ = &loader;
}

void AudioManager::play_sound(int32_t wave_id) {
    if (!platform_ || !loader_) {
        return;
    }

    auto it = cache_.find(wave_id);
    if (it == cache_.end()) {
        try {
            cache_[wave_id] = loader_->load_wave_clip(wave_id);
            it = cache_.find(wave_id);
        } catch (const std::exception&) {
            return;
        }
    }

    if (it != cache_.end() && !it->second.empty()) {
        platform_->play_sound(it->second.data(), it->second.size());
    }
}

void AudioManager::stop_sound(platform::Platform::AudioHandle handle) {
    if (platform_) {
        platform_->stop_sound(handle);
    }
}

bool AudioManager::is_playing(platform::Platform::AudioHandle handle) const {
    if (platform_) {
        return platform_->is_sound_playing(handle);
    }
    return false;
}

void AudioManager::play_bgm(int32_t track) {
    if (current_bgm_track_ == track) {
        return;
    }
    current_bgm_track_ = track;
    std::printf("[BGM] Playing track %d\n", track);
}

void AudioManager::stop_bgm() {
    if (current_bgm_track_ >= 0) {
        std::printf("[BGM] Stopped track %d\n", current_bgm_track_);
        current_bgm_track_ = -1;
    }
}

}  // namespace dlzr::presentation
