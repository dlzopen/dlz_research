# DLZR 架构设计文档

## 抗日之地雷战 1:1 复刻工程

**日期**：2026-10-09  
**状态**：评审中

---

## 1. 架构总览

### 1.1 分层架构图

```
┌─────────────────────────────────────────────────────────────┐
│                    Presentation Layer                        │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────────┐  │
│  │ Renderer │ │  Audio   │ │  Video   │ │   UI/HUD     │  │
│  │ (SDL3)   │ │(SDL3_mix)│ │(mpeg2/flc)│ │  (Custom)    │  │
│  └──────────┘ └──────────┘ └──────────┘ └──────────────┘  │
├─────────────────────────────────────────────────────────────┤
│                    Application Layer                        │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────────┐  │
│  │  Scene   │ │  Input   │ │  Event   │ │  Resource    │  │
│  │  Manager │ │  Handler │ │  Bus     │ │  Manager     │  │
│  └──────────┘ └──────────┘ └──────────┘ └──────────────┘  │
├─────────────────────────────────────────────────────────────┤
│                    Game Logic Layer                         │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────────┐  │
│  │  Battle  │ │ Strategic│ │Character │ │  Dialogue    │  │
│  │  System  │ │  System  │ │  System  │ │  System      │  │
│  └──────────┘ └──────────┘ └──────────┘ └──────────────┘  │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────────┐  │
│  │  Save/   │ │  Hidden  │ │ Crafting │ │  Victory     │  │
│  │  Load    │ │  Level   │ │  System  │ │  Checker     │  │
│  └──────────┘ └──────────┘ └──────────┘ └──────────────┘  │
├─────────────────────────────────────────────────────────────┤
│                    Core Layer (Zero Dependencies)           │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────────┐  │
│  │ Formulas │ │  State   │ │  Data    │ │  Math        │  │
│  │ (atk/def │ │ Machine  │ │ Models   │ │  Utilities   │  │
│  │  /exp/skill)│ │        │ │(POD)     │ │              │  │
│  └──────────┘ └──────────┘ └──────────┘ └──────────────┘  │
├─────────────────────────────────────────────────────────────┤
│                    Platform Abstraction Layer               │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────────┐  │
│  │  Window  │ │  File    │ │  Time    │ │  Thread      │  │
│  │  (SDL3)  │ │  System  │ │  (SDL3)  │ │  (SDL3)      │  │
│  └──────────┘ └──────────┘ └──────────┘ └──────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

### 1.2 依赖方向

```
Presentation → Application → Game Logic → Core ← Platform Abstraction
```

**严格规则**：上层可依赖下层，下层不可依赖上层。Core 层零外部依赖。

---

## 2. 模块划分

### 2.1 Core 层 — 核心逻辑（零依赖）

#### 2.1.1 数据模型 (data_model)

原作结构体的现代化重定义，保持字段布局与原作完全一致。

```cpp
// core/data_model/types.hpp

namespace dlzr::core {

struct People {
    int32_t name_id;
    int32_t portrait_id;
    int32_t face_id;
    int32_t reserved_0c;
    int32_t reserved_10;
    int32_t reserved_14;
    int32_t hp;
    int32_t strength;       // 武力
    int32_t intelligence;   // 智力
    int32_t level;
    int32_t experience;
    int32_t class_type;     // 兵种索引
    int32_t skill_use_count;
    std::array<int32_t, 16> skill_data;
    int32_t skill_exp;
    // ... 对齐至 0x84 字节
};
static_assert(sizeof(People) == 0x84);

struct Fighter {
    int32_t max_hp;
    int32_t current_hp;
    int32_t morale;
    int32_t equip_integrity;
    int32_t defense_modifier;
    // ... 对齐至 0x40 字节
};
static_assert(sizeof(Fighter) == 0x40);

struct ClassType {
    int32_t hp_base;
    int32_t hp_growth;
    int32_t attack_growth;
    int32_t defense_growth;
    int32_t reserved_10;
    int32_t base_attack;
    int32_t base_defense;
    // ... 对齐至 0xA0 字节
};
static_assert(sizeof(ClassType) == 0xA0);

struct Skill {
    std::array<char, 16> name;
    int32_t param1;     // HP effect factor
    int32_t param2;     // morale effect factor
    int32_t range_type;
    // ... 对齐至 0x30 字节
};
static_assert(sizeof(Skill) == 0x30);

} // namespace dlzr::core
```

#### 2.1.2 战斗公式 (formulas)

从原作反编译代码直接提取的计算公式，纯函数实现，无副作用。

```cpp
// core/formulas/combat.hpp

