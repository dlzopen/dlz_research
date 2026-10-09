# DLZR 工程规范文档

## 抗日之地雷战 1:1 复刻工程

**日期**：2026-10-09  
**状态**：评审中

---

## 1. 代码规范

### 1.1 语言标准

- **C++20** (ISO/IEC 14882:2020)
- 编译器要求: MSVC 2022 (v143) / GCC 13 / Clang 17
- 所有编译器 warning 视为 error (`-Werror` / `/WX`)

### 1.2 命名规范

| 类别 | 风格 | 示例 |
|------|------|------|
| 命名空间 | snake_case | `dlzr::core`, `dlzr::logic::battle` |
| 类/结构体 | PascalCase | `People`, `BattleSystem`, `ClassType` |
| 函数/方法 | snake_case | `calc_attack_power()`, `init_battle()` |
| 成员变量 | snake_case + 下划线后缀 | `battle_phase_`, `total_fighter_count_` |
| 局部变量 | snake_case | `raw_atk`, `skill_level` |
| 常量 | UPPER_SNAKE_CASE | `GAME_MODE_BATTLE`, `MORALE_DIVISOR` |
| 枚举值 | PascalCase | `GameMode::Battle`, `BattleCommand::EndVictory` |
| 枚举类 | PascalCase | `enum class GameMode`, `enum class BattleCommand` |
| 文件名 | snake_case | `calc_attack_power.cpp`, `battle_system.hpp` |
| 目录名 | snake_case | `data_model/`, `formulas/`, `state_machine/` |
| 模板参数 | T 前缀 PascalCase | `TValue`, `TContainer` |

### 1.3 格式化规则

使用 `clang-format` 统一格式化，配置文件 `.clang-format`:

```yaml
---
BasedOnStyle: Google
IndentWidth: 4
ColumnLimit: 100
AllowShortFunctionsOnASingleLine: Inline
AllowShortIfStatementsOnASingleLine: false
AllowShortLoopsOnASingleLine: false
BreakBeforeBraces: Attach
PointerAlignment: Left
SortIncludes: true
IncludeBlocks: Regroup
---
```

### 1.4 文件模板

```cpp
// 源文件头注释（仅用于标记原作地址映射的关键文件）
// calc_attack_power.cpp — 攻击力计算 / Attack Power Calculation
// Original address: 0x0042A82A

#include "core/formulas/combat.hpp"

namespace dlzr::core {

int32_t calc_attack_power(
    const People& person,
    const Fighter& fighter,
    const ClassType& class_type,
    int32_t skill_level,
    int32_t item_bonus,
    bool in_battle
) {
    // Layer 1: 基础攻击
    int32_t raw_atk = (class_type.base_attack * 3 * person.strength) / 100
                    + (class_type.attack_growth - 1) * (person.level - 1);

    // Layer 2: 武力×技能缩放
    int32_t attack = ((skill_level + 59)
                     * ((raw_atk * (person.strength + 500)) / 580))
                     / 60;

    // Layer 3: 战斗模式修正
    if (in_battle) {
        attack = (attack * fighter.equip_integrity) / 100;
        attack = ((fighter.morale + 100) * attack) / 200;
    }

    // Layer 4: 物品加成
    attack += item_bonus;

    return attack;
}

} // namespace dlzr::core
```

### 1.5 注释规范

- **不写显而易见的注释**：代码本身应自解释
- **关键公式标注原作地址**：`// Original address: 0x0042A82A`
- **标注整数除法陷阱**：`// 注意: 整数截断除法，与原作 x86 idiv 行为一致`
- **标注 magic number 来源**：`// 59 = SKILL_LEVEL_BASE (原作 0x3B)`
- **不写 TODO 注释**：用 issue tracker 管理待办事项
- **双语注释可选**：核心公式可附英文注释，辅助代码不强制

### 1.6 禁止事项

