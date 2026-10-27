# DLZ Replica 路线图与验收标准

> 视角：REA 逆向工程（证据制）。任何"原版怎么做"的结论必须能指到 `output/ghidra_all/FUN_XXXXXXXX.c` 的具体行或 PE 抽取实测值，否则不许落地。
> 本文件是可执行的规划与验收契约，不是愿景文档。每次推进一个单元（函数 / 子系统）都按"验收闸门"判定是否可进入下一步。
> 配套文档：产品/排期视角见 `docs/05_implementation_roadmap.md`。

---

## 0. 当前基线（证据快照 · 2026-10-26 刷新）

| 维度 | 状态 | 证据 |
|---|---|---|
| 原函数转录 | **755/755 已全部转录为 `rf_<addr>` 桩**（1:1 照抄，0 占位残留） | `output/re_map/function_map.csv`、构建 0 error |
| 全局收口 | **1914 个真独立全局** 全部收口进单一 `GameState g_game_state` | `renames.csv` 1914 行、`gen_replica_stubs.py` |
| 命名诚实度 | `g_const_`=**0**、`gs_00` 残留=**0**；但真·游戏语义名 ≈498/1914，其余为内容分类/区域占位/子结构偏移占位 | `renames.csv` 分类拆分 |
| 结构化区域 | **Fighter / People / ClassType / Skill** 四块字段偏移与二进制 1:1（GameState 成员） | `types.hpp`/`game_data.hpp`/`battle_state.hpp` + Ghidra 对拍 |
| 战斗数值内核（pilot） | **14 个战斗数值函数已语义化转发** + 独立 `ref_*` 逐指令转录 + 随机对拍 **100,000 次 0 分歧**；另建 **clean↔oracle 跨层差分**（9 例 + `SkillGridDiff` 3 例，含 1–2 万次随机对拍 0 分歧） | `replica/battle_calc.cpp`、`tests/test_battle_calc.cpp`、`tests/test_cross_layer_diff.cpp`、`tests/test_skill_grid_diff.cpp` |
| 构建健康 | 主程序 `dlzr.exe` + `dlzr_tests` **构建 0 error**；**测试 238/238 PASS**（含真实 IO 框架 `DatIoTest` **14/14**，用真实 `DLZFULL` 二进制驱动，非 mock） | `cmake --build` 已验证（2026-10-26 续，全绿） |
| 架构分层 | **转录/oracle 层**（`src/replica/`）+ **clean 复刻层**（`src/core` `src/logic` `src/presentation` `src/platform`） | `find dlzr/src -maxdepth 2 -type d` |
| clean 层已有结构 | `logic/`：battle, combat, dialogue, save, level(LevelEventManager), promotion, character, crafting, forging, hidden_level, strategic；`presentation/`：battle, dialogue, title, opening, ending, ui, font, animation, audio 等 | 同名 `.hpp` 存在 |

**诚实边界（不声称 100% RE）**：1914 全局中仅 ≈498 描述性名是真语义；7 张战斗表在 `battle_calc.hpp` 以独立 `extern` 声明，与 `g_game_state.g_misc_blk_5ab0@0x475ab0` 同址异址，运行时 `fread` 装载须统一；`msvc32_compat.h` 的 6 个数学/CRT 垫片为重建语义（仅保链接、不触测试路径）。

---

## 1. 总体策略（不可违背的纪律）

1. **铁律零（证据制）**：只认反编译指令流 + 用法实证。手写产物（含 clean 层代码、renames 名）只是"待验证假设"，落地前必须回 Ghidra 逐指令核对带坐标。
2. **双层保真契约**：
   - **Tier A 1:1（玩家可感知，禁自由发挥）**：Mode 跳转/触发、对话令牌语义与逐字配色、UI 坐标、时序动画、输入响应、数值结果。
   - **Tier B 实现自由（玩家无感）**：内部调度、数据结构、内存布局、模块划分——只要可观测行为等价。
3. **增量验收、互不阻塞**：以"函数"和"子系统"为最小验收单元；一个单元通过闸门即可计入完成度，不必等全局完工。
4. **oracle 优先**：先用 `replica/` 转录层把可观测行为钉死（作为差分测试 oracle），clean 层实现必须与之 0 分歧，再谈重构美化。

---

## 2. 路线图（按子系统分阶段）

每个阶段给出：范围、关键全局/函数证据坐标、交付物、依赖的闸门。

### Phase 0 — 脚手架与命名地基 ✅ 已完成
- 755 函数 `rf_` 桩、1914 全局收口 GameState、`renames.csv` 2669 行。
- **交付物**：`replica_stubs.{h,c}`（生成器产出）、`GameState` 骨架。
- **闸门**：G-Build 绿 + 0 未解析符号 + 0 `g_const_`/`gs_` 残留。

### Phase 1 — 结构化数据地基 ✅ 部分完成（4/多 区域）
- 已完成 Fighter/People/ClassType/Skill 字段偏移 1:1。
- **待做**：其余区域（地图/地形、对话、UI、存档块、精灵缓存等）的字段定型 + ≥1 个消费函数 RE 证实布局。
- **证据坐标**：`types.hpp`/`game_data.hpp`/`battle_state.hpp`；people[] `0x7d0ef8` stride 132；fighter `0x0096ab90` stride 0x40。

### Phase 2 — 战斗数值内核 🟢 内核 pilot 闭环（14/N 函数 + 跨层差分已建）
- 已完成 14 函数：`0x0042A82A`(攻击) `0x0042A98E`(防御) `0x0042A95B`(装备地形防御) `0x00419CA4`(技能等级分桶) `0x0042B042`(物品攻击加成) `0x0042B0EA`(物品防御加成) `0x0042784B`(伤害结算) `0x0042AA92`(攻击经验) `0x0042AB23`(击杀经验) `0x00429E92`(升级累加) `0x0042ABB0`(命中判定·基础) `0x0042AC0B`(命中判定·武器/技能版) **`0x00427830`(资源增减夹取 clamp_stat_delta)** **`0x00429E31`(最大HP max_hp)**。
- **跨层差分已建**（路线图原"最大验收缺口"已补）：`test_cross_layer_diff.cpp` 6 例（技能分桶/攻击/防御/伤害/经验/升级），clean `combat_formulas` ↔ oracle `battle_calc_*` 1–2 万次随机对拍 0 分歧。
- **经验/升级公式已收口并修正 clean 层**：二进制经验纯由"等级差"分桶决定（无 `base_exp`），clean 旧 `calc_experience(killer,victim,base_exp)` 用 `base_exp*mult/4` 是手写猜测——已删除，替换为 `calc_attack_exp`/`calc_kill_exp`（二进制分桶），并新增 `apply_experience_gain` 修复"clean 从不做升级"的 bug（`experience` 语义改为级内经验 0..99，与二进制 `FUN_00429E92` 一致）。
- **命中/暴击（已 RE 实证，2026-10-24 续3）**：**命中机制确实存在，但不在伤害公式内，而是伤害前的独立"攻击是否命中"闸门**。`FUN_0042abb0(攻,守)`：`命中率 = clamp((攻方智力 − 守方智力)×2 + 30, 0, ∞)`；`rand_impl()%100 < 命中率` → 命中。`FUN_0042ac0b` 为武器/技能版，命中率额外 `+= 技能等级(攻方.skill_lueduo@0x58)`。两函数由 `FUN_004138ea`（战斗结算流程）调用，证实为伤害前置判定；`FUN_0041307f`（伤害应用）本身不调 rand、伤害无条件生效。已收口：oracle `battle_calc_hit_chance`/`battle_calc_hit_chance_weapon` + 转发桩 `hit_chance_check_abb0`/`hit_chance_check_weapon_ac0b` + `BattleCalc` 10 万次对拍 **ALL 12 FUNCTIONS 1:1**（比较时用 RNG 种子复位使 forwarder/ref 抽同一随机值）。注：`rand_impl` = MSVC/Borland LCG（种子 `g_pow2_1_448790`：`seed=seed*0x343fd+0x269ec3`,`(seed>>16)&0x7fff`），全工程共用，已确认为真实函数非占位。
- **关键运行时表**：技能阈值 `0x447280`、兵种×物品兼容 `0x95eb88`、共享 stride `0x578d40/44`、地形网格 `0x584390`、地形修正 `0x475ab0`、地图宽 `0x57ba20`。

