#include "platform/sdl3/sdl3_platform.hpp"

#include <cstdio>
#include <stdexcept>

namespace dlzr::platform {

Sdl3Platform::~Sdl3Platform() {
    shutdown();
}

bool Sdl3Platform::init(const PlatformConfig& config) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD)) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }

    // 初始化 SDL3_mixer / Initialize SDL3_mixer
#ifdef DLZR_HAS_SDL3_MIXER
    if (!Mix_Init(MIX_INIT_MP3 | MIX_INIT_OGG)) {
        std::fprintf(stderr, "Mix_Init failed: %s\n", Mix_GetError());
    }

    if (Mix_OpenAudio(0, nullptr) < 0) {
        std::fprintf(stderr, "Mix_OpenAudio failed: %s\n", Mix_GetError());
    }
#endif

    // 计算窗口尺寸 / Calculate window size
    int32_t window_width = config.width * config.render_scale;
    int32_t window_height = config.height * config.render_scale;

    // 窗口标志 / Window flags
    SDL_WindowFlags flags = 0;
    if (config.fullscreen) {
        flags |= SDL_WINDOW_FULLSCREEN;
    }

    window_ = SDL_CreateWindow(config.title.c_str(),
                               window_width, window_height, flags);
    if (!window_) {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return false;
    }

    // 渲染器标志 / Renderer flags
    if (config.vsync) {
        SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
    }

    // 使用 SDL 默认渲染后端（D3D11 / OpenGL / Metal 等由 SDL 按平台选择）。
    // 注：此前曾强制 software 渲染以"绕开 Intel D3D11 驱动崩溃"，但经逆向定位，
    // 那些崩溃实为字体渲染换行逻辑越界写穿堆缓冲（见 font_renderer.cpp
    // render_text_to_buffer）导致的堆损坏，与驱动无关。此处还原默认后端。
    renderer_ = SDL_CreateRenderer(window_, nullptr);
    if (!renderer_) {
        std::fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        return false;
    }
    std::fprintf(stderr, "[Platform] Active renderer: %s\n",
                 SDL_GetRendererName(renderer_) ? SDL_GetRendererName(renderer_) : "(unknown)");

    // 设置逻辑尺寸 / Set logical size for scaling
    SDL_SetRenderLogicalPresentation(renderer_, config.width, config.height,
                                     SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);

    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);

    // Debug: check actual drawable size
    int dw, dh;
    SDL_GetWindowSizeInPixels(window_, &dw, &dh);
    std::fprintf(stderr, "[DEBUG] Window client size: %dx%d\n", dw, dh);

    SDL_AudioSpec spec{SDL_AUDIO_S16LE, 1, 22050};
    audio_device_ = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);
    if (!audio_device_) {
        std::fprintf(stderr, "SDL_OpenAudioDevice failed: %s\n", SDL_GetError());
    }

    std::printf("[Platform] Window: %dx%d, Render: %dx%d, Scale: %dx\n",
                window_width, window_height, config.width, config.height, config.render_scale);

    return true;
}

void Sdl3Platform::shutdown() {
    for (auto& [handle, tex] : textures_) {
        if (tex) {
            SDL_DestroyTexture(tex);
        }
    }
    textures_.clear();

    for (auto& bucket : deferred_destroy_) {
        for (SDL_Texture* t : bucket) {
            if (t) SDL_DestroyTexture(t);
        }
        bucket.clear();
    }
    destroy_gen_ = 0;

    for (auto& [handle, sound] : sounds_) {
        if (sound.stream) {
            SDL_DestroyAudioStream(sound.stream);
        }
    }
    sounds_.clear();

    if (current_music_) {
#ifdef DLZR_HAS_SDL3_MIXER
        Mix_FreeMusic(current_music_);
#endif
        current_music_ = nullptr;
    }
#ifdef DLZR_HAS_SDL3_MIXER
    Mix_CloseAudio();
    Mix_Quit();
#endif

    if (audio_device_) {
        SDL_CloseAudioDevice(audio_device_);
        audio_device_ = 0;
    }

    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    SDL_Quit();
}

