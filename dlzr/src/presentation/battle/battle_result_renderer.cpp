// battle_result_renderer.cpp — 战斗结果画面渲染器实现

#include "presentation/battle/battle_result_renderer.hpp"

#include <cstdio>
#include <string>

namespace dlzr::presentation {

void BattleResultRenderer::init(platform::Platform& platform, FontRenderer& font) {
    platform_ = &platform;
    font_ = &font;
}

void BattleResultRenderer::render(bool is_victory, int32_t score,
                                   const logic::BattleEngine::ScoreBreakdown& breakdown) {
    if (!platform_) return;

    platform_->fill_rect(0, 0, 640, 480, 0, 0, 0);

    platform_->fill_rect(80, 60, 480, 360, 10, 10, 30);
    platform_->draw_rect(80, 60, 480, 360, 180, 160, 100);
    platform_->draw_rect(82, 62, 476, 356, 120, 100, 60);

    if (is_victory) {
        std::string title = "\xCA\xA4\xC0\xFB";  // 胜利
        int32_t title_chars = 2;
        int32_t title_w = title_chars * FontRenderer::CHAR_WIDTH * 2;
        int32_t title_x = (640 - title_w) / 2;
        font_->draw_text(title_x, 80, title, 255, 255, 80);
    } else {
        std::string title = "\xCA\xA7\xB0\xDC";  // 失败
        int32_t title_chars = 2;
        int32_t title_w = title_chars * FontRenderer::CHAR_WIDTH * 2;
        int32_t title_x = (640 - title_w) / 2;
        font_->draw_text(title_x, 80, title, 255, 80, 80);
    }

    int32_t y = 130;
    int32_t label_x = 140;
    int32_t value_x = 360;
    int32_t line_h = 24;

    auto draw_row = [&](const char* label, const char* value,
                        uint8_t lr, uint8_t lg, uint8_t lb,
                        uint8_t vr, uint8_t vg, uint8_t vb) {
        font_->draw_text(label_x, y, label, lr, lg, lb);
        font_->draw_text(value_x, y, value, vr, vg, vb);
        y += line_h;
    };

    char buf[32];

    // 歼敌数 / Enemy kills
    std::snprintf(buf, sizeof(buf), "%d", breakdown.enemy_kills);
    draw_row("\xBD\xC3\xB5\xD0\xCA\xFD\xA3\xBA", buf,  // 歼敌数：
             200, 200, 200, 255, 255, 100);

    // 我方损失 / Player losses
    std::snprintf(buf, sizeof(buf), "%d", breakdown.player_losses);
    draw_row("\xCE\xD2\xB7\xBD\xCB\xD5\xCA\xA7\xA3\xBA", buf,  // 我方损失：
             200, 200, 200, 255, 100, 100);

    // 战斗回合 / Rounds
    std::snprintf(buf, sizeof(buf), "%d", breakdown.rounds);
    draw_row("\xD5\xBD\xB6\xB7\xBB\xD8\xBA\xCF\xA3\xBA", buf,  // 战斗回合：
             200, 200, 200, 255, 255, 100);

    if (is_victory) {
        y += 4;
        platform_->draw_line(label_x, y, value_x + 80, y, 100, 100, 80);
        y += 8;

        // 基础分 / Base
        std::snprintf(buf, sizeof(buf), "+%d", breakdown.base);
        draw_row("\xBB\xF9\xB4\xA1\xB7\xD6\xA3\xBA", buf,  // 基础分：
                 180, 180, 180, 200, 200, 200);

        // 歼敌奖励 / Kill bonus
        std::snprintf(buf, sizeof(buf), "+%d", breakdown.kill_bonus);
        draw_row("\xBD\xC3\xB5\xD0\xBD\xB1\xC0\xF8\xA3\xBA", buf,  // 歼敌奖励：
                 180, 180, 180, 200, 200, 200);

        // 损失扣除 / Loss penalty
        std::snprintf(buf, sizeof(buf), "-%d", breakdown.loss_penalty);
        draw_row("\xCB\xD5\xCA\xA7\xBF\xDB\xB3\xFD\xA3\xBA", buf,  // 损失扣除：
                 180, 180, 180, 255, 100, 100);

        // 速度奖励 / Speed bonus
        std::snprintf(buf, sizeof(buf), "+%d", breakdown.round_bonus);
        draw_row("\xCB\xD9\xB6\xC8\xBD\xB1\xC0\xF8\xA3\xBA", buf,  // 速度奖励：
                 180, 180, 180, 200, 200, 200);

        // 难度系数 / Difficulty
        const char* diff_label;
        if (breakdown.difficulty_pct == 80)
            diff_label = "\xBC\xF2\xB5\xA5";  // 简单
        else if (breakdown.difficulty_pct == 130)
            diff_label = "\xC0\xA7\xC4\xD1";  // 困难
        else
            diff_label = "\xC6\xD5\xCD\xA8";  // 普通

        std::snprintf(buf, sizeof(buf), "%d%%", breakdown.difficulty_pct);
        draw_row("\xC4\xD1\xB6\xC8\xCF\xB5\xCA\xFD\xA3\xBA", buf,  // 难度系数：
                 180, 180, 180, 200, 200, 200);
    }

    y += 4;
    platform_->draw_line(label_x, y, value_x + 80, y, 180, 160, 100);
    y += 8;

    // 总积分 / Total score
    std::string total_label = "\xD7\xDC\xBB\xFD\xB7\xD6\xA3\xBA";  // 总积分：
    font_->draw_text(label_x, y, total_label, 255, 255, 200);

    std::snprintf(buf, sizeof(buf), "%d", score);
    font_->draw_text(value_x, y, buf, 255, 255, 100);

    std::string hint = "\xB5\xE3\xBB\xF7\xBC\xCC\xD0\xF8";  // 点击继续
    int32_t hint_chars = 0;
    size_t hpos = 0;
    while (hpos < hint.size()) {
        uint8_t b = static_cast<uint8_t>(hint[hpos]);
        hpos += (b < 0x80) ? 1 : 2;
        hint_chars++;
    }
    int32_t hint_w = hint_chars * FontRenderer::CHAR_WIDTH;
    int32_t hint_x = (640 - hint_w) / 2;
    font_->draw_text(hint_x, 395, hint, 180, 180, 180);
}

}  // namespace dlzr::presentation
