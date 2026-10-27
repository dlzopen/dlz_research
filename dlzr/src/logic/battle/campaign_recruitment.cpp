// campaign_recruitment.cpp — 战役招募写回实现
// Campaign recruitment write-back implementation
//
// 见 campaign_recruitment.hpp 注释：仅填充数据，部署逻辑复用 init_fighters 现成循环。
// 证据链（铁律零，指到具体行）：
//   · FUN_0041b53c.c:19-23 —— 部署遍历按 fighter 槽位 slot 读 people[slot].class_type 绘制；
//     .c:25-29 —— face > -1 视为有效单元。⇒ 写回须按 slot k 同时填 people[k]+fighters[k]。
//   · types.hpp FightData 注释 —— 运行时 people[0..39]=战役槽（主角+同伴），[40..127]=本关名册。
//   · battle_engine.cpp init_fighters 主循环（行 189-236）—— grid_x!=999 && stage==1 即登场，
//     team 取自 fight.people[i].enemy；体力取 fight.fighters[i].stamina（team0 不乘难度）。

#include "logic/battle/campaign_recruitment.hpp"

#include <cstring>

namespace dlzr::logic {

void apply_campaign_recruitment(core::FightData& fight,
                                 const core::CampaignRoster& roster,
                                 const core::GameData& game) {
    for (const auto& c : roster.companions) {
        const int32_t k = c.people_index;
        // 仅战役同伴槽 [1,39] 合法：
        //   0     = 主角（由 BattleEngine::init 的英雄覆盖分支处理）；
        //   40..127 = 本关名册（由 FIGHT.DAT 装载，不在此覆盖）。
        if (k < 1 || k >= 40) continue;
        if (k >= static_cast<int>(fight.people.size())) continue;
        if (k >= static_cast<int>(game.people.size())) continue;

        const auto& src = game.people[k];
        auto& dst = fight.people[k];

        // —— People → FightPeople 逐字段复制（同 132B 内存布局，仅技能字段命名不同）——
        dst.name = src.name;
        dst.face = src.face;
        dst.pad2 = 0;
        dst.enemy = 0;                  // 我方阵营
        dst.strength = src.strength;
        dst.intelligence = src.intelligence;
        dst.level = src.level;
        dst.experience = src.experience;
        dst.class_type = src.class_type;
        dst.skill_attack      = src.skill_gongji;
        dst.skill_supply      = src.skill_buji;
        dst.skill_motivate    = src.skill_jili;
        dst.skill_medical     = src.skill_yiliao;
        dst.skill_fire        = src.skill_huogong;
        dst.skill_log         = src.skill_gunmu;
        dst.skill_rock        = src.skill_leishi;
        dst.skill_intimidate  = src.skill_weishe;
        dst.skill_harass      = src.skill_saorao;
        dst.skill_sabotage    = src.skill_pohuai;
        dst.skill_plunder     = src.skill_lueduo;
        dst.skill_counterspy  = src.skill_fanjian;
        dst.skill_decoy       = src.skill_yibing;
        dst.skill_supervise   = src.skill_duzhan;
        dst.skill_assassinate = src.skill_ansha;
        dst.skill_poison      = src.skill_shidu;
        dst.skill_airraid     = src.skill_kongxi;
        dst.items = src.items;
        dst.mines = src.mines;

        // —— 体力(HP) 经兵种公式（证据 calc_max_hp FUN_00429e31：hp_base+(lv-1)*hp_growth）——
        // 文件名册单位无 FIGHT.DAT 磁盘 HP（磁盘 HP 仅本关名册），运行时按兵种+等级算。
        int32_t stamina = 100;
        const int32_t cls = src.class_type;
        if (cls >= 0 && cls < core::MAX_CLASS_TYPES) {
            const auto& ct = game.class_types[cls];
            stamina = ct.hp_base + (src.level - 1) * ct.hp_growth;
            if (stamina < 1) stamina = 1;
        }

        auto& ff = fight.fighters[k];
        ff.grid_x = c.grid_x;          // 登场坐标（!=999）
        ff.grid_y = c.grid_y;
        ff.stage = 1;                   // 登场
        ff.stamina = stamina;
        ff.morale = 100;
        ff.stat3 = 100;
        ff.movement = 100;
        ff.facing = c.facing;
        ff.direction = c.facing;
        ff.pos_x = c.grid_x;
        ff.pos_y = c.grid_y;
    }
}

bool recruit_companion(core::GameData& game,
                       int32_t people_index,
                       int32_t grid_x,
                       int32_t grid_y,
                       int32_t facing) {
    // 仅战役同伴槽 [1,39] 合法：
    //   0      = 主角（英雄覆盖分支处理，非招募）；
    //   40..127 = 本关名册（FIGHT.DAT 装载，非战役招募）。
    if (people_index < 1 || people_index >= 40) return false;
    if (people_index >= static_cast<int>(game.people.size())) return false;

    game.campaign_roster.companions.push_back(
        core::RecruitedCompanion{people_index, grid_x, grid_y, facing});
    return true;
}

}  // namespace dlzr::logic
