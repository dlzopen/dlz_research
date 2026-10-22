// battle_engine.cpp — 战斗引擎实现
// Battle engine implementation

#include "logic/battle/battle_engine.hpp"

#include <algorithm>
#include <climits>
#include <cstring>
#include <queue>

namespace dlzr::logic {

void BattleEngine::init(core::FightData& fight, core::GameData& game,
                         int32_t difficulty_level) {
    fight_ = &fight;
    game_ = &game;
    difficulty_level_ = difficulty_level;
    state_ = BattleState{};

    // 主角(大队长) = fight.people[0]（战斗伤害/技能/结局判定均读该条目）。
    // 应用角色选择界面的自定义：头像→face、阵型→class_type、滑块→strength/intelligence。
    // 原版确认路径(FUN_0042e2d6 的 CONFIRM 分支)：
    //   People[0].face        = 头像选择 + 0x67 (= 103 + portrait)
    //   People[0].class_type  = 阵型 → 兵种索引 {1,4,8,11}  (DAT_007d0f24)
    //   People[0].strength    = 武力滑块值  (DAT_007d0f14)
    //   People[0].intelligence= 160 - 武力  (DAT_007d0f18 = 0xa0 - 武力)
    if (game.protagonist_face >= 0 && fight.people.size() > 0) {
        auto& hero = fight.people[0];
        hero.face = game.protagonist_face;
        hero.class_type = game.protagonist_class_type;
        hero.strength = game.protagonist_strength;
        hero.intelligence = game.protagonist_intelligence;
        // 主角等级：原版 CONFIRM 分支(FUN_0042e2d6.c:245-298)只写 face/class/strength/
        // intel/weapon/name，**从不写 level**（已逐行核对，无 DAT_007d0f24 之外的 level 写入）。
        // 故等级继承战役主角名册记录 game.people[0]（孙悟空，PEOPLE.DAT[0].level=1），
        // 而非自创的 protagonist_level 占位（默认10）。
        hero.level = game.people[0].level;
        // 主角名用角色选择界面定制名（默认"主角"），避免沿用 FIGHT.DAT people[0] 的"伪军"。
        std::strncpy(hero.name.data(), game.protagonist_name.c_str(),
                     hero.name.size() - 1);
        hero.name.back() = '\0';

        // ===================================================================
        // 主角起始武器（证据：FUN_0042e2d6.c:257-269，DAT_007d0f24=兵种, DAT_007d0f6e=武器ID）
        // 原版按兵种给武器，写入 People[0].wupin[2]
        // （People 基址 0x7D0EF8；wupin[8] 占 +0x74..+0x7B ⇒ 0x7D0F6E = wupin[2]）：
        //   class 1(手枪队) → 物品 100 转轮式手枪   （用户确认；原误标"勃郎宁手枪"）
        //   class 4(大刀队) → 物品  82 钢刀          （原误标"大刀"）
        //   class 8(步枪队) → 物品 109 老套筒         （原误标"汉阳造八八式"）
        //   其他(骑兵队 11)  → 物品  82 钢刀
        // 物品名实测自 GAME.DAT WuPin @0x43E0（72B/条）：[99]=id100 转轮式手枪，
        //   [81]=id82 钢刀，[108]=id109 老套筒。ID 本身自初版即正确，此前仅注释名写错。
        // ===================================================================
        const int32_t cls = game.protagonist_class_type;
        int32_t weapon = 82;                        // 默认：钢刀
        if (cls == 1)      weapon = 100;            // 手枪队 → 转轮式手枪
        else if (cls == 8) weapon = 109;            // 步枪队 → 老套筒
        hero.items[2] = static_cast<uint8_t>(weapon);
        hero.items[0] = 1;                          // 核桃（物品 1，补给品）

        // ===================================================================
        // 主角技能按兵种（证据：GAME.DAT ClassType.skill_flags @ +0x90，表 @0x0320，96×160B）
        // SkillFlags 16 项顺序 = FightPeople 的 skill_supply(+0x34) … skill_airraid(+0x70)。
        // 原版确认分支从 DAT_0096cc28[class*0xa0 + i] 填这 16 个计数器后，
        // 再强制 skill_supply=1、skill_motivate=1（FUN_0042e2d6.c:271-278）。
        // ⚠️ DAT_0096cc28 在 .data 的 BSS 区（RAW=0x43C00/RSZ=0x5200，VA 0x96cc28 在 RSZ 外），
        //    运行时填充、静态 dump 不到；故此处改用同为按兵种的 ClassType.skill_flags 作为数据源。
        // ===================================================================
        if (cls >= 0 && cls < core::MAX_CLASS_TYPES) {
            const auto& sf = game.class_types[cls].skill_flags;
            hero.skill_supply      = sf.buji;      // 补给
            hero.skill_motivate    = sf.jili;      // 激励
            hero.skill_medical     = sf.yiliao;    // 医疗
            hero.skill_fire        = sf.huogong;   // 火攻
            hero.skill_log         = sf.gunmu;     // 滚木
            hero.skill_rock        = sf.leishi;    // 擂石
            hero.skill_intimidate  = sf.weishe;    // 威慑
            hero.skill_harass      = sf.saorao;    // 骚扰
            hero.skill_sabotage    = sf.pohuai;    // 破坏
            hero.skill_plunder     = sf.lueduo;    // 掠夺
            hero.skill_counterspy  = sf.fanjian;   // 反间
            hero.skill_decoy       = sf.yibing;    // 疑兵
            hero.skill_supervise   = sf.duzhan;    // 督战
            hero.skill_assassinate = sf.ansha;     // 暗杀
            hero.skill_poison      = sf.shidu;     // 施毒
            hero.skill_airraid     = sf.kongxi;    // 空袭
        }
        // 原版确认分支强制前两项为 1（补给 / 激励）
        hero.skill_supply = 1;
        hero.skill_motivate = 1;
    }

    init_grid();
    init_fighters();

    state_.phase = BattlePhase::PLAYER_TURN;
    // 优先让玩家(team 0)先手：若首个活跃单位不是我方，则跳到我方单位。
    // Prefer the player to act first.
    int32_t first_player = -1;
    for (int i = 0; i < BattleState::MAX_FIGHTERS; ++i) {
        if (state_.fighters[i].is_active && is_fighter_alive(i) &&
            state_.fighters[i].team == 0) {
            first_player = i;
            break;
        }
    }
    state_.current_fighter_idx =
        (first_player >= 0) ? first_player : find_next_active_fighter(0);
}

void BattleEngine::init_grid() {
    // grid_param_x/y = 初始相机目标（视口中心），非网格尺寸。网格尺寸 = MAP.DAT 宽高。
    state_.cam_target_x = fight_->intro.grid_param_x;
    state_.cam_target_y = fight_->intro.grid_param_y;

    // ===================================================================
    // 战斗网格 = MAP.DAT 瓦片网格（证据: FUN_0041e97a.c 以 MAP.DAT 宽高
    // DAT_0057ba20/22 为相机钳制边界；FIGHT.DAT 单位 grid_x/grid_y 范围
    // 0-63/0-57 与地图尺寸上限一致）。
    // 相机初始化照搬 FUN_0041e97a 普通模式分支（视口 9×9, 4 格边距）:
    //   cam = clamp(target - 4, 0, map_dim - 9)
    // ===================================================================
    if (map_ && map_->width > 0 && map_->height > 0) {
        state_.map_width = map_->width;
        state_.map_height = map_->height;
    } else {
        // 无地图数据时的保守回退（不应用出现；保持可运行）
        state_.map_width = 20;
        state_.map_height = 20;
    }

    const int32_t w = state_.map_width, h = state_.map_height;
    const int32_t tx = state_.cam_target_x, ty = state_.cam_target_y;
    int32_t cx;
    if (tx < 5) cx = 0;
    else if (tx < w - 5) cx = tx - 4;
    else cx = w - 9;
    int32_t cy;
    if (ty < 5) cy = 0;
    else if (ty < h - 5) cy = ty - 4;
    else cy = h - 9;
    if (cx < 0) cx = 0;
    if (cy < 0) cy = 0;
    state_.cam_x = cx;
    state_.cam_y = cy;

    int32_t cells = state_.grid_cells();
    // TODO(RE): 逆向真实逐格地形来源。FIGHT.DAT intro.grid_terrain[92] 并非
    // MAP.DAT 网格的 1:1 地形，真实地形另有来源。当前暂置全可通行，保证
    // 第一关可玩，待逆向后接入。
    state_.grid_terrain.assign(cells, 0);
    state_.grid_mines.assign(cells, BattleState::PlacedMine{});
    state_.movement_mask.assign(cells, false);
    state_.attack_mask.assign(cells, false);
}

void BattleEngine::init_fighters() {
    state_.fighter_count = 0;

    // 难度系数 / Difficulty multiplier (percent, 100 = normal)
    int32_t diff_pct = 100;
    if (difficulty_level_ == 0) diff_pct = 80;
    else if (difficulty_level_ == 2) diff_pct = 130;

    // --- 敌方 / 原战斗名册单位（来自 FIGHT.DAT.fighters + fight.people）---
    // 【部署判据】只看 grid_x != 999，不看 stage。
    // 证据 FUN_0041b53c.c:19-23：
    //   for (local_8 = 0; local_8 < 0x80; local_8++)
    //     if ((&DAT_0096aba4)[local_8 * 0x10] != 999)
    //       FUN_0041b470(*(int*)(DAT_0096cba8 + (&DAT_007d0f24)[local_8*0x21] * 0xa0), local_8);
    //   · (&DAT_0096aba4)[unit*0x10] 为 int 下标 → 字节步长 = 0x10*4 = 64 = sizeof(FightFighter)，
    //     999 是 grid_x 哨兵（types.hpp: FightFighter.grid_x "999=未部署"），
    //     ⇒ 整句等价于 fighters[unit].grid_x != 999。
    //   · (&DAT_007d0f24)[unit*0x21] 同样为 int 下标 → 步长 0x21*4 = 132 = sizeof(FightPeople)，
    //     且该符号落在记录内 +0x2C ⇒ 读到的正是 FightPeople.class_type。
    //   · 循环内**完全没有 stage 判定**。
    // 此前额外要求 stage==1 属臆造条件，误丢 fight0 槽位 42/43/44
    //   （真实坐标 (18,8)/(17,8)/(18,9)、朝向 2=西、behavior=1 巡逻）。
    for (int i = 0; i < 128; ++i) {
        const auto& src = fight_->fighters[i];
        if (src.grid_x == 999) {
            continue;
        }
        // FIGHT.DAT 每关只有 88 名角色名册 (People[88])，而战斗部署位有 128 个。
        // 部署位 i >= 88 无对应角色数据，必须跳过，否则越界崩溃。
        if (i >= static_cast<int>(fight_->people.size())) {
            continue;
        }

        auto& rt = state_.fighters[i];
        rt.grid_x = src.grid_x;
        rt.grid_y = src.grid_y;
        rt.direction = src.direction;
        rt.is_active = true;   // grid_x != 999 即登场（见上方部署判据证据）
        rt.stamina = src.stamina;
        rt.movement = src.movement;
        rt.people_idx = i;

        const auto& fp = fight_->people[i];
        rt.team = fp.enemy;
        rt.class_type = fp.class_type;
        rt.level = fp.level;
        rt.strength = fp.strength;
        rt.intelligence = fp.intelligence;
        rt.skill_attack = fp.skill_attack;
        rt.items = fp.items;
        rt.mines = fp.mines;

        rt.stats.max_hp = src.stamina;
        rt.stats.current_hp = src.stamina;
        rt.stats.morale = src.morale;
        rt.stats.equip_integrity = 100;
        rt.stats.defense_modifier = src.stat3;

        if (rt.team != 0 && rt.is_active) {
            rt.stats.max_hp = src.stamina * diff_pct / 100;
            rt.stats.current_hp = rt.stats.max_hp;
            rt.stats.morale = src.morale * diff_pct / 100;
        }

        ++state_.fighter_count;
    }

    // --- 我方主角部署（证据: FIGHT.DAT fight0 实测 + confirm_to_battle_flow.md）---
    // 实测 FIGHT.DAT fight0: 全部 88 个 people[] 均为 enemy=1（敌军名册），
    //   仅 fighters[40]/[41] 为 stage=1 登场（敌方孙悟空 @ (6,17)/(7,17)）。
    // 玩家单位不在 FIGHT.DAT 内；主角来自战役大军（game.people[0]），
    //   由 FUN_0042e2d6 的 CONFIRM 分支把角色选择结果写入 fight.people[0]（大队长）。
    //   克隆 init() 已对 fight.people[0] 应用 face/class/strength/intel/level/name/武器/技能。
    // 部署坐标 = intro.grid_param（实测 fight0 = (9,25)），即相机中心 / 主角锚点。
    //   ※ fight.people[0] 的 FIGHT.DAT 磁盘值为"伪军"(enemy=1) 占位，运行时被主角覆盖；
    //     其 HP/体力用兵种 HP 公式（PEOPLE_DAT_FORMAT.md），不取磁盘 stamina。
    //   ※ 玩家大军其余成员（平阳游击队等）按原版属"战役招募"体系，当前克隆尚未建模，
    //     待 RE 接入；首关以主角单员登场（贴合"开局仅大队长"的数据事实），不臆造同伴。
    //   ※ 主角占"敌方循环结束后第一个空闲战斗位"，避免覆盖已部署的敌方单位
    //     （fight0 下即战斗位 0，因 fighters[0] 为 stage=0 预备）。
    {
        int32_t hero_slot = 0;
        for (int i = 0; i < BattleState::MAX_FIGHTERS; ++i) {
            if (!state_.fighters[i].is_active) { hero_slot = i; break; }
        }
        auto& rt = state_.fighters[hero_slot];
        rt.people_idx = 0;                            // 指向被覆盖为大队长的 fight.people[0]
        rt.team = 0;                                  // 我方
        rt.grid_x = fight_->intro.grid_param_x;      // 9
        rt.grid_y = fight_->intro.grid_param_y;      // 25
        // 朝向由槽位数据决定（用户明示 + FIGHT.DAT 实测）。
        // 证据：FightFighter.direction @+0x28（facing @+0x1C 的冗余拷贝），
        //   实测 fight0 fighters[0].direction=0(北)、fighters[40]/[41]=1(南)。
        //   此前硬编码 direction=1(南)，导致主角朝南；槽位 0 数据就是北。
        //   idle 精灵帧亦由朝向决定（FUN_004170aa.c:66 → frame = direction%2），
        //   故朝向同时影响贴图，必须取数据值。
        rt.direction = fight_->fighters[hero_slot].direction;
        rt.is_active = true;
        rt.has_acted = false;
        rt.stamina = 100;
        rt.movement = 100;

        const auto& hero = fight_->people[0];        // 已被 init() 覆盖为角色选择结果
        rt.class_type = hero.class_type;
        rt.level = hero.level;
        rt.strength = hero.strength;
        rt.intelligence = hero.intelligence;
        rt.skill_attack = hero.skill_attack;
        rt.items = hero.items;
        rt.mines = hero.mines;

        // HP 真实公式：max_hp = hp_base + (level-1)*hp_growth（PEOPLE_DAT_FORMAT.md）
        const auto& cls = get_class_type(hero.class_type);
        int32_t hp = cls.hp_base;
        if (hero.level > 1) hp += (hero.level - 1) * cls.hp_growth;
        if (hp <= 0) hp = 100;
        rt.stats.max_hp = hp;
        rt.stats.current_hp = hp;
        rt.stats.morale = 100;
        rt.stats.equip_integrity = 100;
        rt.stats.defense_modifier = 100;

        ++state_.fighter_count;
    }

    // 回合上限：原版公式待逆向；暂用较宽松值，确保战斗可推进到分出胜负。
    // Round limit: original formula pending RE. Generous value for now.
    state_.max_rounds = 50;
}

int32_t BattleEngine::find_next_active_fighter(int32_t from_idx) {
    for (int i = from_idx; i < BattleState::MAX_FIGHTERS; ++i) {
        if (state_.fighters[i].is_active && is_fighter_alive(i)) {
            return i;
        }
    }
    return -1;
}

bool BattleEngine::is_fighter_alive(int32_t fighter_idx) const {
    return state_.fighters[fighter_idx].is_active
        && state_.fighters[fighter_idx].stats.current_hp > 0;
}

int32_t BattleEngine::count_alive_on_team(int32_t team) const {
    int32_t count = 0;
    for (int i = 0; i < BattleState::MAX_FIGHTERS; ++i) {
        if (state_.fighters[i].team == team && is_fighter_alive(i)) {
            ++count;
        }
    }
    return count;
}

const core::FightPeople& BattleEngine::get_fight_people(int32_t fighter_idx) const {
    // 防御：运行时索引可能指向我方自动部署位(>=88)，超出 fight.people 范围。
    // 我方技能/属性已在 RuntimeFighter 上解耦；此处仅敌方路径有效。
    if (fighter_idx < 0 ||
        fighter_idx >= static_cast<int>(fight_->people.size())) {
        return fight_->people[0];
    }
    return fight_->people[fighter_idx];
}

const core::ClassType& BattleEngine::get_class_type(int32_t class_idx) const {
    if (class_idx >= 0 && class_idx < core::MAX_CLASS_TYPES) {
        return game_->class_types[class_idx];
    }
    return game_->class_types[0];
}

int32_t BattleEngine::get_movement_range(int32_t fighter_idx) const {
    if (fighter_idx < 0 || fighter_idx >= BattleState::MAX_FIGHTERS) return 0;
    const auto& fighter = state_.fighters[fighter_idx];
    if (!fighter.is_active) return 0;

    const auto& cls = get_class_type(fighter.class_type);
    int32_t base_range = cls.movement;
    if (base_range <= 0) base_range = 3;

    int32_t move_pct = fighter.movement;
    if (move_pct <= 0) move_pct = 100;

    return std::max(1, base_range * move_pct / 100);
}

void BattleEngine::set_selected_fighter(int32_t idx) {
    state_.selected_fighter_idx = idx;
    if (idx >= 0 && idx < BattleState::MAX_FIGHTERS && state_.fighters[idx].is_active) {
        calc_movement_range(idx);   // 写入 state_.movement_mask（可达集）
        calc_attack_range(idx);     // 写入 state_.attack_mask（攻击范围）
    } else {
        state_.movement_mask.assign(state_.grid_cells(), false);
        state_.attack_mask.assign(state_.grid_cells(), false);
    }
}

int32_t BattleEngine::get_attack_range(int32_t fighter_idx) const {
    if (fighter_idx < 0 || fighter_idx >= BattleState::MAX_FIGHTERS) return 1;
    const auto& fighter = state_.fighters[fighter_idx];
    if (!fighter.is_active) return 1;

    const auto& cls = get_class_type(fighter.class_type);
    int32_t range = cls.select_range;
    if (range <= 0) range = 1;
    if (range > 5) range = 5;
    return range;
}

void BattleEngine::calc_movement_range(int32_t fighter_idx) {
    state_.movement_mask.assign(state_.grid_cells(), false);

    if (fighter_idx < 0 || fighter_idx >= BattleState::MAX_FIGHTERS) return;
    const auto& fighter = state_.fighters[fighter_idx];
    if (!fighter.is_active) return;

    int32_t max_dist = get_movement_range(fighter_idx);
    int32_t start_x = fighter.grid_x;
    int32_t start_y = fighter.grid_y;

    struct Pos { int32_t x, y, dist; };
    std::queue<Pos> bfs;
    std::vector<bool> visited(state_.grid_cells(), false);

    int32_t start_cell = start_y * state_.map_width + start_x;
    if (start_cell < 0 || start_cell >= state_.grid_cells()) return;

    visited[start_cell] = true;
    bfs.push({start_x, start_y, 0});
    state_.movement_mask[start_cell] = true;

    constexpr int dx[] = {0, 0, -1, 1};
    constexpr int dy[] = {-1, 1, 0, 0};

    while (!bfs.empty()) {
        auto [cx, cy, cd] = bfs.front();
        bfs.pop();

        if (cd >= max_dist) continue;

        for (int d = 0; d < 4; ++d) {
            int32_t nx = cx + dx[d];
            int32_t ny = cy + dy[d];
            if (nx < 0 || nx >= state_.map_width) continue;
            if (ny < 0 || ny >= state_.map_height) continue;

            int32_t ncell = ny * state_.map_width + nx;
            if (visited[ncell]) continue;
            if (state_.grid_terrain[ncell] == 99) continue;

            bool blocked = false;
            for (int i = 0; i < BattleState::MAX_FIGHTERS; ++i) {
                if (i == fighter_idx) continue;
                const auto& f = state_.fighters[i];
                if (f.is_active && is_fighter_alive(i) &&
                    f.grid_x == nx && f.grid_y == ny) {
                    blocked = true;
                    break;
                }
            }
            if (blocked) continue;

            visited[ncell] = true;
            state_.movement_mask[ncell] = true;
            bfs.push({nx, ny, cd + 1});
        }
    }
}

void BattleEngine::calc_attack_range(int32_t fighter_idx) {
    state_.attack_mask.assign(state_.grid_cells(), false);

    if (fighter_idx < 0 || fighter_idx >= BattleState::MAX_FIGHTERS) return;
    const auto& fighter = state_.fighters[fighter_idx];
    if (!fighter.is_active) return;

    int32_t range = get_attack_range(fighter_idx);
    int32_t cx = fighter.grid_x;
    int32_t cy = fighter.grid_y;

    for (int y = 0; y < state_.map_height; ++y) {
        for (int x = 0; x < state_.map_width; ++x) {
            int32_t dx = x - cx;
            int32_t dy = y - cy;
            int32_t dist = dx * dx + dy * dy;
            if (dist > 0 && dist <= range * range) {
                state_.attack_mask[y * state_.map_width + x] = true;
            }
        }
    }
}

bool BattleEngine::is_valid_move_target(int32_t grid_x, int32_t grid_y) const {
    if (!state_.in_grid(grid_x, grid_y)) return false;
    int32_t cell = grid_y * state_.map_width + grid_x;
    return state_.movement_mask[cell];
}

bool BattleEngine::is_in_attack_range(int32_t attacker_idx,
                                       int32_t target_x, int32_t target_y) const {
    if (attacker_idx < 0 || attacker_idx >= BattleState::MAX_FIGHTERS) return false;
    const auto& fighter = state_.fighters[attacker_idx];
    int32_t range = get_attack_range(attacker_idx);
    int32_t dx = target_x - fighter.grid_x;
    int32_t dy = target_y - fighter.grid_y;
    return (dx * dx + dy * dy) <= range * range;
}

void BattleEngine::execute_command(const BattleCommand& cmd) {
    if (state_.is_over) return;

    switch (cmd.type) {
    case BattleCommand::MOVE:
        execute_move(cmd);
        break;
    case BattleCommand::ATTACK:
        execute_attack(cmd);
        break;
    case BattleCommand::SKILL:
        execute_skill(cmd);
        break;
    case BattleCommand::WAIT:
        execute_wait(cmd);
        break;
    case BattleCommand::PLACE_MINE:
        execute_place_mine(cmd);
        break;
    case BattleCommand::USE_ITEM:
        execute_use_item(cmd);
        break;
    default:
        break;
    }

    state_.fighters[cmd.fighter_idx].has_acted = true;
    check_victory_conditions();
}

void BattleEngine::execute_move(const BattleCommand& cmd) {
    auto& fighter = state_.fighters[cmd.fighter_idx];

    calc_movement_range(cmd.fighter_idx);
    if (!is_valid_move_target(cmd.target_x, cmd.target_y)) {
        return;
    }

    fighter.grid_x = cmd.target_x;
    fighter.grid_y = cmd.target_y;

    BattleEvent ev;
    ev.type = BattleEvent::FIGHTER_MOVED;
    ev.source_idx = cmd.fighter_idx;
    ev.target_x = cmd.target_x;
    ev.target_y = cmd.target_y;
    state_.push_event(ev);

    check_mine_trigger(cmd.fighter_idx, cmd.target_x, cmd.target_y);
}

void BattleEngine::execute_attack(const BattleCommand& cmd) {
    if (cmd.target_idx < 0 || cmd.target_idx >= BattleState::MAX_FIGHTERS) return;
    const auto& target = state_.fighters[cmd.target_idx];
    if (!is_in_attack_range(cmd.fighter_idx, target.grid_x, target.grid_y)) {
        return;
    }
    apply_damage(cmd.fighter_idx, cmd.target_idx);
}

void BattleEngine::execute_skill(const BattleCommand& cmd) {
    // 我方单位的技能数据已解耦到 RuntimeFighter；当前技能执行仅走敌方名册路径。
    // 玩家技能待接入后统一，这里对非敌方索引直接返回，避免越界/错配。
    if (cmd.fighter_idx < 0 ||
        cmd.fighter_idx >= static_cast<int>(fight_->people.size())) {
        return;
    }
    const auto& fp = get_fight_people(cmd.fighter_idx);

    core::People person{};
    person.strength = fp.strength;
    person.intelligence = fp.intelligence;
    person.level = fp.level;
    person.class_type = fp.class_type;

    int32_t skill_level = 0;
    if (cmd.skill_idx >= 0 && cmd.skill_idx < core::MAX_SKILLS) {
        const auto& skill = game_->skills[cmd.skill_idx];
        int32_t skill_exp = (&fp.skill_attack)[cmd.skill_idx];
        skill_level = get_skill_level(skill_exp);
        auto effect = calc_skill_effect(person, skill, skill_level);

        if (cmd.target_idx >= 0) {
            auto& target = state_.fighters[cmd.target_idx];

            if (effect.hp_effect > 0) {
                target.stats.current_hp -= effect.hp_effect;
            } else if (effect.hp_effect < 0) {
                target.stats.current_hp -= effect.hp_effect;

                if (target.team == 0 && game_) {
                    int32_t healer_pidx = state_.fighters[cmd.fighter_idx].people_idx;
                    int32_t target_pidx = state_.fighters[cmd.target_idx].people_idx;
                    if (healer_pidx == 0 && target_pidx == 7) {
                        game_->ending_flag_protagonist += 2;
                    } else if (healer_pidx == 7 && target_pidx == 0) {
                        game_->ending_flag_protagonist += 1;
                    } else if (healer_pidx == 8 && target_pidx == 7) {
                        game_->ending_flag_xiaofei += 2;
                    } else if (healer_pidx == 7 && target_pidx == 8) {
                        game_->ending_flag_xiaofei += 1;
                    }
                }
            }

            target.stats.morale = std::max(0, target.stats.morale - effect.morale_effect);

            BattleEvent ev;
            ev.type = BattleEvent::SKILL_USED;
            ev.source_idx = cmd.fighter_idx;
            ev.target_idx = cmd.target_idx;
            ev.value = effect.hp_effect;
            state_.push_event(ev);

            if (target.stats.current_hp <= 0) {
                process_death(cmd.target_idx);
                award_kill_exp(cmd.fighter_idx, cmd.target_idx);
            }
        }
    }
}

void BattleEngine::execute_wait(const BattleCommand& cmd) {
    (void)cmd;
}

void BattleEngine::execute_place_mine(const BattleCommand& cmd) {
    auto& fighter = state_.fighters[cmd.fighter_idx];

    if (cmd.mine_slot < 0 || cmd.mine_slot >= 8) return;
    uint8_t mine_idx = fighter.mines[cmd.mine_slot];
    if (mine_idx == 0 || mine_idx >= core::MAX_DILEI) return;

    int32_t cell = cmd.target_y * state_.map_width + cmd.target_x;
    if (cell < 0 || cell >= state_.grid_cells()) return;
    if (state_.grid_terrain[cell] == 99) return;
    if (state_.grid_mines[cell].mine_type != 0) return;

    state_.grid_mines[cell].mine_type = mine_idx;
    state_.grid_mines[cell].owner_team = fighter.team;

    BattleEvent ev;
    ev.type = BattleEvent::MINE_PLACED;
    ev.source_idx = cmd.fighter_idx;
    ev.target_x = cmd.target_x;
    ev.target_y = cmd.target_y;
    ev.value = mine_idx;
    state_.push_event(ev);
}

void BattleEngine::execute_use_item(const BattleCommand& cmd) {
    if (cmd.item_idx < 0 || cmd.item_idx >= core::MAX_WUPIN) return;

    auto& fighter = state_.fighters[cmd.fighter_idx];
    const auto& wupin = game_->items[cmd.item_idx];

    if (wupin.para1 != 0) {
        fighter.stats.current_hp = std::min(fighter.stats.max_hp,
                                             fighter.stats.current_hp + wupin.para1);
    }
    if (wupin.para2 != 0) {
        fighter.stats.morale = std::min(100, fighter.stats.morale + wupin.para2);
    }

    BattleEvent ev;
    ev.type = BattleEvent::ITEM_USED;
    ev.source_idx = cmd.fighter_idx;
    ev.value = cmd.item_idx;
    state_.push_event(ev);
}

void BattleEngine::check_mine_trigger(int32_t fighter_idx, int32_t grid_x, int32_t grid_y) {
    int32_t cell = grid_y * state_.map_width + grid_x;
    if (cell < 0 || cell >= state_.grid_cells()) return;

    auto& mine = state_.grid_mines[cell];
    if (mine.mine_type == 0) return;

    auto& fighter = state_.fighters[fighter_idx];
    if (fighter.team == mine.owner_team) return;

    if (mine.mine_type > 0 && mine.mine_type < core::MAX_DILEI) {
        const auto& dilei = game_->mines[mine.mine_type];
        int32_t damage = dilei.damage;

        if (dilei.effect_type == 0) {
            fighter.stats.current_hp -= damage;
        } else {
            fighter.stats.morale = std::max(0, fighter.stats.morale - damage);
        }

        BattleEvent ev;
        ev.type = BattleEvent::MINE_TRIGGERED;
        ev.source_idx = fighter_idx;
        ev.target_x = grid_x;
        ev.target_y = grid_y;
        ev.value = damage;
        state_.push_event(ev);

        if (fighter.stats.current_hp <= 0) {
            process_death(fighter_idx);
        }
    }

    mine.mine_type = 0;
    mine.owner_team = 0;
}

void BattleEngine::apply_damage(int32_t attacker_idx, int32_t defender_idx) {
    // 使用单位在部署时解耦拷贝的属性，而非按运行时索引反查名册（玩家单位
    // 的索引指向 PEOPLE.DAT，直接反查 fight.people 会得到错误角色的数据）。
    const auto& atk_rt = state_.fighters[attacker_idx];
    const auto& def_rt = state_.fighters[defender_idx];

    core::People atk_person{};
    atk_person.strength = atk_rt.strength;
    atk_person.intelligence = atk_rt.intelligence;
    atk_person.level = atk_rt.level;
    atk_person.class_type = atk_rt.class_type;
    atk_person.items = atk_rt.items;

    core::People def_person{};
    def_person.strength = def_rt.strength;
    def_person.intelligence = def_rt.intelligence;
    def_person.level = def_rt.level;
    def_person.class_type = def_rt.class_type;
    def_person.items = def_rt.items;

    const auto& atk_class = get_class_type(atk_rt.class_type);
    const auto& def_class = get_class_type(def_rt.class_type);

    const auto& atk_fighter = atk_rt;
    const auto& def_fighter = def_rt;

    // 地形防御加成 / Terrain defense bonus for defender
    int32_t def_cell = def_fighter.grid_y * state_.map_width + def_fighter.grid_x;
    int32_t terrain_type = (def_cell >= 0 && def_cell < state_.grid_cells())
                               ? state_.grid_terrain[def_cell] : 0;
    int32_t terrain_bonus = get_terrain_defense_bonus(terrain_type);

    // 物品攻防加成 / Item attack/defense bonuses (RE: 0x0042B042 / 0x0042B0EA)
    int32_t item_atk_bonus = (game_ != nullptr)
                                 ? get_item_attack_bonus(atk_person, *game_)
                                 : 0;
    int32_t item_def_bonus = (game_ != nullptr)
                                 ? get_item_defense_bonus(def_person, *game_)
                                 : 0;

    // 攻击者技能等级 / Attacker's skill level for attack
    int32_t atk_skill_exp = atk_rt.skill_attack;  // Basic attack skill
    int32_t atk_skill_level = get_skill_level(atk_skill_exp);

    int32_t atk_power = calc_attack_power(atk_person, atk_class, atk_fighter.stats, atk_skill_level, true, item_atk_bonus);
    int32_t def_power = calc_defense(def_person, def_class, def_fighter.stats, terrain_bonus, true, item_def_bonus);
    int32_t damage = calc_damage(atk_power, def_power);

    auto& target = state_.fighters[defender_idx];
    target.stats.current_hp -= damage;

    BattleEvent ev;
    ev.type = BattleEvent::DAMAGE_DEALT;
    ev.source_idx = attacker_idx;
    ev.target_idx = defender_idx;
    ev.value = damage;
    state_.push_event(ev);

    if (target.stats.current_hp <= 0) {
        process_death(defender_idx);
        award_kill_exp(attacker_idx, defender_idx);
    }
}

void BattleEngine::process_death(int32_t fighter_idx) {
    auto& fighter = state_.fighters[fighter_idx];
    fighter.stats.current_hp = 0;
    fighter.is_active = false;

    if (game_ && fighter.team == 0) {
        int32_t pidx = fighter.people_idx;
        if (pidx != 0 && pidx < 0x20) {
            bool apply = false;
            if (difficulty_level_ == 2) {
                apply = true;
            } else if (difficulty_level_ == 1 && death_penalty_count_ < 3) {
                apply = true;
            }
            if (apply) {
                auto& protagonist = game_->people[0];
                if (protagonist.level > 1) {
                    protagonist.level -= 1;
                }
                if (state_.fighters[0].stats.current_hp >
                    state_.fighters[0].stats.max_hp) {
                    state_.fighters[0].stats.current_hp =
                        state_.fighters[0].stats.max_hp;
                }
                ++death_penalty_count_;
            }
        }
    }

    BattleEvent ev;
    ev.type = BattleEvent::FIGHTER_DIED;
    ev.target_idx = fighter_idx;
    state_.push_event(ev);
}

void BattleEngine::award_kill_exp(int32_t killer_idx, int32_t victim_idx) {
    const auto& killer_fighter = state_.fighters[killer_idx];
    if (killer_fighter.team != 0 || !game_) return;

    int32_t people_idx = killer_fighter.people_idx;
    if (people_idx < 0 || people_idx >= core::MAX_PEOPLE) return;

    int32_t killer_level = state_.fighters[killer_idx].level;
    int32_t victim_level = state_.fighters[victim_idx].level;
    int32_t base_exp = victim_level * 10;

    int32_t exp = calc_experience(killer_level, victim_level, base_exp);
    game_->people[people_idx].experience += exp;
}

void BattleEngine::advance_turn() {
    int32_t next = find_next_active_fighter(state_.current_fighter_idx + 1);
    if (next < 0) {
        state_.battle_round++;
        check_milestone_events();
        state_.current_fighter_idx = find_next_active_fighter(0);

        for (auto& f : state_.fighters) {
            f.has_acted = false;
        }

        BattleEvent ev;
        ev.type = BattleEvent::ROUND_ADVANCED;
        ev.value = state_.battle_round;
        state_.push_event(ev);
    } else {
        state_.current_fighter_idx = next;
    }

    check_victory_conditions();
}

bool BattleEngine::needs_ai_input() const {
    if (state_.is_over) return false;
    int32_t idx = state_.current_fighter_idx;
    if (idx < 0 || idx >= BattleState::MAX_FIGHTERS) return false;
    const auto& fighter = state_.fighters[idx];
    return fighter.is_active && fighter.team != 0 && !fighter.has_acted;
}

void BattleEngine::execute_ai_turn() {
    if (!needs_ai_input()) return;
    BattleCommand cmd = generate_enemy_command(state_.current_fighter_idx);
    execute_command(cmd);
    advance_turn();
}

int32_t BattleEngine::find_nearest_enemy(int32_t fighter_idx) const {
    const auto& fighter = state_.fighters[fighter_idx];
    int32_t best_idx = -1;
    int32_t best_dist = INT32_MAX;

    for (int i = 0; i < BattleState::MAX_FIGHTERS; ++i) {
        if (i == fighter_idx) continue;
        if (!is_fighter_alive(i)) continue;
        if (state_.fighters[i].team == fighter.team) continue;

        int32_t dx = state_.fighters[i].grid_x - fighter.grid_x;
        int32_t dy = state_.fighters[i].grid_y - fighter.grid_y;
        int32_t dist = dx * dx + dy * dy;
        if (dist < best_dist) {
            best_dist = dist;
            best_idx = i;
        }
    }
    return best_idx;
}

int32_t BattleEngine::find_best_attack_target(int32_t fighter_idx) {
    const auto& fighter = state_.fighters[fighter_idx];
    int32_t range = get_attack_range(fighter_idx);
    int32_t best_idx = -1;
    int32_t best_score = INT32_MIN;

    for (int i = 0; i < BattleState::MAX_FIGHTERS; ++i) {
        if (i == fighter_idx) continue;
        if (!is_fighter_alive(i)) continue;
        if (state_.fighters[i].team == fighter.team) continue;

        const auto& target = state_.fighters[i];
        int32_t dx = target.grid_x - fighter.grid_x;
        int32_t dy = target.grid_y - fighter.grid_y;
        int32_t dist_sq = dx * dx + dy * dy;
        if (dist_sq > range * range) continue;

        int32_t score = 0;
        if (target.stats.current_hp <= 30) score += 50;
        score -= target.stats.current_hp;
        score += (100 - target.stats.morale) / 2;

        if (score > best_score) {
            best_score = score;
            best_idx = i;
        }
    }
    return best_idx;
}

int32_t BattleEngine::bfs_step_toward(int32_t from_x, int32_t from_y,
                                       int32_t to_x, int32_t to_y,
                                       int32_t max_dist) {
    struct Pos { int32_t x, y; };
    std::queue<Pos> bfs;
    std::vector<bool> visited(state_.grid_cells(), false);
    std::vector<int32_t> first_step(state_.grid_cells(), -1);

    int32_t start_cell = from_y * state_.map_width + from_x;
    visited[start_cell] = true;
    bfs.push({from_x, from_y});

    constexpr int dx[] = {0, 0, -1, 1};
    constexpr int dy[] = {-1, 1, 0, 0};

    while (!bfs.empty()) {
        auto [cx, cy] = bfs.front();
        bfs.pop();

        int32_t ccell = cy * state_.map_width + cx;
        int32_t cdist = 0;
        {
            int32_t ddx = cx - from_x;
            int32_t ddy = cy - from_y;
            cdist = (ddx < 0 ? -ddx : ddx) + (ddy < 0 ? -ddy : ddy);
        }
        if (cdist >= max_dist) continue;

        for (int d = 0; d < 4; ++d) {
            int32_t nx = cx + dx[d];
            int32_t ny = cy + dy[d];
            if (nx < 0 || nx >= state_.map_width) continue;
            if (ny < 0 || ny >= state_.map_height) continue;

            int32_t ncell = ny * state_.map_width + nx;
            if (visited[ncell]) continue;
            if (state_.grid_terrain[ncell] == 99) continue;

            bool blocked = false;
            for (int i = 0; i < BattleState::MAX_FIGHTERS; ++i) {
                const auto& f = state_.fighters[i];
                if (f.is_active && is_fighter_alive(i) &&
                    f.grid_x == nx && f.grid_y == ny) {
                    blocked = true;
                    break;
                }
            }
            if (blocked) continue;

            visited[ncell] = true;
            first_step[ncell] = (cdist == 0) ? d : first_step[ccell];
            bfs.push({nx, ny});

            if (nx == to_x && ny == to_y) {
                int32_t step = first_step[ncell];
                if (step >= 0 && step < 4) {
                    return step;
                }
            }
        }
    }

    int32_t best_cell = -1;
    int32_t best_dist = INT32_MAX;
    for (int y = 0; y < state_.map_height; ++y) {
        for (int x = 0; x < state_.map_width; ++x) {
            int32_t cell = y * state_.map_width + x;
            if (!visited[cell]) continue;
            int32_t ddx = x - to_x;
            int32_t ddy = y - to_y;
            int32_t dist = ddx * ddx + ddy * ddy;
            if (dist < best_dist) {
                best_dist = dist;
                best_cell = cell;
            }
        }
    }

    if (best_cell >= 0 && first_step[best_cell] >= 0) {
        return first_step[best_cell];
    }
    return -1;
}

BattleCommand BattleEngine::generate_enemy_command(int32_t fighter_idx) {
    BattleCommand cmd;
    cmd.fighter_idx = fighter_idx;

    const auto& fighter = state_.fighters[fighter_idx];
    uint8_t behavior = fight_->fighters[fighter_idx].behavior;

    if (should_retreat(fighter_idx)) {
        return ai_retreat(fighter_idx);
    }

    BattleCommand skill_cmd = ai_try_skill(fighter_idx);
    if (skill_cmd.type != BattleCommand::NONE) {
        return skill_cmd;
    }

    int32_t attack_target = find_best_attack_target(fighter_idx);
    if (attack_target >= 0) {
        cmd.type = BattleCommand::ATTACK;
        cmd.target_idx = attack_target;
        return cmd;
    }

    BattleCommand mine_cmd = ai_try_mine(fighter_idx);
    if (mine_cmd.type != BattleCommand::NONE) {
        return mine_cmd;
    }

    switch (behavior) {
    case 0:  // idle — hold position
        cmd.type = BattleCommand::WAIT;
        return cmd;
    case 1:  // patrol — move between patrol points
        return ai_patrol(fighter_idx);
    case 4:  // chase — aggressively pursue nearest enemy
        return ai_chase(fighter_idx);
    case 5:  // guard — stay near guard position
        return ai_guard(fighter_idx);
    case 6:  // assist — support a specific ally
        return ai_assist(fighter_idx);
    case 7:  // ambush — wait then strike
        return ai_ambush(fighter_idx);
    default: // chase by default
        return ai_chase(fighter_idx);
    }
}

bool BattleEngine::should_retreat(int32_t fighter_idx) const {
    const auto& f = state_.fighters[fighter_idx];
    if (f.stats.current_hp <= 0) return false;
    float hp_ratio = static_cast<float>(f.stats.current_hp) /
                     static_cast<float>(f.stats.max_hp);
    if (hp_ratio > 0.2f) return false;
    int32_t nearest_enemy = find_nearest_enemy(fighter_idx);
    if (nearest_enemy < 0) return false;
    const auto& enemy = state_.fighters[nearest_enemy];
    int32_t dx = f.grid_x - enemy.grid_x;
    int32_t dy = f.grid_y - enemy.grid_y;
    return (dx * dx + dy * dy) <= 9;
}

int32_t BattleEngine::find_ally_to_heal(int32_t fighter_idx) const {
    const auto& self = state_.fighters[fighter_idx];
    int32_t best_idx = -1;
    int32_t best_need = 0;

    for (int i = 0; i < BattleState::MAX_FIGHTERS; ++i) {
        if (i == fighter_idx) continue;
        const auto& ally = state_.fighters[i];
        if (!ally.is_active || ally.stats.current_hp <= 0) continue;
        if (ally.team != self.team) continue;

        int32_t missing = ally.stats.max_hp - ally.stats.current_hp;
        if (missing <= best_need) continue;

        int32_t dx = self.grid_x - ally.grid_x;
        int32_t dy = self.grid_y - ally.grid_y;
        int32_t dist_sq = dx * dx + dy * dy;
        if (dist_sq > 25) continue;

        best_need = missing;
        best_idx = i;
    }
    return best_idx;
}

BattleCommand BattleEngine::ai_try_skill(int32_t fighter_idx) {
    BattleCommand cmd;
    cmd.type = BattleCommand::NONE;
    cmd.fighter_idx = fighter_idx;

    const auto& fp = get_fight_people(fighter_idx);
    const auto& fighter = state_.fighters[fighter_idx];

    if (fp.skill_medical > 0) {
        int32_t heal_target = find_ally_to_heal(fighter_idx);
        if (heal_target >= 0) {
            const auto& target = state_.fighters[heal_target];
            int32_t dx = fighter.grid_x - target.grid_x;
            int32_t dy = fighter.grid_y - target.grid_y;
            if (dx * dx + dy * dy <= 4) {
                cmd.type = BattleCommand::SKILL;
                cmd.skill_idx = 3;  // 医疗 / Medical skill index
                cmd.target_idx = heal_target;
                cmd.target_x = target.grid_x;
                cmd.target_y = target.grid_y;
                return cmd;
            }
        }
    }

    if (fp.skill_fire > 0 || fp.skill_poison > 0) {
        int32_t best_target = -1;
        int32_t best_score = -1;
        int32_t atk_range = get_attack_range(fighter_idx);

        for (int i = 0; i < BattleState::MAX_FIGHTERS; ++i) {
            const auto& enemy = state_.fighters[i];
            if (!enemy.is_active || enemy.stats.current_hp <= 0) continue;
            if (enemy.team == fighter.team) continue;

            int32_t dx = fighter.grid_x - enemy.grid_x;
            int32_t dy = fighter.grid_y - enemy.grid_y;
            if (dx * dx + dy * dy > atk_range * atk_range + 4) continue;

            int32_t score = enemy.stats.max_hp - enemy.stats.current_hp;
            if (score > best_score) {
                best_score = score;
                best_target = i;
            }
        }

        if (best_target >= 0) {
            const auto& target = state_.fighters[best_target];
            cmd.type = BattleCommand::SKILL;
            cmd.skill_idx = (fp.skill_fire > 0) ? 4 : 15;  // 火攻=4, 施毒=15
            cmd.target_idx = best_target;
            cmd.target_x = target.grid_x;
            cmd.target_y = target.grid_y;
            return cmd;
        }
    }

    if (fp.skill_intimidate > 0) {
        int32_t atk_range = get_attack_range(fighter_idx);
        int32_t best_target = -1;
        int32_t best_morale = -1;

        for (int i = 0; i < BattleState::MAX_FIGHTERS; ++i) {
            const auto& enemy = state_.fighters[i];
            if (!enemy.is_active || enemy.stats.current_hp <= 0) continue;
            if (enemy.team == fighter.team) continue;

            int32_t dx = fighter.grid_x - enemy.grid_x;
            int32_t dy = fighter.grid_y - enemy.grid_y;
            if (dx * dx + dy * dy > atk_range * atk_range + 4) continue;

            if (enemy.stats.morale > best_morale) {
                best_morale = enemy.stats.morale;
                best_target = i;
            }
        }

        if (best_target >= 0) {
            const auto& target = state_.fighters[best_target];
            cmd.type = BattleCommand::SKILL;
            cmd.skill_idx = 7;  // 威慑 / Intimidate skill index
            cmd.target_idx = best_target;
            cmd.target_x = target.grid_x;
            cmd.target_y = target.grid_y;
            return cmd;
        }
    }

    return cmd;
}

BattleCommand BattleEngine::ai_try_mine(int32_t fighter_idx) {
    BattleCommand cmd;
    cmd.type = BattleCommand::NONE;
    cmd.fighter_idx = fighter_idx;

    const auto& fp = get_fight_people(fighter_idx);
    int32_t mine_type = -1;
    int32_t mine_slot_idx = -1;
    for (int i = 0; i < 8; ++i) {
        if (fp.mines[i] > 0) {
            mine_type = fp.mines[i];
            mine_slot_idx = i;
            break;
        }
    }
    if (mine_type < 0) return cmd;

    const auto& fighter = state_.fighters[fighter_idx];
    int32_t nearest_enemy = find_nearest_enemy(fighter_idx);
    if (nearest_enemy < 0) return cmd;

    const auto& enemy = state_.fighters[nearest_enemy];
    int32_t dx = fighter.grid_x - enemy.grid_x;
    int32_t dy = fighter.grid_y - enemy.grid_y;
    if (dx * dx + dy * dy > 16) return cmd;

    int32_t best_x = -1, best_y = -1;
    int32_t best_score = -1;

    for (int oy = -1; oy <= 1; ++oy) {
        for (int ox = -1; ox <= 1; ++ox) {
            if (ox == 0 && oy == 0) continue;
            int32_t mx = enemy.grid_x + ox;
            int32_t my = enemy.grid_y + oy;
            if (mx < 0 || mx >= state_.map_width) continue;
            if (my < 0 || my >= state_.map_height) continue;

            int32_t cell = my * state_.map_width + mx;
            if (state_.grid_mines[cell].mine_type > 0) continue;
            if (state_.fighters[fighter_idx].grid_x == mx &&
                state_.fighters[fighter_idx].grid_y == my) continue;

            int32_t score = 10;
            for (int i = 0; i < BattleState::MAX_FIGHTERS; ++i) {
                const auto& f = state_.fighters[i];
                if (!f.is_active || f.stats.current_hp <= 0) continue;
                if (f.team == fighter.team) {
                    int32_t adx = f.grid_x - mx;
                    int32_t ady = f.grid_y - my;
                    if (adx * adx + ady * ady <= 1) { score = -1; break; }
                }
            }
            if (score < 0) continue;
            if (score > best_score) {
                best_score = score;
                best_x = mx;
                best_y = my;
            }
        }
    }

    if (best_x >= 0) {
        cmd.type = BattleCommand::PLACE_MINE;
        cmd.target_x = best_x;
        cmd.target_y = best_y;
        cmd.mine_slot = mine_slot_idx;
        return cmd;
    }

    return cmd;
}

BattleCommand BattleEngine::ai_patrol(int32_t fighter_idx) {
    BattleCommand cmd;
    cmd.fighter_idx = fighter_idx;

    const auto& fighter = state_.fighters[fighter_idx];
    const auto& src = fight_->fighters[fighter_idx];
    int32_t px = src.target_pos_x;
    int32_t py = src.target_pos_y;

    if (px < 0 || px >= state_.map_width ||
        py < 0 || py >= state_.map_height) {
        return ai_chase(fighter_idx);
    }

    int32_t dx = fighter.grid_x - px;
    int32_t dy = fighter.grid_y - py;
    if (dx * dx + dy * dy <= 1) {
        int32_t nearest = find_nearest_enemy(fighter_idx);
        if (nearest >= 0) {
            const auto& enemy = state_.fighters[nearest];
            int32_t edx = fighter.grid_x - enemy.grid_x;
            int32_t edy = fighter.grid_y - enemy.grid_y;
            if (edx * edx + edy * edy <= 4) {
                cmd.type = BattleCommand::ATTACK;
                cmd.target_idx = nearest;
                return cmd;
            }
        }
        cmd.type = BattleCommand::WAIT;
        return cmd;
    }

    int32_t step = bfs_step_toward(fighter.grid_x, fighter.grid_y, px, py,
                                    get_movement_range(fighter_idx));
    if (step >= 0 && step < 4) {
        constexpr int sdx[] = {0, 0, -1, 1};
        constexpr int sdy[] = {-1, 1, 0, 0};
        int32_t nx = fighter.grid_x + sdx[step];
        int32_t ny = fighter.grid_y + sdy[step];
        int32_t ncell = ny * state_.map_width + nx;
        if (nx >= 0 && nx < state_.map_width &&
            ny >= 0 && ny < state_.map_height &&
            state_.movement_mask[ncell]) {
            cmd.type = BattleCommand::MOVE;
            cmd.target_x = nx;
            cmd.target_y = ny;
            return cmd;
        }
    }

    cmd.type = BattleCommand::WAIT;
    return cmd;
}

BattleCommand BattleEngine::ai_guard(int32_t fighter_idx) {
    BattleCommand cmd;
    cmd.fighter_idx = fighter_idx;

    const auto& fighter = state_.fighters[fighter_idx];
    const auto& src = fight_->fighters[fighter_idx];
    int32_t guard_x = src.target_pos_x;
    int32_t guard_y = src.target_pos_y;

    int32_t nearest = find_nearest_enemy(fighter_idx);
    if (nearest >= 0) {
        const auto& enemy = state_.fighters[nearest];
        int32_t edx = guard_x - enemy.grid_x;
        int32_t edy = guard_y - enemy.grid_y;
        if (edx * edx + edy * edy <= 9) {
            int32_t atk_target = find_best_attack_target(fighter_idx);
            if (atk_target >= 0) {
                cmd.type = BattleCommand::ATTACK;
                cmd.target_idx = atk_target;
                return cmd;
            }
        }
    }

    if (guard_x >= 0 && guard_x < state_.map_width &&
        guard_y >= 0 && guard_y < state_.map_height) {
        int32_t dx = fighter.grid_x - guard_x;
        int32_t dy = fighter.grid_y - guard_y;
        if (dx * dx + dy * dy <= 4) {
            cmd.type = BattleCommand::WAIT;
            return cmd;
        }

        int32_t step = bfs_step_toward(fighter.grid_x, fighter.grid_y,
                                        guard_x, guard_y,
                                        get_movement_range(fighter_idx));
        if (step >= 0 && step < 4) {
            constexpr int sdx[] = {0, 0, -1, 1};
            constexpr int sdy[] = {-1, 1, 0, 0};
            int32_t nx = fighter.grid_x + sdx[step];
            int32_t ny = fighter.grid_y + sdy[step];
            int32_t ncell = ny * state_.map_width + nx;
            if (nx >= 0 && nx < state_.map_width &&
                ny >= 0 && ny < state_.map_height &&
                state_.movement_mask[ncell]) {
                cmd.type = BattleCommand::MOVE;
                cmd.target_x = nx;
                cmd.target_y = ny;
                return cmd;
            }
        }
    }

    cmd.type = BattleCommand::WAIT;
    return cmd;
}

BattleCommand BattleEngine::ai_assist(int32_t fighter_idx) {
    BattleCommand cmd;
    cmd.fighter_idx = fighter_idx;

    const auto& fighter = state_.fighters[fighter_idx];
    const auto& src = fight_->fighters[fighter_idx];
    int32_t target_people = src.target_people;

    int32_t ally_idx = -1;
    for (int i = 0; i < BattleState::MAX_FIGHTERS; ++i) {
        if (i == fighter_idx) continue;
        const auto& ally = state_.fighters[i];
        if (!ally.is_active || ally.stats.current_hp <= 0) continue;
        if (ally.team != fighter.team) continue;
        if (target_people >= 0 && ally.people_idx == target_people) {
            ally_idx = i;
            break;
        }
    }

    if (ally_idx < 0) {
        return ai_chase(fighter_idx);
    }

    const auto& ally = state_.fighters[ally_idx];
    int32_t nearest_to_ally = -1;
    int32_t best_dist = INT32_MAX;
    for (int i = 0; i < BattleState::MAX_FIGHTERS; ++i) {
        const auto& enemy = state_.fighters[i];
        if (!enemy.is_active || enemy.stats.current_hp <= 0) continue;
        if (enemy.team == fighter.team) continue;
        int32_t dx = ally.grid_x - enemy.grid_x;
        int32_t dy = ally.grid_y - enemy.grid_y;
        int32_t dist_sq = dx * dx + dy * dy;
        if (dist_sq < best_dist) {
            best_dist = dist_sq;
            nearest_to_ally = i;
        }
    }

    if (nearest_to_ally >= 0 && best_dist <= 9) {
        int32_t atk_target = find_best_attack_target(fighter_idx);
        if (atk_target >= 0) {
            cmd.type = BattleCommand::ATTACK;
            cmd.target_idx = atk_target;
            return cmd;
        }
    }

    int32_t step = bfs_step_toward(fighter.grid_x, fighter.grid_y,
                                    ally.grid_x, ally.grid_y,
                                    get_movement_range(fighter_idx));
    if (step >= 0 && step < 4) {
        constexpr int sdx[] = {0, 0, -1, 1};
        constexpr int sdy[] = {-1, 1, 0, 0};
        int32_t nx = fighter.grid_x + sdx[step];
        int32_t ny = fighter.grid_y + sdy[step];
        int32_t ncell = ny * state_.map_width + nx;
        if (nx >= 0 && nx < state_.map_width &&
            ny >= 0 && ny < state_.map_height &&
            state_.movement_mask[ncell]) {
            cmd.type = BattleCommand::MOVE;
            cmd.target_x = nx;
            cmd.target_y = ny;
            return cmd;
        }
    }

    cmd.type = BattleCommand::WAIT;
    return cmd;
}

BattleCommand BattleEngine::ai_ambush(int32_t fighter_idx) {
    BattleCommand cmd;
    cmd.fighter_idx = fighter_idx;

    const auto& fighter = state_.fighters[fighter_idx];
    const auto& src = fight_->fighters[fighter_idx];
    int32_t ambush_x = src.target_pos_x;
    int32_t ambush_y = src.target_pos_y;

    int32_t nearest = find_nearest_enemy(fighter_idx);
    if (nearest >= 0) {
        const auto& enemy = state_.fighters[nearest];
        int32_t dx = fighter.grid_x - enemy.grid_x;
        int32_t dy = fighter.grid_y - enemy.grid_y;
        if (dx * dx + dy * dy <= 4) {
            int32_t atk_target = find_best_attack_target(fighter_idx);
            if (atk_target >= 0) {
                cmd.type = BattleCommand::ATTACK;
                cmd.target_idx = atk_target;
                return cmd;
            }
        }
    }

    if (ambush_x >= 0 && ambush_x < state_.map_width &&
        ambush_y >= 0 && ambush_y < state_.map_height) {
        int32_t dx = fighter.grid_x - ambush_x;
        int32_t dy = fighter.grid_y - ambush_y;
        if (dx * dx + dy * dy <= 1) {
            cmd.type = BattleCommand::WAIT;
            return cmd;
        }

        int32_t step = bfs_step_toward(fighter.grid_x, fighter.grid_y,
                                        ambush_x, ambush_y,
                                        get_movement_range(fighter_idx));
        if (step >= 0 && step < 4) {
            constexpr int sdx[] = {0, 0, -1, 1};
            constexpr int sdy[] = {-1, 1, 0, 0};
            int32_t nx = fighter.grid_x + sdx[step];
            int32_t ny = fighter.grid_y + sdy[step];
            int32_t ncell = ny * state_.map_width + nx;
            if (nx >= 0 && nx < state_.map_width &&
                ny >= 0 && ny < state_.map_height &&
                state_.movement_mask[ncell]) {
                cmd.type = BattleCommand::MOVE;
                cmd.target_x = nx;
                cmd.target_y = ny;
                return cmd;
            }
        }
    }

    return ai_chase(fighter_idx);
}

BattleCommand BattleEngine::ai_chase(int32_t fighter_idx) {
    BattleCommand cmd;
    cmd.fighter_idx = fighter_idx;

    const auto& fighter = state_.fighters[fighter_idx];
    int32_t nearest = find_nearest_enemy(fighter_idx);
    if (nearest < 0) {
        cmd.type = BattleCommand::WAIT;
        return cmd;
    }

    const auto& target_fighter = state_.fighters[nearest];
    int32_t move_range = get_movement_range(fighter_idx);

    calc_movement_range(fighter_idx);

    int32_t atk_range = get_attack_range(fighter_idx);
    int32_t best_x = -1, best_y = -1;
    int32_t best_dist = INT32_MAX;

    for (int y = 0; y < state_.map_height; ++y) {
        for (int x = 0; x < state_.map_width; ++x) {
            int32_t cell = y * state_.map_width + x;
            if (!state_.movement_mask[cell]) continue;

            int32_t dx = x - target_fighter.grid_x;
            int32_t dy = y - target_fighter.grid_y;
            int32_t dist_sq = dx * dx + dy * dy;

            if (dist_sq <= atk_range * atk_range) {
                if (dist_sq < best_dist) {
                    best_dist = dist_sq;
                    best_x = x;
                    best_y = y;
                }
            }
        }
    }

    if (best_x >= 0) {
        cmd.type = BattleCommand::MOVE;
        cmd.target_x = best_x;
        cmd.target_y = best_y;
        return cmd;
    }

    int32_t step = bfs_step_toward(fighter.grid_x, fighter.grid_y,
                                    target_fighter.grid_x, target_fighter.grid_y,
                                    move_range);
    if (step >= 0 && step < 4) {
        constexpr int sdx[] = {0, 0, -1, 1};
        constexpr int sdy[] = {-1, 1, 0, 0};
        int32_t nx = fighter.grid_x + sdx[step];
        int32_t ny = fighter.grid_y + sdy[step];
        int32_t ncell = ny * state_.map_width + nx;
        if (nx >= 0 && nx < state_.map_width &&
            ny >= 0 && ny < state_.map_height &&
            state_.movement_mask[ncell]) {
            cmd.type = BattleCommand::MOVE;
            cmd.target_x = nx;
            cmd.target_y = ny;
            return cmd;
        }
    }

    cmd.type = BattleCommand::WAIT;
    return cmd;
}

BattleCommand BattleEngine::ai_retreat(int32_t fighter_idx) {
    BattleCommand cmd;
    cmd.fighter_idx = fighter_idx;

    const auto& fighter = state_.fighters[fighter_idx];
    int32_t nearest = find_nearest_enemy(fighter_idx);
    if (nearest < 0) {
        cmd.type = BattleCommand::WAIT;
        return cmd;
    }

    const auto& enemy = state_.fighters[nearest];
    calc_movement_range(fighter_idx);

    int32_t best_x = fighter.grid_x;
    int32_t best_y = fighter.grid_y;
    int32_t best_dist = 0;

    int32_t edx = fighter.grid_x - enemy.grid_x;
    int32_t edy = fighter.grid_y - enemy.grid_y;
    int32_t cur_dist = edx * edx + edy * edy;

    for (int y = 0; y < state_.map_height; ++y) {
        for (int x = 0; x < state_.map_width; ++x) {
            int32_t cell = y * state_.map_width + x;
            if (!state_.movement_mask[cell]) continue;

            int32_t dx = x - enemy.grid_x;
            int32_t dy = y - enemy.grid_y;
            int32_t dist_sq = dx * dx + dy * dy;

            if (dist_sq > cur_dist && dist_sq > best_dist) {
                best_dist = dist_sq;
                best_x = x;
                best_y = y;
            }
        }
    }

    if (best_x != fighter.grid_x || best_y != fighter.grid_y) {
        cmd.type = BattleCommand::MOVE;
        cmd.target_x = best_x;
        cmd.target_y = best_y;
    } else {
        cmd.type = BattleCommand::WAIT;
    }
    return cmd;
}

void BattleEngine::check_victory_conditions() {
    if (state_.is_over) return;

    int32_t player_alive = count_alive_on_team(0);
    if (player_alive == 0) {
        end_battle(false);
        return;
    }

    switch (state_.victory_condition) {
    case 0:
        if (check_all_enemies_dead()) { end_battle(true); return; }
        break;
    case 1:
        if (!check_protect_target()) { end_battle(false); return; }
        if (check_all_enemies_dead()) { end_battle(true); return; }
        break;
    case 2:
        if (check_survive_rounds()) { end_battle(true); return; }
        break;
    case 3:
        if (check_boss_defeated()) { end_battle(true); return; }
        break;
    case 4:
        if (check_reach_position()) { end_battle(true); return; }
        break;
    case 5:
        if (check_rescue_target()) { end_battle(true); return; }
        break;
    }

    check_round_exhaustion();
}

bool BattleEngine::check_all_enemies_dead() {
    return count_alive_on_team(1) == 0;
}

bool BattleEngine::check_protect_target() {
    if (state_.protect_target_idx < 0) return true;
    for (const auto& f : state_.fighters) {
        if (f.people_idx == state_.protect_target_idx && f.team == 0 && f.is_active) {
            return f.stats.current_hp > 0;
        }
    }
    return false;
}

bool BattleEngine::check_survive_rounds() {
    if (state_.survive_round_target <= 0) return false;
    return state_.battle_round >= state_.survive_round_target;
}

bool BattleEngine::check_boss_defeated() {
    if (state_.boss_target_idx < 0) return false;
    for (const auto& f : state_.fighters) {
        if (f.people_idx == state_.boss_target_idx && f.is_active) {
            return f.stats.current_hp <= 0;
        }
    }
    return true;
}

bool BattleEngine::check_reach_position() {
    if (state_.reach_target_people_idx < 0 ||
        state_.reach_target_x < 0 || state_.reach_target_y < 0) return false;
    for (const auto& f : state_.fighters) {
        if (f.people_idx == state_.reach_target_people_idx &&
            f.team == 0 && f.is_active && f.stats.current_hp > 0) {
            return f.grid_x == state_.reach_target_x &&
                   f.grid_y == state_.reach_target_y;
        }
    }
    return false;
}

bool BattleEngine::check_rescue_target() {
    if (state_.rescue_target_idx < 0) return false;
    int32_t target_x = -1, target_y = -1;
    for (const auto& f : state_.fighters) {
        if (f.people_idx == state_.rescue_target_idx && f.is_active) {
            target_x = f.grid_x;
            target_y = f.grid_y;
            break;
        }
    }
    if (target_x < 0) return false;
    for (const auto& f : state_.fighters) {
        if (f.team == 0 && f.is_active && f.stats.current_hp > 0 &&
            f.people_idx != state_.rescue_target_idx) {
            int32_t dx = f.grid_x - target_x;
            int32_t dy = f.grid_y - target_y;
            if (dx * dx + dy * dy <= 1) return true;
        }
    }
    return false;
}

void BattleEngine::check_round_exhaustion() {
    if (state_.is_over) return;
    if (state_.max_rounds <= 0) return;

    if (state_.battle_round >= state_.max_rounds) {
        int32_t player_alive = count_alive_on_team(0);
        int32_t enemy_alive = count_alive_on_team(1);
        bool victory = player_alive >= enemy_alive;

        BattleEvent ev;
        ev.type = BattleEvent::ROUNDS_EXHAUSTED;
        ev.value = state_.battle_round;
        state_.push_event(ev);

        end_battle(victory);
    }
}

void BattleEngine::check_milestone_events() {
    int32_t round = state_.battle_round;
    if (round == 10 || round == 150 || round == 270) {
        BattleEvent ev;
        ev.type = BattleEvent::MILESTONE_REACHED;
        ev.value = round;
        state_.push_event(ev);
    }
}

void BattleEngine::end_battle(bool victory) {
    if (state_.is_over) return;

    state_.is_over = true;
    state_.is_victory = victory;
    state_.phase = victory ? BattlePhase::VICTORY : BattlePhase::DEFEAT;

    BattleEvent ev;
    ev.type = victory ? BattleEvent::BATTLE_WON : BattleEvent::BATTLE_LOST;
    state_.push_event(ev);
}

int32_t BattleEngine::compute_score() const {
    return compute_score_breakdown().total;
}

BattleEngine::ScoreBreakdown BattleEngine::compute_score_breakdown() const {
    ScoreBreakdown result;
    result.rounds = state_.battle_round;

    if (!state_.is_victory) return result;

    for (int i = 0; i < BattleState::MAX_FIGHTERS; ++i) {
        const auto& f = state_.fighters[i];
        if (!f.is_active) continue;
        if (f.team != 0 && f.stats.current_hp <= 0) ++result.enemy_kills;
        if (f.team == 0 && f.stats.current_hp <= 0) ++result.player_losses;
    }

    result.base = 50;
    result.kill_bonus = result.enemy_kills * 10;
    result.loss_penalty = result.player_losses * 5;
    result.round_bonus = (state_.battle_round < 15) ? (30 - state_.battle_round * 2) : 0;

    if (difficulty_level_ == 0) result.difficulty_pct = 80;
    else if (difficulty_level_ == 2) result.difficulty_pct = 130;

    int32_t raw = result.base + result.kill_bonus - result.loss_penalty + result.round_bonus;
    if (raw < 0) raw = 0;
    result.total = raw * result.difficulty_pct / 100;

    return result;
}

}  // namespace dlzr::logic