namespace dlzr::core {

// 原作地址: 0x0042A82A
// 四层攻击力计算
int32_t calc_attack_power(
    const People& people,
    const Fighter& fighter,
    const ClassType& class_type,
    int32_t skill_level,
    int32_t item_bonus,
    bool in_battle
);

// 原作地址: 0x0042A98E
// 多层防御力计算
int32_t calc_defense(
    const People& people,
    const Fighter& fighter,
    const ClassType& class_type,
    int32_t terrain_bonus,
    int32_t item_bonus,
    bool in_battle
);

// 原作地址: 0x0042B2E0 (calc_skill_damage)
// 技能伤害公式: ((param * intelligence) / 100) * (skill_level + 7) >> 3
int32_t calc_skill_effect(
    int32_t param,
    int32_t intelligence,
    int32_t skill_level
);

// 原作地址: 0x0042B000+ (calc_experience)
// 经验值计算
int32_t calc_experience(
    int32_t killer_level,
    int32_t victim_level,
    int32_t base_exp
);

// HP 计算公式
int32_t calc_max_hp(int32_t hp_base, int32_t hp_growth, int32_t level);

// 技能等级查询 (table at 0x447280, levels 0-10)
int32_t get_skill_level(int32_t skill_use_count);

} // namespace dlzr::core
```

#### 2.1.3 状态机 (state_machine)

原作 game_mode 全局变量的现代化重构。

```cpp
// core/state_machine/game_state.hpp

namespace dlzr::core {

enum class GameMode : int32_t {
    Init            = 0x00,
    Map             = 0x01,
    Battle          = 0x96,
    // ... 完整映射原作 20+ 状态
};

enum class BattleCommand : int32_t {
    EndVictory   = 0x100,
    EndRetreat   = 0x202,
    AdvanceRound = 0x113,
};

class GameStateMachine {
public:
    GameMode current_mode() const;
    void transition(GameMode new_mode);
    void dispatch(BattleCommand cmd);
    
    // 原作 calc_behavior 逻辑
    void advance_battle_round();
    bool check_battle_end() const;
    
private:
    GameMode mode_ = GameMode::Init;
    int32_t battle_phase_ = 0;
    int32_t total_fighter_count_ = 0;
};

} // namespace dlzr::core
```

### 2.2 Game Logic 层 — 游戏系统

#### 2.2.1 战斗系统 (battle_system)

```cpp
// logic/battle/battle_system.hpp

namespace dlzr::logic {

class BattleSystem {
public:
    // 初始化战斗（对应原作 init_battle_data + init_battle_fighters）
    void init_battle(const FightData& fight_data, const PeopleArray& people);
    
    // 战斗主循环（对应原作 calc_behavior 调度）
    void process_round();
    
    // 获取角色行为（对应原作 get_fighter_behavior）
    FighterAction get_fighter_behavior(int32_t fighter_idx);
    
    // 执行行为（对应原作 execute_behavior_animation 等）
    void execute_action(const FighterAction& action);
    
    // 结算战斗（对应原作 finalize_battle）
    BattleResult finalize();
    
