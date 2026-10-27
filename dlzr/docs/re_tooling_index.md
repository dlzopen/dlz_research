# RE 工具与资料索引（地雷战 / DLZ 逆向工程）

> **用途**：把本工程可用的全部逆向（RE）资源集中登记，供后续 RE 工作直接取用，
> 不再以"缺资料 / 分析不了"为由中断。凡涉及"原版怎么做"，优先按本索引逐层查证，
> 并遵守下方【铁律零】。

---

## 0. 铁律零（凌驾一切）

任何"原版怎么做"的论断，**必须**指向下列可证资源中的**具体坐标**（函数名+行号、
数据文件+偏移、或脚本+行号），禁止望文生义 / 结构补全 / 表象反推。
三类禁止行为一律不允许；唯一权威 = 反编译 / 反汇编 / 原始数据实测。

**遇到疑似阻塞时，按此顺序自查，绝大多数"阻塞"会自行消解：**

1. 先查 `output/ghidra_all/FUN_*.c`（REA 出的清晰 C 代码，768 个函数）。
2. 再查 `scripts/*.py`（Python 已写好的数据解析 / 抽取 / 分析代码，约 90 个）。
3. 再查 `template/*.bt`（010 Editor 模板，已登记的二进制数据格式）。
4. 仍不足 → **直接做 REA**：Ghidra 11.3.2 已装在 `ghidra/ghidra_11.3.2_PUBLIC/`，
   或拿 `llvm-objdump` 做字节级反汇编 + PE 段内立即数 xref 搜索（见 §5）。

---

## 1. 工具清单（已就绪，直接可用）

| 工具 | 路径 | 用途 |
|---|---|---|
| **Ghidra 11.3.2** (REA) | `ghidra/ghidra_11.3.2_PUBLIC/` | 反编译 / 反汇编 / 脚本分析；含 PyGhidra、Jython。DLZ.EXE 工程已载入。 |
| **llvm-objdump** | `C:\Program Files\LLVM\bin\llvm-objdump.exe` | 字节级反汇编（capstone/objdump 未装，但 llvm 足够）。 |
| **Python 3** | 环境自带 | 跑 `scripts/*.py`。 |
| **010 Editor** | 外部工具 | 打开 `template/*.bt` 解析原始数据文件。 |

> 注：Ghidra 反编译产物已导出为 `output/ghidra_all/*.c`，**无 GUI 也能查**。
> 只有当需要新函数、新交叉引用或动态分析时才需要启动 Ghidra 本身。

---

## 2. 资源 A — Ghidra 反编译 C 代码（`output/ghidra_all/`）

- **规模**：768 个 `FUN_XXXXXXXX.c`（每个文件 = 一个原函数，文件名即 VA 地址）。
- **命名约定**：`FUN_<8位十六进制地址>.c`，例如 `FUN_0040124c.c` 对应 VA `0x40124c`。
- **内容**：REA 反编译出的**可读 C**（变量 `iVar1`/`uVar1`/`local_xx`、全局 `DAT_xxxxxx`、
  调用点保留原调用）。是研究"原版算法"的第一手证据。
- **典型用法**：
  ```bash
  # 看某个函数的反编译实现
  cat output/ghidra_all/FUN_0042b192.c
  # 搜某全局被哪些函数读写
  grep -rl "DAT_0095eb88" output/ghidra_all/
  # 搜某个调用（例如洪水填充 FUN_00418e29 的调用方）
  grep -rl "FUN_00418e29" output/ghidra_all/
  ```
- **已锚定关键函数（示例，非穷举）**：
  - `FUN_0040124c.c` — 移动预算装配：预算 = `iVar3 + iVar2`（兵种移动力 + 物品加成）。
  - `FUN_0042b192.c` — 8 槽装备物品最大移动加成（含兼容矩阵 `DAT_0095eb88` 校验）。
  - `FUN_00418e29.c` — 地形加权洪水填充（移动可达集核心）。
  - `FUN_0043cc96.c` — 对话名替换令牌插值器（GG/TT/AA/BB/CC/DD）。
  - `FUN_0043e510.c` — 通用块流装载器（GAME.DAT 顺序块 → 运行时数组）。

---

## 3. 资源 B — Python 数据分析脚本（`scripts/`，约 90 个）

按用途分组（同名前缀归一类，便于按需求取用）：

