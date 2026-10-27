// test_cross_layer_diff.cpp — clean ↔ oracle 跨层差分测试
//
// 目的：把同一份逻辑状态同时喂给 clean 层（dlzr::logic::combat）与 oracle 层
//（replica::battle_calc_*，二进制逐指令复刻），断言两者可观测输出逐位一致。
//
// 为什么重要：
//   oracle 是二进制真相（battle_calc.cpp 已逐指令 + 10 万次随机对拍验证）；
//   本测试验证 clean 复刻层是否真·与二进制 1:1（路线图 G-Func / G-Subsys 的跨层闸门）。
//   它抓到的正是"手写单元测试钉住猜测"的反例：clean 层曾声称"严格匹配原作"，
//   实则技能分桶算法与防御地形守卫都与二进制相反/不符——本测试使其立即变红。
//
// 范围说明（诚实保留）：
//   本测试刻意排除"物品攻防加成/兼容矩阵"——oracle 读 g_battlecalc_class_flag_table，
//   clean 读 GameData.wupin_compat，数据源不同，需独立集成测试。
//   地形以同一百分比值分别注入两层，隔离"地形数据来源"差异，只验算术与守卫逻辑。

#include <gtest/gtest.h>
#include <random>
#include <vector>

#include "replica/replica_stubs.h"     // ::GameState g_game_state、::People/::Fighter/::ClassType
#include "replica/battle_calc.hpp"     // battle_calc_attack_power / _defense / _skill_level (extern "C")
#include "logic/combat/combat_formulas.hpp"
#include "core/data_model/types.hpp"
#include "core/data_model/constants.hpp"

namespace {

// 二进制真值技能阈值（battle_calc.cpp:14 默认值；运行时由数据文件装载）。
// 跨层差分须用真值表，不依赖其他用例的全局状态——显式装载，隔离 test_battle_calc 的随机化。
static void load_canonical_skill_thresholds() {
    static const uint32_t canonical[10] = {0, 1, 3, 6, 10, 15, 21, 28, 36, 45};
    for (int i = 0; i < 10; ++i) g_battlecalc_skill_thresholds[i] = canonical[i];
}

// 把 clean 层的一份逻辑状态逐字段镜像进 oracle 的 g_game_state。
// 两层类型不同（dlzr::core::* vs replica::*，replica 为二进制保真版，字段名一致、偏移与二进制 1:1），
// 必须按字段名赋值。比的是"值"，不是内存布局（Tier B 允许 clean 层布局自由）。
void mirror_state(const dlzr::core::People& p, const dlzr::core::ClassType& c, const dlzr::core::Fighter& f,
                  int idx, int cls) {
    auto& rp = g_game_state.people_array[idx];
    rp.strength = p.strength;
    rp.intelligence = p.intelligence;
    rp.level = p.level;
    rp.class_type = p.class_type;
    rp.skill_gongji = p.skill_gongji;
    for (int i = 0; i < 8; ++i) rp.items[i] = p.items[i];

    auto& rc = g_game_state.classtype_array[cls];
    rc.base_attack = c.base_attack;
    rc.attack_growth = c.attack_growth;
    rc.base_defense = c.base_defense;
    rc.defense_growth = c.defense_growth;

    auto& rf = g_game_state.fighter_array[idx];
    rf.morale = f.morale;
    rf.equip_integrity = f.equip_integrity;
    rf.defense_modifier = f.defense_modifier;
    rf.pos_x = f.pos_x;
    rf.pos_y = f.pos_y;
}

}  // namespace

// ============================================================
// 技能分桶：clean::get_skill_level vs oracle::battle_calc_skill_level
// ============================================================
TEST(CrossLayerDiff, SkillLevelBucket) {
    load_canonical_skill_thresholds();   // 显式装载真值表，隔离全局污染
    for (int s = -1; s <= 100; ++s) {
        int32_t clean_lv = dlzr::logic::get_skill_level(s);
        int32_t oracle_lv = static_cast<int32_t>(
            battle_calc_skill_level(static_cast<uint32_t>(s)));
        ASSERT_EQ(clean_lv, oracle_lv) << "skill_use=" << s;
    }
}