    // 查询接口
    const BattleState& state() const;
    bool is_battle_over() const;
    
private:
    BattleState state_;
    core::GameStateMachine state_machine_;
};

} // namespace dlzr::logic
```

#### 2.2.2 战略系统 (strategic_system)

```cpp
// logic/strategic/strategic_system.hpp

namespace dlzr::logic {

class StrategicSystem {
public:
    // 城镇交互（对应原作 5 个 town handler）
    void enter_town(int32_t town_id);
    void shop_buy(int32_t shop_type, int32_t item_id);
    void shop_sell(int32_t item_id);
    void recruit_character(int32_t character_id);
    void craft_item(int32_t recipe_id);
    void place_mine(int32_t mine_type, int32_t position);
    
    // 战略地图（对应原作 12 个 strategic sub-handlers）
    void select_destination(int32_t map_id);
    void trigger_event(int32_t event_id);
    
    // 查询接口
    const StrategicState& state() const;
    
private:
    StrategicState state_;
};

} // namespace dlzr::logic
```

#### 2.2.3 角色系统 (character_system)

```cpp
// logic/character/character_system.hpp

namespace dlzr::logic {

class CharacterSystem {
public:
    // 经验与升级（对应原作 award_experience + calc_experience）
    void award_experience(int32_t char_idx, int32_t exp_amount);
    bool check_level_up(int32_t char_idx);
    void apply_level_up(int32_t char_idx);
    
    // 转职（对应原作 class promotion 12 chains）
    bool can_promote(int32_t char_idx) const;
    void promote_class(int32_t char_idx, int32_t target_class);
    
    // 装备/物品
    void equip_item(int32_t char_idx, int32_t item_slot, int32_t item_id);
    int32_t get_item_attack_bonus(int32_t char_idx) const;
    int32_t get_item_defense_bonus(int32_t char_idx) const;
    
    // 技能
    int32_t get_skill_level(int32_t char_idx, int32_t skill_idx) const;
    void use_skill(int32_t char_idx, int32_t skill_idx);
    
    // 死亡惩罚（原作: kill penalty = death penalty）
    void apply_death_penalty(int32_t victim_idx);
    
private:
    std::array<core::People, 40> people_;
};

} // namespace dlzr::logic
```

#### 2.2.4 对话系统 (dialogue_system)

```cpp
// logic/dialogue/dialogue_system.hpp

namespace dlzr::logic {

class DialogueSystem {
public:
    // 3 层链式触发: FIGHT.DAT → 0x448720 → 0x446030 → TXT.DAT
    void trigger_dialogue(int32_t fight_idx, int32_t trigger_condition);
    void display_next_line();
    bool is_dialogue_active() const;
    
    // 说话者查询（81 speakers）
    int32_t current_speaker() const;
    
private:
    std::vector<DialogueEntry> dialogue_queue_;
    int32_t current_index_ = -1;
};

} // namespace dlzr::logic
```

#### 2.2.5 存档系统 (save_system)

```cpp
// logic/save/save_system.hpp

namespace dlzr::logic {

class SaveSystem {
public:
    // 10 存档位, 28 数据块, 85072 字节
    bool save_game(int32_t slot, const GameState& state);
    bool load_game(int32_t slot, GameState& state);
    bool is_slot_valid(int32_t slot) const;
    
    // 原作二进制存档格式序列化/反序列化
    // 直接读写原作 save{N}.dat 的 28 数据块二进制布局
    // 可与原作游戏存档互相通用
    std::vector<uint8_t> serialize(const GameState& state) const;
    GameState deserialize(const std::vector<uint8_t>& data) const;
    
private:
    static constexpr int32_t SAVE_SLOT_COUNT = 10;
    static constexpr int32_t SAVE_DATA_SIZE = 85072;
    static constexpr int32_t DATA_BLOCK_COUNT = 28;
};

} // namespace dlzr::logic
```

> **存档格式约束**：存档系统必须 100% 兼容原作二进制格式（28 数据块 × 85072 字节），
> 使重制版存档可在原作中加载，原作存档也可在重制版中加载。不使用 JSON 或任何
> 自定义存档格式。

### 2.3 Application 层 — 应用编排

#### 2.3.1 场景管理 (scene_manager)

```cpp
// app/scene/scene_manager.hpp

