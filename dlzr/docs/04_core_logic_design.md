# DLZR 核心逻辑设计文档

## 抗日之地雷战 1:1 复刻工程

**日期**：2026-10-09  
**状态**：评审中

---

## 1. 数值体系

### 1.1 角色属性 (People, 0x84 bytes)

| 偏移 | 字段 | 类型 | 说明 | 取值范围 |
|------|------|------|------|----------|
| +0x00 | name_id | int32 | 名称 ID | 0-39 |
| +0x04 | portrait_id | int32 | 头像 ID | 0-169 |
| +0x08 | face_id | int32 | 面部 ID | 0-169 |
| +0x18 | hp | int32 | 当前 HP | 0 - max_hp |
| +0x1C | strength | int32 | 武力 | 0-99 |
| +0x20 | intelligence | int32 | 智力 | 0-99 |
| +0x24 | level | int32 | 等级 | 1-99 |
| +0x28 | experience | int32 | 经验值 | 0-9999 |
| +0x2C | class_type | int32 | 兵种索引 | 0-89 |
| +0x30 | skill_use_count | int32 | 技能使用次数 | 0-∞ |
| +0x34 | skill_data[16] | int32×16 | 各技能使用计数 | 0-∞ |
| +0x58 | skill_exp | int32 | 技能经验 | 0-9999 |

**HP 计算公式**：
```
max_hp = class_type[class_type].hp_base + (level - 1) * class_type[class_type].hp_growth
```
HP 为运行时计算值，不存储于 People 结构体中。

### 1.2 战斗属性 (Fighter, 0x40 bytes)

| 偏移 | 字段 | 类型 | 说明 |
|------|------|------|------|
| +0x00 | max_hp | int32 | 最大 HP |
| +0x04 | current_hp | int32 | 当前 HP |
| +0x08 | morale | int32 | 士气（影响攻防修正） |
| +0x0C | equip_integrity | int32 | 装备完整度（攻防修正） |
| +0x10 | defense_modifier | int32 | 防御修正系数 |
| +0x20 | equip_data1 | int32 | 装备数据 1（列参数） |
| +0x24 | equip_data2 | int32 | 装备数据 2（行参数） |
| +0x38 | attacker_index | int32 | 攻击者索引 |
| +0x3C | status_flag | int32 | 状态标志 |
| +0x3F | action_type | uint8 | 行动类型 |

### 1.3 兵种属性 (ClassType, 0xA0 bytes)

| 偏移 | 字段 | 类型 | 说明 |
|------|------|------|------|
| +0x00 | hp_base | int32 | 基础 HP |
| +0x04 | hp_growth | int32 | HP 成长率 |
| +0x08 | attack_growth | int32 | 攻击成长率 |
| +0x0C | defense_growth | int32 | 防御成长率 |
| +0x14 | base_attack | int32 | 基础攻击力 |
| +0x18 | base_defense | int32 | 基础防御力 |

---

## 2. 战斗公式

### 2.1 攻击力计算 (0x0042A82A)

四层计算结构：

```
Layer 1 — 基础攻击:
  raw_atk = (class_base_atk × 3 × strength / 100)
          + (attack_growth - 1) × (level - 1)

Layer 2 — 武力×技能缩放:
  attack = (skill_level + 59) × (raw_atk × (strength + 500) / 580) / 60

Layer 3 — 战斗模式修正 (仅 game_mode == 0x96):
  attack = attack × equip_integrity / 100
  attack = attack × (morale + 100) / 200

Layer 4 — 物品加成:
  attack += get_item_attack_bonus(person_idx)
```

**关键常量**：
- SKILL_LEVEL_BASE = 59 (0x3B)
- SKILL_DIVISOR = 60 (0x3C)
- ATTACK_SCALE_DIV = 580 (0x244)
- STRENGTH_BASE = 500
- MORALE_BASE = 100
- MORALE_DIVISOR = 200
- INTEGRITY_DIVISOR = 100