| 类别（前缀） | 代表脚本 | 作用 |
|---|---|---|
| **export_** | `export_game.py` `export_map.py` `export_fight.py` `export_people.py` `export_army.py` `export_goods.py` `export_mine.py` `export_material.py` `export_face.py` `export_img.py` `export_flm.py` `export_anm.py` `export_wave.py` `export_txt.py` `export_title_screens.py` `export_zyb.py` `export_end04.py` `export_dat.py` `extract_all_data.py` | 解析原始 DAT 文件 → 结构化 dump / 媒体导出。**字段映射权威来源**（如 `BinZhong.yidong@+0x24`、`WuPin.para1@+0x20`）。 |
| **dump_** | `dump_fight0.py` `dump_fight_full.py` `dump_fight_people.py` `dump_people_class.py` `dump_panel_strings.py` `dump_candidates.py` `dump_verify_items.py` | 打印特定数据结构实例化内容（调试 / 核对用）。 |
| **analyze_** | `analyze_classes.py` `analyze_map.py` `balance_analysis.py` | 统计 / 平衡性 / 布局分析。 |
| **auto_name_** | `auto_name_by_evidence.py` `auto_name_crossref.py` `auto_name_evidence2.py` `auto_name_globals.py` `auto_name_loaders.py` `auto_name_subsystems.py` | 依据证据给 `DAT_*` / `FUN_*` 自动命名。 |
| **check_** | `check_army_indices.py` `check_buttons.py` `check_face_dat.py` `check_no_bare_addrs.py` | 不变量校验（命名/索引完整性）。 |
| **extract_** | `extract_army_indices.py` `extract_buttons.py` `extract_buttons2.py` `extract_character_network.py` `extract_character_relationships.py` `extract_global_usage.py` `extract_hub_usage.py` | 从二进制/数据中抽取特定索引或关系。 |
| **disasm_** | `disasm_4198e5.py` `disassemble_dlz.py` | 局部反汇编辅助。 |
| **discover_** | `discover_mem_candidates.py` `discover_oracle_candidates.py` | 发现"未解析 → 可还原"的候选（配合 §5 方法）。 |
| **stub_** | `gen_replica_stubs.py` `stub_inventory.py` `resolve_unresolved_abis.py` | 755 桩生成 / 库存盘点 / 未解析 ABI 还原（反向调用图）。 |
| **merge_** | `merge_auto_globals.py` `merge_auto_names.py` `merge_crossref.py` `merge_logic_only.py` `merge_supplement.py` | 合并自动命名 / 交叉引用结果。 |
| **scan_** | `scan_call_arity.py` `scan_calls_multiline.py` `scan_gbk.py` `scan_replica_calls.py` | 调用点 / 编码扫描。 |
| **fix_/repair_/promote_/pick_/restore_/fill_/update_** | `fix_call_sites.py` `fix_call_sites_v2.py` `fix_duplicate_names.py` `repair_renames.py` `promote_void_signatures.py` `pick_rename_candidates.py` `restore_unresolved_signature.py` `fill_choice_labels.py` `fill_level_dialogue.py` `fill_remaining_dialogue.py` `update_level_dialogue.py` | 命名 / 签名 / 调用点修复。 |
| **其他** | `build_re_map.py` `classify_globals.py` `count_call_args.py` `find_scalar_candidates.py` `process_ghidra_batch.py` `regional_merge.py` `semantic_analyze.py` `dlz_algorithm.py` `decompile_dialogue.py` `integrate_dialogue.py` `generate_test_save.py` `parse_save.py` `recover_panel_labels.py` `generate_symbol_table.py` `convert_to_entry_indices.py` `hex_to_entry_index.py` `pe_extract_naked_addrs.py` | 杂项 RE 辅助（映射 / 符号表 / 存档解析等）。 |

> **取用原则**：要"读某个数据结构"，先看对应 `export_*.py`（字段偏移已在其中写死并实测）；
> 要"找某个全局的写入方"，先 `grep` 再配合 §5 的 xref 搜索。

---

## 4. 资源 C — 010 Editor 模板（`template/`，19 个 `.bt`）

二进制数据格式的可视化 / 结构化解析定义：

| 文件 | 解析对象 |
|---|---|
| `dlz_game.bt` | GAME.DAT（兵种/物品/地雷/技能/兼容矩阵/地形表/移动消耗） |
| `dlz_people.bt` | PEOPLE.DAT / 运行时 People（132B，含 `items[8]` @ +0x74） |
| `dlz_fight.bt` | FIGHT.DAT（Intro + Fighter[128] + People[128]） |
| `dlz_army.bt` | 军团/阵型数据 |
| `dlz_town.bt` | TOWN.DAT |
| `dlz_goods.bt` | 物品/商店 |
| `dlz_mine.bt` | DiLei 地雷 |
| `dlz_material.bt` | 锻造材料 |
| `dlz_place.bt` | 地图放置物 |
| `dlz_save.bt` | 存档（28 数据块） |
| `dlz_map.bt` | MAP.DAT 瓦片网格 |
| `dlz_img.bt` `dlz_flm.bt` `dlz_anm.bt` `dlz_blk.bt` `dlz_face.bt` `dlz_wave.bt` `dlz_flc.bt` `TGA.bt` | 图像/动画/音频等媒体格式 |