### Phase 3 — 战斗引擎与结算 🟢 数值内核 + 16-case oracle + T3-core/T3-b 闭环（剩 case9/0x21 残余 RE）
- clean 层已有 `logic/battle/{battle_engine,battle_settlement,battle_state}.hpp` + `logic/combat/combat_formulas.hpp`，并有 `test_battle_settlement` / `test_combat_formulas`。
- **已完成（数值内核侧）**：14 个战斗数值函数已全部语义化转发 + 9 例 clean↔oracle 跨层差分（含本阶段新增 `MaxHp`/`ClampResource`/`HitChance`），武器/物品攻防加成因数据源不同按路线图约定排除（待集成测试）。
- **`FUN_004138ea` RE 已完成（2026-10-24 续，关键结构发现）**：详见 `output/re_map/RE_FUN_004138ea.md`。该例程是"区域伤害/技能效果结算编排"，被 `FUN_004043ea`/`FUN_004214aa` 每次行动调用一次（`DAT_00473158`=技能/行动类型，`param_1`=行动单位）。16 个效果分支已逐 case 钉死证据坐标；并发现两处 clean 层未建模的结构：① **0x21 字节/单位战斗记录数组**（base `0x7d0f18`，被 case 0xa/0xb/0xc 与 `uVar8/uVar9` 预计算读取）；② **多 pass 持久化伤害缓冲** `DAT_007d0488`(HP)/`0x4565d0`(士气)——由 setup pass 清零播种、`FUN_0041307f` 写入基础伤害、`FUN_004138ea` 按技能类型缩放施加。**结论**：完整复刻须把"清零播种→`FUN_0041307f` 写基础伤害→`FUN_004138ea` 按技能缩放"串成一次行动的确定性管线。
- **待做（结构侧 · 5 步已全部执行，见 RE 文档 §9）**：
  1. 新增数据模型：`g_game_state` 加 0x21 战斗记录数组 + per-cell 伤害缓冲（容量=最大网格 130）。**→ 跨层差分用 `BattleGridState`（clean）/oracle 模块全局承载"已播种缓冲+战斗记录"，单 pass 闭环已验证；3-pass 管线（setup→`FUN_0041307f`→本函数）的跨 pass 持久化已接 `GameState` 运行时字段（§9.3② 闭环，续4#2）**。
  2. Oracle 收口：`replica/battle_sim.cpp` 写 `battle_calc_apply_cell_effect`（逐指令转录 16 case，复用已验证 `battle_calc_damage`/`max_hp`/`clamp`/`hit_chance`/`equip_defense`，剔除渲染调用）。**→ ✅ 已落地，并收口 case 1 non-player 阈值 `<2?1` 与全 `local_X` 无符号算术（§9.1/9.2）**。
  3. Clean 镜像：`battle_settlement::apply_cell_effect` 同语义（Tier B 自由结构，但可观测终态须等价）。**→ ✅ 已落地，双语逐位一致**。
  4. 跨层差分：`test_skill_grid_diff.cpp` 加 `SkillPower`/`SkillEffectCell`/`SkillEffectGrid`（**5000 + 20000 + 4000** 次迭代，16 skill_type × 随机占驻/缓冲/记录/技能计数/地形，RNG 种子复位）。**→ ✅ 0 分歧，全量 119/119 PASS（含新增 CellBaseDamage/CellDeathExp）**。
  5. **T3-core（`FUN_0041307f` 确定性 per-cell 伤害核心 · 2026-10-25 收口）**：已完成 RE + 跨层差分闭环。
     - **范围**：`FUN_0041307f.c:49-121`（双层循环确定性 per-cell 基础伤害写入；**无 RNG**）。清路/同阵营（`enemy` 相等）→ 清零两缓冲；否则 `base_dmg=calc_damage(attack,defense,0)`、`local_44=base_dmg*hp_buf/100`、`local_48=max(local_44, hp/20, 1)`、`local_4c=min(local_48, hp)`；扣 HP/士气、写 `attacker_index`/`unit_state=3`、`skill_gongji++`、累加封顶 `0x30` 攻击经验小计（敌方 `>>2`）。
     - **落地**：oracle `battle_calc_apply_cell_damage`（逐指令转录 `replica/battle_sim.cpp`）；clean `apply_base_damage` + `clean_compute_attack_power`/`clean_compute_defense`（`logic/battle/battle_sim.cpp`，复用 `calc_damage`/`max_hp_of`/`get_skill_level` + 镜像二进制 `battle_calc_item_attack/defense_bonus`）；`BattleGridState` 扩 `strength`/`skill_gongji`/`pos_x`/`pos_y` + `class_combat[96][4]` + `current_mode`，`mirror_grid` 同步全部攻防/坐标/模式字段；新增 `TEST(SkillGridDiff, CellBaseDamage)`（**N=10000** 随机：9×9/10×13 双网格 + 0/0x96 双模式 + 负值 HP 锁 uint 环绕 + 同阵营/空占驻清除路径 + 敌方 `>>2` 经验修正）。**→ ✅ 0 分歧，全量 119/119 PASS**。
     - **关键修复（铁律零实证，均为本步暴露）**：
       ① 生成桩 `battle_damage_sim_138ea`(RF:004138ea) 转发体误用裸 `g_misc_blk_3158`/`g_active_unit_index`（应为 `g_game_state.` 成员）→ 已改正 `replica_stubs.c:11936`。
       ② 测试 `mirror_grid` 误将 `Fighter.items` 写 `rf.items`（Fighter 无 items 字段，items 属 People）→ 改 `rp.items`；并核对 `class_combat[][0..3]`↔`classtype_array` 攻防字段映射与 `current_mode` 同步。
       ③ 测试占驻 `items` 播种 `U(rng)%200` 越界（item_id>160 触发 `battle_calc_item_*_bonus` 对 `g_battlecalc_class_flag_table[160*96]`/`g_battlecalc_item_def_bonus[160]` OOB，读 garbage → 防御力凭空 +0x30000000）→ 改为 `% 82`（uVar1≤80，武器加成闸门 `>0x50` 永不触发）；clean 同步补 `battle_calc_item_defense_bonus`（faithful 二进制含 item 加成）。
  6. **T3-b（`FUN_0041307f` 死亡/经验循环 · 2026-10-25 收口）**：已完成 RE + 跨层差分闭环（接 T3-core 之后）。
     - **范围**：`FUN_0041307f.c:146-195`（遍历整网格，凡 `hp_buf[cell]!=0` 且占驻 `current_hp==0 || morale==0` → `active_flag=0` 阵亡；累加 `battle_calc_kill_exp(occ,attacker)` 击杀经验；`count_unit_skills(occ)`(FUN_0042afd2) 敌方物品回收；我方阵亡 `DAT_008331f8(g_aux_counter_b)++`、敌方阵亡 `DAT_008331f0(g_aux_counter_a)++`；我方阵亡且 `1<=occ<0x20` 且 `(g_flow_phase==2 || (g_flow_phase==1 && g_aux_counter_b<3))` → `g_player_units_remaining` 地板 1 递减 + 主将(Fighter[0])HP 封顶 `max_hp(0)`；末尾 `battle_calc_gain_exp(attacker, local_24)` 应用总经验）。**无 RNG**。
     - **落地**：oracle `battle_calc_apply_death_experience` + clean `apply_death_experience`（`logic/battle/battle_sim.cpp`），双语逐指令对称；`BattleGridState` 扩 `experience` + `lost_player`/`lost_enemy`/`battle_win_mode`/`player_units_remaining`/`item_recovery[256]`，`mirror_grid` 同步全部镜像字段；新增 `TEST(SkillGridDiff, CellDeathExp)`（**N=10000** 随机：双网格/双模式/随机阵亡计数/约 30% 单位低血低士气稳定触发阵亡 + 敌方物品回收 + 主将 HP 封顶支路）。
     - **关键修复（铁律零实证，本步暴露）**：oracle 死亡循环原读 `g_grid_rows`（被测试 `mirror_grid` 污染为 13×13，因 `resize_grid_buffers(13,13)` 占位），而 `big=0` 实际应为 9×9，遍历越界产生额外击杀经验（oracle_total=5652 vs clean_total=1096）→ 改为从 `g_battle_double_grid` 派生尺寸（与二进制 `FUN_0041307f.c:36-47` 一致：==0→9×9，else→10×13）。pass-1 `battle_calc_apply_cell_damage` 同步维护 `g_grid_rows`/`g_grid_cols` 防其它读取者踩坑。**→ ✅ 0 分歧，全量 119/119 PASS**。
  7. 残余 RE（2026-10-26 续二十更新）：`0x21` 记录数组 `DAT_007d0f18[idx*0x21]` = **单位智力字段**（已坐实，见 §7 候选④a，非斜向语义）；`FUN_0043de50` = **标准 memmove**（已坐实 + 回归测试，见 §7 候选④b）；`DAT_00473158` 号↔技能名映射、case 9 仓库语义 `FUN_0042b23a`/`0x7db3a8`（§7 候选④c 仍为 forward 集成项）。**→ 单 pass 跨层差分不阻塞，已诚实保留（§9.3①/③）**。