**整数除法注意**：所有中间步骤使用整数截断除法（向零取整），与原作 x86 `idiv` 行为一致。

### 2.2 防御力计算 (0x0042A98E)

```
Layer 1 — 基础防御:
  defense = class_base_defense + (level - 1) × defense_growth

Layer 2 — 战斗修正 (仅 game_mode == 0x96):
  defense = defense × defense_modifier / 100
  defense = defense × (morale + 100) / 200

Layer 3 — 地形加成:
  terrain_bonus = terrain_defense_table[terrain_idx]  // 首字段, stride 20B
  defense += defense × terrain_bonus / 100

Layer 4 — 物品加成:
  defense += get_item_defense_bonus(person_idx)
```

**地形索引获取**：
```
terrain_idx = calc_equip_defense(
    cinematic_pos_x[person_idx × 0x10],  // stride 0x10 = 4 ints
    cinematic_pos_y[person_idx × 0x10]
)
```

### 2.3 技能伤害计算 (0x0042B2E0)

```
skill_level = get_skill_level(people[caster].skill_data[skill_idx])

hp_effect     = ((param1 × intelligence) / 100) × (skill_level + 7) >> 3
morale_effect = ((param2 × intelligence) / 100) × (skill_level + 7) >> 3
```

**16 种技能效果类型**：

| 类型 | 效果 | param1 含义 | param2 含义 |
|------|------|-------------|-------------|
| 0 | 回复 HP | HP 回复系数 | — |
| 1 | 伤害 HP | HP 伤害系数 | — |
| 2 | 提升士气 | — | 士气提升系数 |
| 3 | 降低士气 | — | 士气降低系数 |
| 4 | 火攻 | HP 伤害系数 | — |
| 5 | 滚木 | HP 伤害系数 | — |
| 6 | 擂石 | HP 伤害系数 | — |
| 7 | 威慑 | — | 士气降低系数(30%) |
| 8 | 骚扰 | HP 伤害系数 | 士气降低系数 |
| 9 | 掠夺 | HP 伤害系数 | — |
| 10 | 反间 | — | 士气降低系数 |
| 11 | 疑兵 | — | 防御修正降低 |
| 12 | 督战 | — | 防御修正恢复 |
| 13 | 暗杀 | HP 伤害系数(即死判定) | — |
| 14 | 施毒 | HP 伤害系数 | — |
| 15 | 空袭 | HP 伤害系数 | 士气降低系数 |

### 2.4 技能等级查询

技能等级通过查表获取，表基址 0x447280，等级范围 0-10：

```cpp
int32_t get_skill_level(int32_t skill_use_count) {
    static const int32_t skill_level_table[11] = {
        0, 1, 3, 6, 10, 15, 21, 28, 36, 45, 55
    };
    for (int32_t level = 10; level >= 0; level--) {
        if (skill_use_count >= skill_level_table[level]) {
            return level;
        }
    }
    return 0;
}
```

### 2.5 经验值计算

经验值奖励基于击杀者等级与被击杀者等级差值：

```
base_exp = 经验基础值（由被击杀角色兵种决定）
level_diff = killer_level - victim_level

if (level_diff < -5)    multiplier = 2.0
elif (level_diff < 0)   multiplier = 1.5
elif (level_diff < 5)   multiplier = 1.0
elif (level_diff < 10)  multiplier = 0.75
else                    multiplier = 0.5

final_exp = base_exp × multiplier
```

### 2.6 伤害结算

```
damage = calc_attack_power(attacker) - calc_defense(defender)
if (damage < 1) damage = 1  // 最低 1 点伤害

target.current_hp -= damage

if (target.current_hp <= 0) {
    process_fighter_death(target_idx)
    award_experience(attacker_idx, target_idx)
}
```

---

## 3. 战斗状态机

### 3.1 回合调度 (calc_behavior, 0x0042C454)