// ============================================================
// 攻击力：clean::calc_attack_power vs oracle::battle_calc_attack_power
// 覆盖战斗/非战斗两种模式 + 随机化全部输入，2 万次对拍。
// ============================================================
TEST(CrossLayerDiff, AttackPower) {
    load_canonical_skill_thresholds();   // 显式装载真值表，隔离全局污染
    std::mt19937 rng(0x9E3779B9u);
    std::uniform_int_distribution<int32_t> U(0, 300);
    const int N = 20000;
    for (int t = 0; t < N; ++t) {
        bool in_battle = (t % 2 == 0);
        g_game_state.g_current_mode = in_battle ? dlzr::core::GAME_MODE_BATTLE : 0x01;

        dlzr::core::People p{};
        p.strength = U(rng);
        p.level = 1 + (U(rng) % 40);
        p.class_type = 0;
        p.skill_gongji = U(rng) % 60;
        p.items.fill(0);
        dlzr::core::ClassType c{};
        c.base_attack = U(rng);
        c.attack_growth = 1 + (U(rng) % 20);
        dlzr::core::Fighter f{};
        f.morale = U(rng);
        f.equip_integrity = U(rng);

        int idx = 5, cls = 0;
        mirror_state(p, c, f, idx, cls);

        int32_t oracle = battle_calc_attack_power(idx);
        int32_t clean = dlzr::logic::calc_attack_power(
            p, c, f, dlzr::logic::get_skill_level(p.skill_gongji), in_battle, 0);
        ASSERT_EQ(oracle, clean)
            << "t=" << t << " in_battle=" << in_battle
            << " str=" << p.strength << " lvl=" << p.level
            << " base_atk=" << c.base_attack << " atk_g=" << c.attack_growth
            << " skill=" << p.skill_gongji << " morale=" << f.morale << " integ=" << f.equip_integrity;
    }
}

// ============================================================
// 防御力：clean::calc_defense vs oracle::battle_calc_defense
// 覆盖战斗/非战斗两种模式 + 随机地形索引/百分比 + 随机化全部输入，2 万次对拍。
// 地形以同一百分比值分别注入两层（隔离"地形数据来源"差异）。
// ============================================================
TEST(CrossLayerDiff, Defense) {
    std::mt19937 rng(0x1234ABCDu);
    std::uniform_int_distribution<int32_t> U(0, 300);
    g_battlecalc_map_width = 64;
    for (int k = 0; k < 16; ++k)
        g_battlecalc_terrain_modifier[k] = U(rng) % 101;   // 0..100（非负，避开有/无符号歧义）
    const int N = 20000;
    for (int t = 0; t < N; ++t) {
        bool in_battle = (t % 2 == 0);
        g_game_state.g_current_mode = in_battle ? dlzr::core::GAME_MODE_BATTLE : 0x01;

        dlzr::core::People p{};
        p.level = 1 + (U(rng) % 40);
        p.class_type = 0;
        dlzr::core::ClassType c{};
        c.base_defense = U(rng);
        c.defense_growth = 1 + (U(rng) % 20);
        dlzr::core::Fighter f{};
        f.morale = U(rng);
        f.defense_modifier = U(rng);
        int pos_x = U(rng) % 16, pos_y = U(rng) % 16;
        f.pos_x = pos_x;
        f.pos_y = pos_y;

        int idx = 5, cls = 0;
        int terrain_idx = U(rng) % 16;
        g_battlecalc_terrain_grid[pos_y * g_battlecalc_map_width + pos_x] =
            static_cast<uint16_t>(terrain_idx << 12);

        mirror_state(p, c, f, idx, cls);

        int32_t terrain_pct = g_battlecalc_terrain_modifier[terrain_idx];
        int32_t oracle = battle_calc_defense(idx);
        int32_t clean = dlzr::logic::calc_defense(p, c, f, terrain_pct, in_battle, 0);
        ASSERT_EQ(oracle, clean)
            << "t=" << t << " in_battle=" << in_battle
            << " base_def=" << c.base_defense << " def_g=" << c.defense_growth
            << " lvl=" << p.level << " morale=" << f.morale << " defmod=" << f.defense_modifier
            << " terr_idx=" << terrain_idx << " terr_pct=" << terrain_pct;
    }
}