- **证据坐标**：`FUN_004138ea.c`(16 分支 switch(DAT_00473158))、`FUN_0041307f.c:88/97`(基础伤害写入缓冲)、`FUN_00412030/123f9/127f5/12bba`(缓冲清零播种)、`FUN_0042b23a.c`(case9 消耗品→仓库)、`FUN_0041deaa.c`(Mode150)、`FUN_0041b53c.c`(部署判据)、Fighter `+0x3D action`/`+0x3C active`。

### Phase 4 — 单位与数据装载（DAT/IMG parse）✅ 已实现 + 差分断言闭环（2026-10-25 收口）
- 范围：FIGHT.DAT（88 磁盘条 = people[40..127]）、GAME.DAT（兵种表 base `0x1E0` stride `0xA0`）、ARMY.DAT（精灵）、TOWN.DAT、bigmap.img/bigmine0.img。
- **交付物**：loader 模块 `app/resource/dat_loader.cpp`（`DatLoader`：`load_game_data`/`load_fight`/`load_towns`/`load_map`/`load_blk_group`/`load_dialogues`/`load_wave_clip`）+ 装载后字段与二进制偏移一致性的差分断言 `tests/test_first_level.cpp`（6 测全 PASS）。
- **RE 证据坐标**：`FUN_004292fe.c:15-18`（FIGHT.DAT 装载：seek `fight_index*0x501c`、读 700B intro @0x7d01c8、128×0x40 fighters @0x96ab90、88×0x84 people @0x7d2398=people基址+40×0x84；`:20-23` 头像 `people[40..127]`）、`FUN_00429286`(TOWN.DAT)、`people[unit].class_type → GAME.DAT+0x10 img_index → ARMY.DAT`。
- **差分断言实测钉死**：① FIGHT.DAT 磁盘 88 条→运行时 people[40..127]（`people[40]`=伪军/class50，绝非占位孙悟空）；② fight0 部署位 40/41 grid(6,17)/(7,17)、stage=1；③ 战前对话首条 TXT[21]（大队长）；④ GAME.DAT 兵种表 `@0x1E0`：`[0]=神仙队 [1]=手枪队 [4]=大刀队 [8]=步枪队 [11]=骑兵队`（Python 抽取 DLZFULL/GAME.DAT GBK 解码实证，反驳 data_dictionary.md 误标 0x0320）；⑤ PEOPLE.DAT `people[0]=孙悟空(class0→神仙队)`、`people[4]=李向阳(class1→手枪队)`。FIGHT.DAT 文件大小 963876 = 47×20508（记录尺寸 1:1 吻合）。
- **诚实残留已收口（2026-10-25 续）**：ARMY.DAT 精灵 + `bigmap.img` 地图背景图的 loader 差分断言已加（`tests/test_first_level.cpp` → `AssetLoaderDiffTest.ArmyDatStructureMatchesBinary` / `BigmapImgMatchesBinary`）。证据：`DLZFULL/ARMY.DAT` 大小 = 3983616 = 91×0xAB00（字节级吻合）；`BIGMAP.IMG` header `width_m1@+0x06`/`height_m1@+0x08`、像素索引@+0x10、调色板@+0x10+W×H（6-bit×4）与 loader 解出逐字节一致。属渲染资源，非数值字段，闭环后 Phase 4 全收口。
- **真实 IO 测试框架（2026-10-25 续十八，用户核心诉求）**：`tests/test_dat_io.cpp` 14 用例，用真实 `DLZFULL` 二进制逐字节交叉校验 `DatLoader` 输出（独立重解析 + RE 定点）——`RealFileSizes`/`PeopleLoadedByteIdentical`/`People0IsSunWukong`/`TerrainTableMatchesRawParse`/`TerrainStrideIs20Not40`/`ClassMoveCostMatchesRaw`/`WupinCompatIsLastBlock`/`DialoguesDecode593`/`DialogueDecodeIndependentCount`/`Map0Dimensions`/`Fight0Loads`/`BlkGroup11Loads`/`WaveClipLoads`/`MissingFileThrows` 全 PASS。实证定点：GAME=56588/PEOPLE=5280/TXT=110900/MAP=120690/FIGHT=963876/BLK=15805952/WAVE=12992170；PEOPLE[0]=孙悟空(GBK CB EF CE F2 BF D5)；地形@0xA0 20B/记录；class_move_cost[0]=[1×14,99,99]；TXT 593 条；map0 20×26/blk11；fight0"村庄风云"。

### Phase 5 — 地图与地形 / 寻路
- 范围：地形网格 `0x584390`（`(row*g_map_grid_width+col)` ushort `>>12`）、地形修正 `0x475ab0`、地图宽 `0x57ba20`、寻路代价（被误归为 battle 的缓存块之一，须正名）；`g_pathfind_grid[MAP_GRID_CAP]`（`MAP_GRID_CAP=16384`，跨层差分已建 28+ 处 `=1`/`=0` 写入）。
- **依赖（已就绪）**：Phase 4 已装载真实地形表（`GAME.DAT` 块#1 @`0x1E0` stride `0xA0`、地形修正 `0x475ab0`）、地形网格（`MAP.DAT` tile `>>12` stride 0x40）、地图宽 `0x57ba20`——全已接 `GameState`。
- **证据坐标**：`FUN_00424566.c:15`（地形修正表）、`FUN_00416c70.c:24`（地形网格 `tile>>12`）、`FUN_00412030.c:51`（HP 缓冲→士气缓冲 `memmove` 播种 0x208B，即 `FUN_0043de50`）。
- **交付物 / 闸（G-Subsys）**：地图模型 + 寻路器 + 与 `replica` 地形取值 0 分歧。属最顺的自然下一步（候选 1，见 §7）。