// 系统保留键：这些键属于 OS（截图/滚动锁定/暂停），不得转发给游戏逻辑。
// 否则任何"任意键继续/快进"处理器都会把它们当成操作键吞掉，
// 典型症状：按 PrintScreen 想截图，却把开场幻灯片翻到了下一页。
static bool is_system_reserved_key(SDL_Keycode key) {
    return key == SDLK_PRINTSCREEN ||
           key == SDLK_SCROLLLOCK ||
           key == SDLK_PAUSE;
}

bool Sdl3Platform::poll_event(Event& out_event) {
    SDL_Event sdl_event;
    while (SDL_PollEvent(&sdl_event)) {
        switch (sdl_event.type) {
            case SDL_EVENT_QUIT:
                out_event.type = Event::Type::Quit;
                return true;
            case SDL_EVENT_KEY_DOWN:
                // 吞掉系统保留键，继续取下一个事件（不生成 KeyDown）
                if (is_system_reserved_key(sdl_event.key.key)) {
                    break;
                }
                out_event.type = Event::Type::KeyDown;
                out_event.key = static_cast<int32_t>(sdl_event.key.key);
                return true;
            case SDL_EVENT_KEY_UP:
                if (is_system_reserved_key(sdl_event.key.key)) {
                    break;
                }
                out_event.type = Event::Type::KeyUp;
                out_event.key = static_cast<int32_t>(sdl_event.key.key);
                return true;
            case SDL_EVENT_MOUSE_BUTTON_DOWN: {
                out_event.type = Event::Type::MouseDown;
                out_event.mouse_x = static_cast<int32_t>(sdl_event.button.x);
                out_event.mouse_y = static_cast<int32_t>(sdl_event.button.y);
                out_event.mouse_button = sdl_event.button.button;
                return true;
            }
            case SDL_EVENT_MOUSE_BUTTON_UP:
                SDL_ConvertEventToRenderCoordinates(renderer_, &sdl_event);
                out_event.type = Event::Type::MouseUp;
                out_event.mouse_x = static_cast<int32_t>(sdl_event.button.x);
                out_event.mouse_y = static_cast<int32_t>(sdl_event.button.y);
                out_event.mouse_button = sdl_event.button.button;
                return true;
            case SDL_EVENT_MOUSE_MOTION:
                SDL_ConvertEventToRenderCoordinates(renderer_, &sdl_event);
                out_event.type = Event::Type::MouseMove;
                out_event.mouse_x = static_cast<int32_t>(sdl_event.motion.x);
                out_event.mouse_y = static_cast<int32_t>(sdl_event.motion.y);
                return true;
            case SDL_EVENT_MOUSE_WHEEL:
                out_event.type = Event::Type::MouseWheel;
                out_event.wheel_x = static_cast<int32_t>(sdl_event.wheel.x);
                out_event.wheel_y = static_cast<int32_t>(sdl_event.wheel.y);
                return true;

            // Touch events
            case SDL_EVENT_FINGER_DOWN:
                out_event.type = Event::Type::TouchDown;
                out_event.touch_id = static_cast<int32_t>(sdl_event.tfinger.fingerID);
                out_event.touch_x = sdl_event.tfinger.x;
                out_event.touch_y = sdl_event.tfinger.y;
                out_event.touch_pressure = sdl_event.tfinger.pressure;
                return true;
            case SDL_EVENT_FINGER_UP:
                out_event.type = Event::Type::TouchUp;
                out_event.touch_id = static_cast<int32_t>(sdl_event.tfinger.fingerID);
                out_event.touch_x = sdl_event.tfinger.x;
                out_event.touch_y = sdl_event.tfinger.y;
                out_event.touch_pressure = sdl_event.tfinger.pressure;
                return true;
            case SDL_EVENT_FINGER_MOTION:
                out_event.type = Event::Type::TouchMove;
                out_event.touch_id = static_cast<int32_t>(sdl_event.tfinger.fingerID);
                out_event.touch_x = sdl_event.tfinger.x;
                out_event.touch_y = sdl_event.tfinger.y;
                out_event.touch_pressure = sdl_event.tfinger.pressure;
                return true;

            // Gamepad events
            case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
                out_event.type = Event::Type::GamepadButtonDown;
                out_event.gamepad_button = static_cast<int32_t>(sdl_event.gbutton.button);
                return true;
            case SDL_EVENT_GAMEPAD_BUTTON_UP:
                out_event.type = Event::Type::GamepadButtonUp;
                out_event.gamepad_button = static_cast<int32_t>(sdl_event.gbutton.button);
                return true;
            case SDL_EVENT_GAMEPAD_AXIS_MOTION:
                out_event.type = Event::Type::GamepadAxis;
                out_event.gamepad_axis = static_cast<int32_t>(sdl_event.gaxis.axis);
                out_event.gamepad_axis_value = sdl_event.gaxis.value / 32767.0f;
                return true;

            case SDL_EVENT_TEXT_INPUT:
                out_event.type = Event::Type::TextInput;
                std::strncpy(out_event.text, sdl_event.text.text,
                             sizeof(out_event.text) - 1);
                out_event.text[sizeof(out_event.text) - 1] = '\0';
                return true;

            default:
                break;
        }
    }
    out_event.type = Event::Type::None;
    return false;
}