| 禁止 | 替代方案 |
|------|----------|
| `using namespace std;` (头文件中) | 显式 `std::` 前缀 |
| 裸指针管理内存 | `std::unique_ptr` / `std::array` / `std::vector` |
| C 风格类型转换 | `static_cast<>` / `reinterpret_cast<>` |
| `#define` 常量 | `constexpr` / `const` |
| `malloc` / `free` | `new` / `delete` (或更好: 智能指针) |
| 全局可变状态 | 依赖注入 / 单例(仅限平台层) |
| 递归包含 | `#pragma once` |
| `goto` | 结构化控制流 |

---

## 2. 目录规范

### 2.1 源码目录

```
src/
├── core/               # 核心逻辑层 — 零外部依赖
│   ├── data_model/     # 数据结构定义
│   ├── formulas/       # 战斗公式（纯函数）
│   └── state_machine/  # 状态机
├── logic/              # 游戏逻辑层 — 依赖 core
│   ├── battle/         # 战斗系统
│   ├── strategic/      # 战略系统
│   ├── character/      # 角色系统
│   ├── dialogue/       # 对话系统
│   ├── save/           # 存档系统
│   ├── hidden_level/   # 隐藏关卡
│   └── crafting/       # 锻造系统
├── app/                # 应用层 — 编排逻辑
│   ├── scene/          # 场景管理
│   ├── input/          # 输入处理
│   └── resource/       # 资源管理
├── presentation/       # 表现层 — SDL3 渲染/音频
│   ├── ui/             # UI 组件
│   └── font/           # 字体渲染
└── platform/           # 平台抽象层
    ├── sdl3/           # SDL3 实现
    └── web/            # Web 平台特定
```

### 2.2 测试目录

```
tests/
├── core/               # 核心逻辑测试
│   ├── test_combat_formulas.cpp
│   ├── test_experience.cpp
│   ├── test_skill_effect.cpp
│   └── test_state_machine.cpp
├── logic/              # 游戏逻辑测试
│   ├── test_battle_system.cpp
│   ├── test_character_system.cpp
│   ├── test_class_promotion.cpp
│   └── test_save_system.cpp
├── app/                # 应用层测试
│   └── test_resource_manager.cpp
└── integration/        # 集成测试
    ├── test_full_battle.cpp
    └── test_full_game_flow.cpp
```

### 2.3 资源目录

```
assets/
├── dat/                # 原始 DAT 文件（运行时直读，不入库，gitignore）
│   ├── PEOPLE.DAT      # 角色数据
│   ├── GAME.DAT        # 兵种数据
│   ├── FIGHT.DAT       # 关卡数据
│   ├── TOWN.DAT        # 城镇数据
│   ├── TXT.DAT         # 对话数据（XOR 0x51 加密）
│   ├── WAVE.DAT        # 音频资源
│   └── ...             # 其他原始 DAT 文件
├── img/                # 提取的 PNG 图像（仅开发期预览用）
│   ├── backgrounds/    # 背景图
│   ├── sprites/        # 精灵图
│   ├── ui/             # UI 元素
│   └── animations/     # 动画帧序列
├── wave/               # 提取的 WAV 音频（仅开发期预览用）
├── video/              # MPEG/FLC 过场（不入库，gitignore）
└── font/               # HZK16 字体文件
```

> **注意**：运行时直接读取 `dat/` 下的原始二进制文件，不需要 JSON/CSV 等中间格式。
> `img/` 和 `wave/` 下的提取文件仅用于开发期素材预览和调试，不随重制版发布。
> 玩家需指定原作游戏数据目录路径，重制版从中直接读取所有数据。

### 2.4 文件命名

| 类型 | 命名规则 | 示例 |
|------|----------|------|
| 头文件 | snake_case.hpp | `combat.hpp`, `battle_system.hpp` |
| 源文件 | snake_case.cpp | `combat.cpp`, `battle_system.cpp` |
| 测试文件 | test_snake_case.cpp | `test_combat_formulas.cpp` |
| CMake 文件 | CMakeLists.txt | 每个目录一个 |
| 配置文件 | kebab-case.ext | `.clang-format`, `.clang-tidy` |

