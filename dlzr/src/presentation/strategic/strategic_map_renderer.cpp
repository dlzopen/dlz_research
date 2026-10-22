// strategic_map_renderer.cpp — 战略地图渲染实现
//
// 人物信息面板（右上角）1:1 复刻 FUN_0041be02：坐标/配色/字段逐行对齐战斗信息面板
// draw_info_panel（同一原函数）。证据见 battle_renderer.cpp::draw_info_panel 注释
// 与 output/ghidra_all/FUN_0041be02.c。

#include "presentation/strategic/strategic_map_renderer.hpp"

#include "core/data_model/palette.hpp"

#include <cstdio>
#include <cstring>

namespace dlzr::presentation {

// 占位布局：把城镇摆到图上（纯几何，无虚构语义）
static constexpr int32_t TOWN_RADIUS = 12;
static constexpr int32_t GRID_COLS = 7;
static constexpr int32_t GRID_X0 = 80;
static constexpr int32_t GRID_Y0 = 60;
static constexpr int32_t GRID_DX = 80;
static constexpr int32_t GRID_DY = 100;

static void town_pos(int32_t index, int32_t& x, int32_t& y) {
    x = GRID_X0 + (index % GRID_COLS) * GRID_DX;
    y = GRID_Y0 + (index / GRID_COLS) * GRID_DY;
}

// 城镇在 TOWN.DAT 中是否真实存在：至少有一个有效建筑标签位置 (0 < x < 999)
static bool is_town_present(const core::Town& town) {
    for (int32_t b = 0; b < 6; ++b) {
        if (town.name_x[b] > 0 && town.name_x[b] < 999) {
            return true;
        }
    }
    return false;
}

void StrategicMapRenderer::init(platform::Platform& platform, FontRenderer& font,
                                 ImageCache& images, app::FaceLoader& face) {
    platform_ = &platform;
    font_ = &font;
    images_ = &images;
    face_loader_ = &face;
}

void StrategicMapRenderer::render(const core::GameData& game_data, int32_t current_town) {
    images_->draw_image("BIGMAP.IMG", 0, 0);

    // 原版战略地图单位光标以 150ms 为周期闪烁（WM_TIMER：DAT_007db4e8=0x96(150)
    // 触发 FUN_0043c2dd → FUN_0041b8e9 重绘当前关卡地图与单位光标）。以下 1:1 复刻
    // 该 150ms/帧闪烁：当前位置单位标记在 150ms 窗口内亮/灭交替。
    const bool blink_on = ((platform_->ticks_ms() / 150) % 2) == 0;

    // 数据驱动：仅绘制 TOWN.DAT 中真实存在的城镇
    for (int32_t i = 0; i < core::MAX_TOWNS; ++i) {
        if (!is_town_present(game_data.towns[i])) continue;
        int32_t x = 0, y = 0;
        town_pos(i, x, y);
        draw_town(i, x, y, i == current_town,
                  (i == current_town) ? blink_on : false);
    }

    platform_->fill_rect(0, 440, 640, 40, 30, 30, 50);

    if (current_town >= 0 && current_town < core::MAX_TOWNS &&
        is_town_present(game_data.towns[current_town])) {
        char label[32];
        std::snprintf(label, sizeof(label), "No.%d", current_town + 1);
        font_->draw_text(10, 450, label, 255, 255, 200);
    }

    font_->draw_text(220, 10, "\xD5\xBD\xC2\xD5\xB5\xD8\xCD\xBC", 255, 255, 255);

    // 人物信息面板（右上角，原版 FUN_0041be02）：显示主角(people[0])核心数值。
    draw_unit_panel(game_data, 0);
}

void StrategicMapRenderer::draw_int(int32_t x, int32_t y, int32_t value,
                                    uint8_t r, uint8_t g, uint8_t b) {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%d", value);
    font_->draw_text(x, y, buf, r, g, b);
}

platform::TextureHandle StrategicMapRenderer::get_face_texture(int32_t face_idx) {
    if (!face_loader_ || !platform_) return 0;
    if (face_idx < 0 || face_idx >= app::FaceLoader::FACE_COUNT) return 0;
    auto it = face_tex_cache_.find(face_idx);
    if (it != face_tex_cache_.end()) return it->second;

    platform::TextureHandle tex = 0;
    const uint8_t* rgba = face_loader_->get_face_rgba(face_idx);
    if (rgba) {
        tex = platform_->create_texture(app::FaceLoader::FACE_WIDTH,
                                        app::FaceLoader::FACE_HEIGHT);
        if (tex) {
            platform_->update_texture(tex, rgba,
                                      app::FaceLoader::FACE_WIDTH * 4);
        }
    }
    face_tex_cache_[face_idx] = tex;
    return tex;
}

void StrategicMapRenderer::draw_unit_panel(const core::GameData& game_data,
                                            int32_t unit_index) {
    if (unit_index < 0 || unit_index >= core::MAX_PEOPLE) return;
    const core::People& p = game_data.people[unit_index];

    const core::ClassType* ct = nullptr;
    int32_t ct_idx = p.class_type;
    if (ct_idx >= 0 && ct_idx < core::MAX_CLASS_TYPES) ct = &game_data.class_types[ct_idx];

    // 面板底 (459,32,168,207) 色板199=(32,68,0) 深绿。证据 FUN_0041be02.c:39,64。
    platform_->fill_rect(459, 32, 168, 207,
                         core::kOriginalPalette[199 * 4 + 0],
                         core::kOriginalPalette[199 * 4 + 1],
                         core::kOriginalPalette[199 * 4 + 2]);

    // 头像 80×100 @ (464,38)。证据 FUN_0041be02.c:73。
    platform::TextureHandle ftex = get_face_texture(p.face);
    if (ftex) platform_->draw_texture(ftex, 464, 38);

    // 面板文字色 = 调色板 0x48=(164,252,252) 亮青。证据 FUN_0041be02.c:70。
    const uint8_t TR = core::kOriginalPalette[72 * 4 + 0];
    const uint8_t TG = core::kOriginalPalette[72 * 4 + 1];
    const uint8_t TB = core::kOriginalPalette[72 * 4 + 2];

    // 姓名（头像右侧）。证据 FUN_0041be02.c:75 @ (548,38)。
    std::string name(p.name.data(), ::strnlen(p.name.data(), p.name.size()));
    if (!name.empty()) font_->draw_text(548, 38, name.c_str(), TR, TG, TB);

    // 武力 / 智力。证据 FUN_0041be02.c:76-82 @ (548,58)/(548,78)。
    font_->draw_text(548, 58, "\xCE\xE4\xC1\xA6", TR, TG, TB);   // 武力
    draw_int(592, 58, p.strength, TR, TG, TB);
    font_->draw_text(548, 78, "\xD6\xC7\xC1\xA6", TR, TG, TB);   // 智力
    draw_int(592, 78, p.intelligence, TR, TG, TB);

    // 兵种名（无独立标签，直接显示兵种名）。证据 FUN_0041be02.c:80 @ (548,98)。
    if (ct) {
        std::string cls(ct->name.data(), ::strnlen(ct->name.data(), ct->name.size()));
        if (!cls.empty()) font_->draw_text(548, 98, cls.c_str(), TR, TG, TB);
    }

    // 等级 / 经验。证据 FUN_0041be02.c:81-84 @ (548,118)/(548,138)。
    font_->draw_text(548, 118, "\xB5\xC8\xBC\xB6", TR, TG, TB);  // 等级
    draw_int(592, 118, p.level, TR, TG, TB);
    font_->draw_text(548, 138, "\xBE\xAD\xD1\xE9", TR, TG, TB);  // 经验
    draw_int(592, 138, p.experience, TR, TG, TB);

    // 体力条：白底 (464,142,80,4) + 三档彩色填充。证据 FUN_0041be02.c:96,114。
    // 战略地图单位满血 -> 绿(0x85)。
    platform_->fill_rect(464, 142, 80, 4, 255, 255, 255);
    int32_t max_hp = 0;
    if (ct) max_hp = ct->hp_base + (p.level - 1) * ct->hp_growth;
    platform_->fill_rect(464, 142, 80, 4,
                         core::kOriginalPalette[0x85 * 4 + 0],
                         core::kOriginalPalette[0x85 * 4 + 1],
                         core::kOriginalPalette[0x85 * 4 + 2]);

    // 攻击 / 防御（兵种基础值）。证据 FUN_0041be02.c:85-90 @ (464,160)/(548,160)。
    int32_t atk = 0, def = 0;
    if (ct) { atk = ct->base_attack; def = ct->base_defense; }
    font_->draw_text(464, 160, "\xB9\xA5\xBB\xF7", TR, TG, TB);   // 攻击
    draw_int(512, 160, atk, TR, TG, TB);
    font_->draw_text(548, 160, "\xB7\xC0\xD3\xA6", TR, TG, TB);   // 防御
    draw_int(592, 160, def, TR, TG, TB);

    // 体力 cur/max。证据 FUN_0041be02.c:91-95 @ (464,180)。
    font_->draw_text(464, 180, "\xCC\xE5\xC1\xA6", TR, TG, TB);   // 体力
    draw_int(512, 180, max_hp, TR, TG, TB);
    font_->draw_text(544, 180, "/", TR, TG, TB);
    draw_int(560, 180, max_hp, TR, TG, TB);

    // 士气 cur/100。证据 FUN_0041be02.c:115-118 @ (464,200)。
    // 战略地图无战斗，士气=100（原版 FUN_0041be02.c:118 值=100）。
    font_->draw_text(464, 200, "\xCA\xB0\xC6\xF8", TR, TG, TB);   // 士气
    draw_int(512, 200, 100, TR, TG, TB);
    font_->draw_text(544, 200, "/", TR, TG, TB);
    draw_int(560, 200, 100, TR, TG, TB);

    // 敌方红标"敌"。证据 FUN_0041be02.c:67-71 @ (464,220)。
    if (p.enemy != 0) {
        font_->draw_text(464, 220, "\xB5\xD0", 252, 0, 0);   // 敌（红）
    }
}

void StrategicMapRenderer::draw_town(int32_t index, int32_t x, int32_t y,
                                  bool is_current, bool blink) {
    uint8_t r = is_current ? 255 : 180;
    uint8_t g = is_current ? 255 : 140;
    uint8_t b = is_current ? 0 : 50;

    platform_->fill_rect(x - TOWN_RADIUS, y - TOWN_RADIUS,
                         TOWN_RADIUS * 2, TOWN_RADIUS * 2, r, g, b);
    platform_->draw_rect(x - TOWN_RADIUS, y - TOWN_RADIUS,
                         TOWN_RADIUS * 2, TOWN_RADIUS * 2, 255, 255, 255);

    // 当前位置单位光标：150ms 闪烁（亮/灭交替），1:1 对应原版地图单位光标。
    // 灭帧只保留底框，亮帧叠加高亮标记，模拟"人物闪烁"。
    if (is_current && blink) {
        platform_->fill_rect(x - TOWN_RADIUS + 4, y - TOWN_RADIUS + 4,
                             TOWN_RADIUS * 2 - 8, TOWN_RADIUS * 2 - 8, 255, 80, 80);
        platform_->draw_rect(x - TOWN_RADIUS - 2, y - TOWN_RADIUS - 2,
                             TOWN_RADIUS * 2 + 4, TOWN_RADIUS * 2 + 4, 255, 255, 0);
    }

    // 标签用城镇序号（真实名称不在数据中，占位阶段不编造专名）
    char label[16];
    std::snprintf(label, sizeof(label), "%d", index + 1);
    int32_t tw = 0, th = 0;
    font_->measure_text(label, tw, th);
    font_->draw_text(x - tw / 2, y + TOWN_RADIUS + 2, label, 255, 255, 255);
}

int32_t StrategicMapRenderer::town_at_position(int32_t screen_x, int32_t screen_y,
                                                const core::GameData& game_data) const {
    for (int i = 0; i < core::MAX_TOWNS; ++i) {
        if (!is_town_present(game_data.towns[i])) continue;
        int32_t x = 0, y = 0;
        town_pos(i, x, y);
        int32_t dx = screen_x - x;
        int32_t dy = screen_y - y;
        if (dx >= -TOWN_RADIUS && dx <= TOWN_RADIUS &&
            dy >= -TOWN_RADIUS && dy <= TOWN_RADIUS) {
            return i;
        }
    }
    return -1;
}

}  // namespace dlzr::presentation