### Phase 6 — 对话系统（Tier A 强约束）🟡 数据/令牌层就绪，差"接主循环整屏渲染"
- 范围：对话令牌语义 + **逐字配色**（Tier A 禁自由发挥）。`renames.csv` 中 `dialogue*` 类 ≈414 个描述性名，多数待用法实证。
- **已完成（数据/令牌层）**：TXT.DAT XOR 0x51 GBK 解码 593 条、`NNN ` 行首聚合+内嵌换行保留、`parse_dialogue` 剥离 `GG/TT/xx`；令牌 AA/BB/CC/DD 逐字配色语义已落地（`FUN_004061df.c:39-52` → `dialogue_token_palette_index` A=0xb7/B=0x85/C=0x89/D=0x4f/其余=0x48）。
- **缺口（候选 2，见 §7）**：上述数据/令牌层尚未"接进游戏主循环整屏渲染"（Tier A 强约束：逐字配色不得自由发挥）。
- **交付物 / 闸（G-Subsys + G-TierA 局部）**：`logic/dialogue/dialogue_manager.hpp` 驱动整屏渲染 + 令牌/配色差分测试。

### Phase 7 — UI / 渲染 / 输入 / 动画
- 范围：`presentation/{ui,font,animation,battle,...}` + `platform/{sdl3,mock,web}` + `app/{input,scene}`。DDraw 表面、`g_ddraw` vtable（无 NULL 检查，须真链 dsound/ddraw）换成 SDL3。
- **Tier A**：UI 坐标、时序动画、输入响应逐项核对。

### Phase 8 — 模式状态机 / 主循环 🟡 真实数据装载点已收口，Mode 跳转行为测试未建
- 范围：`current_mode@0x7db4e8`、Mode150 战后链路、`LevelEventManager` 重写控制流（替代原版状态机）。
- **已完成（数据装载闭环）**：真实数据经 `DatLoader` 统一装载点已收口进 `GameState`（§9.3② 闭环，续4#2）。
- **缺口**：各 Mode 跳转的 Tier A 行为测试尚未建立（依赖候选 1/2 子系统接入后补）。
- **交付物 / 闸（G-TierA）**：状态机 + 各 Mode 跳转的 Tier A 行为测试。

### Phase 9 — 存读档
- 范围：`save_game@0x7d01c8`（29 块 fwrite），基址以本工程 save/load 代码重派生（`.bt` 注释地址不可信）。
- **交付物**：与原始存档字节级可互读（或等价语义）。

### Phase 10 — 音频（MCI/DSound → SDL3/WebAudio）
- 范围：MCI 音频 API、DSound；换成 `platform/` 抽象。

### Phase 11 — 集成与全量 Tier A 1:1
- 范围：主循环串起所有子系统，玩家可感知清单逐项核对（Mode 跳转、对话配色、UI 坐标、时序、输入、数值）。

---

## 3. 验收闸门（Gate）框架

任一单元（函数或子系统）要"通过并进入下一步"，必须逐条满足其对应闸门。**不达标 = 该单元不得标记为完成，回退到对应层 RE。**

### G-Build（构建门 · 每次提交必过）
- [ ] `cmake --build dlzr/build --config Release` **0 error**。
- [ ] `dlzr_tests` **全部 PASS**（当前 227/227 基线，新增用例不得回退）。
- [ ] 0 未解析外部符号、0 链接 red。
- **判定**：不绿 = 任何"功能完成"声明无效，必须先复位绿。

### G-Data（数据地基门 · 新区域/子系统开工前）
- [ ] 该区域依赖的 `GameState` 字段偏移**逐 Ghidra 指令核对 1:1**（带 `FUN_*.c:行` 坐标）。
- [ ] ≥1 个消费函数 RE 证实该布局被**正确使用**（不仅是声明）。

### G-Func（函数语义化门 · 单函数从占位→已语义化）
必须同时满足：
- [ ] **(a) 证据**：Ghidra `FUN_<addr>.c` 指令流坐标（文件:行），逐指令可追溯。
- [ ] **(b) 命名**：真语义名（非占位/非内容分类）写入 `renames.csv`，evidence 栏带坐标。
- [ ] **(c) 可观测行为钉死**：若数值/行为可观测 → 入 `replica/battle_calc`（或同类语义模块）+ `ref_*` 逐指令转录 + **差分对拍 N ≥ 100,000 次 0 分歧**。
- [ ] **(d) 跨层一致**：若由 clean 层实现 → clean 实现须通过**跨层差分（clean vs replica oracle）0 分歧**。
- [ ] **(e) 桩替换**：`rf_<addr>` 桩体替换为转发或真实实现（生成器 `BATTLE_CALC_FWD` 模式可复用）。

### G-Subsys（子系统门 · 子系统封闭）
- [ ] 该子系统内 ≥1 个**集成行为测试**（Mode 跳转 / 对话令牌 / 数值结果）0 分歧。
- [ ] G-Build 绿 + 子系统内所有 G-Func 已过的函数无回退。

### G-TierA（全量 1:1 门 · Phase 11）
- [ ] 玩家可感知清单逐项核对通过：Mode 跳转/触发、对话令牌语义+逐字配色、UI 坐标、时序动画、输入响应、数值结果。

---

## 4. "验证通过 → 下一步" 触发规则

| 层级 | 触发条件（验证通过） | 动作（进入下一步） |
|---|---|---|
| 函数级 | G-Func (a–e) 全过 | 该地址升"已语义化"，`renames.csv` 标注，计入完成度；解锁其消费方函数 |
| 子系统级 | G-Subsys 全过 | 该子系统标记"封闭"，进入下一 Phase |
| 跨层 | clean ↔ replica oracle 差分 0 分歧 | clean 层该模块可脱离 oracle 独立演进（Tier B 自由重构） |
| 全程阻断 | 任一 Gate 失败 / build·test red | **回退**到对应层 RE，**禁止带猜测落地**（铁律零） |

**推进纪律（用户强制）**：除"需用户在多方案中选"或"不可逆破坏数据"两类情形外，一路跑到底，不在中间步骤停下等确认；验证通过直接做下一步，做完再汇报。

---

## 5. 完成度度量（实时口径）

- **函数语义化**：已语义化 **14/755**（战斗数值 pilot；含 `0x0042784B` 伤害结算、`0x0042AA92`/`0x0042AB23` 经验、`0x00429E92` 升级、`0x0042ABB0`/`0x0042AC0B` 命中判定、`0x00427830` 资源夹取、`0x00429E31` 最大HP）；其余 741 已转录为 `rf_` 桩但未命名/未差分验证。
- **`FUN_004138ea` 编排函数**：16 分支区域伤害/技能结算已收口进 oracle `replica/battle_sim.cpp`(`battle_calc_settle_skill_grid`/`apply_cell_effect`) + clean `logic/battle/battle_sim.cpp`，并通过 `SkillGridDiff` 跨层差分 29000 次迭代 0 分歧。**`rf_004138ea` 桩体尚未 forward 到该实现**（属 3-pass 管线集成残余，见 §9.3）——当前真实可观测行为由 oracle 承载，桩为同义 1:1 转录冗余。
- **全局真语义**：≈**498/1914**（描述性名，证据强度不等）；其余为内容分类/区域占位/子结构偏移占位。
- **子系统**：battle 子系统 pilot **已完成跨层差分闭环**——`FUN_004138ea` 16 分支 `replica`(oracle) ↔ `logic/battle`(clean) 0 分歧（SkillGridDiff 5 测含 CellDeathExp + 9 例 BattleCalc 跨层，全量 119/119 PASS）。T3-core（`FUN_0041307f` per-cell 伤害核心）与 T3-b（死亡/经验循环）已并入同闸门。剩余为 3-pass 管线集成（setup→`FUN_0041307f`→`FUN_004138ea` 的跨 pass 缓冲持久化）与 case 9 仓库语义，作为残余集成项。
- 口径由 `scripts/build_re_map.py` 刷新，状态以 `output/re_map/status_summary.md` 为准。
- **构建门总口径（2026-10-26 续）**：`dlzr_tests` 全量 **238/238 PASS**（含 `DatIoTest` 真实 IO 14/14）；Phase 5（战斗内移动范围 1:1）+ 候选 2（Phase 6 对话整屏驱动）已收口；函数语义化覆盖率仍为 **14/755**（pilot），741 桩待候选 3 规模化深化。

