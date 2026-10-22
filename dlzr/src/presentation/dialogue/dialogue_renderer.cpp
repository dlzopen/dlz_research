#include "presentation/dialogue/dialogue_renderer.hpp"

#include "app/resource/tga_loader.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <filesystem>
#include <vector>

namespace dlzr::presentation {

void DialogueRenderer::init(platform::Platform& platform, FontRenderer& font,
                              ImageCache& images,
                              const std::string& game_data_dir) {
    platform_ = &platform;
    font_ = &font;
    images_ = &images;

    // 加载对话框面板 FRAME00.TGA（404x127，8bpp + 文件内嵌调色板）。
    // 证据 FUN_00424847.c:14 — FUN_00426202(s_frame00_tga, &DAT_007db580, 0x194, 0x7f)。
    // 必须用文件内嵌调色板：实测与 kOriginalPalette 的 60 个用色全部不同。
    // 索引 0 = 透明色键（FUN_00422a78 param_6 = 0）。
    namespace fs = std::filesystem;
    try {
        auto img = app::TgaLoader::load(fs::path(game_data_dir) / "FRAME00.TGA");
        auto rgba = app::TgaLoader::to_rgba(img);
        for (size_t i = 0; i < img.pixels.size(); ++i) {
            if (img.pixels[i] == 0) rgba[i * 4 + 3] = 0;
        }
        panel_texture_ = platform_->create_texture(img.width, img.height);
        if (panel_texture_) {
            platform_->update_texture(panel_texture_, rgba.data(),
                                      img.width * 4);
        }
    } catch (const std::exception& e) {
        std::fprintf(stderr, "DialogueRenderer: FRAME00.TGA load failed: %s\n",
                     e.what());
        panel_texture_ = 0;
    }
}

void DialogueRenderer::draw_face(const uint8_t* face_rgba) {
    if (!face_rgba || !platform_) return;

    // 复用同一张头像纹理（尺寸固定 80x100），避免逐帧销毁+重建导致
    // 在途渲染批次引用已释放纹理（D3D11 present 时 RtlFreeHeap 崩溃）。
    // 证据 FUN_0043d82f.c:41 — FUN_00422f68(0x82,0x134,0x50,100,face) 直接 blit。
    if (face_texture_ == 0) {
        face_texture_ = platform_->create_texture(FACE_W, FACE_H);
    }
    if (face_texture_ == 0) return;

    platform_->update_texture(face_texture_, face_rgba, FACE_W * 4);
    platform_->draw_texture(face_texture_, FACE_X, FACE_Y);
}

void DialogueRenderer::render(const std::string& speaker_name,
                                const std::string& text,
                                int32_t visible_chars,
                                const uint8_t* face_rgba) {
    if (!platform_) return;
    (void)speaker_name;   // 原版对话框不绘制说话人名。证据 FUN_0043d82f.c（仅面板+头像+文本）。

    // 面板图 FRAME00.TGA 404x127 @ (118,296)。
    // 证据 FUN_0043d82f.c:35-38 + FUN_00424847.c:14。
    if (panel_texture_) {
        platform_->draw_texture(panel_texture_, PANEL_X, PANEL_Y);
    } else {
        // 资源缺失兜底：深色面板（不臆造装饰，仅保证可读）。
        platform_->fill_rect(PANEL_X, PANEL_Y, PANEL_W, PANEL_H, 16, 16, 20);
    }

    const bool has_face = (face_rgba != nullptr);
    if (has_face) {
        // 原版 FUN_0043d82f.c:41 直接 FUN_00422f68(0x82,0x134,0x50,100,face) blit 头像，
        // 无眨眼/表情/嘴部动画修饰。这里同样直接绘制原始头像，不自创动画。
        draw_face(face_rgba);
    }

    if (!text.empty() && visible_chars > 0) {
        // 文本起点：有头像 (222,312) / 无头像 (142,312)。
        // 证据 FUN_0043d82f.c:26-31,96 — (0x8e + (有头像?0x50:0), 0x138)。
        const int32_t tx = has_face ? 222 : 142;
        const int32_t ty = TEXT_Y;                       // 312 (0x138)
        // 每行 18(有头像)/23(无头像) 全角字。
        // 证据 DAT_0098d814=0x24/0x2e：FUN_0043dd47.c:14 / FUN_0043d56b.c:21；
        // 行宽 = 字数 x 8 << 1 = 288/368px，与面板内宽吻合（FUN_0043d82f.c:39）。
        const int32_t chars_per_line =
            has_face ? CHARS_PER_LINE_FACE : CHARS_PER_LINE_NOFACE;

        // 逐字配色 = 调色板索引（非 RGB 值！）。
        // 证据 FUN_004061df.c:39-52,100 — FUN_00405560(索引,0,0) 设置调色板色号，
        //   'A' -> 0xb7, 'B' -> 0x85, 'C' -> 0x89, 'D' -> 0x4f,
        //   正文/复位 -> 0x48。RGB 取 kOriginalPalette 实测值。
        struct Seg { std::string s; uint8_t r, g, b; };
        std::vector<Seg> segs;
        std::string normal;
        auto pal = [](uint8_t idx) {
            const uint8_t r = core::kOriginalPalette[idx * 4 + 0];
            const uint8_t g = core::kOriginalPalette[idx * 4 + 1];
            const uint8_t b = core::kOriginalPalette[idx * 4 + 2];
            return std::array<uint8_t, 3>{r, g, b};
        };
        const auto normal_col = pal(0x48);   // (164,252,252) 亮青
        auto flush = [&]() {
            if (!normal.empty()) {
                segs.push_back({normal, normal_col[0], normal_col[1], normal_col[2]});
                normal.clear();
            }
        };
        auto tok_col = [&pal](char c) -> std::array<uint8_t, 3> {
            switch (c) {
                case 'A': return pal(0xb7);   // 友方名 (252,0,0) 红
                case 'B': return pal(0x85);   // 敌方名 (128,152,64) 绿
                case 'C': return pal(0x89);   // 物品/技术名 (252,184,24) 金
                case 'D': return pal(0x4f);   // 地名 (168,196,252) 淡蓝
                default:  return pal(0x48);
            }
        };

        size_t i = 0;
        while (i < text.size()) {
            uint8_t b = static_cast<uint8_t>(text[i]);
            bool marker = (b == 'A' || b == 'B' || b == 'C' || b == 'D') &&
                          (i + 1 < text.size() && text[i + 1] == static_cast<char>(b));
            if (marker) {
                flush();
                auto col = tok_col(static_cast<char>(b));
                i += 2;  // 跳过 "AA"/"BB"/"CC"/"DD" 标记对
                if (i < text.size()) {
                    uint8_t c = static_cast<uint8_t>(text[i]);
                    int n = (c < 0x80) ? 1 : 2;
                    if (i + n > text.size()) n = 1;
                    segs.push_back({text.substr(i, n), col[0], col[1], col[2]});
                    i += n;
                }
            } else {
                uint8_t c = static_cast<uint8_t>(text[i]);
                int n = (c < 0x80) ? 1 : 2;
                if (i + n > text.size()) n = 1;
                normal.append(text, i, n);
                i += n;
            }
        }
        flush();

        auto seg_chars = [](const std::string& s) {
            int n = 0;
            size_t p = 0;
            while (p < s.size()) {
                uint8_t bb = static_cast<uint8_t>(s[p]);
                p += (bb < 0x80) ? 1 : 2;
                ++n;
            }
            return n;
        };

        int32_t remaining = visible_chars;
        int32_t cx = 0, cy = 0;
        for (const auto& seg : segs) {
            if (remaining <= 0) break;
            int sc = seg_chars(seg.s);
            int draw = (sc < remaining) ? sc : remaining;
            size_t p = 0;
            for (int k = 0; k < draw; ++k) {
                uint8_t bb = static_cast<uint8_t>(seg.s[p]);
                int n = (bb < 0x80) ? 1 : 2;
                std::string_view ch(&seg.s[p], n);
                // 全角字宽 16px 网格（FUN_004061df.c:98 GBK uVar1 += 0x10）。
                int32_t draw_x = tx + cx * FontRenderer::CHAR_WIDTH;
                int32_t draw_y = ty + cy * LINE_HEIGHT;
                font_->draw_text(draw_x, draw_y, ch, seg.r, seg.g, seg.b);
                cx++;
                if (cx >= chars_per_line) { cx = 0; cy++; }
                p += n;
            }
            remaining -= draw;
        }
    }
}

}  // namespace dlzr::presentation
