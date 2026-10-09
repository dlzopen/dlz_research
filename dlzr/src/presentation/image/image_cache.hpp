#pragma once

#include "app/resource/img_loader.hpp"
#include "platform/platform.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>

namespace dlzr::presentation {

class ImageCache {
public:
    void init(platform::Platform& platform, const std::string& game_data_dir);

    void draw_image(const std::string& name, int32_t x, int32_t y);
    void draw_image(const std::string& name, int32_t x, int32_t y,
                    int32_t w, int32_t h);

    int32_t image_width(const std::string& name);
    int32_t image_height(const std::string& name);

private:
    struct CachedImage {
        platform::TextureHandle texture{0};
        int32_t width{0};
        int32_t height{0};
    };

    platform::Platform* platform_{nullptr};
    std::string game_dir_;
    std::unordered_map<std::string, CachedImage> cache_;

    CachedImage& load_or_get(const std::string& name);
};

}  // namespace dlzr::presentation
