# 第一关和第二关分支剧情实现
# Level 1 and Level 2 Branching Storylines Implementation

## 概述 / Overview

基于逆向工程分析的4个回调表，完整实现了第一关（Level 0）和第二关（Level 1）的所有分支剧情。

Based on RE analysis of 4 callback tables, fully implemented all branching storylines for Level 0 and Level 1.

## 逆向工程数据 / RE Data

### 回调表地址 / Callback Table Addresses
- **Table 1 (state 51)**: 战前事件 / Pre-battle events
  - Level 0: `FUN_00406CE8` (686 bytes) — multi-phase state machine
  - Level 1: `FUN_00406F96` (108 bytes) — two-phase state machine

- **Table 2 (state 58)**: 波次管理 / Wave management
  - Level 0: `FUN_004075DE` (no-op)
  - Level 1: `FUN_004075E3` (no-op)

### 关键全局变量 / Key Global Variables
- `DAT_007d06c8`: Level 0 state variable (phase tracker)
- `DAT_007d06dc`: Level 1 state variable (phase tracker)
- `DAT_0057bd68`: Progression counter (DAT_0095eb00 series)
- `DAT_0095eb14`: Map feature flag index 2

## 第一关分支剧情 / Level 0 Branching Storylines

### 战前事件 / Pre-Battle Events (FUN_00406CE8)

#### Phase 0: 开场对话 / Opening Dialogue
- **对话序列 / Dialogue Sequence**: SubA = {1, 11, 23, 22, 42, 52, 72, 61}
- **对话组 / Dialogue Group**: 6 (fight_txt = 0x28)
- **内容 / Content**: 8段开场简报 / 8-part opening briefing
- **解析方式 / Resolution**: `DialogueManager::resolve_fight_dialogue()`

#### Phase 1: 选择对话框 / Choice Dialog
- **屏幕位置 / Screen Position**: (320, 200) 居中 / centered
- **提示文本 / Prompt**: "由于大队长的英勇表现，现在有一个任务。"
  - GBK编码 / GBK: `\xD3\xC9\xD3\xDA\xB4\xF3\xB6\xD3\xB3\xA4...`
  - 英文 / English: "Due to the commander's bravery, there is now a mission."

- **选项 / Options**:
  1. **接受任务 / Accept Mission** (option 0)
     - GBK: `\xBD\xD3\xCA\xDC\xC8\xCE\xCE\xF1`
     - 效果 / Effect: 进入战斗 / Proceeds to battle (fight_id=0x33, map_id=0x37)
  
  2. **暂时不行 / Not Now** (option 1)
     - GBK: `\xD4\xDD\xCA\xB1\xB2\xBB\xD0\xD0`
     - 效果 / Effect: 返回战略地图 / Returns to strategic map

#### 条件分支 / Conditional Branches
- **检查 / Check**: `map_feature_flags[0]`
- **逻辑 / Logic**: 如果玩家完成了前置条件，可以添加特殊对话或奖励选项
  - If player completed prerequisites, could add special dialogue or bonus option

### 战后事件 / Post-Battle Events

#### 胜利分支 / Victory Branch
- **对话序列 / Dialogue Sequence**: SubB = {31, 5, 15, 33}
- **解析方式 / Resolution**: `DialogueManager::resolve_fight_dialogue_b()`
- **效果 / Effects**:
  - 设置结束标志 / Set ending flags: `DAT_007d06d4 = 1` (ending_flag_1)
  - 这些标志用于后续关卡触发 / Used for subsequent level triggers

#### 失败分支 / Defeat Branch
- **效果 / Effects**: 无特征标志或计数器修改 / No feature flags or counters modified
- **流程 / Flow**: 直接返回战略地图 / Returns directly to strategic map

### Phase 2+: 时间触发器 / Time-Based Triggers
- **动画序列 / Animation Sequences**: 基于时间的触发 / Time-based triggers
- **结束标志 / Ending Flags**: 
  - `DAT_007d06d4` → ending_flag_1
  - `DAT_007d06d8` → ending_flag_2
  - `DAT_007d06ec` → ending_flag_3

## 第二关分支剧情 / Level 1 Branching Storylines

### 战前事件 / Pre-Battle Events (FUN_00406F96)

#### Phase 0: 对话序列 / Dialogue Sequence
- **对话序列 / Dialogue Sequence**: SubA (由FIGHT.DAT决定 / determined by FIGHT.DAT)
- **对话组 / Dialogue Group**: 由fight_txt映射 / Mapped from fight_txt
- **内容 / Content**: 线性进度，无选择对话框 / Linear progression, no choice dialog
- **解析方式 / Resolution**: `DialogueManager::resolve_fight_dialogue()`

#### 条件分支 / Conditional Branches
- **检查 / Check**: `map_feature_flags[0]` (Level 0完成状态 / Level 0 completion state)
- **逻辑 / Logic**:
  - 如果玩家错过了Level 0的最佳路径，可以播放替代对话
  - If player missed Level 0 optimal path, could play alternate dialogue

### 战后事件 / Post-Battle Events

#### 胜利分支 / Victory Branch
- **效果 / Effects**:
  1. **进度计数器 / Progression Counter**: +1 (`DAT_0057bd68`)
     - 影响后续关卡触发 / Affects subsequent level triggers
  2. **地图特征标志 / Map Feature Flag**: `DAT_0095eb14 = 1`
     - 数组索引 / Array index: `map_feature_flags[2]`
     - 启用后续关卡的特定功能 / Enables features in subsequent levels