void Sdl3Platform::clear(uint8_t r, uint8_t g, uint8_t b) {
    SDL_SetRenderDrawColor(renderer_, r, g, b, 255);
    SDL_RenderClear(renderer_);
}

void Sdl3Platform::present() {
    SDL_RenderPresent(renderer_);
    flush_deferred_destroy();
}

TextureHandle Sdl3Platform::create_texture(int32_t width, int32_t height) {
    // 用 STREAMING 纹理：本项目所有纹理均为"创建一次 + 上传一次 + 渲染多次"，
    // STREAMING 允许后续按需 UpdateTexture 且 SDL 会管理 GPU 上传。
    // （此前曾误改为 STATIC 以规避"Intel D3D11 驱动崩溃"，但经定位那些崩溃
    // 实为字体堆缓冲越界写穿所致，与纹理访问模式无关，故还原 STREAMING。）
    SDL_Texture* tex = SDL_CreateTexture(
        renderer_, SDL_PIXELFORMAT_ABGR8888,
        SDL_TEXTUREACCESS_STREAMING, width, height);
    if (!tex) {
        std::fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
        return 0;
    }

    TextureHandle handle = next_texture_++;
    textures_[handle] = tex;
    return handle;
}

void Sdl3Platform::update_texture(TextureHandle tex_handle,
                                   const uint8_t* rgba_data,
                                   int32_t pitch) {
    auto it = textures_.find(tex_handle);
    if (it == textures_.end() || !it->second) {
        return;
    }

    SDL_Texture* tex = it->second;
    float width, height;
    SDL_GetTextureSize(tex, &width, &height);

    SDL_Rect rect{0, 0, static_cast<int>(width), static_cast<int>(height)};
    SDL_UpdateTexture(tex, &rect, rgba_data, pitch);
}

void Sdl3Platform::draw_texture(TextureHandle tex_handle,
                                 int32_t dst_x, int32_t dst_y) {
    auto it = textures_.find(tex_handle);
    if (it == textures_.end() || !it->second) {
        return;
    }

    SDL_Texture* tex = it->second;
    float width, height;
    SDL_GetTextureSize(tex, &width, &height);

    SDL_FRect dst{static_cast<float>(dst_x), static_cast<float>(dst_y),
                  static_cast<float>(width), static_cast<float>(height)};
    SDL_RenderTexture(renderer_, tex, nullptr, &dst);
}

