// battle_intro_renderer.cpp — 战斗开场简报渲染器实现

#include "presentation/battle/battle_intro_renderer.hpp"

#include <cstring>

namespace dlzr::presentation {

void BattleIntroRenderer::init(platform::Platform& platform, FontRenderer& font) {
    platform_ = &platform;
    font_ = &font;
}

static std::string array_to_string(const char* data, size_t max_len) {
    size_t len = 0;
    while (len < max_len && data[len] != '\0') ++len;
    return std::string(data, len);
}

void BattleIntroRenderer::render(const core::FightIntro& intro) {
    if (!platform_) return;

    platform_->fill_rect(0, 0, 640, 480, 0, 0, 0);

    platform_->fill_rect(40, 40, 560, 400, 20, 20, 40);
    platform_->draw_rect(40, 40, 560, 400, 180, 160, 100);
    platform_->draw_rect(42, 42, 556, 396, 120, 100, 60);

    std::string name = array_to_string(intro.name.data(), intro.name.size());
    if (!name.empty()) {
        int32_t name_chars = 0;
        size_t pos = 0;
        while (pos < name.size()) {
            uint8_t b = static_cast<uint8_t>(name[pos]);
            pos += (b < 0x80) ? 1 : 2;
            name_chars++;
        }
        int32_t name_w = name_chars * FontRenderer::CHAR_WIDTH;
        int32_t name_x = (640 - name_w) / 2;
        font_->draw_text(name_x, TITLE_Y, name, 255, 255, 100);
    }

    std::string diff_label = "\xC4\xD1\xB6\xC8\xA3\xBA";  // 难度：
    font_->draw_text(TEXT_X, INFO_Y, diff_label, 200, 200, 200);

    char diff_val[16];
    std::snprintf(diff_val, sizeof(diff_val), "%d", intro.difficulty);
    font_->draw_text(TEXT_X + 7 * FontRenderer::CHAR_WIDTH, INFO_Y,
                     diff_val, 255, 200, 100);

    std::string success_label = "\xCA\xA4\xC0\xFB\xCC\xF5\xBC\xFE\xA3\xBA";  // 胜利条件：
    font_->draw_text(TEXT_X, INFO_Y + LINE_HEIGHT, success_label, 100, 255, 100);

    std::string success_text = array_to_string(intro.success_text.data(), intro.success_text.size());
    if (!success_text.empty()) {
        font_->draw_text(TEXT_X, INFO_Y + LINE_HEIGHT * 2, success_text,
                         220, 220, 220);
    }

    std::string fail_label = "\xCA\xA7\xB0\xDC\xCC\xF5\xBC\xFE\xA3\xBA";  // 失败条件：
    font_->draw_text(TEXT_X, INFO_Y + LINE_HEIGHT * 4, fail_label, 255, 100, 100);

    std::string fail_text = array_to_string(intro.failure_text.data(), intro.failure_text.size());
    if (!fail_text.empty()) {
        font_->draw_text(TEXT_X, INFO_Y + LINE_HEIGHT * 5, fail_text,
                         220, 220, 220);
    }

    std::string hint = "\xB5\xE3\xBB\xF7\xBB\xF2\xB0\xB4\xBF\xD5\xB8\xF1\xBC\xFC\xBC\xCC\xD0\xF8";  // 点击或按空格键继续
    int32_t hint_chars = 0;
    size_t hpos = 0;
    while (hpos < hint.size()) {
        uint8_t b = static_cast<uint8_t>(hint[hpos]);
        hpos += (b < 0x80) ? 1 : 2;
        hint_chars++;
    }
    int32_t hint_w = hint_chars * FontRenderer::CHAR_WIDTH;
    int32_t hint_x = (640 - hint_w) / 2;
    font_->draw_text(hint_x, 400, hint, 180, 180, 180);
}

}  // namespace dlzr::presentation