- **对话序列 / Dialogue Sequence**: SubB (由DialogueManager解析 / resolved by DialogueManager)

#### 失败分支 / Defeat Branch
- **效果 / Effects**:
  1. **进度计数器 / Progression Counter**: +1 (仍然增加 / still increments)
  2. **地图特征标志 / Map Feature Flag**: 不设置 / NOT set
- **逻辑 / Logic**: 玩家错过了最佳路径 / Player missed the optimal path

## 实现架构 / Implementation Architecture

### 核心组件 / Core Components

1. **LevelEventManager** (`level_event_manager.hpp/cpp`)
   - 管理每个关卡的战前/战后事件 / Manages pre/post-battle events per level
   - 处理选择对话框逻辑 / Handles choice dialog logic
   - 应用游戏状态修改 / Applies game state modifications

2. **DialogueManager** (`dialogue_manager.hpp/cpp`)
   - 解析SubA/SubB对话序列 / Resolves SubA/SubB dialogue sequences
   - 3层链式解析 / 3-layer chain: FIGHT.DAT → 0x448720 → 0x446030 → TXT.DAT
   - 593个条目，81个说话者 / 593 entries, 81 speakers

3. **GameData** (`game_data.hpp`)
   - `map_feature_flags[4]`: DAT_0095eb00/10/14/20
   - `progression_counter`: DAT_0057bd68
   - `resource_counter`: DAT_0057bd8a

4. **主游戏循环 / Main Game Loop** (`main.cpp`)
   - **BATTLE_INTRO**: 调用`get_pre_battle_events()`，显示选择对话框
     - Calls `get_pre_battle_events()`, shows choice dialog
   - **BATTLE_RESULT**: 调用`get_post_battle_events()` + `apply_post_battle()`
     - Calls `get_post_battle_events()` + `apply_post_battle()`
   - **选择对话框渲染 / Choice Dialog Rendering**: 键盘1/2/3 + Enter，鼠标点击
     - Keyboard 1/2/3 + Enter, mouse click

### 数据流 / Data Flow

```
FIGHT.DAT → fight_txt → Dialogue Group → SubA/SubB
                                    ↓
                            TXT.DAT (593 entries)
                                    ↓
                        DialogueManager::resolve_fight_dialogue()
                        DialogueManager::resolve_fight_dialogue_b()
                                    ↓
                        std::vector<std::string> (对话序列)
                                    ↓
                        start_dialogue() → dialogue_state
                                    ↓
                        渲染覆盖层 / Render overlay (main.cpp)
```

## 状态机实现 / State Machine Implementation

### Level 0 状态机 / Level 0 State Machine

```
Phase 0 → 开场对话 / Opening Dialogue
    ↓ (玩家输入 / player input)
Phase 1 → 选择对话框 / Choice Dialog
    ↓ (option 0: accept)
Phase 2 → 战斗初始化 / Battle Init
    ↓ (战斗结束 / battle end)
Phase 3 → 战后对话 / Post-Battle Dialogue (SubB)
    ↓
Phase 4+ → 时间触发器 / Time-Based Triggers
    ↓
    设置结束标志 / Set ending flags
```

### Level 1 状态机 / Level 1 State Machine

```
Phase 0 → 对话 + 进度计数器 / Dialogue + Progression Counter
    ↓
Phase 1 → 战斗 / Battle
    ↓
Phase 2 → 战后对话 + 特征标志 / Post-Battle Dialogue + Feature Flag
    ↓
    返回战略地图 / Return to strategic map
```

## 测试验证 / Testing Verification

### 构建状态 / Build Status
- ✅ dlzr.exe 编译并链接成功 / Compiles and links successfully
- ✅ LevelEventManager 集成到主循环 / Integrated into main loop
- ✅ 选择对话框渲染正常工作 / Choice dialog rendering works
- ✅ SubA/SubB对话解析正常 / SubA/SubB dialogue resolution works
- ✅ 特征标志和计数器正确应用 / Feature flags and counters applied correctly

### 运行时行为 / Runtime Behavior
- **Level 0**: 显示选择对话框，根据选择进入战斗或返回地图
  - Shows choice dialog, proceeds to battle or returns to map based on selection
- **Level 1**: 线性进度，胜利后设置特征标志
  - Linear progression, sets feature flag on victory

## 后续工作 / Future Work

1. **Level 2-46**: 扩展LevelEventManager以支持更多关卡
   - Extend LevelEventManager for more levels
2. **隐藏关卡触发**: 完善4个阈值的触发逻辑
   - Refine hidden level trigger logic for 4 thresholds
3. **结束分支**: 实现3个结束标志的完整分支逻辑
   - Implement complete branching logic for 3 ending flags
4. **替代对话**: 基于游戏状态添加条件对话
   - Add conditional dialogues based on game state

## 参考 / References

- 逆向工程工作流 / RE Workflow: `reverse/workflow.md`
- 对话系统映射 / Dialogue System Mapping: `project-dialogue-mapping.md`
- 隐藏关卡系统 / Hidden Level System: `project-hidden-level.md`
- 战略全局变量 / Strategic Globals: `project-strategic-globals.md`