void Sdl3Platform::draw_texture_rect(TextureHandle tex_handle,
                                      int32_t dst_x, int32_t dst_y,
                                      int32_t dst_w, int32_t dst_h) {
    auto it = textures_.find(tex_handle);
    if (it == textures_.end() || !it->second) {
        return;
    }

    SDL_Texture* tex = it->second;
    SDL_FRect dst{static_cast<float>(dst_x), static_cast<float>(dst_y),
                  static_cast<float>(dst_w), static_cast<float>(dst_h)};
    SDL_RenderTexture(renderer_, tex, nullptr, &dst);
}

void Sdl3Platform::draw_texture_sub(TextureHandle tex_handle,
                                     int32_t src_x, int32_t src_y,
                                     int32_t src_w, int32_t src_h,
                                     int32_t dst_x, int32_t dst_y) {
    auto it = textures_.find(tex_handle);
    if (it == textures_.end() || !it->second) {
        return;
    }

    SDL_Texture* tex = it->second;
    SDL_FRect src{static_cast<float>(src_x), static_cast<float>(src_y),
                  static_cast<float>(src_w), static_cast<float>(src_h)};
    SDL_FRect dst{static_cast<float>(dst_x), static_cast<float>(dst_y),
                  static_cast<float>(src_w), static_cast<float>(src_h)};
    SDL_RenderTexture(renderer_, tex, &src, &dst);
}

void Sdl3Platform::draw_rect(int32_t x, int32_t y, int32_t w, int32_t h,
                              uint8_t r, uint8_t g, uint8_t b) {
    SDL_SetRenderDrawColor(renderer_, r, g, b, 255);
    SDL_FRect rect{static_cast<float>(x), static_cast<float>(y),
                   static_cast<float>(w), static_cast<float>(h)};
    SDL_RenderRect(renderer_, &rect);
}

void Sdl3Platform::fill_rect(int32_t x, int32_t y, int32_t w, int32_t h,
                              uint8_t r, uint8_t g, uint8_t b) {
    SDL_SetRenderDrawColor(renderer_, r, g, b, 255);
    SDL_FRect rect{static_cast<float>(x), static_cast<float>(y),
                   static_cast<float>(w), static_cast<float>(h)};
    SDL_RenderFillRect(renderer_, &rect);
}

void Sdl3Platform::draw_line(int32_t x1, int32_t y1, int32_t x2, int32_t y2,
                              uint8_t r, uint8_t g, uint8_t b) {
    SDL_SetRenderDrawColor(renderer_, r, g, b, 255);
    SDL_RenderLine(renderer_,
                   static_cast<float>(x1), static_cast<float>(y1),
                   static_cast<float>(x2), static_cast<float>(y2));
}

void Sdl3Platform::destroy_texture(TextureHandle tex_handle) {
    auto it = textures_.find(tex_handle);
    if (it != textures_.end()) {
        if (it->second) {
            // 延后释放：推入当前代队列，待 2 帧后的 present 真正销毁。
            deferred_destroy_[destroy_gen_].push_back(it->second);
        }
        textures_.erase(it);
    }
}

void Sdl3Platform::flush_deferred_destroy() {
    // 释放 (destroy_gen_ + 1) % 3 这一代：即 2 帧前请求销毁的纹理。
    // 此刻对应帧的渲染批次已随此前 present 提交并被 GPU 退役，释放安全。
    const int free_idx = (destroy_gen_ + 1) % 3;
    for (SDL_Texture* t : deferred_destroy_[free_idx]) {
        if (t) SDL_DestroyTexture(t);
    }
    deferred_destroy_[free_idx].clear();
    destroy_gen_ = (destroy_gen_ + 1) % 3;
}

