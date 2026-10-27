// battle_animation_controller.cpp — 战斗动画序列控制器实现
// Battle animation sequence controller implementation

#include "presentation/battle/battle_animation_controller.hpp"
#include "platform/platform.hpp"
#include "presentation/font/font_renderer.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace dlzr::presentation {

void BattleAnimationController::init(int32_t grid_origin_x, int32_t grid_origin_y,
                                      int32_t cell_size) {
    grid_origin_x_ = grid_origin_x;
    grid_origin_y_ = grid_origin_y;
    cell_size_ = cell_size;
    for (auto& e : entries_) {
        e = BattleAnimEntry{};
    }
}

BattleAnimEntry* BattleAnimationController::find_free_entry() {
    for (auto& e : entries_) {
        if (!e.active) {
            e = BattleAnimEntry{};
            e.active = true;
            return &e;
        }
    }
    return nullptr;
}

float BattleAnimationController::grid_to_screen_x(int32_t grid_x) const {
    return static_cast<float>(grid_origin_x_ + grid_x * cell_size_);
}

float BattleAnimationController::grid_to_screen_y(int32_t grid_y) const {
    return static_cast<float>(grid_origin_y_ + grid_y * cell_size_);
}

float BattleAnimationController::ease_out_quad(float t) {
    return t * (2.0f - t);
}

float BattleAnimationController::ease_in_out_sine(float t) {
    return -0.5f * (std::cos(3.14159265f * t) - 1.0f);
}

void BattleAnimationController::queue_move(int32_t entity_idx,
                                            int32_t from_x, int32_t from_y,
                                            int32_t to_x, int32_t to_y) {
    auto* e = find_free_entry();
    if (!e) return;
    e->type = BattleAnimType::MOVE;
    e->entity_idx = entity_idx;
    e->start_x = grid_to_screen_x(from_x);
    e->start_y = grid_to_screen_y(from_y);
    e->end_x = grid_to_screen_x(to_x);
    e->end_y = grid_to_screen_y(to_y);
    e->current_x = e->start_x;
    e->current_y = e->start_y;
    e->duration_ms = MOVE_DURATION_MS;
    e->elapsed_ms = 0;
}

void BattleAnimationController::queue_attack(int32_t attacker_idx,
                                              int32_t attacker_x, int32_t attacker_y,
                                              int32_t target_idx,
                                              int32_t target_x, int32_t target_y) {
    auto* e = find_free_entry();
    if (!e) return;
    e->type = BattleAnimType::ATTACK;
    e->entity_idx = attacker_idx;
    e->target_idx = target_idx;
    float ax = grid_to_screen_x(attacker_x);
    float ay = grid_to_screen_y(attacker_y);
    float tx = grid_to_screen_x(target_x);
    float ty = grid_to_screen_y(target_y);
    e->start_x = ax;
    e->start_y = ay;
    float dx = tx - ax;
    float dy = ty - ay;
    float dist = std::sqrt(dx * dx + dy * dy);
    float lunge = std::min(dist * 0.4f, static_cast<float>(cell_size_) * 0.6f);
    float nx = (dist > 0.0f) ? dx / dist : 0.0f;
    float ny = (dist > 0.0f) ? dy / dist : 0.0f;
    e->end_x = ax + nx * lunge;
    e->end_y = ay + ny * lunge;
    e->current_x = ax;
    e->current_y = ay;
    e->duration_ms = ATTACK_DURATION_MS;
    e->elapsed_ms = 0;
}

void BattleAnimationController::queue_impact(int32_t target_x, int32_t target_y) {
    auto* e = find_free_entry();
    if (!e) return;
    e->type = BattleAnimType::IMPACT;
    e->start_x = grid_to_screen_x(target_x) + cell_size_ * 0.5f;
    e->start_y = grid_to_screen_y(target_y) + cell_size_ * 0.5f;
    e->duration_ms = IMPACT_DURATION_MS;
    e->elapsed_ms = 0;
    e->color_r = 255;
    e->color_g = 255;
    e->color_b = 200;
}

void BattleAnimationController::queue_damage_number(int32_t target_x, int32_t target_y,
                                                     int32_t damage, bool is_heal) {
    auto* e = find_free_entry();
    if (!e) return;
    e->type = is_heal ? BattleAnimType::HEAL : BattleAnimType::DAMAGE_NUMBER;
    e->start_x = grid_to_screen_x(target_x) + cell_size_ * 0.5f;
    e->start_y = grid_to_screen_y(target_y);
    e->end_y = e->start_y - cell_size_ * 0.8f;
    e->current_x = e->start_x;
    e->current_y = e->start_y;
    e->value = damage;
    e->duration_ms = DAMAGE_NUMBER_DURATION_MS;
    e->elapsed_ms = 0;
    if (is_heal) {
        e->color_r = 0;
        e->color_g = 255;
        e->color_b = 0;
    } else {
        e->color_r = 255;
        e->color_g = 80;
        e->color_b = 0;
    }
}

