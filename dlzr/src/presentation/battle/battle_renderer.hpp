#pragma once

// battle_renderer.hpp — 战斗场景渲染器
// Battle scene renderer with real assets

#include "logic/battle/battle_state.hpp"
#include "core/data_model/game_data.hpp"
#include "platform/platform.hpp"
#include "presentation/font/font_renderer.hpp"
#include "presentation/image/image_cache.hpp"

namespace dlzr::presentation {

class BattleRenderer {
public:
    void init(platform::Platform& platform, FontRenderer& font, ImageCache& images);

    void render(const logic::BattleState& state,
                const core::FightData& fight_data,
                const core::GameData& game_data);

    void set_grid_origin(int32_t x, int32_t y) { grid_origin_x_ = x; grid_origin_y_ = y; }
    void set_cell_size(int32_t size) { cell_size_ = size; }

    void render_command_menu(int32_t menu_x, int32_t menu_y, int32_t hover_item);
    void render_selected_highlight(int32_t grid_x, int32_t grid_y);
    void render_status_text(const char* text);

private:
    platform::Platform* platform_{nullptr};
    FontRenderer* font_{nullptr};
    ImageCache* images_{nullptr};

    const core::FightData* fight_data_{nullptr};
    const core::GameData* game_data_{nullptr};

    int32_t grid_origin_x_{40};
    int32_t grid_origin_y_{40};
    int32_t cell_size_{40};

    void draw_grid(const logic::BattleState& state);
    void draw_mines(const logic::BattleState& state);
    void draw_fighters(const logic::BattleState& state);
    void draw_hp_bar(int32_t screen_x, int32_t screen_y, int32_t current_hp, int32_t max_hp);
    void draw_ui(const logic::BattleState& state);

    const char* get_fighter_name(const logic::RuntimeFighter& fighter) const;
    const char* get_class_name(const logic::RuntimeFighter& fighter) const;
};

}  // namespace dlzr::presentation