Platform::AudioHandle Sdl3Platform::play_sound(const uint8_t* pcm_data,
                                                size_t size,
                                                int32_t sample_rate) {
    if (!audio_device_ || !pcm_data || size == 0) {
        return 0;
    }

    SDL_AudioSpec spec{SDL_AUDIO_S16LE, 1, sample_rate};
    SDL_AudioStream* stream = SDL_CreateAudioStream(&spec, &spec);
    if (!stream) {
        std::fprintf(stderr, "SDL_CreateAudioStream failed: %s\n", SDL_GetError());
        return 0;
    }

    if (!SDL_BindAudioStream(audio_device_, stream)) {
        std::fprintf(stderr, "SDL_BindAudioStream failed: %s\n", SDL_GetError());
        SDL_DestroyAudioStream(stream);
        return 0;
    }

    if (!SDL_PutAudioStreamData(stream, pcm_data, static_cast<int>(size))) {
        std::fprintf(stderr, "SDL_PutAudioStreamData failed: %s\n", SDL_GetError());
        SDL_DestroyAudioStream(stream);
        return 0;
    }

    if (!SDL_ResumeAudioStreamDevice(stream)) {
        std::fprintf(stderr, "SDL_ResumeAudioStreamDevice failed: %s\n", SDL_GetError());
        SDL_DestroyAudioStream(stream);
        return 0;
    }

    AudioHandle handle = next_sound_++;
    sounds_[handle] = SoundInstance{stream, true};
    return handle;
}

void Sdl3Platform::stop_sound(AudioHandle handle) {
    auto it = sounds_.find(handle);
    if (it == sounds_.end()) {
        return;
    }

    if (it->second.stream) {
        SDL_PauseAudioStreamDevice(it->second.stream);
        SDL_DestroyAudioStream(it->second.stream);
    }
    sounds_.erase(it);
}

bool Sdl3Platform::is_sound_playing(AudioHandle handle) {
    auto it = sounds_.find(handle);
    if (it == sounds_.end() || !it->second.stream) {
        return false;
    }

    int available = SDL_GetAudioStreamAvailable(it->second.stream);
    if (available == 0) {
        it->second.playing = false;
    }
    return it->second.playing;
}

bool Sdl3Platform::play_music(const std::string& path, bool loop) {
#ifdef DLZR_HAS_SDL3_MIXER
    if (current_music_) {
        Mix_HaltMusic();
        Mix_FreeMusic(current_music_);
        current_music_ = nullptr;
    }

    current_music_ = Mix_LoadMUS(path.c_str());
    if (!current_music_) {
        std::fprintf(stderr, "Mix_LoadMUS failed: %s (path: %s)\n", Mix_GetError(), path.c_str());
        return false;
    }

    int loops = loop ? -1 : 0;
    if (Mix_PlayMusic(current_music_, loops) < 0) {
        std::fprintf(stderr, "Mix_PlayMusic failed: %s\n", Mix_GetError());
        Mix_FreeMusic(current_music_);
        current_music_ = nullptr;
        return false;
    }

    return true;
#else
    (void)path; (void)loop;
    return false;
#endif
}

void Sdl3Platform::stop_music() {
#ifdef DLZR_HAS_SDL3_MIXER
    if (current_music_) {
        Mix_HaltMusic();
        Mix_FreeMusic(current_music_);
        current_music_ = nullptr;
    }
#endif
}

void Sdl3Platform::set_music_volume(int32_t volume) {
#ifdef DLZR_HAS_SDL3_MIXER
    // SDL_mixer volume: 0-128
    Mix_VolumeMusic(volume);
#else
    (void)volume;
#endif
}

void Sdl3Platform::delay_ms(uint32_t ms) {
    SDL_Delay(ms);
}

uint32_t Sdl3Platform::ticks_ms() {
    return SDL_GetTicks();
}

void Sdl3Platform::start_text_input() {
    SDL_StartTextInput(window_);
}

void Sdl3Platform::stop_text_input() {
    SDL_StopTextInput(window_);
}

std::unique_ptr<Platform> create_sdl3_platform() {
    return std::make_unique<Sdl3Platform>();
}

}  // namespace dlzr::platform