```
void calc_behavior(int command) {
    switch (command) {
    case 0x100:  // 胜利结束
    case 0x202:  // 撤退结束
        → 清理 UI, 重置状态, finalize_battle()
        → game_mode = 1 (地图模式)
        break;
        
    case 0x113:  // 推进回合
        battle_phase++
        
        if (battle_phase 为奇数) {
            → 渲染当前画面, flip, return
        }
        
        // 偶数回合
        if (battle_phase == 10)   → battle_delay(0x385)
        if (battle_phase == 150)  → battle_delay(0x386)
        if (battle_phase == 270)  → battle_delay(0x387)
        
        if (battle_phase >= total_fighter_count × 2 - 2) {
            → 回合耗尽, 结束战斗
        }
        
        → 渲染当前画面, flip
        break;
    }
}
```

### 3.2 游戏模式状态表 (game_mode @ 0x7DB4E8)

| 值 | 十进制 | 处理器地址 | 说明 |
|----|--------|-----------|------|
| 0x00 | 0 | 0x0042C215 | 初始化 |
| 0x01 | 1 | 0x0042C620 | 地图模式 / 获取战斗行为 |
| 0x02 | 2 | 0x0042CFEA | 状态 2 |
| 0x03 | 3 | 0x0042D217 | 状态 3 |
| 0x04 | 4 | 0x0042D686 | 状态 4 (最大: 1560B) |
| 0x05 | 5 | 0x00430084 | 状态 5 |
| 0x06 | 6 | 0x00430253 | 状态 6 |
| 0x07 | 7 | 0x0042F938 | 状态 7 |
| 0x32 | 50 | 0x004304E0 | 特殊前期状态 |
| 0x33 | 51 | 0x00430C9F | 后处理 51 |
| 0x34 | 52 | 0x00434B9B | 后处理 52 |
| 0x35 | 53 | 0x00432130 | 后处理 53 |
| 0x36 | 54 | 0x0043776C | 后处理 54 |
| 0x37 | 55 | - | 锻造系统 |
| 0x96 | 150 | - | 战斗模式 |

### 3.3 战斗 AI 行为模型

**核心发现**：原作"AI"并非动态决策，而是**脚本回放**。

敌方行为由 FIGHT.DAT 中预定义的行为脚本序列驱动：

```
FIGHT.DAT 行为脚本结构:
├── Intro (368 bytes)
│   ├── grid_terrain[92]  // 13×7 地形网格
│   └── 初始配置数据
├── Fighter[128] (8192 bytes)
│   └── 每个 Fighter 的行为码序列
└── People[88] (11616 bytes)
    └── 参战角色配置
```

行为码解释器：
```cpp
void execute_ai_behavior(Fighter& fighter, const BehaviorScript& script) {
    for (const auto& command : script.commands) {
        switch (command.type) {
        case BEHAVIOR_MOVE:     // 移动到指定格
            move_fighter(fighter, command.target_pos);
            break;
        case BEHAVIOR_ATTACK:   // 攻击指定目标
            perform_attack(fighter, command.target_idx);
            break;
        case BEHAVIOR_SKILL:    // 使用技能
            perform_skill(fighter, command.skill_idx, command.target_pos);
            break;
        case BEHAVIOR_WAIT:     // 等待
            break;
        case BEHAVIOR_FLEE:     // 逃跑
            remove_fighter(fighter);
            break;
        }
    }
}
```

---

## 4. 转职系统

### 4.1 转职链结构

90 兵种分为 12 条转职链，最大 3 阶：

```
链 1:  双枪队(0) → 冲锋队(1) → 野战队(5)
链 2:  民兵队(4) → 步枪队(6) → 机枪队(7) → 重机枪队(8)
链 3:  土炮队(12) → 重炮队(14)
链 4:  手枪队(21) → 武工队(17) → 飞虎队(18)
链 5:  儿童团(19) → ...
链 6:  摩托队(35) → ...
链 7-12: ... (含独特兵种链)

可转职兵种: 17 个
最大转职阶数: 3
存在汇聚现象: 不同初始兵种可转至同一高级兵种
```