// ============================================================
// 伤害结算：clean::calc_damage vs oracle::battle_calc_damage
// RE: 0x0042784B。覆盖攻击<防御(返回 incoming 而非 1)、相等、攻击>防御、incoming 取大四种语义；
// 随机化含负值（battle_engine 可能因负士气产出负攻防），2 万次对拍。
// ============================================================
TEST(CrossLayerDiff, Damage) {
    std::mt19937 rng(0xCAFEBABEu);
    std::uniform_int_distribution<int32_t> U(-500, 2000);
    const int N = 20000;
    for (int t = 0; t < N; ++t) {
        int32_t atk = U(rng);
        int32_t def = U(rng);
        int32_t inc = U(rng);
        int32_t clean = dlzr::logic::calc_damage(atk, def, inc);
        int32_t oracle = static_cast<int32_t>(battle_calc_damage(
            static_cast<uint32_t>(atk), static_cast<uint32_t>(def), static_cast<uint32_t>(inc)));
        ASSERT_EQ(clean, oracle)
            << "t=" << t << " atk=" << atk << " def=" << def << " inc=" << inc;
    }
}

// ============================================================
// 经验公式：clean::calc_attack_exp / calc_kill_exp vs oracle
// RE: 0x0042AA92 / 0x0042AB23。覆盖同级、高/低等级差、边界，1 万次对拍。
// ============================================================
TEST(CrossLayerDiff, Exp) {
    std::mt19937 rng(0x5EA7F00Du);
    std::uniform_int_distribution<int32_t> U(1, 99);
    const int N = 10000;
    int a_idx = 5, b_idx = 6;   // killer / victim
    for (int t = 0; t < N; ++t) {
        int32_t killer_lv = U(rng);
        int32_t victim_lv = U(rng);
        g_game_state.people_array[a_idx].level = killer_lv;
        g_game_state.people_array[b_idx].level = victim_lv;

        // 攻击经验：clean(attacker=killer, defender=victim)；oracle 取 (attacker, defender)
        int32_t clean_atk = dlzr::logic::calc_attack_exp(killer_lv, victim_lv);
        int32_t oracle_atk = battle_calc_attack_exp(a_idx, b_idx);
        ASSERT_EQ(clean_atk, oracle_atk)
            << "t=" << t << " killer=" << killer_lv << " victim=" << victim_lv;

        // 击杀经验：clean(killer, victim)；oracle 取 (defender=victim, attacker=killer) 匹配二进制约定
        int32_t clean_kill = dlzr::logic::calc_kill_exp(killer_lv, victim_lv);
        int32_t oracle_kill = battle_calc_kill_exp(b_idx, a_idx);
        ASSERT_EQ(clean_kill, oracle_kill)
            << "t=" << t << " killer=" << killer_lv << " victim=" << victim_lv;
    }
}

// ============================================================
// 升级累加：clean::apply_experience_gain vs oracle::battle_calc_gain_exp
// RE: 0x00429E92。验证 total=exp_mod+gained; exp_mod=total%100; level=min(level+total/100,99)。
// 1 万次随机（含多级别升级与封顶）。
// ============================================================
TEST(CrossLayerDiff, LevelUp) {
    std::mt19937 rng(0x1E5E1F00u);
    std::uniform_int_distribution<int32_t> U(0, 99);
    std::uniform_int_distribution<int32_t> G(0, 500);
    const int N = 10000;
    int idx = 7;
    for (int t = 0; t < N; ++t) {
        int32_t exp_mod = U(rng);
        int32_t level = U(rng);
        int32_t gained = G(rng);

        // clean 层
        dlzr::core::People p{};
        p.experience = exp_mod;
        p.level = level;
        bool clean_lu = dlzr::logic::apply_experience_gain(p, gained);
        int32_t clean_exp = p.experience;
        int32_t clean_lv = p.level;

        // oracle 层（读写 g_game_state.people_array[idx]）
        g_game_state.people_array[idx].experience = exp_mod;
        g_game_state.people_array[idx].level = level;
        int32_t oracle_lu = battle_calc_gain_exp(idx, gained);
        int32_t oracle_exp = g_game_state.people_array[idx].experience;
        int32_t oracle_lv = g_game_state.people_array[idx].level;

        ASSERT_EQ(clean_lu, (oracle_lu != 0)) << "t=" << t;
        ASSERT_EQ(clean_exp, oracle_exp) << "t=" << t << " em=" << exp_mod << " lv=" << level << " g=" << gained;
        ASSERT_EQ(clean_lv, oracle_lv)   << "t=" << t << " em=" << exp_mod << " lv=" << level << " g=" << gained;
    }
}

