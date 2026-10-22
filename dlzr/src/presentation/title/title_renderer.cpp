#include "presentation/title/title_renderer.hpp"
#include "app/resource/img_loader.hpp"

#include <cstdio>
#include <filesystem>

namespace dlzr::presentation {

void TitleRenderer::init(platform::Platform& platform, FontRenderer& font,
                          ImageCache& images, const std::string& game_data_dir) {
    platform_ = &platform;
    font_ = &font;
    images_ = &images;
    game_dir_ = game_data_dir;
}

void TitleRenderer::load_background() {
    if (bg_loaded_ || !platform_) return;

    try {
        auto path = std::filesystem::path(game_dir_) / "BG03.IMG";
        auto img = app::ImgLoader::load(path);
        auto rgba = app::ImgLoader::to_rgba(img);

        bg_texture_ = platform_->create_texture(img.width, img.height);
        if (bg_texture_) {
            platform_->update_texture(bg_texture_, rgba.data(), img.width * 4);
        }
        bg_loaded_ = true;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "TitleRenderer: failed to load BG03.IMG: %s\n",
                     e.what());
    }
}

void TitleRenderer::load_animation_frames() {
    if (anim_loaded_ || !platform_) return;

    try {
        auto path = std::filesystem::path(game_dir_) / "MINE02.IMG";
        auto full = app::ImgLoader::load(path);

        const int32_t frame_w = ANIM_FRAME_WIDTH;
        const int32_t frame_h = ANIM_FRAME_HEIGHT;
        const int32_t stride = frame_w * frame_h;

        auto rgba = app::ImgLoader::to_rgba(full);

        for (int f = 0; f < ANIM_FRAME_COUNT; ++f) {
            std::vector<uint8_t> frame_rgba(stride * 4);
            for (int y = 0; y < frame_h; ++y) {
                int src_row = f * frame_h + y;
                for (int x = 0; x < frame_w; ++x) {
                    int src_idx = (src_row * full.width + x) * 4;
                    int dst_idx = (y * frame_w + x) * 4;
                    frame_rgba[dst_idx + 0] = rgba[src_idx + 0];
                    frame_rgba[dst_idx + 1] = rgba[src_idx + 1];
                    frame_rgba[dst_idx + 2] = rgba[src_idx + 2];
                    uint8_t pal_idx = full.pixels[src_row * full.width + x];
                    frame_rgba[dst_idx + 3] = (pal_idx == 4) ? 0 : 255;
                }
            }

            anim_frames_[f] = platform_->create_texture(frame_w, frame_h);
            if (anim_frames_[f]) {
                platform_->update_texture(anim_frames_[f], frame_rgba.data(),
                                          frame_w * 4);
            }
        }
        anim_loaded_ = true;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "TitleRenderer: failed to load MINE02.IMG: %s\n",
                     e.what());
    }
}

void TitleRenderer::render(int32_t selected_option, int32_t difficulty_level) {
    if (!platform_) return;

    if (!bg_loaded_) {
        load_background();
    }
    if (!anim_loaded_) {
        load_animation_frames();
        last_anim_time_ = platform_->ticks_ms();
    }

    uint32_t now = platform_->ticks_ms();
    if (now - last_anim_time_ >= 80) {
        anim_frame_ = (anim_frame_ + 1) % ANIM_FRAME_COUNT;
        last_anim_time_ = now;
    }

    // Draw modified background (buttons erased)
    if (bg_texture_) {
        platform_->draw_texture(bg_texture_, 0, 0);
    }

    // Draw hovered button image only
    if (selected_option >= 0 && selected_option < OPTION_COUNT) {
        const char* button_images[5] = {
            "MENU50.IMG", "MENU51.IMG", "MENU52.IMG", "MENU53.IMG", "MENU54.IMG",
        };
        static constexpr int32_t MENU_X = 315;
        images_->draw_image(button_images[selected_option], MENU_X, MENU_Y[selected_option]);
    }

    // Draw rotating mine animation
    if (selected_option >= 0 && selected_option < OPTION_COUNT && anim_loaded_) {
        int32_t frame_idx = anim_frame_ % ANIM_FRAME_COUNT;
        if (anim_frames_[frame_idx]) {
            platform_->draw_texture(anim_frames_[frame_idx], ANIM_X,
                                    MENU_Y[selected_option]);
        }
    }
}

}  // namespace dlzr::presentation