---

## 6. 已知风险与诚实保留

1. `msvc32_compat.h` 的 6 个数学/CRT 垫片（`__allshl` 等）为**重建语义**（参数布局按 Ghidra 寄存器恢复），仅保链接、不触测试路径，待逐函数 RE 正名。
2. 7 张战斗表在 `battle_calc.hpp` 以独立 `extern` 声明，与 `g_game_state.g_misc_blk_5ab0@0x475ab0` **同址异址**；运行时 `fread` 装载须统一到一处（当前不阻塞测试，待集成验证）。
3. 二进制 PE 静态段无这些表的字节（运行时 fread 装载），故表**内容**只能验证控制流/索引正确性，不能静态断言数值。
4. `g_ddraw` 在 `replica_stubs.c:22048` 无 NULL 检查被解引用 → dsound/ddraw 必须**真链**（已链），不得桩成失败。
5. clean 层 ↔ replica 层的**跨层差分测试已建立**（`test_cross_layer_diff.cpp`，9 例，1–2 万次随机对拍 0 分歧），原"最大验收缺口"已补（2026-10-24）。经验/升级公式已收口并修正 clean 层（删除错误 `calc_experience`，修复"不做升级"bug）。命中机制已 RE 实证为独立函数 `FUN_0042ABB0`/`FUN_0042AC0B`（伤害前命中闸门，武器版命中率 += 技能等级），已收口进 oracle + `BattleCalc` 14 函数对拍（含 `MaxHp`/`ClampResource`/`HitChance` 跨层）。
6. **`anim_frame_offset` 误命名正名（2026-10-24 续）**：`renames.csv` 中 `0x00429E31` 旧名 `anim_frame_offset`（含编造证据坐标 `FUN_00429e31.c:907-913`）系望文生义——Ghidra 实码为 `ClassType.hp_base + (level-1)*ClassType.hp_growth` = **最大HP**，与 clean `calc_max_hp` 逐位一致。已正名为 `max_hp_getter_9e31`（转发至 `battle_calc_max_hp`），旧名撤销。此即铁律零"望文生义"陷阱的现场证伪，已就地修正。

7. **`FUN_004138ea` 跨层差分闭环（2026-10-26）** + **T3-core `FUN_0041307f` per-cell 伤害核心（2026-10-25）**：16 分支 oracle(`replica/battle_sim.cpp`)/clean(`logic/battle/battle_sim.cpp`) 双语逐位一致，跨层差分 4 测 0 分歧；全量 118/118 PASS。跨层差分闸门**反例价值**正面体现：首跑即抛 SEH 0xc0000094（除零），根因为"有符号误读二进制无符号"——case 1 `local_5c=(uVar9*缓冲)/100` 被转录为 `int32_t`，当 `morale>100` 时 `100-108` 有符号得负→`local_5c` 假非零→除零；二进制实码 `100U-morale` 无符号回绕→不除。已按 Ghidra 把所有 `local_X` 收口为 `uint32_t`（§9.2）。另修正 case 1 non-player 贡献阈值 `<2?1`（二进制 `:172-173`，非 case 0 的 `<3?2`）。剩余诚实缺口：① case 9 仓库 `FUN_0042b23a`/`0x7db3a8` 双端 no-op 延后；② 3-pass 跨 pass 缓冲持久化未接 `GameState` 运行时；③ `0x21` 记录数组对 `idx>0` 斜向语义、`FUN_0043de50` 播种源待实证（§9.3）。
8. **T3-b `FUN_0041307f` 死亡/经验循环（2026-10-25）**：接 T3-core 之后，双语（oracle `battle_calc_apply_death_experience` / clean `apply_death_experience`）逐指令转录 `FUN_0041307f.c:146-195`——阵亡判定(`hp==0||morale==0`→`active_flag=0`)、击杀经验 `battle_calc_kill_exp(occ,attacker)` 累加、敌方物品回收 `count_unit_skills`(FUN_0042afd2)、阵亡计数(`g_aux_counter_b/a`)、主将 HP 封顶(`max_hp(0)`)+剩余单位倒数地板 1、末尾 `battle_calc_gain_exp` 应用总经验；`BattleGridState` 扩 `experience`/`lost_player`/`lost_enemy`/`battle_win_mode`/`player_units_remaining`/`item_recovery[256]`，`mirror_grid` 同步。新增 `TEST(SkillGridDiff, CellDeathExp)`（N=10000）。**全量 119/119 PASS**。修复：oracle 死亡循环原读 `g_grid_rows`（被测试 `resize_grid_buffers(13,13)` 占位污染成 13×13，而 `big=0` 应 9×9，遍历越界致 oracle_total=5652 vs clean_total=1096）→ 改从 `g_battle_double_grid` 派生尺寸（与二进制 `:36-47` 一致），pass-1 同步维护 `g_grid_rows`/`g_grid_cols` 防其它读取者踩坑。
9. **regen 保真 backlog 已清零（2026-10-26 续十八/十九）**：原 5/6 个 stateful-stub 失败实为 `bak_promote` 转录体两类系统性 bug——① `int32_t*` 指针-hack 索引放大 4×（people/skill 数组按 int 元素步进而非字节步长，落到错字节）；② 硬编码绝对地址 `0x833414` 写入（replica 未映射 → SEH 0xc0000005）。两处已修（`replica_stubs.c` 改直接数组索引 `people_array[N].enemy`/`skill_array[N].movement_index` + 写 `g_game_state.g_grid_833414[(y*W)+x]`），全量 **227/227 PASS**。残留工程债（未清，2026-10-26 续实测更正）：`replica_stubs` 仍依赖 `.bak_*` 手修态 + 手工编辑；本 turn 验证"重生成可复现绿态"为**假绿**——生成器输出偏离 3733 行且自身 17 编译错误编不过，须把手工补丁折回 `gen_replica_stubs.py` 方能收口（候选 3 regen 阻塞项）。

---

## 7. 下一步候选 / 推进 Backlog（2026-10-26 编排 · 对应"4 & 1 & 2 & 3"）

当前已站在 **Phase 4 收官线 + Phase 5 起点**。下列四项为已识别的下一步候选，按"依赖最少 / 杠杆最高"排序。除④为纯 RE 勘误外，①②③均需单独开 Phase 子任务并走对应 Gate。