namespace dlzr::app {

class SceneManager {
public:
    void push_scene(std::unique_ptr<Scene> scene);
    void pop_scene();
    void switch_scene(std::unique_ptr<Scene> scene);
    
    void update(float dt);
    void render();
    
private:
    std::vector<std::unique_ptr<Scene>> scene_stack_;
};

// 具体场景
class TitleScene : public Scene { /* ... */ };
class StrategicMapScene : public Scene { /* ... */ };
class BattleScene : public Scene { /* ... */ };
class TownScene : public Scene { /* ... */ };
class DialogueScene : public Scene { /* ... */ };
class SaveLoadScene : public Scene { /* ... */ };

} // namespace dlzr::app
```

#### 2.3.2 输入处理 (input_handler)

```cpp
// app/input/input_handler.hpp

namespace dlzr::app {

class InputHandler {
public:
    void process_events();
    
    bool is_mouse_clicked(int32_t button) const;
    Vec2i mouse_position() const;
    bool is_key_pressed(KeyCode key) const;
    
    // 原作 UI 坐标映射（640×480 基准）
    Vec2i screen_to_game_coords(Vec2i screen_pos) const;
    
private:
    Vec2i mouse_pos_;
    std::array<bool, 5> mouse_buttons_;
    std::array<bool, 256> keys_;
};

} // namespace dlzr::app
```

#### 2.3.3 资源管理 (resource_manager)

> **数据读取策略**：所有游戏数据在运行时直接读取原始二进制文件（PEOPLE.DAT、
> FIGHT.DAT、GAME.DAT、TOWN.DAT、TXT.DAT、WAVE.DAT、IMG/ANM 等）。
> 不依赖 JSON/CSV 等中间格式。用户需拥有原作游戏数据目录。

```cpp
// app/resource/resource_manager.hpp

namespace dlzr::app {

class ResourceManager {
public:
    // DAT 文件加载器
    GameData load_game_dat(const fs::path& path);
    FightData load_fight_dat(const fs::path& path);
    PeopleArray load_people_dat(const fs::path& path);
    TownArray load_town_dat(const fs::path& path);
    TextData load_txt_dat(const fs::path& path);  // XOR 0x51 解密
    
    // 图像资源加载
    Texture load_img(const fs::path& path);       // 8-bit 调色板索引
    SpriteSheet load_anm(const fs::path& path);   // RLE 动画解码
    Texture load_blk(const fs::path& path, int32_t blk_id);
    
    // 音频资源加载
    SoundBuffer load_wave(const fs::path& path, int32_t wave_id);
    
    // 视频资源
    VideoStream load_mpeg(const fs::path& path);
    AnimationStream load_flc(const fs::path& path);
    
private:
    std::unordered_map<std::string, std::any> cache_;
};

} // namespace dlzr::app
```

### 2.4 Presentation 层 — 表现层

#### 2.4.1 渲染器 (renderer)

```cpp
// presentation/renderer.hpp

namespace dlzr::presentation {

class Renderer {
public:
    bool init(int32_t width, int32_t height, const std::string& title);
    void shutdown();
    
    // 基础渲染（对应原作 DirectDraw blit 操作）
    void clear(Color color);
    void draw_sprite(const Texture& tex, Vec2i pos, Rect src_rect = {});
    void draw_tile(const Texture& tile, Vec2i pos);
    void draw_text(const std::string& text, Vec2i pos, Color color);
    
    // 调色板操作（原作 8-bit 索引色）
    void set_palette(const Palette& palette);
    
    // 整数倍缩放
    void set_scale_factor(int32_t factor);
    