---

## 3. 构建流程

### 3.1 CMake 结构

```cmake
# 根 CMakeLists.txt
cmake_minimum_required(VERSION 3.24)
project(dlzr VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 选项
option(DLZR_BUILD_TESTS "Build unit tests" ON)
option(DLZR_BUILD_TOOLS "Build development tools" ON)
option(DLZR_WEB_BUILD  "Build for Web (Emscripten)" OFF)

# 子模块
add_subdirectory(src/core)
add_subdirectory(src/logic)
add_subdirectory(src/app)
add_subdirectory(src/presentation)
add_subdirectory(src/platform)

# 主可执行文件
add_executable(dlzr src/main.cpp)
target_link_libraries(dlzr PRIVATE dlzr_app dlzr_presentation dlzr_platform)

# 测试
if(DLZR_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
```

### 3.2 构建命令

```bash
# Windows (MSVC)
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release

# Windows (MinGW)
cmake -B build -G "MinGW Makefiles"
cmake --build build --config Release

# Linux / macOS
cmake -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# Web (Emscripten)
emcmake cmake -B build-web -DDLR_WEB_BUILD=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-web -j$(nproc)

# 运行测试
ctest --test-dir build --output-on-failure
```

### 3.3 构建目标

| 目标 | 类型 | 说明 |
|------|------|------|
| `dlzr` | executable | 主游戏可执行文件 |
| `dlzr_core` | static lib | 核心逻辑库（可独立测试） |
| `dlzr_logic` | static lib | 游戏逻辑库 |
| `dlzr_app` | static lib | 应用层库 |
| `dlzr_presentation` | static lib | 表现层库 |
| `dlzr_platform` | static lib | 平台抽象层库 |
| `dlzr_tests` | executable | 测试可执行文件 |

### 3.4 依赖管理

```cmake
# CMake FetchContent — 自动下载依赖
include(FetchContent)

# SDL3
FetchContent_Declare(SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.x
)
FetchContent_MakeAvailable(SDL3)

# Google Test (仅测试)
if(DLZR_BUILD_TESTS)
    FetchContent_Declare(googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    FetchContent_MakeAvailable(googletest)
endif()
```

> **注意**：不引入 nlohmann/json 或其他 JSON 库。运行时直接读写原始二进制格式，
> 无需 JSON 序列化。

---

## 4. 测试规范

### 4.1 测试框架

- **Google Test 1.14+** 用于单元测试
- **核心逻辑层 (core/)** 测试覆盖率目标 ≥ 90%
- **游戏逻辑层 (logic/)** 测试覆盖率目标 ≥ 70%

### 4.2 测试命名

```cpp
// 测试文件: test_combat_formulas.cpp

// 测试套件: 模块名
// 测试用例: 功能_场景_预期
TEST(CombatFormulas, AttackPower_BaseCase) { ... }
TEST(CombatFormulas, AttackPower_WithMoraleBonus) { ... }
TEST(CombatFormulas, AttackPower_WithEquipmentDegradation) { ... }
TEST(CombatFormulas, Defense_WithTerrainBonus) { ... }
TEST(CombatFormulas, Defense_ZeroDamage) { ... }
```

### 4.3 核心公式测试模板

