#pragma once

// battle_renderer.hpp — 战斗场景渲染器
// Battle scene renderer with original-asset HUD.
//
// HUD 还原证据（全部来自 output/ghidra_all/ 反编译指令流，非推断）:
//  - 全屏边框 KUANG04.IMG: FUN_0041b698.c:14  FUN_00422889(0,0,0x280,0x1e0,&DAT_005d73a8,...)
//  - 顶部图标条 BUTTON00.TGA(28×312,13 图标): FUN_0041b698.c:15-37 + FUN_00424847.c:18
//      13 图标 y=4, 28×24, x = 88,116,144,172,200,228,256,284,312,340,500,528,556
//  - 视口原点 (14,32) 像素 + 48px 格: FUN_0041be02.c:31  FUN_00426810(p1,p2,0xe,0x20,0x1b0,0x1b0)
//      且 hovered cell = (mouse_x-14)/48  -> 真 client 原点 = (14,32), 非旧克隆值(38,56)。
//      (38,56) 仅见于 FUN_0041e97a.c 的 SetCursorPos，是含 24px 窗框的屏幕坐标。
//  - 悬停单位信息面板: FUN_0041be02.c
//      面板底 (459,32,168,207) 色板199; 头像 80×100 @ (464,38); HP白条 (464,142,80,4)
//  - 小地图: FUN_00417b47.c / FUN_00416e3c.c
//      格 = min(clamp(168/w),clamp(212/h)), clamp(v)=(v>=3?v:2);
//      左上 = (459+(168-cell*w)/2, 252+(212-cell*h)/2); 视野框色 0x48=(164,252,252) 亮青
//      地形 = 整张地图缩印(DAT_005701f8 预渲染位图等价于 map_texture_ 缩放)
//      单位点: 我方 0xb7=(252,0,0) 红, 敌方 0xbf=(252,140,0) 橙
//  - 单位精灵 ARMY.DAT: army_loader::get_action_frame_rgba(entry, action, dir, counter)
//      entry = game_data.class_types[fighter.class_type].img_index (ClassType+0x10)
//  - 单位头像 FACE.DAT: face_loader::get_face_rgba(face) (FightPeople+0x10)

#include "logic/battle/battle_state.hpp"
#include "core/data_model/game_data.hpp"
#include "platform/platform.hpp"
#include "presentation/font/font_renderer.hpp"
#include "presentation/image/image_cache.hpp"
#include "app/resource/army_loader.hpp"
#include "app/resource/face_loader.hpp"

#include <unordered_map>
#include <chrono>

namespace dlzr::presentation {

class BattleRenderer {
public:
    // 注入 ARMY/FACE 加载器，供单位精灵与头像使用。
    void init(platform::Platform& platform, FontRenderer& font, ImageCache& images,
              app::ArmyLoader& army, app::FaceLoader& face);

    // 注入 MAP.DAT 地图与 BLK.DAT 瓦片组（每场战斗设置一次）。
    void set_battle_map(const core::FightMap& map, const core::BlkGroupData& blk);

    void render(const logic::BattleState& state,
                const core::FightData& fight_data,
                const core::GameData& game_data);

    void render_command_menu(int32_t menu_x, int32_t menu_y, int32_t hover_item);
    void render_selected_highlight(int32_t grid_x, int32_t grid_y);
    void render_movement_range(const logic::BattleState& state);
    void render_status_text(const char* text);

private:
    platform::Platform* platform_{nullptr};
    FontRenderer* font_{nullptr};
    ImageCache* images_{nullptr};
    app::ArmyLoader* army_loader_{nullptr};
    app::FaceLoader* face_loader_{nullptr};

    const core::FightData* fight_data_{nullptr};
    const core::GameData* game_data_{nullptr};

    // 视口原点（client 空间，证据 FUN_0041be02: 0xe,0x20）。
    int32_t grid_origin_x_{14};
    int32_t grid_origin_y_{32};
    int32_t cell_size_{48};

    // MAP.DAT 瓦片地图（整图预烘焙为一张纹理，每帧 draw_texture_sub 取视口）
    core::FightMap map_;
    core::BlkGroupData blk_;
    platform::TextureHandle map_texture_{0};
    int32_t map_tex_w_{0};
    int32_t map_tex_h_{0};
    bool map_dirty_{true};
    int32_t cam_x_{0};   // render() 时从 state 同步，供无 state 参数的绘制函数用
    int32_t cam_y_{0};

    // 精灵 idle 帧翻转：原版用 SetTimer(...,0x37=55ms) 驱动 DAT_007d5114 每 55ms 翻转一次
    // （证据 FUN_004249a2.c: SetTimer(DAT_007db4ec,1,0x37,0) 置 DAT_007d5114=1）。
    // 故 idle 帧必须按 55ms 时间基准翻转，绝不能每帧翻转（那会是 ~60fps 闪烁）。
    int32_t idle_frame_{0};                       // 当前 idle 帧 0/1
    int64_t frame_acc_ms_{0};                     // 累积毫秒，达 55ms 翻一帧
    std::chrono::steady_clock::time_point last_frame_time_{};

    // 精灵/头像纹理缓存（键 = entry*FRAME_COUNT+frame 等）
    std::unordered_map<int32_t, platform::TextureHandle> army_tex_cache_;
    std::unordered_map<int32_t, platform::TextureHandle> face_tex_cache_;

    bool world_to_screen(int32_t gx, int32_t gy, int32_t& sx, int32_t& sy) const;
    void draw_map(const logic::BattleState& state);
    void draw_kuang_frame();
    void draw_icon_bar();
    void draw_minimap(const logic::BattleState& state);
    void draw_mines(const logic::BattleState& state);
    void draw_fighters(const logic::BattleState& state);
    void draw_status_effects(const logic::BattleState& state);
    void draw_info_panel(const logic::BattleState& state, int32_t fighter_idx);

    platform::TextureHandle get_army_texture(int32_t entry, int32_t frame);
    platform::TextureHandle get_face_texture(int32_t face_idx);

    const char* get_fighter_name(const logic::RuntimeFighter& fighter) const;
    const char* get_class_name(const logic::RuntimeFighter& fighter) const;

    // 在 (x,y) 用面板文字色绘制一个整数（与 GBK 标签分两次绘制，规避 GBK/ASCII 混排）。
    void draw_int(int32_t x, int32_t y, int32_t value,
                  uint8_t r, uint8_t g, uint8_t b);
};

}  // namespace dlzr::presentation