// ============================================================
// 命中判定：clean::calc_hit_chance / calc_hit_chance_weapon vs oracle
// RE: 0x0042ABB0 / 0x0042AC0B。RNG 为二进制共用 LCG，跨层差分须用「种子复位」使
// clean 与 oracle 抽同一随机值（与 test_battle_calc 的 forwarder/ref 对拍同机制）。
// 2 万次随机对拍 0 分歧。
// ============================================================
TEST(CrossLayerDiff, HitChance) {
    load_canonical_skill_thresholds();   // 显式装载真值阈值，保证 weapon 版分桶与 oracle 一致
    std::mt19937 rng(0xBADC0DEu);
    std::uniform_int_distribution<int32_t> I(-200, 200);   // 智力可负（与二进制 people 域一致）
    std::uniform_int_distribution<int32_t> S(0, 400);      // skill_lueduo 计数
    const int N = 20000;
    int a_idx = 5, d_idx = 6;
    for (int t = 0; t < N; ++t) {
        // 构造 clean 层状态
        dlzr::core::People atk{}, def{};
        atk.intelligence = I(rng);
        atk.skill_lueduo = S(rng);
        def.intelligence = I(rng);

        // 镜像进 oracle 的 g_game_state（intelligence + skill_lueduo）
        g_game_state.people_array[a_idx].intelligence = atk.intelligence;
        g_game_state.people_array[a_idx].skill_lueduo = atk.skill_lueduo;
        g_game_state.people_array[d_idx].intelligence = def.intelligence;

        int32_t skill_lv = dlzr::logic::get_skill_level(atk.skill_lueduo);

        // 种子复位：clean 与 oracle 抽同一随机值（rand_impl 共用全局种子）
        uint32_t seed0 = (uint32_t)g_game_state.g_pow2_1_448790;
        g_game_state.g_pow2_1_448790 = (int32_t)seed0;
        int32_t clean_h = dlzr::logic::calc_hit_chance(atk, def);
        g_game_state.g_pow2_1_448790 = (int32_t)seed0;
        int32_t oracle_h = battle_calc_hit_chance(a_idx, d_idx);
        ASSERT_EQ(clean_h, oracle_h) << "t=" << t
            << " atk_int=" << atk.intelligence << " def_int=" << def.intelligence;

        // 武器版
        g_game_state.g_pow2_1_448790 = (int32_t)seed0;
        int32_t clean_w = dlzr::logic::calc_hit_chance_weapon(atk, def, skill_lv);
        g_game_state.g_pow2_1_448790 = (int32_t)seed0;
        int32_t oracle_w = battle_calc_hit_chance_weapon(a_idx, d_idx);
        ASSERT_EQ(clean_w, oracle_w) << "t=" << t
            << " atk_int=" << atk.intelligence << " def_int=" << def.intelligence
            << " skill_lv=" << skill_lv;
    }
}

// ============================================================
// 最大 HP：clean::calc_max_hp vs oracle::battle_calc_max_hp
// RE: 0x00429E31。max_hp = class.hp_base + (level-1)*class.hp_growth。
// 确定性（无 RNG），1 万次随机对拍。注意：二进制 00429e31 旧名 anim_frame_offset 系误命名，实为 max HP。
// ============================================================
TEST(CrossLayerDiff, MaxHp) {
    load_canonical_skill_thresholds();
    std::mt19937 rng(0x4D415850u);   // "MAXP"
    std::uniform_int_distribution<int32_t> U(-50, 200);
    const int N = 10000;
    int idx = 9, cls = 0;
    for (int t = 0; t < N; ++t) {
        dlzr::core::People p{};
        p.level = 1 + (U(rng) % 99);
        p.class_type = cls;
        dlzr::core::ClassType c{};
        c.hp_base = U(rng);
        c.hp_growth = 1 + (U(rng) % 20);

        // 镜像进 oracle 的 g_game_state
        g_game_state.people_array[idx].level = p.level;
        g_game_state.people_array[idx].class_type = cls;
        g_game_state.classtype_array[cls].hp_base = c.hp_base;
        g_game_state.classtype_array[cls].hp_growth = c.hp_growth;

        int32_t clean = dlzr::logic::calc_max_hp(p, c);
        int32_t oracle = battle_calc_max_hp(idx);
        ASSERT_EQ(clean, oracle) << "t=" << t
            << " lv=" << p.level << " base=" << c.hp_base << " g=" << c.hp_growth;
    }
}