```cpp
#include <gtest/gtest.h>
#include "core/formulas/combat.hpp"

namespace dlzr::core::test {

// 攻击力计算: 基础情况（无战斗修正）
TEST(CombatFormulas, AttackPower_BaseCase) {
    People person{};
    person.strength = 80;
    person.level = 10;
    person.class_type = 0;
    person.skill_use_count = 15;  // skill_level = 5

    ClassType cls{};
    cls.base_attack = 50;
    cls.attack_growth = 8;

    Fighter fighter{};
    
    int32_t result = calc_attack_power(person, fighter, cls, 5, 0, false);
    
    // Layer 1: (50 * 3 * 80 / 100) + (8-1) * (10-1) = 120 + 63 = 183
    // Layer 2: (5+59) * (183 * (80+500) / 580) / 60 = 64 * (183*580/580) / 60 = 64*183/60 = 195
    EXPECT_EQ(result, 195);
}

// 攻击力计算: 战斗模式（含士气和装备完整度）
TEST(CombatFormulas, AttackPower_BattleMode) {
    People person{};
    person.strength = 80;
    person.level = 10;
    person.class_type = 0;

    ClassType cls{};
    cls.base_attack = 50;
    cls.attack_growth = 8;

    Fighter fighter{};
    fighter.morale = 100;          // (100+100)/200 = 1.0x
    fighter.equip_integrity = 100; // 100/100 = 1.0x

    int32_t result = calc_attack_power(person, fighter, cls, 5, 0, true);
    
    // 战斗修正: × 100/100 × (100+100)/200 = × 1.0
    // 结果应与无修正相同
    EXPECT_EQ(result, 195);
}

// 防御力计算: 含地形加成
TEST(CombatFormulas, Defense_WithTerrainBonus) {
    // ...
}

// 技能伤害公式
TEST(SkillEffect, HpDamage_BaseCase) {
    // hp_effect = ((param1 * intelligence) / 100) * (skill_level + 7) >> 3
    int32_t result = calc_skill_effect(80, 86, 5);
    // ((80 * 86) / 100) * (5 + 7) >> 3 = 68 * 12 >> 3 = 816 >> 3 = 102
    EXPECT_EQ(result, 102);
}

} // namespace dlzr::core::test
```

### 4.4 测试数据

- 使用原作 DAT 文件的真实二进制数据作为测试输入（通过 `dat_loader` 加载）
- 关键公式至少 20 组测试用例，覆盖边界值
- 使用原作 `save0.dat` 二进制存档作为存档系统测试基准（直接读取原始格式验证反序列化）
- 测试所需的原始 DAT 文件通过 CTest `add_test` 的 `WORKING_DIRECTORY` 指向 `assets/dat/`

---

## 5. CI/CD 规范

### 5.1 GitHub Actions 配置

```yaml
# .github/workflows/ci.yml
name: CI

on:
  push:
    branches: [main, develop]
  pull_request:
    branches: [main]

jobs:
  build-windows:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v4
      - name: Configure CMake
        run: cmake -B build -G "Visual Studio 17 2022"
      - name: Build
        run: cmake --build build --config Release
      - name: Test
        run: ctest --test-dir build --output-on-failure

  build-web:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: mymindstorm/setup-emsdk@v14
      - name: Configure
        run: emcmake cmake -B build-web -DDLR_WEB_BUILD=ON
      - name: Build
        run: cmake --build build-web

  build-linux:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Install deps
        run: sudo apt-get install -y libsdl3-dev
      - name: Configure
        run: cmake -B build -DCMAKE_BUILD_TYPE=Release
      - name: Build
        run: cmake --build build -j$(nproc)
      - name: Test
        run: ctest --test-dir build --output-on-failure
```

### 5.2 分支策略

| 分支 | 用途 | 合并规则 |
|------|------|----------|
| `main` | 稳定发布版 | 仅从 `develop` 合并 |
| `develop` | 开发主线 | 从 feature 分支 PR 合并 |
| `feature/*` | 功能开发 | 完成后 PR 到 `develop` |
| `fix/*` | Bug 修复 | 完成后 PR 到 `develop` 或 `main` |
| `release/*` | 发布准备 | 从 `develop` 创建，合并到 `main` |

### 5.3 PR 要求

- 所有 CI 检查通过（编译 + 测试）
- 代码经 clang-format 格式化
- 无 clang-tidy warning
- 核心逻辑变更必须附带单元测试
- 至少 1 人 code review（当团队 > 1 人时）

