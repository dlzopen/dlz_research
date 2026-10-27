#pragma once

#include "platform/platform.hpp"
#include "core/data_model/palette.hpp"
#include "presentation/font/font_renderer.hpp"
#include "presentation/image/image_cache.hpp"

#include <cstdint>
#include <string>

namespace dlzr::presentation {

// 逐字配色令牌 → 调色板索引（RE 证据 FUN_004061df.c:39-52，逐指令实证）。
//   原版仅处理 A/B/C/D 四类；其余 ASCII（含 G/T 与其它字符）一律 0x48 正文色。
//   GG/TT 属解析层语义，在抵达渲染层前已被 dialogue_text_util::format_dialogue_text 处理：
//   GG→展开为 "AA"+主角名（渲染层按 FUN_004061df.c:39-42 着红 0xb7）、TT→剥离；
//   故渲染层只会见到 AA/BB/CC/DD 配色标记与裸文本，映射规则同上。
//   RGB 取 core::kOriginalPalette[idx*4..]（调色板已 RE，未在此重复）。
uint8_t dialogue_token_palette_index(char c);

// 逐字配色分段（RE 实证 FUN_004061df.c:39-52,100）：
//   扫描对话串，AA/BB/CC/DD 标记对→调色板索引→RGB；其余字符→0x48 正文色。
//   返回按绘制顺序排列的彩色分段；每个分段内的字共用同一 RGB。
//   令牌字符本身不绘制、仅着色紧随其后的 1 个字模（与二进制一致）。
//   说话人名由调用方（main.cpp build_dialogue_display）前置进 text（默认色内联绘制，
//   原版 FUN_0043cc96:44-49 写名 + FUN_0043d82f 绘制），本函数只负责整串配色。
struct DialogueColoredSegment {
    std::string text;   // 该分段原始字节（可能含多字节 GBK）
    uint8_t r{0}, g{0}, b{0};
};
std::vector<DialogueColoredSegment> dialogue_build_segments(const std::string& text);

class DialogueRenderer {
public:
    void init(platform::Platform& platform, FontRenderer& font,
              ImageCache& images, const std::string& game_data_dir);

    // text 已含说话人名（由调用方前置，默认色内联）+ 正文。visible_chars 控制整页显示进度。
    void render(const std::string& text,
                int32_t visible_chars,
                const uint8_t* face_rgba = nullptr);

    // ---- 对话框版面（RE 实证，640x480 坐标系）----
    // 证据 FUN_0043d82f.c / FUN_0043cfe1.c：
    //   面板图 FRAME00.TGA 404x127 贴于 (0x76,0x128)=(118,296)（:35-38）
    //   头像 80x100 @ (0x82,0x134)=(130,308)（:41 FUN_00422f68）
    //   文本起点 (0x8e+(有头像?0x50:0), 0x138) = 有头像(222,312)/无头像(142,312)（:26-31,96）
    //   5 行 x 20px 行高（:82-84 local_15c*0x14）
    //   每行 18 全角字(有头像, DAT_0098d814=0x24, FUN_0043dd47.c:14)
    //          / 23 全角字(无头像, 0x2e, FUN_0043d56b.c:21)
    //   面板素材 = FRAME00.TGA（FUN_00424847.c:14 FUN_00426202(s_frame00_tga,...,0x194,0x7f)），
    //   用文件内嵌调色板（与 kOriginalPalette 不同，实测 60 个用色全部不同）；
    //   索引 0 = 透明色键（FUN_00422a78 param_6=0）。
    static constexpr int32_t PANEL_X = 118;   // 0x76
    static constexpr int32_t PANEL_Y = 296;   // 0x128
    static constexpr int32_t PANEL_W = 404;   // 0x194
    static constexpr int32_t PANEL_H = 127;   // 0x7f
    static constexpr int32_t TEXT_Y = 312;    // 0x138
    static constexpr int32_t LINE_HEIGHT = 20;      // 0x14
    static constexpr int32_t LINES_PER_PAGE = 5;
    static constexpr int32_t CHARS_PER_LINE_FACE = 18;   // 有头像
    static constexpr int32_t CHARS_PER_LINE_NOFACE = 23; // 无头像

    static constexpr int32_t FACE_X = 130;    // 0x82
    static constexpr int32_t FACE_Y = 308;    // 0x134
    static constexpr int32_t FACE_W = 80;
    static constexpr int32_t FACE_H = 100;

private:
    platform::Platform* platform_{nullptr};
    FontRenderer* font_{nullptr};
    ImageCache* images_{nullptr};

    platform::TextureHandle face_texture_{0};
    platform::TextureHandle panel_texture_{0};   // FRAME00.TGA 面板（索引0透明）

    void draw_face(const uint8_t* face_rgba);
};

}  // namespace dlzr::presentation