void BattleAnimationController::queue_death(int32_t entity_idx,
                                             int32_t grid_x, int32_t grid_y) {
    auto* e = find_free_entry();
    if (!e) return;
    e->type = BattleAnimType::DEATH;
    e->entity_idx = entity_idx;
    e->start_x = grid_to_screen_x(grid_x);
    e->start_y = grid_to_screen_y(grid_y);
    e->duration_ms = DEATH_DURATION_MS;
    e->elapsed_ms = 0;
}

void BattleAnimationController::queue_skill_cast(int32_t caster_idx,
                                                  int32_t grid_x, int32_t grid_y) {
    auto* e = find_free_entry();
    if (!e) return;
    e->type = BattleAnimType::SKILL_CAST;
    e->entity_idx = caster_idx;
    e->start_x = grid_to_screen_x(grid_x);
    e->start_y = grid_to_screen_y(grid_y);
    e->duration_ms = SKILL_CAST_DURATION_MS;
    e->elapsed_ms = 0;
    e->color_r = 200;
    e->color_g = 150;
    e->color_b = 255;
}

void BattleAnimationController::queue_skill_cast(int32_t caster_idx,
                                                  int32_t grid_x, int32_t grid_y,
                                                  int32_t skill_type) {
    queue_skill_cast(caster_idx, grid_x, grid_y);
    for (auto& e : entries_) {
        if (!e.active || e.type != BattleAnimType::SKILL_CAST) continue;
        if (e.entity_idx != caster_idx) continue;
        switch (skill_type) {
        case 0:  e.color_r = 220; e.color_g = 200; e.color_b = 60;  break;
        case 1:  e.color_r = 60;  e.color_g = 200; e.color_b = 220; break;
        case 2:  e.color_r = 60;  e.color_g = 220; e.color_b = 60;  break;
        case 3:  e.color_r = 255; e.color_g = 120; e.color_b = 20;  break;
        case 4:  e.color_r = 160; e.color_g = 120; e.color_b = 60;  break;
        case 5:  e.color_r = 160; e.color_g = 160; e.color_b = 160; break;
        case 6:  e.color_r = 160; e.color_g = 60;  e.color_b = 200; break;
        case 7:  e.color_r = 180; e.color_g = 180; e.color_b = 40;  break;
        case 8:  e.color_r = 220; e.color_g = 40;  e.color_b = 40;  break;
        case 9:  e.color_r = 200; e.color_g = 140; e.color_b = 40;  break;
        case 10: e.color_r = 60;  e.color_g = 100; e.color_b = 220; break;
        case 11: e.color_r = 100; e.color_g = 180; e.color_b = 220; break;
        case 12: e.color_r = 220; e.color_g = 220; e.color_b = 220; break;
        case 13: e.color_r = 120; e.color_g = 20;  e.color_b = 140; break;
        case 14: e.color_r = 40;  e.color_g = 160; e.color_b = 40;  break;
        case 15: e.color_r = 255; e.color_g = 60;  e.color_b = 60;  break;
        default: break;
        }
        break;
    }
}

void BattleAnimationController::update(int32_t delta_ms) {
    for (auto& e : entries_) {
        if (!e.active) continue;

        e.elapsed_ms += delta_ms;
        float t = static_cast<float>(e.elapsed_ms) /
                  static_cast<float>(e.duration_ms);
        if (t > 1.0f) t = 1.0f;

        switch (e.type) {
        case BattleAnimType::MOVE: {
            float et = ease_in_out_sine(t);
            e.current_x = e.start_x + (e.end_x - e.start_x) * et;
            e.current_y = e.start_y + (e.end_y - e.start_y) * et;
            break;
        }
        case BattleAnimType::ATTACK: {
            if (t < 0.4f) {
                float lt = ease_out_quad(t / 0.4f);
                e.current_x = e.start_x + (e.end_x - e.start_x) * lt;
                e.current_y = e.start_y + (e.end_y - e.start_y) * lt;
            } else {
                float rt = ease_out_quad((t - 0.4f) / 0.6f);
                e.current_x = e.end_x + (e.start_x - e.end_x) * rt;
                e.current_y = e.end_y + (e.start_y - e.end_y) * rt;
            }
            break;
        }
        case BattleAnimType::DAMAGE_NUMBER:
        case BattleAnimType::HEAL: {
            e.current_y = e.start_y + (e.end_y - e.start_y) * t;
            break;
        }
        default:
            break;
        }

        if (e.elapsed_ms >= e.duration_ms) {
            e.active = false;
        }
    }
}

bool BattleAnimationController::get_move_offset(int32_t entity_idx,
                                                 float& offset_x, float& offset_y) const {
    for (const auto& e : entries_) {
        if (!e.active || e.type != BattleAnimType::MOVE) continue;
        if (e.entity_idx != entity_idx) continue;
        float base_x = grid_to_screen_x(0);
        float base_y = grid_to_screen_y(0);
        offset_x = e.current_x - base_x;
        offset_y = e.current_y - base_y;
        return true;
    }
    return false;
}