// ============================================================
// 资源增减夹取：clean::clamp_stat_delta vs oracle::battle_calc_clamp_resource
// RE: 0x00427830。min(cap, base+delta)（无符号加法溢出则取 cap）。确定性，2 万次对拍。
// ============================================================
TEST(CrossLayerDiff, ClampResource) {
    std::mt19937 rng(0xC0DEBEEFu);
    std::uniform_int_distribution<int32_t> S(-300, 300);    // base / delta
    std::uniform_int_distribution<uint32_t> C(0, 300);      // cap
    const int N = 20000;
    for (int t = 0; t < N; ++t) {
        int32_t base = S(rng);
        int32_t delta = S(rng);
        uint32_t cap = C(rng);
        int32_t clean = dlzr::logic::clamp_stat_delta(base, delta, (int32_t)cap);
        int32_t oracle = battle_calc_clamp_resource(base, delta, cap);
        ASSERT_EQ(clean, oracle) << "t=" << t
            << " base=" << base << " delta=" << delta << " cap=" << cap;
    }
}

// ============================================================
// 等级奖励：clean::calc_level_bonus vs oracle::calc_level_bonus_a199
// RE: 0x0042A199。遍历 32 单位，未装备(enemy==0)且已部署(active_flag!=0)者平均 level，
// bonus=avg-(fight_index*2+1)；<2→0；按 flow_phase 缩放(0→0,1→/2,else*4/5)；
// fight_index>=0x24→0。2 万次随机对拍（含无部署/边界/各 flow 分支）。
// ============================================================
TEST(CrossLayerDiff, LevelBonus) {
    std::mt19937 rng(0x1EBE1F00u);
    std::uniform_int_distribution<int32_t> L(0, 99);     // level
    std::uniform_int_distribution<int>     E(0, 1);      // enemy 0/1
    std::uniform_int_distribution<int>     A(0, 1);      // active_flag 0/1
    std::uniform_int_distribution<int32_t> F(0, 0x30);   // fight_index (含 >=0x24 分支)
    std::uniform_int_distribution<int32_t> P(0, 2);      // flow_phase 0/1/2
    const int N = 20000;
    std::vector<dlzr::core::People>  people(0x20);
    std::vector<dlzr::core::Fighter> fighters(0x20);
    for (int t = 0; t < N; ++t) {
        int32_t fight_index = F(rng);
        int32_t flow_phase  = P(rng);
        for (int i = 0; i < 0x20; ++i) {
            int32_t enemy  = E(rng);
            int32_t lvl    = L(rng);
            int     active = A(rng);
            // oracle 层状态（g_game_state）
            g_game_state.people_array[i].enemy = enemy;
            g_game_state.people_array[i].level = lvl;
            g_game_state.fighter_array[i].active_flag = static_cast<uint8_t>(active);
            // clean 层状态（同值镜像）
            people[i].enemy = enemy;
            people[i].level = lvl;
            fighters[i].active_flag = static_cast<uint8_t>(active);
        }
        g_game_state.g_fight_index = fight_index;
        g_game_state.g_flow_phase  = flow_phase;

        int32_t clean  = dlzr::logic::calc_level_bonus(people, fighters, fight_index, flow_phase);
        int32_t oracle = calc_level_bonus_a199();
        ASSERT_EQ(clean, oracle) << "t=" << t
            << " fi=" << fight_index << " fp=" << flow_phase;
    }
}
