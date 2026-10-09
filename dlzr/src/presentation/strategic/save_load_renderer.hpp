#pragma once

// 存档界面渲染器 / Save/Load UI renderer

#include "app/resource/save_loader.hpp"
#include "platform/platform.hpp"
#include "presentation/font/font_renderer.hpp"

#include <cstdint>

namespace dlzr::presentation {

enum class SaveLoadMode {
    SAVE,
    LOAD
};

class SaveLoadRenderer {
public:
    void init(platform::Platform& platform, FontRenderer& font,
              const app::SaveLoader& save_loader);
    void render(SaveLoadMode mode, int32_t selected_slot);

    int32_t slot_at_position(int32_t screen_x, int32_t screen_y) const;

private:
    platform::Platform* platform_{nullptr};
    FontRenderer* font_{nullptr};
    const app::SaveLoader* save_loader_{nullptr};
};

}  // namespace dlzr::presentation
