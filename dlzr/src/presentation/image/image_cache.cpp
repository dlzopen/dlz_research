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

void ImageCache::draw_image(const std::string& name,
                              int32_t dst_x, int32_t dst_y,
                              int32_t src_x, int32_t src_y,
                              int32_t src_w, int32_t src_h) {
    if (!platform_) return;
    auto& img = load_or_get(name);
    if (img.texture) {
        platform_->draw_texture_sub(img.texture, src_x, src_y, src_w, src_h,
                                    dst_x, dst_y);
    }
}

void ImageCache::draw_image_with_transparent_color(const std::string& name,
                                                   int32_t x, int32_t y,
                                                   uint8_t transparent_color_index) {
    if (!platform_) return;

    // Cache transparent version with special key
    std::string cache_key = name + "#transparent";
    auto it = cache_.find(cache_key);
    if (it == cache_.end()) {
        // Load and create transparent version
        auto& base_img = load_or_get(name);
        if (!base_img.texture) return;

        // Load the image again to get pixel data
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

        // Set alpha=0 for pixels matching transparent color index
        size_t pixel_count = static_cast<size_t>(decoded.width) * decoded.height;
        for (size_t i = 0; i < pixel_count; ++i) {
            if (decoded.pixels[i] == transparent_color_index) {
                rgba[i * 4 + 3] = 0;  // Set alpha to 0
            }
        }

        // Create texture
        CachedImage transparent_img;
        transparent_img.width = decoded.width;
        transparent_img.height = decoded.height;
        transparent_img.texture = platform_->create_texture(decoded.width, decoded.height);
        if (transparent_img.texture) {
            platform_->update_texture(transparent_img.texture, rgba.data(),
                                      decoded.width * 4);
        }
        cache_[cache_key] = transparent_img;
        it = cache_.find(cache_key);
    }

    if (it != cache_.end() && it->second.texture) {
        platform_->draw_texture(it->second.texture, x, y);
    }
}

int32_t ImageCache::image_width(const std::string& name) {
    return load_or_get(name).width;
}

int32_t ImageCache::image_height(const std::string& name) {
    return load_or_get(name).height;
}

}  // namespace dlzr::presentation