int32_t BattleAnimationController::get_death_alpha(int32_t entity_idx) const {
    for (const auto& e : entries_) {
        if (!e.active || e.type != BattleAnimType::DEATH) continue;
        if (e.entity_idx != entity_idx) continue;
        float t = static_cast<float>(e.elapsed_ms) /
                  static_cast<float>(e.duration_ms);
        if (t > 1.0f) t = 1.0f;
        return static_cast<int32_t>(255.0f * (1.0f - t));
    }
    return 255;
}

void BattleAnimationController::render(platform::Platform& plat,
                                        FontRenderer& font) const {
    for (const auto& e : entries_) {
        if (!e.active) continue;

        float t = static_cast<float>(e.elapsed_ms) /
                  static_cast<float>(e.duration_ms);
        if (t > 1.0f) t = 1.0f;

        switch (e.type) {
        case BattleAnimType::DAMAGE_NUMBER:
        case BattleAnimType::HEAL: {
            int32_t alpha = static_cast<int32_t>(255.0f * (1.0f - t * t));
            if (alpha < 0) alpha = 0;
            uint8_t cr = static_cast<uint8_t>(e.color_r * alpha / 255);
            uint8_t cg = static_cast<uint8_t>(e.color_g * alpha / 255);
            uint8_t cb = static_cast<uint8_t>(e.color_b * alpha / 255);
            char buf[32];
            if (e.type == BattleAnimType::HEAL) {
                std::snprintf(buf, sizeof(buf), "+%d", e.value);
            } else {
                std::snprintf(buf, sizeof(buf), "-%d", e.value);
            }
            int32_t text_w = static_cast<int32_t>(std::strlen(buf)) *
                             FontRenderer::CHAR_WIDTH;
            int32_t draw_x = static_cast<int32_t>(e.current_x) - text_w / 2;
            int32_t draw_y = static_cast<int32_t>(e.current_y);
            font.draw_text(draw_x, draw_y, buf, cr, cg, cb);
            if (e.type == BattleAnimType::HEAL) {
                int32_t sx = static_cast<int32_t>(e.start_x);
                int32_t sy = static_cast<int32_t>(e.start_y);
                float pulse = (t < 0.5f) ? (t * 2.0f) : (2.0f - t * 2.0f);
                int32_t pad = static_cast<int32_t>(cell_size_ * 0.1f * pulse);
                int32_t ga = static_cast<int32_t>(120.0f * (1.0f - t));
                uint8_t gg = static_cast<uint8_t>(200 * ga / 255);
                plat.fill_rect(sx - pad, sy - pad,
                                   cell_size_ + pad * 2, cell_size_ + pad * 2,
                                   0, gg, 0);
            }
            break;
        }
        case BattleAnimType::IMPACT: {
            float expand = ease_out_quad(t);
            int32_t radius = static_cast<int32_t>(cell_size_ * 0.6f * expand);
            int32_t alpha = static_cast<int32_t>(255.0f * (1.0f - t));
            uint8_t cr = static_cast<uint8_t>(e.color_r * alpha / 255);
            uint8_t cg = static_cast<uint8_t>(e.color_g * alpha / 255);
            uint8_t cb = static_cast<uint8_t>(e.color_b * alpha / 255);
            int32_t cx = static_cast<int32_t>(e.start_x);
            int32_t cy = static_cast<int32_t>(e.start_y);
            plat.draw_rect(cx - radius, cy - radius,
                               radius * 2, radius * 2, cr, cg, cb);
            break;
        }
        case BattleAnimType::SKILL_CAST: {
            float pulse = (t < 0.5f) ? (t * 2.0f) : (2.0f - t * 2.0f);
            int32_t alpha = static_cast<int32_t>(180.0f * pulse);
            uint8_t cr = static_cast<uint8_t>(e.color_r * alpha / 255);
            uint8_t cg = static_cast<uint8_t>(e.color_g * alpha / 255);
            uint8_t cb = static_cast<uint8_t>(e.color_b * alpha / 255);
            int32_t sx = static_cast<int32_t>(e.start_x);
            int32_t sy = static_cast<int32_t>(e.start_y);
            int32_t pad = static_cast<int32_t>(cell_size_ * 0.15f * pulse);
            plat.draw_rect(sx - pad, sy - pad,
                               cell_size_ + pad * 2, cell_size_ + pad * 2,
                               cr, cg, cb);
            break;
        }
        default:
            break;
        }
    }
}

bool BattleAnimationController::has_active_animations() const {
    for (const auto& e : entries_) {
        if (e.active) return true;
    }
    return false;
}

bool BattleAnimationController::is_busy() const {
    return has_active_animations();
}

}  // namespace dlzr::presentation
