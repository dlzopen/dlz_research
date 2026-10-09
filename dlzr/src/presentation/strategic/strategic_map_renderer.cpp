// strategic_map_renderer.cpp — 战略地图渲染实现

#include "presentation/strategic/strategic_map_renderer.hpp"

namespace dlzr::presentation {

static constexpr int32_t TOWN_POS_X[19] = {
    80, 160, 240, 320, 400, 480, 560,
    80, 160, 240, 320, 400, 480, 560,
    80, 160, 240, 320, 400
};
static constexpr int32_t TOWN_POS_Y[19] = {
    60,  60,  60,  60,  60,  60,  60,
    160, 160, 160, 160, 160, 160, 160,
    260, 260, 260, 260, 260
};

static constexpr int32_t TOWN_RADIUS = 12;

struct TownEdge {
    int32_t from;
    int32_t to;
};

static constexpr TownEdge TOWN_EDGES[] = {
    {0, 1},   // 石家庄 — 太原
    {0, 2},   // 石家庄 — 保定
    {0, 6},   // 石家庄 — 郑州
    {0, 9},   // 石家庄 — 邯郸
    {0, 14},  // 石家庄 — 阳泉
    {1, 2},   // 太原 — 保定
    {1, 14},  // 太原 — 阳泉
    {1, 15},  // 太原 — 长治
    {2, 3},   // 保定 — 北京
    {2, 8},   // 保定 — 沧州
    {2, 16},  // 保定 — 丰台
    {3, 4},   // 北京 — 天津
    {3, 11},  // 北京 — 承德
    {3, 16},  // 北京 — 丰台
    {4, 5},   // 天津 — 济南
    {4, 8},   // 天津 — 沧州
    {5, 7},   // 济南 — 德州
    {5, 10},  // 济南 — 徐州
    {5, 12},  // 济南 — 青岛
    {6, 9},   // 郑州 — 邯郸
    {6, 10},  // 郑州 — 徐州
    {6, 17},  // 郑州 — 洛阳
    {6, 18},  // 郑州 — 开封
    {7, 8},   // 德州 — 沧州
    {9, 15},  // 邯郸 — 长治
    {10, 13}, // 徐州 — 连云港
    {12, 13}, // 青岛 — 连云港
    {14, 15}, // 阳泉 — 长治
    {17, 18}, // 洛阳 — 开封
};
static constexpr int32_t TOWN_EDGE_COUNT = sizeof(TOWN_EDGES) / sizeof(TOWN_EDGES[0]);

static const char* TOWN_NAMES[19] = {
    "\xCA\xAF\xBC\xD2\xD7\xAF",       // 石家庄
    "\xCC\xAB\xD4\xAD",               // 太原
    "\xB1\xA3\xB6\xA8",               // 保定
    "\xB1\xB1\xBE\xA9",               // 北京
    "\xCC\xEC\xBD\xF2",               // 天津
    "\xBC\xC3\xC4\xCF",               // 济南
    "\xD6\xA3\xD6\xDD",               // 郑州
    "\xB5\xC2\xD6\xDD",               // 德州
    "\xE3\xF4\xD6\xDD",               // 沧州
    "\xBA\xAB\xB5\xA4",               // 邯郸
    "\xD0\xEC\xD6\xDD",               // 徐州
    "\xB3\xD0\xB5\xBA",               // 承德
    "\xC7\xE0\xB5\xBA",               // 青岛
    "\xC1\xAC\xD4\xC6",               // 连云港
    "\xD1\xF3\xC8\xAA",               // 阳泉
    "\xB3\xA4\xD6\xCE",               // 长治
    "\xB7\xE2\xCC\xA8",               // 丰台
    "\xC2\xE4\xD1\xF4",               // 洛阳
    "\xBF\xAA\xB7\xE2",               // 开封
};

void StrategicMapRenderer::init(platform::Platform& platform, FontRenderer& font,
                                 ImageCache& images) {
    platform_ = &platform;
    font_ = &font;
    images_ = &images;
}

void StrategicMapRenderer::render(const core::GameData& game_data, int32_t current_town) {
    images_->draw_image("BIGMAP.IMG", 0, 0);

    draw_paths();

    for (int i = 0; i < core::MAX_TOWNS; ++i) {
        draw_town(i, TOWN_POS_X[i], TOWN_POS_Y[i], i == current_town);
    }

    platform_->fill_rect(0, 440, 640, 40, 30, 30, 50);

    int32_t ct = current_town;
    if (ct >= 0 && ct < core::MAX_TOWNS) {
        font_->draw_text(10, 450, TOWN_NAMES[ct], 255, 255, 200);
    }

    font_->draw_text(220, 10, "\xD5\xBD\xC2\xD5\xB5\xD8\xCD\xBC", 255, 255, 255);
}

void StrategicMapRenderer::draw_town(int32_t index, int32_t x, int32_t y, bool is_current) {
    uint8_t r = is_current ? 255 : 180;
    uint8_t g = is_current ? 255 : 140;
    uint8_t b = is_current ? 0 : 50;

    platform_->fill_rect(x - TOWN_RADIUS, y - TOWN_RADIUS,
                         TOWN_RADIUS * 2, TOWN_RADIUS * 2, r, g, b);
    platform_->draw_rect(x - TOWN_RADIUS, y - TOWN_RADIUS,
                         TOWN_RADIUS * 2, TOWN_RADIUS * 2, 255, 255, 255);

    if (index < core::MAX_TOWNS) {
        int32_t tw = 0, th = 0;
        font_->measure_text(TOWN_NAMES[index], tw, th);
        font_->draw_text(x - tw / 2, y + TOWN_RADIUS + 2, TOWN_NAMES[index],
                         255, 255, 255);
    }
}

void StrategicMapRenderer::draw_paths() {
    for (int32_t i = 0; i < TOWN_EDGE_COUNT; ++i) {
        int32_t a = TOWN_EDGES[i].from;
        int32_t b = TOWN_EDGES[i].to;
        platform_->draw_line(TOWN_POS_X[a], TOWN_POS_Y[a],
                             TOWN_POS_X[b], TOWN_POS_Y[b],
                             180, 160, 100);
    }
}

int32_t StrategicMapRenderer::town_at_position(int32_t screen_x, int32_t screen_y,
                                                const core::GameData& /*game_data*/) const {
    for (int i = 0; i < core::MAX_TOWNS; ++i) {
        int32_t dx = screen_x - TOWN_POS_X[i];
        int32_t dy = screen_y - TOWN_POS_Y[i];
        if (dx >= -TOWN_RADIUS && dx <= TOWN_RADIUS &&
            dy >= -TOWN_RADIUS && dy <= TOWN_RADIUS) {
            return i;
        }
    }
    return -1;
}

}  // namespace dlzr::presentation