---

## 6. 版本管理

### 6.1 版本号

遵循语义化版本 (Semantic Versioning):

```
MAJOR.MINOR.PATCH

0.1.0  — Phase 0 完成（工程基建）
0.2.0  — Phase 1 完成（战斗核心）
0.3.0  — Phase 2 完成（战略层）
0.4.0  — Phase 3 完成（剧情完善）
1.0.0  — Phase 4 完成（多端发布）
```

### 6.2 Git 提交规范

```
<type>(<scope>): <description>

type:
  feat     — 新功能
  fix      — Bug 修复
  refactor — 重构（不改变行为）
  test     — 测试相关
  docs     — 文档
  build    — 构建系统
  chore    — 杂项

scope:
  core, logic, app, presentation, platform, ci, docs

示例:
  feat(core): implement calc_attack_power four-layer formula
  fix(logic): correct integer truncation in skill_effect calculation
  test(core): add 20 unit tests for calc_defense with terrain bonus
  docs(architecture): update data flow diagram for battle system
```

---

## 7. 静态分析

### 7.1 clang-tidy 配置

```yaml
# .clang-tidy
Checks: >
  bugprone-*,
  cert-*,
  cppcoreguidelines-*,
  modernize-*,
  performance-*,
  portability-*,
  readability-*,
  -modernize-use-trailing-return-type,
  -readability-magic-numbers,
  -cppcoreguidelines-avoid-magic-numbers

WarningsAsErrors: ''

HeaderFilterRegex: 'src/.*'

CheckOptions:
  - key: cert-dcl16-c.IgnoreMacros
    value: true
  - key: readability-identifier-naming.NamespaceCase
    value: lower_case
  - key: readability-identifier-naming.ClassCase
    value: CamelCase
  - key: readability-identifier-naming.FunctionCase
    value: lower_case
```

### 7.2 内存安全

- **桌面端**: AddressSanitizer + UndefinedBehaviorSanitizer 在 CI 中运行
- **Web 端**: Emscripten 内置安全检查
- **发布前**: Valgrind memcheck 完整扫描

```bash
# 带 sanitizer 构建
cmake -B build-san -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer"
cmake --build build-san
./build-san/tests/dlzr_tests
```

---

## 8. 素材管理规范

### 8.1 素材来源

所有游戏素材从原作 `DLZFULL/` 目录提取，不自行创建或修改：

| 素材类型 | 来源 | 格式 | 数量 |
|----------|------|------|------|
| 背景/UI | IMG 文件 | 8-bit PNG | 100+ |
| 动画帧 | ANM 文件 | 序列 PNG | 1030+ |
| 音效 | WAVE.DAT | WAV (22050Hz 16-bit mono) | 152 |
| 过场 | MPEG 文件 | MPG | 33 |
| FLC 动画 | FLC 文件 | FLC | 4 |
| 字体 | HZK16 + ASC16 | 点阵字体 | 2 文件 |

### 8.2 素材 Git 管理

```gitignore
# .gitignore
# 大素材文件不入库
assets/dat/
assets/video/
*.mpg
*.flc
*.dat

# 但保留小素材（UI、精灵）
!assets/img/
!assets/wave/
!assets/font/
```

大素材（视频、DAT 原始文件）通过 Git LFS 或单独的资源包分发。

### 8.3 素材转换脚本

```
scripts/
├── export_assets.py    # 从 DLZFULL/ 提取全部素材到 assets/
├── convert_img.py      # IMG → PNG 批量转换（仅开发期预览）
├── convert_anm.py      # ANM → PNG 序列批量转换（仅开发期预览）
└── convert_wave.py     # WAVE.DAT → WAV 批量提取（仅开发期预览）
```

> **注意**：无 `generate_json.py`。运行时直接读取原始 DAT 二进制文件，
> 不需要数据格式转换步骤。上述脚本仅用于开发期素材预览和调试。
