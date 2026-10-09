// forging_renderer.cpp — 锻造界面渲染器实现
// Forging/crafting screen renderer implementation

#include "presentation/forging/forging_renderer.hpp"

#include <cstdio>
#include <string>

namespace dlzr::presentation {

void ForgingRenderer::init(platform::Platform& platform, FontRenderer& font,
                            ImageCache& images) {
    platform_ = &platform;
    font_ = &font;
    images_ = &images;
}

void ForgingRenderer::render(int32_t selected_tab, int32_t selected_item,
                              int32_t quantity,
                              const logic::ForgingSystem& forging) {
    if (!platform_) return;

    platform_->fill_rect(0, 0, 640, 480, 0, 0, 0);

    platform_->fill_rect(40, 30, 560, 420, 20, 15, 10);
    platform_->draw_rect(40, 30, 560, 420, 160, 130, 80);

    std::string title = "\xB6\xCD\xD4\xEC";  // 锻造
    int32_t title_w = 2 * FontRenderer::CHAR_WIDTH * 2;
    font_->draw_text(320 - title_w / 2, 45, title, 255, 220, 100);

    render_tabs(selected_tab);
    render_item_grid(selected_tab, selected_item);

    int32_t price = forging.get_price_per_unit(selected_tab);
    render_quantity_display(quantity, price);
    render_material_list(selected_item, forging);
    render_buttons();
}

void ForgingRenderer::render_tabs(int32_t selected_tab) {
    constexpr int32_t TAB_X = 60;
    constexpr int32_t TAB_Y = 80;
    constexpr int32_t TAB_W = 80;
    constexpr int32_t TAB_H = 30;
    constexpr int32_t TAB_GAP = 10;

    const char* tab_names[] = {
        "\xC0\xD7\xB4\xEF\xD2\xBB",  // 雷达一
        "\xC0\xD7\xB4\xEF\xB6\xFE",  // 雷达二
        "\xC0\xD7\xB4\xEF\xC8\xFD",  // 雷达三
        "\xC0\xD7\xB4\xEF\xCB\xC4",  // 雷达四
    };

    for (int i = 0; i < 4; ++i) {
        int32_t x = TAB_X + (i % 2) * (TAB_W + TAB_GAP);
        int32_t y = TAB_Y + (i / 2) * (TAB_H + TAB_GAP);

        uint8_t r = (i == selected_tab) ? 80 : 40;
        uint8_t g = (i == selected_tab) ? 60 : 30;
        uint8_t b = (i == selected_tab) ? 30 : 15;
        platform_->fill_rect(x, y, TAB_W, TAB_H, r, g, b);

        uint8_t br = (i == selected_tab) ? 200 : 120;
        uint8_t bg = (i == selected_tab) ? 180 : 100;
        uint8_t bb = (i == selected_tab) ? 100 : 60;
        platform_->draw_rect(x, y, TAB_W, TAB_H, br, bg, bb);

        font_->draw_text(x + 10, y + 8, tab_names[i], br, bg, bb);
    }
}

void ForgingRenderer::render_item_grid(int32_t selected_tab,
                                        int32_t selected_item) {
    constexpr int32_t GRID_X = 220;
    constexpr int32_t GRID_Y = 80;
    constexpr int32_t CELL_W = 48;
    constexpr int32_t CELL_H = 48;
    constexpr int32_t COLS = 5;

    for (int i = 0; i < 10; ++i) {
        int32_t col = i % COLS;
        int32_t row = i / COLS;
        int32_t x = GRID_X + col * (CELL_W + 4);
        int32_t y = GRID_Y + row * (CELL_H + 4);

        platform_->fill_rect(x, y, CELL_W, CELL_H, 30, 25, 15);

        if (i == selected_item) {
            platform_->draw_rect(x - 1, y - 1, CELL_W + 2, CELL_H + 2,
                                 255, 220, 100);
        } else {
            platform_->draw_rect(x, y, CELL_W, CELL_H, 100, 80, 50);
        }

        char idx_buf[8];
        std::snprintf(idx_buf, sizeof(idx_buf), "%d", i + 1);
        font_->draw_text(x + 18, y + 16, idx_buf, 150, 150, 150);
    }
}

void ForgingRenderer::render_quantity_display(int32_t quantity, int32_t price) {
    constexpr int32_t X = 60;
    constexpr int32_t Y = 200;

    std::string qty_label = "\xCA\xFD\xC1\xBF\xA3\xBA";  // 数量：
    font_->draw_text(X, Y, qty_label, 200, 200, 200);

    char qty_buf[16];
    std::snprintf(qty_buf, sizeof(qty_buf), "%d", quantity);
    font_->draw_text(X + 5 * FontRenderer::CHAR_WIDTH, Y, qty_buf,
                     255, 255, 100);

    if (price > 0) {
        std::string cost_label = "\xBD\xF0\xB6\xEE\xA3\xBA";  // 金额：
        font_->draw_text(X, Y + 30, cost_label, 200, 200, 200);

        int32_t total_cost = price * quantity;
        char cost_buf[16];
        std::snprintf(cost_buf, sizeof(cost_buf), "%d", total_cost);
        font_->draw_text(X + 5 * FontRenderer::CHAR_WIDTH, Y + 30, cost_buf,
                         255, 200, 100);
    }
}

void ForgingRenderer::render_material_list(int32_t selected_item,
                                            const logic::ForgingSystem& forging) {
    constexpr int32_t X = 60;
    constexpr int32_t Y = 280;
    constexpr int32_t LINE_H = 22;

    std::string header = "\xCB\xF9\xD0\xE8\xB2\xC4\xC1\xCF\xA3\xBA";  // 所需材料：
    font_->draw_text(X, Y, header, 200, 180, 100);

    for (int m = 0; m < logic::ForgingSystem::NUM_MATERIALS; ++m) {
        const auto& mat = forging.get_material(m);
        int32_t y = Y + 25 + m * LINE_H;

        font_->draw_text(X + 10, y, mat.name.data(), 180, 180, 180);

        int32_t stock = forging.get_material_stock(m);
        char stock_buf[16];
        std::snprintf(stock_buf, sizeof(stock_buf), "[%d]", stock);
        font_->draw_text(X + 180, y, stock_buf, 150, 200, 150);
    }
}

void ForgingRenderer::render_buttons() {
    constexpr int32_t BTN_X = 420;
    constexpr int32_t BTN_Y = 360;
    constexpr int32_t BTN_W = 80;
    constexpr int32_t BTN_H = 30;
    constexpr int32_t BTN_GAP = 10;

    const char* btn_labels[] = {
        "\xB7\xB5\xBB\xD8",   // 返回
        "\xC8\xB7\xC8\xCF",   // 确认
        "\xCA\xFD\xC1\xBF +",  // 数量 +
        "\xCA\xFD\xC1\xBF -",  // 数量 -
    };

    for (int i = 0; i < 4; ++i) {
        int32_t x = BTN_X + (i % 2) * (BTN_W + BTN_GAP);
        int32_t y = BTN_Y + (i / 2) * (BTN_H + BTN_GAP);

        platform_->fill_rect(x, y, BTN_W, BTN_H, 50, 40, 25);
        platform_->draw_rect(x, y, BTN_W, BTN_H, 180, 150, 80);
        font_->draw_text(x + 10, y + 8, btn_labels[i], 220, 200, 150);
    }
}

}  // namespace dlzr::presentation
