#include "platform/mock/mock_platform.hpp"

namespace dlzr::platform {

bool MockPlatform::init(const PlatformConfig& /*config*/) {
    return true;
}

void MockPlatform::shutdown() {
    textures_.clear();
}

bool MockPlatform::poll_event(Event& out_event) {
    out_event.type = Event::Type::None;
    return false;
}

void MockPlatform::clear(uint8_t /*r*/, uint8_t /*g*/, uint8_t /*b*/) {
    clear_count_++;
}

void MockPlatform::present() {
    present_count_++;
}

TextureHandle MockPlatform::create_texture(int32_t width, int32_t height) {
    TextureHandle handle = next_texture_++;
    textures_[handle] = TextureData{width, height, {}};
    return handle;
}

void MockPlatform::update_texture(TextureHandle tex,
                                  const uint8_t* rgba_data,
                                  int32_t pitch) {
    auto it = textures_.find(tex);
    if (it != textures_.end() && rgba_data) {
        size_t size = static_cast<size_t>(it->second.width * it->second.height * 4);
        it->second.data.assign(rgba_data, rgba_data + size);
    }
}

void MockPlatform::draw_texture(TextureHandle /*tex*/,
                                int32_t /*dst_x*/, int32_t /*dst_y*/) {
}

void MockPlatform::draw_texture_rect(TextureHandle /*tex*/,
                                     int32_t /*dst_x*/, int32_t /*dst_y*/,
                                     int32_t /*dst_w*/, int32_t /*dst_h*/) {
}

void MockPlatform::destroy_texture(TextureHandle tex) {
    textures_.erase(tex);
}

void MockPlatform::draw_rect(int32_t /*x*/, int32_t /*y*/,
                             int32_t /*w*/, int32_t /*h*/,
                             uint8_t /*r*/, uint8_t /*g*/, uint8_t /*b*/) {
}

void MockPlatform::fill_rect(int32_t /*x*/, int32_t /*y*/,
                             int32_t /*w*/, int32_t /*h*/,
                             uint8_t /*r*/, uint8_t /*g*/, uint8_t /*b*/) {
}

Platform::AudioHandle MockPlatform::play_sound(const uint8_t* /*pcm_data*/,
                                               size_t /*size*/,
                                               int32_t /*sample_rate*/) {
    return next_sound_++;
}

void MockPlatform::stop_sound(AudioHandle /*handle*/) {
}

bool MockPlatform::is_sound_playing(AudioHandle /*handle*/) {
    return false;
}

bool MockPlatform::play_music(const std::string& /*path*/, bool /*loop*/) {
    return true;
}

void MockPlatform::stop_music() {
}

void MockPlatform::set_music_volume(int32_t /*volume*/) {
}

void MockPlatform::delay_ms(uint32_t ms) {
    current_time_ += ms;
}

uint32_t MockPlatform::ticks_ms() {
    return current_time_;
}

}  // namespace dlzr::platform