### 4.2 转职条件

```cpp
bool can_promote(const People& person, int32_t target_class) {
    // 1. 检查当前兵种是否在转职链中
    auto chain = get_promotion_chain(person.class_type);
    if (!chain.contains(target_class)) return false;
    
    // 2. 检查是否为链中的下一级
    if (chain.next_after(person.class_type) != target_class) return false;
    
    // 3. 检查等级要求（由 GAME.DAT 定义）
    if (person.level < get_promotion_level_requirement(target_class)) return false;
    
    return true;
}
```

---

## 5. 关卡数据结构

### 5.1 FIGHT.DAT 结构 (47 × 20508 bytes)

```
FightData (20508 bytes per fight):
├── Intro (368 bytes, offset 0x0000)
│   ├── fight_name[64]        // 关卡名称 (GBK)
│   ├── grid_terrain[92]      // 13×7 地形类型网格
│   ├── difficulty_coeff      // 难度系数
│   ├── victory_condition     // 胜利条件 ID (7 种)
│   ├── failure_condition     // 失败条件
│   ├── max_rounds            // 最大回合数
│   ├── treasure_box[4]       // 宝箱配置
│   └── intro_dialogue_idx    // 开场对话索引
│
├── Fighter[128] (8192 bytes, offset 0x0170)
│   └── 每个 64 bytes:
│       ├── position (col, row)
│       ├── people_idx → People 数组索引
│       ├── team (0=玩家, 1=敌方, 2=NPC)
│       ├── behavior_script_offset
│       ├── behavior_script_length
│       ├── ai_aggression
│       └── equipment[4]
│
└── People[88] (11616 bytes, offset 0x2170)
    └── 每个 ~132 bytes:
        ├── 临时角色属性覆盖
        ├── 装备配置
        ├── 地雷配置
        └── 物品配置
```

### 5.2 胜利条件 (7 种)

| ID | 条件 | 说明 |
|----|------|------|
| 0 | 全灭敌军 | 所有敌方 Fighter.current_hp ≤ 0 |
| 1 | 保护目标 | 指定 People 存活 |
| 2 | 坚守 N 回合 | battle_phase ≥ N |
| 3 | 击败 Boss | 指定 People 被击杀 |
| 4 | 到达指定地点 | 指定 People 到达网格坐标 |
| 5 | 营救目标 | 接触指定 People |
| 6 | 复合条件 | 多条件 AND/OR |

---

## 6. 城镇系统

### 6.1 TOWN.DAT 结构 (19 × 346 bytes)

```
TownData (346 bytes per town):
├── town_name[32]         // 城镇名称 (GBK)
├── ambient_sound[3]      // 3 个环境音效 ID
├── building[6]           // 6 个建筑配置
│   ├── building_type     // 建筑类型
│   ├── position (x, y)
│   └── flags[1]          // flags[1] = 动画标志
├── shop[3]               // 3 种商店类型
│   ├── shop_type         // 武器/防具/物品
│   ├── item_list[10]     // 可售物品列表
│   └── price_modifier    // 价格系数
└── event_trigger         // 城镇事件触发器
```

### 6.2 城镇交互功能

| 功能 | 对应原作 | 说明 |
|------|----------|------|
| 武器商店 | shop_type 0 | 购买武器装备 |
| 防具商店 | shop_type 1 | 购买防具 |
| 物品商店 | shop_type 2 | 购买消耗品 |
| 锻造系统 | mode 55 | 4 页 × 10 物品, 10×10 配方矩阵 |
| 招募角色 | recruit | 花费金钱招募战败角色 |
| 布雷 | mine_place | 选择地雷类型和位置 |
| 存档 | save_game | 10 个存档位 |

---

## 7. 对话系统

### 7.1 三层链式结构