    void present();
    
private:
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    SDL_Texture* back_buffer_ = nullptr;  // 640×480 逻辑分辨率
    int32_t scale_factor_ = 1;
};

} // namespace dlzr::presentation
```

---

## 3. 核心数据流

### 3.1 战斗流程数据流

```
┌─────────────┐
│ FIGHT.DAT   │──→ BattleSystem::init_battle()
│ GAME.DAT    │       ├─→ 加载 128 Fighter 数据
│ PEOPLE.DAT  │       ├─→ 加载 88 People 数据
│ MAP.DAT     │       ├─→ 加载地形网格 (13×7)
│ BLK.DAT     │       └─→ 加载地图块
└─────────────┘
        │
        ▼
┌─────────────────────────────────────────────┐
│ BattleSystem::process_round()               │
│   │                                         │
│   ├─→ GameStateMachine::advance_battle_round│
│   │     └─→ battle_phase++                  │
│   │                                         │
│   ├─→ get_fighter_behavior(fighter_idx)     │
│   │     ├─→ [玩家] 等待输入 → 返回行为      │
│   │     └─→ [AI] 读取 FIGHT.DAT 行为脚本    │
│   │           └─→ 返回行为码                 │
│   │                                         │
│   ├─→ execute_action(action)                │
│   │     ├─→ calc_attack_power()             │
│   │     ├─→ calc_defense()                  │
│   │     ├─→ calc_skill_effect()             │
│   │     ├─→ 更新 Fighter.current_hp         │
│   │     ├─→ 更新 People.experience          │
│   │     └─→ 播放动画/音效                   │
│   │                                         │
│   └─→ check_battle_end()                    │
│         ├─→ 胜利 → finalize_battle()        │
│         ├─→ 失败 → finalize_battle()        │
│         └─→ 回合耗尽 → finalize_battle()    │
└─────────────────────────────────────────────┘
        │
        ▼
┌─────────────┐
│ BattleResult│──→ StrategicSystem
│ (经验/物品/ │       ├─→ award_experience()
│  状态变更)  │       ├─→ 更新角色状态
└─────────────┘       └─→ 检查隐藏关卡触发
```

### 3.2 存档数据流

```
SaveSystem::save_game(slot, state)
    │
    ├─→ 按原作布局序列化 28 数据块（纯二进制，无 JSON）:
    │     ├─ Block 0: 游戏模式 + 难度 (4B)
    │     ├─ Block 1: 关卡索引 (4B)
    │     ├─ Block 2-41: 40 角色状态 (40 × 0x84 = 3360B)
    │     ├─ Block 42-57: 仓库物品 (160 × 4B = 640B)
    │     ├─ Block 58: 剧情标记 (14 × 4B = 56B)
    │     └─ ... 共 28 块, 总计 ~85072B
    │
    └─→ 写入 save{slot}.dat（与原作存档格式完全一致，可互操作）

SaveSystem::load_game(slot, state)
    │
    ├─→ 读取 save{slot}.dat（原作格式或重制版保存的格式，二者等价）
    ├─→ 反序列化 28 数据块
    └─→ 恢复 GameState
```

---

## 4. 原作架构映射

### 4.1 原作程序架构 → 复刻架构

| 原作组件 | 地址 | 复刻模块 | 说明 |
|----------|------|----------|------|
| WinMain | 0x4258b9 | main() + App | 入口点 |
| InitChain | 0x424d7f | App::init() | 初始化链 |
| WndProc | 0x425641 | InputHandler | 消息处理 |
| Dispatcher | 0x425340 | SceneManager | 状态分发 |
| game_mode | 0x7DB4E8 | GameStateMachine | 全局状态 |
| people[] | 0x7D0EF8 | CharacterSystem::people_ | 角色数组 |
| fighter[] | 0x96AB90 | BattleState::fighters_ | 战斗人员 |
| class_type[] | 0x96CBAC | GameData::classes | 兵种数据 |
| DirectDraw | COM | Renderer (SDL3) | 渲染层 |
| DirectSound | COM | AudioSystem (SDL3_mixer) | 音频层 |
| MCI | Win32 API | VideoPlayer (libmpeg2) | 视频层 |

### 4.2 原作全局状态 → 复刻依赖注入

原作通过全局变量（game_mode, people[], fighter[] 等）传递状态，复刻版改为显式依赖注入：

```cpp
// 原作风格（全局状态）
extern int game_mode;
extern People people[40];
extern Fighter fighter[8];

