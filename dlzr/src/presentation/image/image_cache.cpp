#include "presentation/image/image_cache.hpp"
#include "app/resource/tga_loader.hpp"

#include <cstdio>
#include <filesystem>

namespace dlzr::presentation {

void ImageCache::init(platform::Platform& platform,
                       const std::string& game_data_dir) {
    platform_ = &platform;
    game_dir_ = game_data_dir;
}

void ImageCache::set_mod_dir(const std::string& mod_dir) {
    mod_dir_ = mod_dir;
    cache_.clear();
}

ImageCache::CachedImage& ImageCache::load_or_get(const std::string& name) {
    auto it = cache_.find(name);
    if (it != cache_.end()) {
        return it->second;
    }

    CachedImage img;
    try {
        namespace fs = std::filesystem;
        fs::path path;

        if (!mod_dir_.empty()) {
            fs::path mod_path = fs::path(mod_dir_) / name;
            if (fs::exists(mod_path)) {
                path = mod_path;
            }
        }
        if (path.empty()) {
            path = fs::path(game_dir_) / name;
        }

        core::DecodedImage decoded;
        std::string ext = path.extension().string();
        if (ext == ".TGA" || ext == ".tga") {
            decoded = app::TgaLoader::load(path);
        } else {
            decoded = app::ImgLoader::load(path);
        }
        auto rgba = app::ImgLoader::to_rgba(decoded);

        img.width = decoded.width;
        img.height = decoded.height;
        img.texture = platform_->create_texture(decoded.width, decoded.height);
        if (img.texture) {
            platform_->update_texture(img.texture, rgba.data(),
                                      decoded.width * 4);
        }
    } catch (const std::exception& e) {
        std::fprintf(stderr, "ImageCache: failed to load %s: %s\n",
                     name.c_str(), e.what());
    }

    cache_[name] = img;
    return cache_[name];
}

void ImageCache::draw_image(const std::string& name,
                              int32_t x, int32_t y) {
    if (!platform_) return;
    auto& img = load_or_get(name);
    if (img.texture) {
        platform_->draw_texture(img.texture, x, y);
    }
}

void ImageCache::draw_image(const std::string& name,
                              int32_t x, int32_t y,
                              int32_t w, int32_t h) {
    if (!platform_) return;
    auto& img = load_or_get(name);
    if (img.texture) {
        platform_->draw_texture_rect(img.texture, x, y, w, h);
    }
}

int32_t ImageCache::image_width(const std::string& name) {
    return load_or_get(name).width;
}

int32_t ImageCache::image_height(const std::string& name) {
    return load_or_get(name).height;
}

}  // namespace dlzr::presentation