```
Layer 1: FIGHT.DAT
  └── 关卡触发条件 → 对话组 ID
  
Layer 2: 函数指针表 (0x448720 → 0x446030)
  └── 5 张表, 共 181 个回调函数
  └── Table1: 134 entries
  └── Table5: 94 entries
  
Layer 3: TXT.DAT (XOR 0x51 加密)
  └── 593 条对话条目
  └── 81 个说话者
  └── 每条: speaker_id + text_offset + text_length
```

### 7.2 对话触发条件

| 触发模式 | 数量 | 说明 |
|----------|------|------|
| 战斗开始 | ~47 | 每关开场对话 |
| 回合触发 | ~100 | 特定回合数触发 |
| HP 阈值 | ~50 | 角色 HP 降至阈值 |
| 击杀触发 | ~80 | 击杀特定角色 |
| 位置触发 | ~30 | 到达指定网格位置 |
| 对话选择 | 0 | 无分支选择，纯线性 |

---

## 8. 隐藏关卡系统

### 8.1 触发机制

```
触发条件: DAT_006223a8 ≥ 2800 (战斗结束后累计)
触发时机: 第 36 关战斗结束后检查

DAT_006223a8 增长方式:
  - 每场战斗结束后根据表现增加
  - 具体增量由战斗评分公式决定
```

### 8.2 双模式系统

| 模式 | 触发阈值 | 说明 |
|------|----------|------|
| 模拟 | 2800 | 战术重放，回顾历史战斗 |
| 虚构 | 2800+ | 叙事扩展，新增战斗( fight=40 ) |

### 8.3 四级阈值

| 阈值 | 解锁内容 |
|------|----------|
| 2800 | 模拟模式入口 |
| 3000 | 虚构关卡第一层 |
| 3300 | 虚构关卡第二层 |
| 3600 | 虚构关卡最终层 |

---

## 9. 存档系统

### 9.1 存档格式

```
save{slot}.dat (85072 bytes):
├── Header (魔数 + 版本号)
├── Block 0: 游戏模式 + 难度 (4B)
├── Block 1: 关卡索引 fight_index (4B) @ 0x7D06A0
├── Block 2: 城镇索引 town (4B) @ 0x7D0484
├── Block 3-16: 剧情标记 story[14] (56B)
├── Block 17-56: 40 角色状态 (40 × 0x84 = 3360B) @ 0x7D0EF8
├── Block 57-72: 仓库物品 warehouse[160] (640B)
├── Block 73-84: 其他全局状态
└── ... 共 28 块
```

### 9.2 118 个全局变量映射

28 个存档块对应 118 个战略全局变量，关键变量：

| 变量 | 地址 | 说明 |
|------|------|------|
| fight_index | 0x7D06A0 | 当前关卡索引 |
| town | 0x7D0484 | 当前城镇索引 |
| game_mode | 0x7DB4E8 | 游戏模式 |
| people_array | 0x7D0EF8 | 40 角色数组基址 |
| current_user_index | 0x473158 | 当前操作角色 |
| hidden_counter | 0x6223A8 | 隐藏关卡计数器 |

---

## 10. 难度系统

### 10.1 双层难度

```
Runtime Difficulty (玩家选择):
  - Easy:   全局攻防修正 ×0.8 (敌方)
  - Normal: 无修正
  - Hard:   全局攻防修正 ×1.2 (敌方)

Per-Battle Coefficient (关卡内置):
  - 每关在 FIGHT.DAT Intro 中定义 difficulty_coeff
  - 影响敌方 AI 激进度、增援数量、资源分配
```

---

## 11. 评分系统

### 11.1 战斗评分公式

```
score = base_score
      + kill_bonus × kill_count
      + survival_bonus × surviving_ally_count
      + round_bonus × (max_rounds - actual_rounds)
      + treasure_bonus × treasure_collected
      - penalty × friendly_death_count  // 死亡惩罚 = 死刑（友军死亡→全员降等级）
```

**注意**：原作中 kill penalty 实际为 death penalty — 友军死亡导致所有盟友降等级，而非反滚雪球机制。