### 候选 4 — minor 非阻塞 RE 勘误（铁律零诚实保留项 · 优先级分离但列首）
- **④a `0x21` 记录数组 = 单位智力字段（已坐实，非"斜向语义"）**：`DAT_007d0f18[idx*0x21]`（stride 0x21*4=0x84=132B=People 步长；`0x7d0f18/1c/24` 为同记录 +0/+4/+0xC 三 int 字段）经 RE 实证为**单位 intelligence(智力)**：`FUN_0042abb0.c:13` `((&DAT_007d0f18)[a*0x21]-[b*0x21])*2+0x1e` 正是已确认**命中率公式 `(攻智-守智)*2+30`**；`FUN_0042ac0b` 同；`FUN_0042acb9.c:28` 从 People 数组(0x7d51c8, stride 0x84)拷入 0x7d0f18 → 战斗期智力镜像。`FUN_004138ea.c:65/69` 读它进 `uVar8/uVar9` 技能范围计算。路线图旧写"idx>0 斜向语义"为**误记，已更正**（无斜向/方向语义）。
- **④b `FUN_0043de50` = 标准 memmove（已坐实 + 已加回归测试）**：664B RTL（前向/后向拷贝 + 4 字节对齐快路径 + 字节尾），由 `FUN_00412030.c:51` 把 HP 缓冲 `0x7d0488` 播种进士气缓冲 `0x4565d0`（0x208B）。replica 转录为 `memmove_impl`（`replica_stubs.c:28751`，委托 `memmove()`）。`tests/test_replica_stubs.cpp::ReplicaStubs.MemmoveImplByteExact` 钉死字节级语义含重叠反向拷贝（区分 memmove/memcpy）。
- **④c case 9 仓库语义（已读源，仍为诚实缺口）**：`FUN_0042b23a` 遍历单位战斗物品记录 `(&DAT_007d0f6c)[slot+unit*0x84]`（8 槽），取首个非空槽 `item_id-1`，`FUN_0042920d` 校验可入库则 `*(short*)(&DAT_007db3a8+item_id*2)++`（全局仓库计数 0x7db3a8），并清槽。`rf_004138ea` 当前未 forward case 9 → 仓库不更新。修复=把 `case 9` 接 `rf_004138ea` 转发集成（§9.3①/③），与 battle_calc 仓库逻辑对齐。
- 以上均**不阻塞当前 227/227 绿态**；④a/④b 已坐实，④c 为剩余 forward 集成项。

### 候选 1 — Phase 5 地图与地形 / 寻路（最顺的自然下一步）
- **依赖（已就绪）**：Phase 4 已装载真实地形表（`GAME.DAT` 块#1 @`0x1E0` stride `0xA0`、地形修正 `0x475ab0`）、地形网格（`MAP.DAT` tile `>>12` stride 0x40）、地图宽 `0x57ba20`、`g_pathfind_grid[MAP_GRID_CAP]`（`MAP_GRID_CAP=16384`，跨层差分已建 28+ 处 `=1`/`=0` 写入）。
- **范围**：寻路代价（原误归 battle 缓存块之一，须正名）；与 `replica` 地形取值 0 分歧。
- **证据坐标**：`FUN_00424566.c:15`（地形修正表）、`FUN_00416c70.c:24`（地形网格 `tile>>12`）、`FUN_00412030.c:51`（HP→士气缓冲 `memmove` 播种，即 `FUN_0043de50`）。
- **已收口（2026-10-26 → 2026-10-26 续）**：
  - 独立 `PathFinder`（`map_model.cpp`）= 地形加权 Dijkstra，`move_cost = class_move_cost[兵种][地形>>12]`，与二进制 `FUN_00418e29`（697B，4 向预算受限洪水填充，步进代价 `DAT_0095e4f8[(tile>>12)+兵种*0x10]`）**同源代价模型**。
  - 新增 `MapPathTest.FloodFillReachableMatchesDijkstra`：把 `FUN_00418e29` 语义重演（`binary_flood_reach`）与 clean `PathFinder::reachable_count`（独立实现 Dijkstra）在真实 map0 上多预算（1/2/3/5/8/15/30/200）可达数差分 → **0 分歧**（229/229 全绿）。
  - 更正 `map_model.hpp` 注释：`FUN_00418e29` 已反编译（非"尚未反编译"），方向性代价差异标注为已知 residual。
- **★ 战斗内移动范围 1:1 已收口（2026-10-26 续）**：
  - **关键 RE 更正**：前版"剩余 BLOCKER = FIGHT.DAT `intro.grid_terrain[92]` 到 MAP.DAT 的 92→MAP 坐标映射未逆"是**误判**。铁律零复核证据链：
    - `FUN_00416c70.c:24` 把 **MAP.DAT 地形**装入 `DAT_00584390`（`map_width*2*map_height` 个 ushort，索引 `(y*w+x)`，高 4 位 = 地形旗标）；
    - `FUN_0042a95b.c:10` `get_terrain(x,y)=DAT_00584390[(y*w+x)]>>0xc`；
    - `FUN_00418e29.c:14-54` 移动范围洪水填充读 `DAT_0058438e/90/92`（相邻 ±1ushort 偏移，巧妙指向"进入格"地形）作步进代价 = `class_move_cost[进入格地形>>12][兵种]`，预算 = 移动力。
    - `FIGHT.DAT intro.grid_terrain[92]`（DAT_007d030c）经全量扫描**从未写入地形缓冲**，仅在 `FUN_004084e8/FUN_0040d29b/FUN_0043b4f1` 作为坐标/指针表读取 → 实为坐标/覆盖层，**非移动地形来源**。
  - **落地**：`BattleEngine::init_grid` 现把 `grid_terrain` 从 `map_->tiles` 的 `(tile>>12)&0xF` 填充（取代全 0 占位）；`calc_movement_range`（`battle_engine.cpp:383`）由均匀 `cd+1` BFS 改为**地形加权 Dijkstra**（代价 = `class_move_cost[邻格地形旗标][兵种]`，`>=99` 不可通行，预算 = `get_movement_range`）；`place_mine` / `bfs_step_toward` 的 `==99` 硬障碍判定统一改走新 helper `is_hard_obstacle(cell)`（全兵种均 99 即硬墙）。
  - **交叉校验（真实数据）**：新增 `test_movement_range.cpp` 两用例 —— `GridTerrainFilledFromMap`（grid_terrain 确实由 MAP.DAT 填充、旗标 0-15）；`MatchesBinaryFloodFill`（用真实 fight0/map0 把 `calc_movement_range` 可达集 vs `FUN_00418e29` 重演**逐格 0 分歧**）。全量 **231/231 全绿**（229+2）。
  - 结论：Phase 5「战斗内移动范围 1:1」**已达成**，原"92→MAP 坐标映射"伪 BLOCKER 撤销。

### 候选 2 — Phase 6 对话整屏渲染接入主循环（Tier A）✅ 已收口（2026-10-26 续）
- **现状（已落地）**：对话数据/令牌层（见上）已就绪；新增 `DialogueScene` 主循环级整屏播放驱动，把 `DialogueManager` + `DialogueRenderer` 串成可玩的整屏对话。
- **驱动实证（铁律零，逐指令）** `FUN_0043d82f`（整屏对话驱动）：5 行/页（`while local_15c<5`）、每行 18(有头像)/23(无头像) 字模槽（`DAT_0098d814=0x24/0x2e` → 18/23 槽，`FUN_0043cfe1.c:34`/`FUN_0043d454.c:16`）、面板 FRAME00@(118,296,404,127)、头像@(130,308,80,100)、文本起点(222,312)/(142,312)、行高 20、字模固定 16px 单宽（`FUN_004061df.c:98`）；逐行揭示（local_15c 1→5 后 99=全显）、鼠标抬起(0x202)跳全显并推进到下一页、文本耗尽退出。
- **落地**：`src/presentation/dialogue/dialogue_scene.hpp/.cpp` 新增 `DialogueScene`：
  - 公开常量/坐标与二进制严格一致（面板/头像/文本/行高/行数/每行槽数，已断言）。
  - 静态 `paginate(text,has_face)`：按 18/23 可见字模槽换行（AA/BB/CC/DD 标记对占 0 槽、不拆行），再按 5 行/页分组。
  - 推进状态机 `advance()`：当前页未显→跳全显（二进制 0x202→99）；已显→下一页/下一条目；末尾→finished。
  - `run(Platform&)` 主循环入口：逐行自动揭示→点击/按键跳过或推进→结束（可接入 app 主循环）。
  - GG/TT 名称替换钩子（`set_name_resolver`，默认不替换，替换串待 RE 确认——诚实残留）。
