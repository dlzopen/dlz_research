// battle_renderer.cpp — 战斗场景渲染器实现

#include "presentation/battle/battle_renderer.hpp"

namespace dlzr::presentation {

void BattleRenderer::init(platform::Platform& platform, FontRenderer& font,
                           ImageCache& images) {
    platform_ = &platform;
    font_ = &font;
    images_ = &images;
}

void BattleRenderer::render(const logic::BattleState& state,
                             const core::FightData& fight_data,
                             const core::GameData& game_data) {
    if (!platform_) return;

    fight_data_ = &fight_data;
    game_data_ = &game_data;

    images_->draw_image("BG01.IMG", 0, 0);

    draw_grid(state);
    draw_mines(state);
    draw_fighters(state);
    draw_ui(state);
}

void BattleRenderer::draw_grid(const logic::BattleState& state) {
    for (int row = 0; row < logic::BattleState::GRID_ROWS; ++row) {
        for (int col = 0; col < logic::BattleState::GRID_COLS; ++col) {
            int32_t idx = row * logic::BattleState::GRID_COLS + col;
            if (idx >= 92) continue;

            int32_t terrain = state.grid_terrain[idx];
            int32_t x = grid_origin_x_ + col * cell_size_;
            int32_t y = grid_origin_y_ + row * cell_size_;

            uint8_t r = 64, g = 128, b = 64;
            if (terrain > 0) {
                r = 128; g = 96; b = 64;
            }

            platform_->fill_rect(x, y, cell_size_, cell_size_, r, g, b);
            platform_->draw_rect(x, y, cell_size_, cell_size_, 32, 32, 32);
        }
    }
}

void BattleRenderer::draw_mines(const logic::BattleState& state) {
    if (!game_data_) return;

    for (int row = 0; row < logic::BattleState::GRID_ROWS; ++row) {
        for (int col = 0; col < logic::BattleState::GRID_COLS; ++col) {
            int32_t idx = row * logic::BattleState::GRID_COLS + col;
            if (idx >= 92) continue;

            const auto& mine = state.grid_mines[idx];
            if (mine.mine_type == 0) continue;
            if (mine.owner_team != 0) continue;

            int32_t x = grid_origin_x_ + col * cell_size_;
            int32_t y = grid_origin_y_ + row * cell_size_;

            int32_t cx = x + cell_size_ / 2;
            int32_t cy = y + cell_size_ / 2;
            int32_t radius = cell_size_ / 4;

            platform_->fill_rect(cx - radius, cy - radius,
                                 radius * 2, radius * 2,
                                 200, 60, 60);
            platform_->draw_rect(cx - radius, cy - radius,
                                 radius * 2, radius * 2,
                                 255, 100, 100);

            if (mine.mine_type > 0 && mine.mine_type < core::MAX_DILEI) {
                const char* name = game_data_->mines[mine.mine_type].name.data();
                if (name[0]) {
                    font_->draw_text(x + 2, y + cell_size_ - 14, name, 255, 200, 200);
                }
            }
        }
    }
}

void BattleRenderer::draw_fighters(const logic::BattleState& state) {
    for (int i = 0; i < logic::BattleState::MAX_FIGHTERS; ++i) {
        const auto& fighter = state.fighters[i];
        if (!fighter.is_active || fighter.stats.current_hp <= 0) continue;

        int32_t x = grid_origin_x_ + fighter.grid_x * cell_size_;
        int32_t y = grid_origin_y_ + fighter.grid_y * cell_size_;

        int32_t fighter_size = cell_size_ - 8;
        int32_t fx = x + 4;
        int32_t fy = y + 4;

        uint8_t r = (fighter.team == 0) ? 64 : 192;
        uint8_t g = (fighter.team == 0) ? 128 : 64;
        uint8_t b = (fighter.team == 0) ? 192 : 64;

        platform_->fill_rect(fx, fy, fighter_size, fighter_size, r, g, b);

        if (i == state.current_fighter_idx) {
            platform_->draw_rect(fx - 2, fy - 2, fighter_size + 4, fighter_size + 4, 255, 255, 0);
        }

        draw_hp_bar(fx, fy - 6, fighter.stats.current_hp, fighter.stats.max_hp);

        // Draw fighter name label
        const char* name = get_fighter_name(fighter);
        if (name && name[0]) {
            font_->draw_text(fx, fy + fighter_size + 2, name, 255, 255, 255);
        }
    }
}

void BattleRenderer::draw_hp_bar(int32_t screen_x, int32_t screen_y,
                                  int32_t current_hp, int32_t max_hp) {
    if (max_hp <= 0) return;

    int32_t bar_width = cell_size_ - 8;
    int32_t bar_height = 4;
    int32_t hp_width = (current_hp * bar_width) / max_hp;
    if (hp_width < 0) hp_width = 0;
    if (hp_width > bar_width) hp_width = bar_width;

    platform_->fill_rect(screen_x, screen_y, bar_width, bar_height, 64, 64, 64);

    uint8_t r = (current_hp > max_hp / 2) ? 64 : 192;
    uint8_t g = (current_hp > max_hp / 2) ? 192 : 64;
    platform_->fill_rect(screen_x, screen_y, hp_width, bar_height, r, g, 0);
}

void BattleRenderer::draw_ui(const logic::BattleState& state) {
    platform_->fill_rect(0, 380, 640, 100, 20, 20, 40);
    platform_->draw_rect(0, 380, 640, 100, 100, 100, 160);

    int32_t cur = state.current_fighter_idx;
    if (cur >= 0 && cur < logic::BattleState::MAX_FIGHTERS) {
        const auto& f = state.fighters[cur];
        if (f.is_active) {
            // Fighter name
            const char* name = get_fighter_name(f);
            if (name && name[0]) {
                font_->draw_text(10, 390, name, 255, 255, 200);
            }

            // Class name
            const char* class_name = get_class_name(f);
            if (class_name && class_name[0]) {
                font_->draw_text(10, 410, class_name, 200, 200, 255);
            }

            // HP display
            char buf[64];
            std::snprintf(buf, sizeof(buf), "HP: %d/%d", f.stats.current_hp, f.stats.max_hp);
            font_->draw_text(200, 390, buf, 255, 255, 255);

            // Team label
            const char* team_label = (f.team == 0) ? "\xCE\xD2\xB7\xBD" : "\xB5\xD0\xB7\xBD";
            font_->draw_text(200, 410, team_label, 200, 200, 255);
        }
    }

    // Command hint
    font_->draw_text(400, 390, "\xD1\xA1\xD6\xD0\xC4\xBF\xEA: \xB9\xA5\xBB\xF7", 200, 255, 200);

    // Round counter
    char round_buf[32];
    std::snprintf(round_buf, sizeof(round_buf), "\xD6\xD8\xCA\xFD: %d", state.battle_round);
    font_->draw_text(400, 410, round_buf, 180, 180, 180);
}

const char* BattleRenderer::get_fighter_name(const logic::RuntimeFighter& fighter) const {
    if (!fight_data_) return "";
    int32_t pidx = fighter.people_idx;
    if (pidx >= 0 && pidx < 88) {
        return fight_data_->people[pidx].name.data();
    }
    return "";
}

const char* BattleRenderer::get_class_name(const logic::RuntimeFighter& fighter) const {
    if (!fight_data_ || !game_data_) return "";
    int32_t pidx = fighter.people_idx;
    if (pidx >= 0 && pidx < 88) {
        int32_t class_idx = fight_data_->people[pidx].class_type;
        if (class_idx >= 0 && class_idx < core::MAX_CLASS_TYPES) {
            return game_data_->class_types[class_idx].name.data();
        }
    }
    return "";
}

void BattleRenderer::render_command_menu(int32_t menu_x, int32_t menu_y, int32_t hover_item) {
    static constexpr int32_t ITEM_H = 24;
    static constexpr int32_t W = 80;
    static constexpr int32_t COUNT = 6;

    int32_t total_h = COUNT * ITEM_H;
    platform_->fill_rect(menu_x - 2, menu_y - 2, W + 4, total_h + 4, 20, 20, 40);
    platform_->draw_rect(menu_x - 2, menu_y - 2, W + 4, total_h + 4, 180, 180, 220);

    static const char* labels[6] = {
        "\xB9\xA5\xBB\xF7",   // 攻击
        "\xBC\xBC\xC4\xDC",   // 技能
        "\xCE\xEF\xC6\xB7",   // 物品
        "\xB5\xC8\xB4\xFD",   // 等待
        "\xD2\xC6\xB6\xAF",   // 移动
        "\xB5\xD8\xC0\xD7",   // 地雷
    };

    for (int32_t i = 0; i < COUNT; ++i) {
        int32_t y = menu_y + i * ITEM_H;
        if (i == hover_item) {
            platform_->fill_rect(menu_x, y, W, ITEM_H, 80, 80, 140);
        }
        font_->draw_text(menu_x + 8, y + 4, labels[i], 255, 255, 255);
    }
}

void BattleRenderer::render_selected_highlight(int32_t grid_x, int32_t grid_y) {
    int32_t x = grid_origin_x_ + grid_x * cell_size_;
    int32_t y = grid_origin_y_ + grid_y * cell_size_;
    platform_->draw_rect(x - 1, y - 1, cell_size_ + 2, cell_size_ + 2, 255, 255, 0);
    platform_->draw_rect(x - 3, y - 3, cell_size_ + 6, cell_size_ + 6, 200, 200, 0);
}

void BattleRenderer::render_status_text(const char* text) {
    platform_->fill_rect(0, 380, 640, 20, 20, 20, 50);
    font_->draw_text(10, 384, text, 255, 255, 200);
}

}  // namespace dlzr::presentation