void calc_attack_power(int person_idx) {
    int cls = people[person_idx].class_type;
    if (game_mode == 0x96) { /* ... */ }
}

// 复刻风格（依赖注入）
namespace dlzr::core {

int32_t calc_attack_power(
    const People& person,
    const Fighter& fighter,
    const ClassType& class_type,
    int32_t skill_level,
    int32_t item_bonus,
    bool in_battle
) {
    int32_t raw_atk = (class_type.base_attack * 3 * person.strength) / 100
                    + (class_type.attack_growth - 1) * (person.level - 1);
    // ...
    if (in_battle) {
        raw_atk = (raw_atk * fighter.equip_integrity) / 100;
        raw_atk = ((fighter.morale + 100) * raw_atk) / 200;
    }
    // ...
}

}
```

---

## 5. 目录结构

```
dlzr/
├── CMakeLists.txt              # 根构建文件
├── docs/                       # 设计文档
│   ├── 01_project_overview.md
│   ├── 02_tech_selection.md
│   ├── 03_architecture_design.md
│   ├── 04_core_logic_design.md
│   ├── 05_implementation_roadmap.md
│   └── 06_engineering_standards.md
├── src/
│   ├── core/                   # 核心逻辑层（零依赖）
│   │   ├── CMakeLists.txt
│   │   ├── data_model/
│   │   │   ├── types.hpp       # People, Fighter, ClassType, Skill
│   │   │   ├── game_data.hpp   # 全局游戏数据
│   │   │   └── constants.hpp   # 常量定义
│   │   ├── formulas/
│   │   │   ├── combat.hpp      # calc_attack_power, calc_defense
│   │   │   ├── combat.cpp
│   │   │   ├── experience.hpp  # calc_experience, award_experience
│   │   │   ├── experience.cpp
│   │   │   ├── skill.hpp       # calc_skill_effect, get_skill_level
│   │   │   └── skill.cpp
│   │   └── state_machine/
│   │       ├── game_state.hpp
│   │       └── game_state.cpp
│   ├── logic/                  # 游戏逻辑层
│   │   ├── CMakeLists.txt
│   │   ├── battle/
│   │   │   ├── battle_system.hpp
│   │   │   ├── battle_system.cpp
│   │   │   ├── battle_ai.hpp
│   │   │   ├── battle_ai.cpp
│   │   │   ├── battle_renderer.hpp
│   │   │   └── battle_renderer.cpp
│   │   ├── strategic/
│   │   │   ├── strategic_system.hpp
│   │   │   ├── strategic_system.cpp
│   │   │   ├── town_system.hpp
│   │   │   └── town_system.cpp
│   │   ├── character/
│   │   │   ├── character_system.hpp
│   │   │   ├── character_system.cpp
│   │   │   ├── class_promotion.hpp
│   │   │   └── class_promotion.cpp
│   │   ├── dialogue/
│   │   │   ├── dialogue_system.hpp
│   │   │   └── dialogue_system.cpp
│   │   ├── save/
│   │   │   ├── save_system.hpp
│   │   │   └── save_system.cpp
│   │   ├── hidden_level/
│   │   │   ├── hidden_level.hpp
│   │   │   └── hidden_level.cpp
│   │   └── crafting/
│   │       ├── crafting_system.hpp
│   │       └── crafting_system.cpp
│   ├── app/                    # 应用层
│   │   ├── CMakeLists.txt
│   │   ├── app.hpp
│   │   ├── app.cpp
│   │   ├── scene/
│   │   │   ├── scene_manager.hpp
│   │   │   ├── scene_manager.cpp
│   │   │   ├── title_scene.hpp
│   │   │   ├── battle_scene.hpp
│   │   │   ├── strategic_scene.hpp
│   │   │   ├── town_scene.hpp
│   │   │   └── dialogue_scene.hpp
│   │   ├── input/
│   │   │   ├── input_handler.hpp
│   │   │   └── input_handler.cpp
│   │   └── resource/
│   │       ├── resource_manager.hpp
│   │       ├── resource_manager.cpp
│   │       ├── dat_loader.hpp    # DAT 文件解析
│   │       ├── dat_loader.cpp
│   │       ├── img_loader.hpp    # IMG 格式解码
│   │       ├── img_loader.cpp
│   │       ├── anm_loader.hpp    # ANM RLE 动画解码
│   │       └── anm_loader.cpp
│   ├── presentation/           # 表现层
│   │   ├── CMakeLists.txt
│   │   ├── renderer.hpp
│   │   ├── renderer.cpp
│   │   ├── audio_system.hpp
│   │   ├── audio_system.cpp
│   │   ├── video_player.hpp
│   │   ├── video_player.cpp
│   │   ├── ui/
│   │   │   ├── hud.hpp
│   │   │   ├── hud.cpp
│   │   │   ├── menu.hpp
│   │   │   └── menu.cpp
│   │   └── font/
│   │       ├── hzk_font.hpp     # HZK16 中文字体
│   │       └── hzk_font.cpp
│   └── platform/               # 平台抽象层
│       ├── CMakeLists.txt
│       ├── platform.hpp
│       ├── sdl3/
│       │   ├── sdl3_platform.hpp
│       │   └── sdl3_platform.cpp
│       └── web/
│           ├── web_platform.hpp
│           └── web_platform.cpp
├── tests/                      # 单元测试
│   ├── CMakeLists.txt
│   ├── core/
│   │   ├── test_combat_formulas.cpp
│   │   ├── test_experience.cpp
│   │   ├── test_skill_effect.cpp
│   │   └── test_state_machine.cpp
│   └── logic/
│       ├── test_battle_system.cpp
│       ├── test_character_system.cpp
│       └── test_save_system.cpp
├── assets/                     # 游戏素材（从原作提取）
│   ├── dat/                    # 原始 DAT 文件
│   ├── img/                    # 提取的 PNG 图像
│   ├── wave/                   # 提取的 WAV 音频
│   ├── video/                  # MPEG/FLC 过场
│   └── font/                   # HZK16 字体
├── tools/                      # 开发工具
│   ├── dat_viewer/             # DAT 文件查看器
│   └── save_editor/            # 存档编辑器
├── scripts/                    # 构建/部署脚本
│   ├── build_web.sh
│   ├── build_switch.sh
│   └── deploy.sh
└── .github/
    └── workflows/
        └── ci.yml              # GitHub Actions CI
```

---

## 6. 接口设计原则

### 6.1 Core 层接口

- **纯函数优先**：所有计算公式为纯函数，输入确定则输出确定
- **POD 数据结构**：数据模型为 Plain Old Data，可序列化/反序列化
- **无动态分配**：Core 层不使用 new/malloc，全部使用栈或预分配数组
- **static_assert 校验**：所有结构体大小通过 static_assert 验证与原作一致

### 6.2 Logic 层接口

- **依赖 Core 层**：调用 Core 层公式和数据模型
- **状态封装**：每个系统封装自己的状态，不暴露内部数据
- **事件驱动**：系统间通过事件总线通信，不直接引用

### 6.3 Application/Presentation 层接口

- **实现依赖注入**：通过接口抽象，支持 mock 测试
- **资源生命周期**：RAII 管理所有 SDL 资源
- **场景模式**：通过场景栈管理游戏状态切换
