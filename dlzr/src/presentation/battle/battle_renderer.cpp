// battle_renderer.cpp — 战斗场景渲染器实现
// 战斗背景 = MAP.DAT 瓦片地图；HUD 全部使用原版资源（KUANG04/BUTTON00/ARMY/FACE）。
// 证据见 battle_renderer.hpp 顶部注释（来源 output/ghidra_all/ 反编译）。

#include "presentation/battle/battle_renderer.hpp"
#include "core/data_model/palette.hpp"
#include "logic/combat/combat_formulas.hpp"

#include <cstdio>
#include <chrono>

namespace dlzr::presentation {

void BattleRenderer::init(platform::Platform& platform, FontRenderer& font,
                           ImageCache& images, app::ArmyLoader& army,
                           app::FaceLoader& face) {
    platform_ = &platform;
    font_ = &font;
    images_ = &images;
    army_loader_ = &army;
    face_loader_ = &face;
}

void BattleRenderer::set_battle_map(const core::FightMap& map,
                                     const core::BlkGroupData& blk) {
    map_ = map;
    blk_ = blk;
    map_dirty_ = true;
}

bool BattleRenderer::world_to_screen(int32_t gx, int32_t gy,
                                      int32_t& sx, int32_t& sy) const {
    // 证据: FUN_0041be02.c — cell = (mouse_x - 14)/48; 像素原点 = (14,32)。
    sx = (gx - cam_x_) * cell_size_ + grid_origin_x_;
    sy = (gy - cam_y_) * cell_size_ + grid_origin_y_;
    const int32_t vw = 9 * cell_size_, vh = 9 * cell_size_;
    return sx >= grid_origin_x_ && sx < grid_origin_x_ + vw &&
           sy >= grid_origin_y_ && sy < grid_origin_y_ + vh;
}

void BattleRenderer::render(const logic::BattleState& state,
                             const core::FightData& fight_data,
                             const core::GameData& game_data) {
    if (!platform_) return;

    // 与输入处理器共用同一套屏幕映射（state 已由调用方设为 (14,32,48)）。
    grid_origin_x_ = state.grid_origin_x;
    grid_origin_y_ = state.grid_origin_y;
    cell_size_ = state.cell_size;
    cam_x_ = state.cam_x;
    cam_y_ = state.cam_y;

    fight_data_ = &fight_data;
    game_data_ = &game_data;

    // 每帧重建：main 每帧 clear 全屏。
    platform_->fill_rect(0, 0, 640, 480, 0, 0, 0);

    // 1) 全屏边框 KUANG04（画在最底；地图/面板/小地图随后覆盖其透明孔位）。
    draw_kuang_frame();
    // 2) 战斗地图视口 (14,32,432,432)。
    draw_map(state);
    // 3) 地雷标记。
    draw_mines(state);
    // 4) 单位精灵（ARMY.DAT）。
    draw_fighters(state);
    // 5) 状态特效图标。
    draw_status_effects(state);
    // 6) 玩家选中单位：可达区域 + 选中框。
    if (state.selected_fighter_idx >= 0 &&
        state.selected_fighter_idx < logic::BattleState::MAX_FIGHTERS &&
        state.fighters[state.selected_fighter_idx].is_active) {
        render_movement_range(state);
        const auto& sf = state.fighters[state.selected_fighter_idx];
        render_selected_highlight(sf.grid_x, sf.grid_y);
    }
    // 7) 小地图（右侧）。
    draw_minimap(state);
    // 8) 顶部图标条（覆盖在边框之上）。
    draw_icon_bar();
    // 9) 悬停/选中单位信息面板（右侧）。
    int32_t panel_idx = -1;
    if (state.selected_fighter_idx >= 0 &&
        state.selected_fighter_idx < logic::BattleState::MAX_FIGHTERS &&
        state.fighters[state.selected_fighter_idx].is_active) {
        panel_idx = state.selected_fighter_idx;
    } else if (state.current_fighter_idx >= 0 &&
               state.current_fighter_idx < logic::BattleState::MAX_FIGHTERS &&
               state.fighters[state.current_fighter_idx].is_active) {
        panel_idx = state.current_fighter_idx;
    }
    if (panel_idx >= 0) {
        draw_info_panel(state, panel_idx);
    }

    // idle 帧翻转逻辑已移除（原 SE 依据有误）。
    // 证据 FUN_004170aa.c:66：idle 单位显示的是"朝向 % 2"决定的固定帧，
    //   不存在 55ms 周期翻转；此前对 FUN_004249a2.c 的 SetTimer(0x37) 解读
    //   误用于 idle，造成全员高频切帧闪烁。SetTimer 实际驱动的是移动/攻击
    //   动画计数（move/attack 分支），与静止单位无关。
}

void BattleRenderer::draw_map(const logic::BattleState& state) {
    if (map_.width <= 0 || map_.height <= 0 || blk_.tiles.empty()) return;
    constexpr int32_t TS = 48;

    if (map_dirty_) {
        map_tex_w_ = map_.width * TS;
        map_tex_h_ = map_.height * TS;
        std::vector<uint8_t> rgba(
            static_cast<size_t>(map_tex_w_) * map_tex_h_ * 4, 0);
        for (int32_t y = 0; y < map_.height; ++y) {
            for (int32_t x = 0; x < map_.width; ++x) {
                const uint16_t tv =
                    map_.tiles[static_cast<size_t>(y) * map_.width + x];
                const uint32_t tile_idx = tv & 0x0FFF;  // 低12位瓦片号
                if (tile_idx >= blk_.tiles.size()) continue;
                const auto& tile = blk_.tiles[tile_idx];
                uint8_t* dst = rgba.data() +
                    (static_cast<size_t>(y) * TS * map_tex_w_ + x * TS) * 4;
                for (int32_t py = 0; py < TS; ++py) {
                    uint8_t* row = dst + static_cast<size_t>(py) * map_tex_w_ * 4;
                    const uint8_t* src =
                        tile.data() + static_cast<size_t>(py) * TS;
                    for (int32_t px = 0; px < TS; ++px) {
                        const uint32_t pi = src[px] * 4;
                        row[px * 4 + 0] = core::kOriginalPalette[pi + 0];
                        row[px * 4 + 1] = core::kOriginalPalette[pi + 1];
                        row[px * 4 + 2] = core::kOriginalPalette[pi + 2];
                        row[px * 4 + 3] = core::kOriginalPalette[pi + 3];
                    }
                }
            }
        }
        if (map_texture_ != 0) platform_->destroy_texture(map_texture_);
        map_texture_ = platform_->create_texture(map_tex_w_, map_tex_h_);
        platform_->update_texture(map_texture_, rgba.data(), map_tex_w_ * 4);
        map_dirty_ = false;
    }

    // 视口 9×9 格（证据 FUN_00417547: 普通模式 local_14=9, local_8=9）。
    int32_t src_w = 9 * TS, src_h = 9 * TS;
    int32_t sx = cam_x_ * TS, sy = cam_y_ * TS;
    if (sx + src_w > map_tex_w_) src_w = map_tex_w_ - sx;
    if (sy + src_h > map_tex_h_) src_h = map_tex_h_ - sy;
    if (src_w <= 0 || src_h <= 0) return;
    platform_->draw_texture_sub(map_texture_, sx, sy, src_w, src_h,
                                grid_origin_x_, grid_origin_y_);
}

void BattleRenderer::draw_kuang_frame() {
    // 全屏边框（640×480）。证据 FUN_0041b698.c:14。
    images_->draw_image("KUANG04.IMG", 0, 0);
}

void BattleRenderer::draw_icon_bar() {
    // BUTTON00.TGA: 28×312 = 13 行图标，每行 28×24。证据 FUN_0041b698.c:15-37 + FUN_00424847.c:18。
    static constexpr int32_t kIconX[13] = {
        88, 116, 144, 172, 200, 228, 256, 284, 312, 340, 500, 528, 556};
    for (int32_t i = 0; i < 13; ++i) {
        // 源 = 第 i 行 (0, i*24, 28, 24)，画到 (x, 4)。
        images_->draw_image("BUTTON00.TGA", kIconX[i], 4, 0, i * 24, 28, 24);
    }
}

void BattleRenderer::draw_minimap(const logic::BattleState& state) {
    const int32_t w = state.map_width;
    const int32_t h = state.map_height;
    if (w <= 0 || h <= 0) return;

    // 小地图格 = min(clamp(168/w), clamp(212/h)), clamp(v)=(v>=3?v:2)。
    // 证据 FUN_00416e3c.c。
    auto clamp = [](int32_t v) { return v < 3 ? 2 : v; };
    int32_t cw = clamp(168 / w);
    int32_t ch = clamp(212 / h);
    int32_t cell = (cw < ch) ? cw : ch;
    int32_t mm_w = cell * w;
    int32_t mm_h = cell * h;
    int32_t mm_x = 459 + (168 - mm_w) / 2;   // 0x1cb = 459
    int32_t mm_y = 252 + (212 - mm_h) / 2;   // 0xfc = 252

    // 地形：原版用预渲染位图 DAT_005701f8（整张地图缩印）。克隆复用已烘焙的
    // map_texture_（kOriginalPalette 瓦片），整张缩印到小地图框，观感等价。
    // 证据 FUN_00417b47.c:18（draw_texture_rect 缩放 blit）。
    if (map_texture_ != 0) {
        platform_->draw_texture_rect(map_texture_, mm_x, mm_y, mm_w, mm_h);
    } else {
        platform_->fill_rect(mm_x, mm_y, mm_w, mm_h,
                             core::kOriginalPalette[199 * 4 + 0],
                             core::kOriginalPalette[199 * 4 + 1],
                             core::kOriginalPalette[199 * 4 + 2]);
    }

    // 单位点：我方 0xb7=(252,0,0) 红；敌方 0xbf=(252,140,0) 橙。
    // 证据 FUN_00417b47.c 点色（截图 2× 像素采样实证：我方红 / 敌方橙）。
    for (int i = 0; i < logic::BattleState::MAX_FIGHTERS; ++i) {
        const auto& f = state.fighters[i];
        if (!f.is_active || f.stats.current_hp <= 0) continue;
        int32_t px = mm_x + f.grid_x * cell;
        int32_t py = mm_y + f.grid_y * cell;
        if (f.team == 0) {
            platform_->fill_rect(px, py, cell, cell, 252, 0, 0);
        } else {
            platform_->fill_rect(px, py, cell, cell, 252, 140, 0);
        }
    }

    // 视野框（9×9 视口），色 0x48=(164,252,252) 亮青。证据 FUN_00417b47.c:60-64。
    int32_t vx = mm_x + cam_x_ * cell;
    int32_t vy = mm_y + cam_y_ * cell;
    platform_->draw_rect(vx, vy, cell * 9, cell * 9, 164, 252, 252);
}

void BattleRenderer::draw_mines(const logic::BattleState& state) {
    if (!game_data_) return;

    for (size_t idx = 0; idx < state.grid_mines.size(); ++idx) {
        const auto& mine = state.grid_mines[idx];
        if (mine.mine_type == 0) continue;
        if (mine.owner_team != 0) continue;

        const int32_t col = static_cast<int32_t>(idx) % state.map_width;
        const int32_t row = static_cast<int32_t>(idx) / state.map_width;
        int32_t x, y;
        if (!world_to_screen(col, row, x, y)) continue;

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

platform::TextureHandle BattleRenderer::get_army_texture(int32_t entry,
                                                          int32_t frame) {
    if (!army_loader_ || !platform_) return 0;
    int32_t key = entry * app::ArmyLoader::FRAME_COUNT + frame;
    auto it = army_tex_cache_.find(key);
    if (it != army_tex_cache_.end()) return it->second;

    platform::TextureHandle tex = 0;
    const uint8_t* rgba = army_loader_->get_frame_rgba(entry, frame);
    if (rgba) {
        tex = platform_->create_texture(app::ArmyLoader::ICON_WIDTH,
                                        app::ArmyLoader::ICON_HEIGHT);
        if (tex) {
            platform_->update_texture(tex, rgba,
                                      app::ArmyLoader::ICON_WIDTH * 4);
        }
    }
    army_tex_cache_[key] = tex;
    return tex;
}

platform::TextureHandle BattleRenderer::get_face_texture(int32_t face_idx) {
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

void BattleRenderer::draw_fighters(const logic::BattleState& state) {
    for (int i = 0; i < logic::BattleState::MAX_FIGHTERS; ++i) {
        const auto& fighter = state.fighters[i];
        if (!fighter.is_active || fighter.stats.current_hp <= 0) continue;

        int32_t x, y;
        if (!world_to_screen(fighter.grid_x, fighter.grid_y, x, y)) continue;

        // 一格只画一个单位（下标最小者胜出）。
        // 证据 FUN_004170aa.c:51-70：精灵是按**格子**取的——
        //   uVar4 = (&DAT_0057bf10)[cell*2]（该格的单位 id），再据此取
        //   DAT_007db4f8[uVar4] 精灵。占格表里一格只存一个单位 id。
        // 实测 fight0 槽位 45(碉堡)/98(吕正操)/99(黄敬) 坐标同为 (0,0)，
        //   原版只画一个（截图左上角正是碉堡），这解释了"吕正操没有登场"。
        //   此前克隆对所有单位逐个绘制，会把三者叠在一起。
        bool duplicate_cell = false;
        for (int j = 0; j < i; ++j) {
            const auto& prev = state.fighters[j];
            if (!prev.is_active || prev.stats.current_hp <= 0) continue;
            if (prev.grid_x == fighter.grid_x && prev.grid_y == fighter.grid_y) {
                // 用 goto/标志位代替 continue（本处在内层 for 中）
                duplicate_cell = true;
                break;
            }
        }
        if (duplicate_cell) continue;

        // 单位精灵 = ARMY.DAT[ class_types[class_type].img_index ]。
        int32_t entry = 0;
        if (game_data_ && fighter.class_type >= 0 &&
            fighter.class_type < core::MAX_CLASS_TYPES) {
            entry = game_data_->class_types[fighter.class_type].img_index;
        }
        // idle 精灵帧 = 朝向 % 2（稳态，不随时翻转）。
        // 证据 FUN_004170aa.c:66：idle 分支 local_908 = *(uint*)(&DAT_0096abb8 + unit*0x40) % 2；
        //   同一字段在 move/attack 分支被当作 dir（%4），故该字段即单位朝向。
        //   ⇒ 原版 idle 单位显示的是由其朝向决定的固定帧，并非每 55ms 切换的动画。
        //   此前按 55ms 时间累积翻转 idle_frame_，导致"所有角色高频切帧闪烁"。
        int32_t frame = ((fighter.direction % 2) + 2) % 2;   // 归一化到 0/1
        platform::TextureHandle tex = (entry >= 0 && entry < app::ArmyLoader::ENTRY_COUNT)
                                          ? get_army_texture(entry, frame) : 0;
        if (tex) {
            platform_->draw_texture(tex, x, y);
        } else {
            // 资源缺失兜底：用方块代替（不应走到）。
            uint8_t r = (fighter.team == 0) ? 64 : 192;
            uint8_t g = (fighter.team == 0) ? 128 : 64;
            uint8_t b = (fighter.team == 0) ? 192 : 64;
            platform_->fill_rect(x + 4, y + 4, cell_size_ - 8, cell_size_ - 8, r, g, b);
        }

        if (i == state.current_fighter_idx) {
            platform_->draw_rect(x - 2, y - 2, cell_size_ + 4, cell_size_ + 4, 255, 255, 0);
        }
    }
}

void BattleRenderer::draw_status_effects(const logic::BattleState& state) {
    for (int i = 0; i < logic::BattleState::MAX_FIGHTERS; ++i) {
        const auto& fighter = state.fighters[i];
        if (!fighter.is_active || fighter.stats.current_hp <= 0) continue;

        int16_t flags = fighter.stats.status_flag;
        if (flags == 0) continue;

        int32_t x, y;
        if (!world_to_screen(fighter.grid_x, fighter.grid_y, x, y)) continue;

        int32_t icon_size = 6;
        int32_t ix = x + cell_size_ - icon_size - 1;
        int32_t iy = y + 1;

        struct StatusIcon {
            int bit;
            uint8_t r, g, b;
        };
        static constexpr StatusIcon icons[] = {
            {0, 0,   200, 0  },   // 施毒 poison — green
            {1, 200, 0,   0  },   // 破坏 defense down — red
            {2, 200, 200, 0  },   // 骚扰 harassment — yellow
            {3, 160, 0,   200},   // 威慑 intimidation — purple
            {4, 0,   180, 200},   // 激励 motivate buff — cyan
            {5, 200, 128, 0  },   // 督战 supervise — orange
        };

        int drawn = 0;
        for (const auto& icon : icons) {
            if (!(flags & (1 << icon.bit))) continue;
            int32_t dx = ix - drawn * (icon_size + 1);
            platform_->fill_rect(dx, iy, icon_size, icon_size,
                                 icon.r, icon.g, icon.b);
            platform_->draw_rect(dx, iy, icon_size, icon_size,
                                 255, 255, 255);
            ++drawn;
        }
    }
}

void BattleRenderer::draw_info_panel(const logic::BattleState& state,
                                      int32_t fighter_idx) {
    const auto& f = state.fighters[fighter_idx];

    // 面板底 (459,32,168,207) 色板199=(32,68,0) 深绿。证据 FUN_0041be02.c:39,64。
    platform_->fill_rect(459, 32, 168, 207,
                         core::kOriginalPalette[199 * 4 + 0],
                         core::kOriginalPalette[199 * 4 + 1],
                         core::kOriginalPalette[199 * 4 + 2]);

    // 头像 80×100 @ (464,38)。证据 FUN_0041be02.c:73。
    int32_t face_idx = -1;
    if (fight_data_ && f.people_idx >= 0 && f.people_idx < 128) {
        face_idx = fight_data_->people[f.people_idx].face;
    }
    platform::TextureHandle ftex = get_face_texture(face_idx);
    if (ftex) {
        platform_->draw_texture(ftex, 464, 38);
    }

    // 面板文字色 = 调色板 0x48=(164,252,252) 亮青。证据 FUN_0041be02.c:70。
    const uint8_t TR = core::kOriginalPalette[72 * 4 + 0];
    const uint8_t TG = core::kOriginalPalette[72 * 4 + 1];
    const uint8_t TB = core::kOriginalPalette[72 * 4 + 2];

    // 姓名（头像右侧）。证据 FUN_0041be02.c:75 @ (548,38)。
    const char* name = get_fighter_name(f);
    if (name && name[0]) {
        font_->draw_text(548, 38, name, TR, TG, TB);
    }

    // 武力 / 智力（标签 + 数值分列）。证据 FUN_0041be02.c:76-82 @ (548,58)/(548,78)。
    font_->draw_text(548, 58, "\xCE\xE4\xC1\xA6", TR, TG, TB);   // 武力
    draw_int(592, 58, f.strength, TR, TG, TB);
    font_->draw_text(548, 78, "\xD6\xC7\xC1\xA6", TR, TG, TB);   // 智力
    draw_int(592, 78, f.intelligence, TR, TG, TB);

    // 兵种名（无独立标签，直接显示兵种名）。证据 FUN_0041be02.c:80 @ (548,98)。
    const char* cls = get_class_name(f);
    if (cls && cls[0]) {
        font_->draw_text(548, 98, cls, TR, TG, TB);
    }

    // 等级 / 经验。证据 FUN_0041be02.c:81-84 @ (548,118)/(548,138)。
    font_->draw_text(548, 118, "\xB5\xC8\xBC\xB6", TR, TG, TB);  // 等级
    draw_int(592, 118, f.level, TR, TG, TB);
    font_->draw_text(548, 138, "\xBE\xAD\xD1\xE9", TR, TG, TB);  // 经验
    int32_t exp_val = 0;
    if (fight_data_ && f.people_idx >= 0 && f.people_idx < 128) {
        exp_val = fight_data_->people[f.people_idx].experience;
    }
    draw_int(592, 138, exp_val, TR, TG, TB);

    // HP 白条底 (464,142,80,4) + 三档彩色填充。证据 FUN_0041be02.c:96,114。
    platform_->fill_rect(464, 142, 80, 4, 255, 255, 255);
    int32_t hpw = 0;
    uint8_t hr = core::kOriginalPalette[0x85 * 4 + 0];   // 绿 pal[0x85]
    uint8_t hg = core::kOriginalPalette[0x85 * 4 + 1];
    uint8_t hb = core::kOriginalPalette[0x85 * 4 + 2];
    if (f.stats.max_hp > 0) {
        int32_t ratio = (f.stats.current_hp * 100) / f.stats.max_hp;
        hpw = (f.stats.current_hp * 80) / f.stats.max_hp;
        if (hpw < 0) hpw = 0;
        if (hpw > 80) hpw = 80;
        // 三档: >66% 绿(0x85), >33% 黄(0x8a), 否则红(0xb7)。证据 FUN_0041be02.c:100-111。
        if (ratio > 66) {
            hr = core::kOriginalPalette[0x85 * 4 + 0];
            hg = core::kOriginalPalette[0x85 * 4 + 1];
            hb = core::kOriginalPalette[0x85 * 4 + 2];
        } else if (ratio > 33) {
            hr = core::kOriginalPalette[0x8a * 4 + 0];
            hg = core::kOriginalPalette[0x8a * 4 + 1];
            hb = core::kOriginalPalette[0x8a * 4 + 2];
        } else {
            hr = core::kOriginalPalette[0xb7 * 4 + 0];
            hg = core::kOriginalPalette[0xb7 * 4 + 1];
            hb = core::kOriginalPalette[0xb7 * 4 + 2];
        }
    }
    platform_->fill_rect(464, 142, hpw, 4, hr, hg, hb);

    // 攻击 / 防御（原版按当前单位战斗属性**计算**的值，非兵种基础值）。
    // 证据 FUN_0041be02.c:85-90：攻击 = FUN_0042a82a(DAT_00473008)，防御 = FUN_0042a98e(DAT_00473008)，
    // 二者均以 武力/等级/兵种/技能等级/装备加成/士气/地形 实时计算（含 DAT_007db4e8==0x96 的战斗模式分支）。
    // 与 apply_damage 实际伤害结算使用同一公式（calc_attack_power / calc_defense），保证"看到=打到"。
    int32_t atk = 0, def = 0;
    if (game_data_ && f.class_type >= 0 && f.class_type < core::MAX_CLASS_TYPES) {
        core::People panel_person{};
        panel_person.level = f.level;
        panel_person.class_type = f.class_type;
        panel_person.strength = f.strength;
        panel_person.intelligence = f.intelligence;
        panel_person.items = f.items;

        const auto& panel_class = game_data_->class_types[f.class_type];
        const core::Fighter& panel_stats = f.stats;
        int32_t skill_lv = logic::get_skill_level(f.skill_attack);
        int32_t item_atk = logic::get_item_attack_bonus(panel_person, *game_data_);
        int32_t item_def = logic::get_item_defense_bonus(panel_person, *game_data_);

        // 地形防御加成取单位当前所处格地形（证据 FUN_0042a98e.c:14,21：FUN_0042a95b 由坐标取地形）。
        int32_t terrain_type = 0;
        int32_t tcell = f.grid_y * state.map_width + f.grid_x;
        if (tcell >= 0 && tcell < static_cast<int32_t>(state.grid_terrain.size())) {
            terrain_type = state.grid_terrain[tcell];
        }
        int32_t terrain_bonus = logic::get_terrain_defense_bonus(terrain_type);

        atk = logic::calc_attack_power(panel_person, panel_class, panel_stats,
                                       skill_lv, true, item_atk);
        def = logic::calc_defense(panel_person, panel_class, panel_stats,
                                  terrain_bonus, true, item_def);
    }
    font_->draw_text(464, 160, "\xB9\xA5\xBB\xF7", TR, TG, TB);   // 攻击
    draw_int(512, 160, atk, TR, TG, TB);
    // 防御：GBK = 防 B7 C0 + 御 D3 F9。此前沿用 D3 A6（碍），面板误显"防碍"。
    font_->draw_text(548, 160, "\xB7\xC0\xD3\xF9", TR, TG, TB);   // 防御
    draw_int(592, 160, def, TR, TG, TB);

    // 体力 cur/max。证据 FUN_0041be02.c:91-95 @ (464,180)。
    font_->draw_text(464, 180, "\xCC\xE5\xC1\xA6", TR, TG, TB);  // 体力
    draw_int(512, 180, f.stats.current_hp, TR, TG, TB);
    font_->draw_text(544, 180, "/", TR, TG, TB);
    draw_int(560, 180, f.stats.max_hp, TR, TG, TB);

    // 士气 cur/100。证据 FUN_0041be02.c:115-118 @ (464,200)。
    // 士气：GBK = 士 CA BF + 气 C6 F8。此前沿用 CA B0（拾），面板误显"拾气"。
    font_->draw_text(464, 200, "\xCA\xBF\xC6\xF8", TR, TG, TB);  // 士气
    draw_int(512, 200, f.stats.morale, TR, TG, TB);
    font_->draw_text(544, 200, "/", TR, TG, TB);
    draw_int(560, 200, 100, TR, TG, TB);

    // 敌方红标"敌"。证据 FUN_0041be02.c:67-71 @ (464,220)。
    if (f.team != 0) {
        font_->draw_text(464, 220, "\xB5\xD0", 252, 0, 0);   // 敌（红）
    }

    // 回合（当前战斗回合数）。证据 FUN_0041be02.c:65-66（单位信息面板分支，
    //   无论我方/敌方均在此绘制）：标签 DAT_00446f4c="回合 " @ (0x224,0xdc)=(548,220)，
    //   值 DAT_00475aa4 @ (0x254,0xdc)=(596,220)。此前错画在 (564,220)/(608,220)，偏移 16px。
    font_->draw_text(548, 220, "\xBB\xD8\xBA\xCF", TR, TG, TB);   // 回合
    draw_int(596, 220, state.battle_round + 1, TR, TG, TB);
}

void BattleRenderer::draw_int(int32_t x, int32_t y, int32_t value,
                              uint8_t r, uint8_t g, uint8_t b) {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%d", value);
    font_->draw_text(x, y, buf, r, g, b);
}

const char* BattleRenderer::get_fighter_name(const logic::RuntimeFighter& fighter) const {
    if (!fight_data_) return "";
    int32_t pidx = fighter.people_idx;
    if (pidx >= 0 && pidx < 128) {
        return fight_data_->people[pidx].name.data();
    }
    return "";
}

const char* BattleRenderer::get_class_name(const logic::RuntimeFighter& fighter) const {
    if (!fight_data_ || !game_data_) return "";
    int32_t pidx = fighter.people_idx;
    if (pidx >= 0 && pidx < 128) {
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
    int32_t x, y;
    if (!world_to_screen(grid_x, grid_y, x, y)) return;
    platform_->draw_rect(x - 1, y - 1, cell_size_ + 2, cell_size_ + 2, 255, 255, 0);
    platform_->draw_rect(x - 3, y - 3, cell_size_ + 6, cell_size_ + 6, 200, 200, 0);
}

void BattleRenderer::render_movement_range(const logic::BattleState& state) {
    for (int row = 0; row < state.map_height; ++row) {
        for (int col = 0; col < state.map_width; ++col) {
            int32_t idx = row * state.map_width + col;
            if (idx < 0 || idx >= static_cast<int32_t>(state.movement_mask.size())) continue;
            if (!state.movement_mask[idx]) continue;

            int32_t x, y;
            if (!world_to_screen(col, row, x, y)) continue;
            platform_->draw_rect(x + 1, y + 1, cell_size_ - 2, cell_size_ - 2, 0, 170, 255);
        }
    }
}

void BattleRenderer::render_status_text(const char* text) {
    platform_->fill_rect(0, 380, 640, 20, 20, 20, 50);
    font_->draw_text(10, 384, text, 255, 255, 200);
}

}  // namespace dlzr::presentation
