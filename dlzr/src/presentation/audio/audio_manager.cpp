#include "presentation/audio/audio_manager.hpp"

#include <filesystem>

namespace fs = std::filesystem;

namespace dlzr::presentation {

void AudioManager::init(platform::Platform& platform, const app::DatLoader& loader,
                        const std::string& game_data_dir) {
    platform_ = &platform;
    loader_ = &loader;
    game_data_dir_ = game_data_dir;
}

void AudioManager::set_mod_dir(const std::string& mod_dir) {
    mod_dir_ = mod_dir;
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

    if (current_bgm_track_ >= 0 && platform_) {
        platform_->set_music_volume(0);
    }

    current_bgm_track_ = track;
    fade_from_volume_ = 0;
    fade_target_volume_ = base_volume_;
    fade_duration_ms_ = 600;
    fade_elapsed_ms_ = 0;

    if (!platform_) {
        return;
    }

    fs::path music_dir = fs::path(game_data_dir_) / "music";
    std::string track_name = "track" + std::to_string(track);
    fs::path mp3_path = music_dir / (track_name + ".mp3");
    fs::path ogg_path = music_dir / (track_name + ".ogg");

    if (!mod_dir_.empty()) {
        fs::path mod_music = fs::path(mod_dir_) / "music";
        fs::path mod_mp3 = mod_music / (track_name + ".mp3");
        fs::path mod_ogg = mod_music / (track_name + ".ogg");
        if (fs::exists(mod_mp3)) {
            mp3_path = mod_mp3;
            ogg_path = mod_ogg;
        } else if (fs::exists(mod_ogg)) {
            mp3_path = mod_mp3;
            ogg_path = mod_ogg;
        }
    }

    if (fs::exists(mp3_path)) {
        platform_->play_music(mp3_path.string(), true);
        platform_->set_music_volume(0);
    } else if (fs::exists(ogg_path)) {
        platform_->play_music(ogg_path.string(), true);
        platform_->set_music_volume(0);
    } else {
        std::printf("[BGM] Music file not found for track %d\n", track);
    }
}

void AudioManager::stop_bgm() {
    if (current_bgm_track_ >= 0) {
        fade_from_volume_ = base_volume_;
        fade_target_volume_ = 0;
        fade_duration_ms_ = 400;
        fade_elapsed_ms_ = 0;
        if (platform_) {
            platform_->stop_music();
        }
        current_bgm_track_ = -1;
    }
}

void AudioManager::set_music_volume(int32_t volume) {
    base_volume_ = volume;
    if (platform_ && fade_from_volume_ < 0) {
        platform_->set_music_volume(volume);
    }
}

void AudioManager::fade_bgm(int32_t target_volume, int32_t duration_ms) {
    fade_from_volume_ = (fade_target_volume_ >= 0) ? fade_target_volume_ : base_volume_;
    fade_target_volume_ = target_volume;
    fade_duration_ms_ = duration_ms;
    fade_elapsed_ms_ = 0;
}

void AudioManager::update(uint32_t delta_ms) {
    if (fade_target_volume_ < 0 || !platform_) return;
    if (fade_duration_ms_ <= 0) {
        platform_->set_music_volume(fade_target_volume_);
        fade_from_volume_ = fade_target_volume_;
        return;
    }

    fade_elapsed_ms_ += static_cast<int32_t>(delta_ms);
    float t = static_cast<float>(fade_elapsed_ms_) /
              static_cast<float>(fade_duration_ms_);
    if (t > 1.0f) t = 1.0f;

    int32_t vol = fade_from_volume_ +
                  static_cast<int32_t>((fade_target_volume_ - fade_from_volume_) * t);
    if (vol < 0) vol = 0;
    if (vol > 128) vol = 128;
    platform_->set_music_volume(vol);

    if (t >= 1.0f) {
        fade_from_volume_ = fade_target_volume_;
    }
}

}  // namespace dlzr::presentation