- **差分测试**：`tests/test_dialogue_scene.cpp` 7 用例（整屏坐标一致 / 18·23 槽换行 / 5 行分页 / 标记 0 槽不拆 / 推进状态机 / 空序列 / 名称钩子）全 PASS。与既有 `test_dialogue_render.cpp`（令牌配色）共同覆盖 Tier A 逐字配色 + 整屏版面。
- **闸（G-Subsys + G-TierA 局部）**：满足。残诚实项：GG/TT 确切替换串待 RE；头像 RGBA 数据源由 app 接线（`set_face_provider` 机制已留）。

### 候选 3 — 规模化深化桩函数 RE（regen 收口 + ABI 还原）
- **✅ regen 收口达成（2026-10-26 续2 · 真绿，非假绿）**：生成器 `gen_replica_stubs.py` 重生成的 `replica_stubs.c/.h` **本身 0 编译错误**，`dlzr_tests` 全量 **238/238 PASS**（含 `StubRegistry.AllDeclaredSymbolsLinked` 链接门）。收口手段全部折回生成器，重生成可复现绿态，`.bak_*` 手工层正式废弃（仅作历史参考，regen 即唯一来源）。
- **折回生成器的关键修复（带坐标）**：
  1. `fix_memmove_calls`（line ~393）：兜底分支原样保留函数名 `src[idx:p]`，修 C2059（`int32_t *(...)` 缺标识符）。
  2. `BODY_OVERRIDE` 表（line ~334）+ `sig_override.csv`：6 个绿态已验证原语（memmove_impl / nearest_color_match_21d10 / remap_bytes_lut / ptr_read_u32/u64/u16）用绿态实现替代逐字转录，修 C2664 实参类型不匹配。
  3. **`count_unit_skills`（RF:0042afd2 · 2026-10-26 续2 新增）**：转录体 `*(int16_t*)(&g_game_state.g_state_blk_b3a6 + uVar1*2) += 1` 是 Ghidra 转录伪影——`g_state_blk_b3a6` 是 `int32_t` 标量（struct idx 1327），紧邻 `g_flow_phase`（idx 1334），`uVar1==4` 时该指针算术正好越界落到 `g_flow_phase` 低 16 位，致 `SkillGridDiff.CellDeathExp` 失败（`g_flow_phase` 1→5）。证据 `output/ghidra_all/FUN_0042afd2.c:1-22`（敌方单位 8 物品槽→`DAT_007db3a6[uVar1]++`）。覆盖为 `if(enemy!=0){ for s<8: item=items[s]; if(item) g_item_recovery[item]++; }`（与 clean 层 `logic/battle/battle_sim.cpp:711-717` 逐字对齐），`g_item_recovery` 经 `battle_calc.hpp` 已声明。
- **诚实保留**：regen 与历史手修绿文件仍有 ~316 函数体差异（转录风格/数组访问口径），但全部 238 测试断言 0 分歧，说明差异不在被测可观测路径。语义覆盖率仍为 **14/755**（pilot），741 桩待后续 ABI 还原深化。
- **🔧 ABI 还原批次·一（2026-10-26 续3 · 调用点还原签名，折回 `sig_override.csv`）**：`resolve_unresolved_abis.py` 反向调用图显示 **21 个 unresolved 桩**（`()`+形参作未初始化局部 → 调用方传参被丢弃的潜在 UB，仅因在 data-load/渲染路径未被 238 战斗测试触发）。机制：生成器对 `sig_override.csv` 命中项发真实参数签名、不再把形参当局部重声明（`gen_replica_stubs.py` line ~1226）。逐函数从 `FUN_*.c` 首行 Ghidra 解析签名（形参数目权威）+ 反向调用图校验**调用点实参数目一致性**后，把 **16 个一致（SAFE）桩** 的签名折回 `sig_override.csv`：
  - 6 参：`004052f3`(draw_masked_sprite) / `00405417` / `00421a20`(双向扫描相等单元) / `00421af3`(沿行扫描) / `00422571`(rect test) / `004226b6`(Blt 封装)
  - 7 参：`00422a78`(透明色键块拷贝) / `00427313`(draw formatted value)
  - 8 参：`004240b9`(alpha crossfade) / `00423d96`(clipped blit)
  - 9 参：`00423cb1`(colorkey blit) ｜10 参：`00423bd7`(surface rect copy) ｜11 参：`00423cb1` 见上 ｜4 参：`0043e510`(fread_impl，消除 dst/width/count/FILE* 被丢弃的 UB)
  - leaf（0 调用方，直接折）：`0042241f`(Bresenham) / `00423269` / `00423438`(horizontal wipe blit) / `00423bd7` 见上
  - **结果**：regen → `cmake --build` **0 error**；`dlzr_tests` 仍 **238/238**；`resolve_unresolved_abis.py` 未解析数 **21 → 5**。证据链：每个签名 = `FUN_<addr>.c` 首行 Ghidra 解析 + `arg-count` 跨调用点一致性校验（全 16 个 best 计数 = total）。
- **⚠️ 5 个 RISK 桩刻意不还原（诚实保留）**：`00422203`(6:11/2:1) / `00422627`(6:65/3:1/7:2) / `00422769`(8:204/3:1/9:1) / `0042728b`(4:85/3:12) / `004412a0`(2:8/1:1)。反向调用图显示其调用点实参数目**与 Ghidra 签名不一致**（如 `FUN_00419cdf.c` 12 处给 `0042728b` 传 3 参而非 4，`FUN_004237a8.c` 给 `00422769` 传 3 参而非 8）。C++ 下固定签名会在少数调用方触发 C2660；根治须改 `FUN_*.c` 源体调用点实参（属渲染/DDraw 路径，无单测覆盖验证，不在本批范围）。故保留 `()`+局部形参现状，绿态不回退。**根因实证（2026-10-26 续四）**：自写 PE 解码器解析 `DLZFULL/DLZ.exe`（ImageBase=0x400000, i386, 5 section）证实其为 **Borland `register` 调用约定**——前 3 参数走 EAX/EDX/ECX 寄存器、其余才压栈；连已知 3 参 cdecl 的 `memmove_impl`(FUN_0043de50) 调用点都**显示 0 个栈压入**（参数在寄存器）。5 桩 Ghidra 签名缺参，正是寄存器传参被漏解所致；其真 arity 须寄存器级反汇编（本环境无可用工具）才能定，铁律零禁止推测丢弃的寄存器参数 → 保持 `()` 桩为唯一合规选择。详见续四。
- **工作流（ABI 还原，须落 `FUN_*.c` 源体）**：`resolve_unresolved_abis.py --candidates` 产出"可还原"批次 → 从调用点还原真实 ABI → 写进 `FUN_*.c` 源签名 → 折回生成器（或手工同步 committed）→ 补 oracle 差分。
- **🧪 1&2&3 推进收口（2026-10-26 续四）**：
  - **任务1（RISK 桩）DONE**：5 桩经 PE 解码实证为 Borland `register` 约定，寄存器传参被 Ghidra 漏解，真 arity 不可安全还原 → 保持 `()` 桩（铁律零合规），全量绿态守住。
  - **任务2（oracle 差分提拉）DONE**：`test_replica_stubs.cpp` 新增 8 用例（+5 `CopySkipChar*` +3 `PaletteFindNearest*`），覆盖 2 个新纯函数桩 **`copy_skip_char`(0042207c)** 与 **`palette_find_nearest_ex`(00421ddb)**（含"首个低于阈值索引 / 全程越界返全局最近(受 20000 硬上限约束) / 阈值边界 d==阈值不返回"锚点）。期间**暴露并修复 00421ddb 真实转录缺陷**：原 `GHIDRA_FIELD`/`CONCAT22` 重建调色板指针字节序倒置（0x10000→0x1）致解引用崩溃；另 B 项反编译混淆（`CONCAT12((undefined1)param_2,param_1._2_2_)&0xff`，含指针低字节）与兄弟 00421d10 同源，已在 `BODY_OVERRIDE["00421ddb"]` 订正为 `(param_1>>16)&0xff`。修复折回 `gen_replica_stubs.py`，regen 复现绿态。
  - **oracle 覆盖口径（独立于 Phase 2 战斗数值 14/755 pilot）**：`test_replica_stubs.cpp` 纯函数桩 brute-force oracle 对拍现 **16 符号**（rand_impl / memmove_impl / check_span_room / delta_direction_code / copy_int_block / remap_bytes_lut / nearest_color_match_21d10 / swap_ints / rect_overlap / rle_byte_expand / copy_skip_char / palette_find_nearest_ex / **strcpy_impl / strcat_impl / abs_int_de40 / codepage_to_charset**）；`test_stateful_stubs.cpp` 另含受控 `g_game_state` 的 stateful oracle 差分（其中 abs_int_de40 / codepage_to_charset 亦有受控用例交叉印证）。
  - **终态（1&2&3 收口）**：`cmake --build --config Release` **0 error**；`dlzr_tests` 全量 **246/246 PASS**（238 + 本批 8）。`StubRegistry.AllDeclaredSymbolsLinked` 链接门绿。