> **注意（诚实标注）**：部分 `.bt` 模板的字段偏移已被后续 PE/Python 实证**修订**
> （例如兵种 stride 真实为 `0xB0` 而非 0xa0、移动力在 `+0x24`）。
> 模板是**起点**而非真理；凡与 `scripts/export_*.py` 或 `output/ghidra_all/*.c` 冲突，
> 以**实测数据 + 反编译**为准（见 `dat_loader.cpp` 顶部注释的基址实证）。

---

## 5. 硬核 RE 技法（无需 Ghidra GUI 也能推进）

### 5.1 PE 字节级 xref 搜索（定位全局引用）
扫 `.text` 段 4 字节小端立即数，定位对某全局的引用（比依赖 Ghidra 符号更硬）：
```bash
# 例：找谁引用了 0x47258c（对话脚本指针表）
python - <<'PY'
import sys
data = open(r"DLZFULL/DLZ.EXE","rb").read()
# 解析 PE 找到 .text 文件偏移/RVA 后，搜 b"\x8c\x25\x47\x00" 等立即数模式
PY
```

### 5.2 GAME.DAT 直接解析（Python）
GAME.DAT 是顺序块布局，关键基址（已三重实证）：
```
+0x0000 What0            (160B)
+0x00A0 DiXingModifier[16] (640B)
+0x1E0  ClassType[96]    (15360B)  ← 兵种真实基址（非 data_dictionary 的 0x320）
+0x3F20 BingZhongYiDong (1536B, 地形移动消耗)
+0x43E0 WuPin[160]       (11520B)  ← 物品，para1@+0x20
+0x7260 Material[10]
+0x73A0 DiLeiRecipe[10]
+0x7404 DiLei[10]
+0x75E4 JiNeng[16]
+0x78E4 SelectFanwei[16]
+0x7DF0 ApplyFanwei[48]
+0xA310 WuPinValid[160×96] (15360B, 兼容矩阵 = 文件末尾)
```
→ 直接拿 Python 读这些偏移即可拿到兵种移动力（8/4/4…）、物品 para1 等，
   无需反编译即可验证数据布局（见 `scripts/export_game.py`）。

### 5.3 运行时表 vs 文件表区分
- `.data` 未初始化区全局（如兵种/物品运行数组）PE 抽不到 → 由 GAME.DAT 顺序块运行时装载，
  查引用前先 `grep` 谁写它（`FUN_0043e510` 逐块 `param_2*param_3` 累加偏移）。
- 抽全局前先判 `rva − vaddr < rawsz`：在 PE 内才直接抽，否则走 GAME.DAT 偏移。

---

## 6. 工作流示范：移动预算分叉闭合（已落地）

证明"按本索引可终结分叉"的实例：

1. **怀疑点**：干净层 `get_movement_range` = `cls.movement * fighter.movement/100`（求积、无物品），
   与二进制行为疑似分叉。
2. **查证**（资源 A）：`FUN_0040124c.c:32` 预算 `= iVar3 + iVar2`；
   `FUN_0042b192.c:16-21` 给出 `iVar2` = 8 槽装备物品 `max(WuPin[item-1].para1)`（含兼容矩阵校验）。
3. **佐证**（资源 B）：`scripts/export_game.py` 确认 `WuPin.para1@+0x20`、
   `ClassType.movement@+0x24`；`scripts/export_map.py` 确认地形旗标取 `(tile>>12)&0xF`。
4. **落地**：`battle_engine.cpp::get_movement_range` 改为
   `base = cls.movement; bonus = max(兼容物品 WuPin.para1); return base+bonus;`
   （求和 + 物品加成），并改写 `test_movement_range.cpp` 闸门由"锁旧契约"改为"对齐二进制公式"。
5. **验证**：`dlzr_tests.exe` 全绿（287/287），`MatchesBinaryFloodFill` 与 `BudgetIsSumOfClassMovementPlusMaxItemBonus` 同过。

> **结论**：当三类资源齐备 + Ghidra 已装，"分析不了"不成立。任何新分叉都用 §0→§5 的同一套路闭合。

---

## 7. 当前已闭合 / 进行中的 RE 结论速查

- **移动预算** = 兵种移动力 + 兼容装备物品 `WuPin.para1` 最大（求和）。（§6）
- **对话令牌** GG/TT/AA/BB/CC/DD 名替换令牌**确存**于二进制（`FUN_0043cc96.c`）。
- **地形旗标** = `(tile>>12)&0xF`，地形来自 MAP.DAT（非 FIGHT.DAT `grid_terrain[92]`）。
- **兵种表真实基址** GAME.DAT `0x1E0`，stride `0xB0`（176B），移动力 `+0x24`。
- **WuPin** 基址 `0x43E0`，stride 72B，`para1` @ `+0x20`。
- **兼容矩阵** `wupin_compat[item][class]` 来自 GAME.DAT 末尾 15360 字节。

（更多结论见 `dlzr/docs/replica_roadmap.md` 与各 `test_*.cpp` 注释。）
