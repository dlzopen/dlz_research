#pragma once

#include "platform/platform.hpp"
#include "presentation/font/font_renderer.hpp"
#include "presentation/image/image_cache.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace dlzr::presentation {

class EndingRenderer {
public:
    void init(platform::Platform& platform, FontRenderer& font,
              ImageCache& images);

    void render(int32_t ending_id, const std::string& ending_text);

private:
    platform::Platform* platform_{nullptr};
    FontRenderer* font_{nullptr};
    ImageCache* images_{nullptr};
};

}  // namespace dlzr::presentation