- **🧪 覆盖抬升续轮（2026-10-26 续五 · "继续"）**：
  - **新覆盖 4 纯函数符号 + 11 用例**：`strcpy_impl`(0043e190) / `strcat_impl`(0043e1a0) 经典 CRT 字对齐+0x7efefeff 双字零检测拷贝/拼接（含对齐与非对齐路径、空串、长串多字块）；`abs_int_de40`(0043de40) 整数绝对值（INT_MIN 两补码回绕）；`codepage_to_charset`(00441f40) 码号→字符集 switch 查表（4 已知映射 + 默认 0）。全部逐字节/逐值对拍绿。
  - **发现脚本沉淀**：`scripts/discover_oracle_candidates.py`（纯函数打分）、`scripts/discover_mem_candidates.py`（指针内存类）、内联标量算术发现（无指针/全局/risky，内部≤2，≤45 行）—— 三网合计仅 7+37 候选，且剔除 noop 包装 / Win32 I/O 包装（mci/message/fopen/file_read/ClearScreen/play_sfx/load_palette_file）/ PE 间隙全局扫描后，**可独立 oracle 的高价值纯函数已接近挖尽**。
  - **剩余走向（需用户择向）**：① stateful 桩（count_units_flag_loaded / any_unit_flag_loaded 等）依赖 DAT_ 全局多为 PE 间隙地址，须先把真实内存布局接进 `g_game_state` 才能受控 oracle；② 5 RISK 桩须寄存器级反汇编工具方可 ABI 还原；③ I/O 桩须 Win32 mock 才能单测。三者皆非"无依赖纯函数"，收益递减。
  - **终态（续五）**：`dlzr_tests` 全量 **257/257 PASS**（246 + 本批 11）。0 error。
- **目标（更新）**：regen 收口已完成（绿态自洽）；unresolved 21→5（保持 `()` 桩，Borland register 约定实证不可还原）；pure 桩 oracle 差分已扩展至 16 纯函数符号 + stateful 集（257/257）。清洁纯函数候选已近枯竭，下一动作取决于上述三个走向的用户择向。
- **🧪 覆盖抬升续轮（2026-10-26 续六 · "继续"）· 结构性墙实测**：
  - **清洁纯函数已彻底枯竭**：纯标量发现 36 候选里 26 个是 noop / Win32 I/O 包装 / SEH / 文件读，无可用纯算术 oracle 目标；确认续五"井已挖干"论断。
  - **stateful 安全子集已近饱和**：`test_stateful_stubs.cpp` 已有 40 个 `TEST_F` 受控用例；本轮仅能再加 2 个**确属安全**（只写/取标量字段、无数组越界）的行为锁：`set_text_colors`(00405560，三色各 `&0xff`) 与 `get_global_ptr_4fdc0`(返 `&g_rtstate_fdc0` 标量地址)。全量 **260/260 PASS**。
  - **⚠️ 结构性墙（铁律零诚实保留 · 阻断进一步 stateful 覆盖）**：发现 `GameState` 把许多"数组区域"全局**声明为标量**（`int32_t g_rtstate_b250; /* 标量 */`），但桩体用 `&g_game_state.X + 大偏移` 当数组基址访问——如 `zero_128x20_structs`(00401000) 写 `&g_rtstate_b250 + local_8*0x14` 达 0x80×0x14 字、`grid_cell_table_reset_199f9`(004199f9) 写 `g_buf_blk_3208[0..0x7f]`、`count_nonzero_shorts`(0043a7c0) 读 `g_tbl_blk_2810[0..0x1f]`、`count_slots_1d22`(00431d22) 读 `people_array[0].items[param_1*0x84+slot]`（items 仅 `uint8_t[8]`，下标越界）。分类器实测 **59 个"标量基址数组滥用"桩**须 struct 重构才可安全对拍；且分类器有漏网（`items[0x84+]` 越界被误判安全）。**这些桩若写假绿测试 = 断言 UB/越界内存，违反铁律零，一律不做。**
  - **结论**：pure 清洁 oracle 与"安全 stateful"行为锁均已近饱和；剩余 stateful 覆盖（~59 标量基址数组桩 + AI 战斗逻辑桩）被**数组区域建模缺口**阻断。真实解法是把"数组区域"全局在 `GameState` 里**按 Ghidra 证据声明为真实数组**（如 `int32_t g_rtstate_b250[0x80*0x14]`），并改写所有访问点。这是架构级改动，会影响 1914 全局布局与既有 262 测试，需用户定夺路径（见下条择向）。
  - **隔离陷阱实证（续六补 · 铁律零）**：续六 B 路径又撞墙——`collect_active_slot_idx`(00402d3f) 用 `(&g_datatbl_b030)[0..0x7f]` 写 0x80 字（g_datatbl_b030 标量基址滥用，漏网）；更关键：`mode37_init_e407`(0040e407) 看似"无条件写两标量"的清洁 setter，实则内嵌 `play_mpeg_clip`/`scene_render_dispatch` 解引用 null 图形指针，孤立态 **SEH 0xc0000005 崩溃**——证明"标量 setter 包装"若含图形/音频子调用亦不可孤立对拍，已撤除。净增仅为 `audio_stop_if_playing`(0043c76b) 2 行为锁（g_flag_c 条件写 g_ui_blk_d80c）。
  - **终态（续六）**：`dlzr_tests` 全量 **262/262 PASS**（257 + 本批 5 尝试 − 1 SEH 撤除 = 净 +2 行为锁 + 续五 11 = 262）。0 error。清洁可 oracle 的安全集判定为**饱和**：再往下只剩（a）标量基址数组滥用桩（须 struct 重构）或（b）内嵌图形/音频子调用的"伪 setter"（孤立态崩，不可测）。
- **🅰️ 下一步架构分叉（需用户拍板 · 不可盲选）**：
  - **A. 数组区域 struct 重构**：扫描 Ghidra 把每个"被当数组用的标量区域"按其实测尺寸声明为数组，改写所有访问点 → 解锁 ~59 标量基址桩 + 真实语义化。工作量中，但触及 1914 全局布局、需全量回归。
  - **B. AI/战斗逻辑 stateful 桩直测**：挑 `people_cond_a/b`、`collect_active_slot_idx`、`ai_find_enemy_unit_wrap` 等**已正确声明为真实数组**（people_array/fighter_array）且无越界的桩，布设 fighter_array/people_array 受控态对拍（不碰标量基址区域）。纯增量、零风险，但覆盖的是"已数组化"子集，绕开结构性墙。
  - **C. 暂停覆盖抬升，转 Phase 推进**：覆盖抬升边际收益已低（纯函数枯竭 + 结构性墙），资源转回 Phase 5/6/8 主线（地图/对话/数据装载）更有杠杆。
