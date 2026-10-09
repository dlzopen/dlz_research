# DLZR — 抗日之地雷战 1:1 复刻工程

经典国产战棋游戏《抗日之地雷战》(1998 西山居) 高精度复刻工程。

## 项目状态

**设计阶段** — 工程文档体系已完成，待进入实施。

## 文档索引

| 文档 | 说明 |
|------|------|
| [01_project_overview.md](docs/01_project_overview.md) | 项目总览：背景、目标、范围、里程碑、验收标准 |
| [02_tech_selection.md](docs/02_tech_selection.md) | 技术选型：SDL3 + C++20 选型论证 |
| [03_architecture_design.md](docs/03_architecture_design.md) | 架构设计：四层分层架构、模块划分、接口定义 |
| [04_core_logic_design.md](docs/04_core_logic_design.md) | 核心逻辑：战斗公式、状态机、AI、关卡数据结构 |
| [05_implementation_roadmap.md](docs/05_implementation_roadmap.md) | 实施路线图：5 阶段 22 周分阶段交付计划 |
| [06_engineering_standards.md](docs/06_engineering_standards.md) | 工程规范：代码规范、构建流程、测试规范 |

## 技术栈

- **语言**: C++20
- **平台抽象**: SDL3
- **构建系统**: CMake 3.24+
- **目标平台**: Windows / Linux / macOS / HTML(Web) / Nintendo Switch

## 逆向工程基线

本项目基于已完成的完整逆向工程成果：
- 768/771 函数反编译 (99.6%)
- 17 种 DAT 文件格式完整解析
- 45 个核心函数精修 C 代码
- 完整战斗公式、AI 行为、数值体系文档

详见 [dlz_research 项目](../)。

## License

MIT
