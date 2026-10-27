/* 自动生成：dlzr 复刻函数（C，rf_xxxx） */
/* 来源 output/ghidra_all/FUN_*.c —— 逐行照抄反编译。证据制下的 1:1 转录。 */
/* 本文件由 scripts/gen_replica_stubs.py 生成，勿手工编辑（改转录器后重跑）。 */
#ifndef DLZR_REPLICA_STUBS_H
#define DLZR_REPLICA_STUBS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "battle_calc.hpp"  /* 已语义化转发的战斗数值函数（extern "C"）声明 */
/* 原版为 Win32 程序，反编译含平台/窗口过程/MCI 等类型与 API 声明 */
#ifndef _WIN32
typedef void* HWND; typedef void* HINSTANCE; typedef void* HICON; typedef void* HCURSOR;
typedef void* HGDIOBJ; typedef void* HDC; typedef void* HMENU; typedef void* HMODULE; typedef void* HANDLE;
typedef void* HBRUSH; typedef void* HGLOBAL; typedef void* LPVOID; typedef const char* LPCSTR;
typedef unsigned int UINT; typedef unsigned long ULONG; typedef long LONG; typedef unsigned short WORD;
typedef unsigned long DWORD; typedef int BOOL; typedef unsigned char BYTE;
typedef unsigned long WPARAM; typedef long LPARAM; typedef long LRESULT; typedef unsigned long MCIDEVICEID;
typedef unsigned long MCIERROR; typedef void* HLOCAL;
#endif
#ifdef _WIN32
#include <windows.h>   /* 声明中用到 HWND/HANDLE/DWORD 等，需在声明前可见 */
#include <mmsystem.h>  /* MCI 音频 API */
#include <dsound.h>   /* DirectSoundCreate（原版音频初始化） */
#include <ddraw.h>    /* DirectDrawCreate（原版图形初始化，副本运行时依赖其 vtable） */
#endif
/* 函数指针虚表转发类型（原版经数据虚表调用 **(code **)(addr)(args)）
   C 下用空参数列表 = 未指定参数（K&R 风格），可被任意实参调用，
   既消除 C2660（参数个数不匹配）也兼容链式/无参虚调用。 */
typedef int (*ghidra_func_t)();
/* Ghidra 拼接/取字节 宏（CONCATxx/SUB4x）与联合字段访问宏——转录期占位，运行时收口 GameState */
#ifndef CONCAT22
#define CONCAT22(lo,hi) ((int32_t)(((uint16_t)(lo)) | ((uint32_t)(uint16_t)(hi) << 16)))
#endif
#ifndef CONCAT11
#define CONCAT11(lo,hi) ((int16_t)(((uint8_t)(lo)) | ((uint16_t)(uint8_t)(hi) << 8)))
#endif
#ifndef CONCAT31
#define CONCAT31(a,b) ((int32_t)(((uint32_t)(a) & 0xffffff) | ((uint32_t)(uint8_t)(b) << 24)))
#endif
#ifndef CONCAT12
#define CONCAT12(a,b) ((int32_t)(((uint32_t)(uint8_t)(a)) | ((uint32_t)(uint16_t)(b) << 8)))
#endif
#ifndef CONCAT21
#define CONCAT21(a,b) ((int32_t)(((uint32_t)(uint16_t)(a)) | ((uint32_t)(uint8_t)(b) << 16)))
#endif
#ifndef CONCAT44
#define CONCAT44(a,b) ((int64_t)((uint64_t)(a) | ((uint64_t)(b) << 32)))
#endif
#ifndef SUB41
#define SUB41(x,n) ((uint8_t)((uint32_t)(x) >> (8 * (n))))
#endif
/* 联合字段 X._O_S_ -> 按字节偏移 O 把 X 重新解释为 int32_t（转录占位） */
#define GHIDRA_FIELD(X,O) (*(int32_t *)((char *)&(X) + (O)))

/* === 全局状态收口：单一 GameState 结构（原 1565+ 个 gs_<addr> 全局归并）=== 
   证据制铁律零：每条成员保留 Ghidra 反编译确定的原类型（标量/指针/函数指针），
   不臆造语义类型/分组。成员名沿用 gs_<addr>（可被 renames.csv 改名机制重命名）。
   字符串/数据标签（const char[]）因 C 不允许零长数组成员，保持独立全局。
*/

/* 结构化区域 Fighter @ 0x0096ab90  stride 0x40 * 128  (1:1 内存->结构体；字段名 RE 实证，未知用 unknown_*) */
typedef struct Fighter {
    int32_t max_hp;
    int32_t current_hp;
    int32_t morale;
    int32_t equip_integrity;
    int32_t defense_modifier;
    int32_t unknown_14;
    int32_t unknown_18;
    int32_t unknown_1c;
    int32_t pos_x;
    int32_t pos_y;
    int32_t unknown_28;
    int32_t unknown_2c;
    int32_t unknown_30;
    int32_t unknown_34;
    int32_t attacker_index;
    uint8_t active_flag;
    uint8_t ai_action;
    uint8_t flag2;
    uint8_t unit_state;
} Fighter;
/* static_assert(sizeof(Fighter)==0x40) 见 types.hpp（C 模式不在此重复） */

/* 结构化区域 People @ 0x007d0ef8  stride 0x84 * 128  (1:1 内存->结构体；字段名 RE 实证，未知用 unknown_*) */
typedef struct People {
    char name[16];
    int32_t face;
    int32_t pad2;
    int32_t enemy;
    int32_t strength;
    int32_t intelligence;
    int32_t level;
    int32_t experience;
    int32_t class_type;
    int32_t skill_gongji;
    int32_t skill_buji;
    int32_t skill_jili;
    int32_t skill_yiliao;
    int32_t skill_huogong;
    int32_t skill_gunmu;
    int32_t skill_leishi;
    int32_t skill_weishe;
    int32_t skill_saorao;
    int32_t skill_pohuai;
    int32_t skill_lueduo;
    int32_t skill_fanjian;
    int32_t skill_yibing;
    int32_t skill_duzhan;
    int32_t skill_ansha;
    int32_t skill_shidu;
    int32_t skill_kongxi;
    uint8_t items[8];
    uint8_t mines[8];
} People;
/* static_assert(sizeof(People)==0x84) 见 types.hpp（C 模式不在此重复） */

/* 结构化区域 ClassType @ 0x0096cbac  stride 0xa0 * 96  (1:1 内存->结构体；字段名 RE 实证，未知用 unknown_*) */
typedef struct ClassType {
    int32_t hp_base;
    int32_t hp_growth;
    int32_t attack_growth;
    int32_t defense_growth;
    int32_t movement;
    int32_t base_attack;
    int32_t base_defense;
    int32_t unknown_1c;
    int32_t unknown_20;
    int32_t unknown_24;
    int32_t unknown_28;
    int32_t unknown_2c;
    int32_t select_range;
    int32_t apply_range;
    int32_t promotion_class;
    int32_t unknown_3c;
    uint8_t pad[96];
} ClassType;
/* static_assert(sizeof(ClassType)==0xa0) 见 types.hpp（C 模式不在此重复） */

/* 结构化区域 Skill @ 0x0057ba40  stride 0x30 * 16  (1:1 内存->结构体；字段名 RE 实证，未知用 unknown_*) */
typedef struct Skill {
    char name[16];
    int32_t param1;
    int32_t param2;
    int32_t field_18;
    int32_t movement_index;
    int32_t sprite_index;
    int32_t field_24;
    int32_t field_28;
    int32_t field_2c;
} Skill;
/* static_assert(sizeof(Skill)==0x30) 见 types.hpp（C 模式不在此重复） */
#ifndef MAP_GRID_CAP
#define MAP_GRID_CAP 16384
#endif
typedef struct GameState {
    int32_t g_rodata_5000[16];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_rodata_5040[1];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_rodata_5044[23];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_rodata_50a0;  /* DAT_/PTR_004450a0 (标量) */
    int32_t g_rodata_50b0;  /* DAT_/PTR_004450b0 (标量) */
    int32_t g_rodata_50c0;  /* DAT_/PTR_004450c0 (标量) */
    int32_t g_rodata_50d0;  /* DAT_/PTR_004450d0 (标量) */
    int32_t g_rodata_50e0;  /* DAT_/PTR_004450e0 (标量) */
    int32_t g_rodata_50f0[8];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_rodata_5110[956];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_datatbl_6000;  /* DAT_/PTR_00446000 (标量) */
    int32_t g_datatbl_6004;  /* DAT_/PTR_00446004 (标量) */
    int32_t g_datatbl_6008;  /* DAT_/PTR_00446008 (标量) */
    int32_t g_datatbl_6010;  /* DAT_/PTR_00446010 (标量) */
    int32_t g_datatbl_6014;  /* DAT_/PTR_00446014 (标量) */
    int32_t g_datatbl_601c;  /* DAT_/PTR_0044601c (标量) */
    int32_t g_datatbl_6020;  /* DAT_/PTR_00446020 (标量) */
    int32_t g_datatbl_6024;  /* DAT_/PTR_00446024 (标量) */
    int32_t g_pow2_2_446030[256];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_datatbl_6430;  /* DAT_/PTR_00446430 (标量) */
    int32_t g_datatbl_6434;  /* DAT_/PTR_00446434 (标量) */
    int32_t* g_datatbl_6438;  /* DAT_/PTR_00446438 (指针/缓冲区，按实际用法判定) */
    int32_t g_datatbl_643c;  /* DAT_/PTR_0044643c (标量) */
    int32_t g_datatbl_6458;  /* DAT_/PTR_00446458 (标量) */
    int32_t g_datatbl_645c;  /* DAT_/PTR_0044645c (标量) */
    int32_t g_datatbl_6460;  /* DAT_/PTR_00446460 (标量) */
    int32_t g_datatbl_6470[20];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_datatbl_64c0[20];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_datatbl_6510[48];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_datatbl_65d0[47];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_datatbl_668c[49];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_datatbl_6750[47];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_datatbl_680c;  /* DAT_/PTR_0044680c (标量) */
    int32_t g_datatbl_6818;  /* DAT_/PTR_00446818 (标量) */
    int32_t g_datatbl_6824;  /* DAT_/PTR_00446824 (标量) */
    int32_t g_datatbl_6830;  /* DAT_/PTR_00446830 (标量) */
    int32_t g_datatbl_683c;  /* DAT_/PTR_0044683c (标量) */
    int32_t g_datatbl_6848;  /* DAT_/PTR_00446848 (标量) */
    int32_t g_datatbl_6854;  /* DAT_/PTR_00446854 (标量) */
    int32_t g_datatbl_6860;  /* DAT_/PTR_00446860 (标量) */
    int32_t g_datatbl_686c;  /* DAT_/PTR_0044686c (标量) */
    int32_t g_datatbl_6878;  /* DAT_/PTR_00446878 (标量) */
    int32_t g_datatbl_6884;  /* DAT_/PTR_00446884 (标量) */
    int32_t g_datatbl_6890;  /* DAT_/PTR_00446890 (标量) */
    int32_t g_datatbl_689c;  /* DAT_/PTR_0044689c (标量) */
    int32_t g_datatbl_68a8;  /* DAT_/PTR_004468a8 (标量) */
    int32_t g_datatbl_68b4;  /* DAT_/PTR_004468b4 (标量) */
    int32_t g_datatbl_68c0;  /* DAT_/PTR_004468c0 (标量) */
    int32_t g_datatbl_68cc;  /* DAT_/PTR_004468cc (标量) */
    int32_t g_datatbl_68d8;  /* DAT_/PTR_004468d8 (标量) */
    int32_t g_datatbl_68e4;  /* DAT_/PTR_004468e4 (标量) */
    int32_t g_datatbl_68f0;  /* DAT_/PTR_004468f0 (标量) */
    int32_t g_datatbl_68fc;  /* DAT_/PTR_004468fc (标量) */
    int32_t g_datatbl_6908;  /* DAT_/PTR_00446908 (标量) */
    int32_t g_datatbl_6914;  /* DAT_/PTR_00446914 (标量) */
    int32_t g_datatbl_6920;  /* DAT_/PTR_00446920 (标量) */
    int32_t g_datatbl_692c;  /* DAT_/PTR_0044692c (标量) */
    int32_t g_datatbl_6934;  /* DAT_/PTR_00446934 (标量) */
    int32_t g_datatbl_693c;  /* DAT_/PTR_0044693c (标量) */
    int32_t g_datatbl_6948;  /* DAT_/PTR_00446948 (标量) */
    int32_t g_datatbl_6954;  /* DAT_/PTR_00446954 (标量) */
    int32_t g_datatbl_6960;  /* DAT_/PTR_00446960 (标量) */
    int32_t g_datatbl_696c;  /* DAT_/PTR_0044696c (标量) */
    int32_t g_datatbl_6978;  /* DAT_/PTR_00446978 (标量) */
    int32_t g_datatbl_6984;  /* DAT_/PTR_00446984 (标量) */
    int32_t g_datatbl_6990;  /* DAT_/PTR_00446990 (标量) */
    int32_t g_datatbl_699c;  /* DAT_/PTR_0044699c (标量) */
    int32_t g_datatbl_69a8;  /* DAT_/PTR_004469a8 (标量) */
    int32_t g_datatbl_69b4;  /* DAT_/PTR_004469b4 (标量) */
    int32_t g_datatbl_69c0;  /* DAT_/PTR_004469c0 (标量) */
    int32_t g_datatbl_69cc;  /* DAT_/PTR_004469cc (标量) */
    int32_t g_datatbl_69dc;  /* DAT_/PTR_004469dc (标量) */
    int32_t g_datatbl_69e4;  /* DAT_/PTR_004469e4 (标量) */
    int32_t g_datatbl_69ec;  /* DAT_/PTR_004469ec (标量) */
    int32_t g_datatbl_6a00;  /* DAT_/PTR_00446a00 (标量) */
    int32_t g_datatbl_6a08;  /* DAT_/PTR_00446a08 (标量) */
    int32_t g_datatbl_6a18;  /* DAT_/PTR_00446a18 (标量) */
    int32_t g_datatbl_6a88;  /* DAT_/PTR_00446a88 (标量) */
    int32_t g_datatbl_6a90;  /* DAT_/PTR_00446a90 (标量) */
    int32_t g_datatbl_6ab0;  /* DAT_/PTR_00446ab0 (标量) */
    int32_t g_datatbl_6aec;  /* DAT_/PTR_00446aec (标量) */
    int32_t g_datatbl_6af4;  /* DAT_/PTR_00446af4 (标量) */
    int32_t g_datatbl_6bb0;  /* DAT_/PTR_00446bb0 (标量) */
    int32_t g_datatbl_6bbc;  /* DAT_/PTR_00446bbc (标量) */
    int32_t g_datatbl_6bc8;  /* DAT_/PTR_00446bc8 (标量) */
    int32_t g_datatbl_6be4;  /* DAT_/PTR_00446be4 (标量) */
    int32_t g_datatbl_6bec;  /* DAT_/PTR_00446bec (标量) */
    int32_t g_datatbl_6bf4;  /* DAT_/PTR_00446bf4 (标量) */
    int32_t g_datatbl_6bfc;  /* DAT_/PTR_00446bfc (标量) */
    int32_t g_datatbl_6c04;  /* DAT_/PTR_00446c04 (标量) */
    int32_t g_datatbl_6c10;  /* DAT_/PTR_00446c10 (标量) */
    int32_t g_datatbl_6c18;  /* DAT_/PTR_00446c18 (标量) */
    int32_t g_datatbl_6c20;  /* DAT_/PTR_00446c20 (标量) */
    int32_t g_datatbl_6c28;  /* DAT_/PTR_00446c28 (标量) */
    int32_t g_datatbl_6c2c;  /* DAT_/PTR_00446c2c (标量) */
    int32_t g_datatbl_6c34;  /* DAT_/PTR_00446c34 (标量) */
    int32_t g_datatbl_6c44;  /* DAT_/PTR_00446c44 (标量) */
    int32_t g_datatbl_6c60;  /* DAT_/PTR_00446c60 (标量) */
    int32_t g_datatbl_6c68;  /* DAT_/PTR_00446c68 (标量) */
    int32_t g_datatbl_6c70;  /* DAT_/PTR_00446c70 (标量) */
    int32_t g_datatbl_6c78;  /* DAT_/PTR_00446c78 (标量) */
    int32_t g_datatbl_6c84;  /* DAT_/PTR_00446c84 (标量) */
    int32_t g_datatbl_6c90;  /* DAT_/PTR_00446c90 (标量) */
    int32_t g_datatbl_6c9c;  /* DAT_/PTR_00446c9c (标量) */
    int32_t g_datatbl_6ca4;  /* DAT_/PTR_00446ca4 (标量) */
    int32_t g_datatbl_6ca8;  /* DAT_/PTR_00446ca8 (标量) */
    int32_t g_datatbl_6cac;  /* DAT_/PTR_00446cac (标量) */
    int32_t g_datatbl_6cbc;  /* DAT_/PTR_00446cbc (标量) */
    int32_t g_datatbl_6cc8;  /* DAT_/PTR_00446cc8 (标量) */
    int32_t g_datatbl_6ccc;  /* DAT_/PTR_00446ccc (标量) */
    int32_t g_datatbl_6cd8;  /* DAT_/PTR_00446cd8 (标量) */
    int32_t g_datatbl_6ce8;  /* DAT_/PTR_00446ce8 (标量) */
    int32_t g_datatbl_6cf4;  /* DAT_/PTR_00446cf4 (标量) */
    int32_t g_datatbl_6d00;  /* DAT_/PTR_00446d00 (标量) */
    int32_t g_datatbl_6d0c;  /* DAT_/PTR_00446d0c (标量) */
    int32_t g_datatbl_6d18;  /* DAT_/PTR_00446d18 (标量) */
    int32_t g_datatbl_6d24;  /* DAT_/PTR_00446d24 (标量) */
    int32_t g_datatbl_6d2c;  /* DAT_/PTR_00446d2c (标量) */
    int32_t g_datatbl_6d30;  /* DAT_/PTR_00446d30 (标量) */
    int32_t g_datatbl_6d34;  /* DAT_/PTR_00446d34 (标量) */
    int32_t g_datatbl_6d40;  /* DAT_/PTR_00446d40 (标量) */
    int32_t g_datatbl_6d48;  /* DAT_/PTR_00446d48 (标量) */
    int32_t g_datatbl_6d54;  /* DAT_/PTR_00446d54 (标量) */
    int32_t g_datatbl_6d5c;  /* DAT_/PTR_00446d5c (标量) */
    int32_t g_datatbl_6d64;  /* DAT_/PTR_00446d64 (标量) */
    int32_t g_datatbl_6d70;  /* DAT_/PTR_00446d70 (标量) */
    int32_t g_datatbl_6d74;  /* DAT_/PTR_00446d74 (标量) */
    int32_t g_datatbl_6d7c;  /* DAT_/PTR_00446d7c (标量) */
    int32_t g_datatbl_6d88;  /* DAT_/PTR_00446d88 (标量) */
    int32_t g_datatbl_6d94;  /* DAT_/PTR_00446d94 (标量) */
    int32_t g_datatbl_6da0;  /* DAT_/PTR_00446da0 (标量) */
    int32_t g_datatbl_6dac;  /* DAT_/PTR_00446dac (标量) */
    int32_t g_datatbl_6db8;  /* DAT_/PTR_00446db8 (标量) */
    int32_t g_datatbl_6dc4;  /* DAT_/PTR_00446dc4 (标量) */
    int32_t g_datatbl_6dcc;  /* DAT_/PTR_00446dcc (标量) */
    int32_t g_datatbl_6dd4;  /* DAT_/PTR_00446dd4 (标量) */
    int32_t g_datatbl_6ddc;  /* DAT_/PTR_00446ddc (标量) */
    int32_t g_datatbl_6df0;  /* DAT_/PTR_00446df0 (标量) */
    int32_t g_datatbl_6df8;  /* DAT_/PTR_00446df8 (标量) */
    int32_t g_datatbl_6e00;  /* DAT_/PTR_00446e00 (标量) */
    int32_t g_datatbl_6e08;  /* DAT_/PTR_00446e08 (标量) */
    int32_t g_datatbl_6e10;  /* DAT_/PTR_00446e10 (标量) */
    int32_t g_datatbl_6e18;  /* DAT_/PTR_00446e18 (标量) */
    int32_t g_datatbl_6e1c;  /* DAT_/PTR_00446e1c (标量) */
    int32_t g_datatbl_6e20;  /* DAT_/PTR_00446e20 (标量) */
    int32_t g_datatbl_6e28[13];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_datatbl_6e5c;  /* DAT_/PTR_00446e5c (标量) */
    int32_t g_datatbl_6e60;  /* DAT_/PTR_00446e60 (标量) */
    int32_t g_datatbl_6f2c;  /* DAT_/PTR_00446f2c (标量) */
    int32_t g_datatbl_6f34;  /* DAT_/PTR_00446f34 (标量) */
    int32_t g_datatbl_6f3c;  /* DAT_/PTR_00446f3c (标量) */
    int32_t g_datatbl_6f44;  /* DAT_/PTR_00446f44 (标量) */
    int32_t g_datatbl_6f4c;  /* DAT_/PTR_00446f4c (标量) */
    int32_t g_datatbl_6f54;  /* DAT_/PTR_00446f54 (标量) */
    int32_t g_datatbl_6f5c;  /* DAT_/PTR_00446f5c (标量) */
    int32_t g_datatbl_6f64;  /* DAT_/PTR_00446f64 (标量) */
    int32_t g_datatbl_6f6c;  /* DAT_/PTR_00446f6c (标量) */
    int32_t g_datatbl_6f74;  /* DAT_/PTR_00446f74 (标量) */
    int32_t g_datatbl_6f7c;  /* DAT_/PTR_00446f7c (标量) */
    int32_t g_datatbl_6f84;  /* DAT_/PTR_00446f84 (标量) */
    int32_t g_datatbl_6f8c;  /* DAT_/PTR_00446f8c (标量) */
    int32_t g_datatbl_6f94;  /* DAT_/PTR_00446f94 (标量) */
    int32_t g_datatbl_6f98;  /* DAT_/PTR_00446f98 (标量) */
    int32_t g_datatbl_6fa0;  /* DAT_/PTR_00446fa0 (标量) */
    int32_t g_datatbl_6fa4;  /* DAT_/PTR_00446fa4 (标量) */
    int32_t g_datatbl_6fac;  /* DAT_/PTR_00446fac (标量) */
    int32_t g_datatbl_6fb4;  /* DAT_/PTR_00446fb4 (标量) */
    int32_t g_datatbl_6fbc;  /* DAT_/PTR_00446fbc (标量) */
    int32_t g_datatbl_6fc4;  /* DAT_/PTR_00446fc4 (标量) */
    int32_t g_datatbl_6fc8;  /* DAT_/PTR_00446fc8 (标量) */
    int32_t g_datatbl_6fcc;  /* DAT_/PTR_00446fcc (标量) */
    int32_t g_datatbl_6fd0;  /* DAT_/PTR_00446fd0 (标量) */
    int32_t g_datatbl_6fd4;  /* DAT_/PTR_00446fd4 (标量) */
    int32_t g_datatbl_6fd8;  /* DAT_/PTR_00446fd8 (标量) */
    int32_t g_datatbl_6fdc;  /* DAT_/PTR_00446fdc (标量) */
    int32_t g_datatbl_6fe4;  /* DAT_/PTR_00446fe4 (标量) */
    int32_t g_datatbl_6fe8;  /* DAT_/PTR_00446fe8 (标量) */
    int32_t g_datatbl_6ff0;  /* DAT_/PTR_00446ff0 (标量) */
    int32_t g_datatbl_6ffc;  /* DAT_/PTR_00446ffc (标量) */
    int32_t g_datatbl_7008;  /* DAT_/PTR_00447008 (标量) */
    int32_t g_datatbl_7014;  /* DAT_/PTR_00447014 (标量) */
    int32_t g_datatbl_7020;  /* DAT_/PTR_00447020 (标量) */
    int32_t g_datatbl_702c;  /* DAT_/PTR_0044702c (标量) */
    int32_t g_datatbl_7038;  /* DAT_/PTR_00447038 (标量) */
    int32_t g_datatbl_7044;  /* DAT_/PTR_00447044 (标量) */
    int32_t g_datatbl_704c;  /* DAT_/PTR_0044704c (标量) */
    int32_t g_datatbl_7054;  /* DAT_/PTR_00447054 (标量) */
    int32_t g_datatbl_705c;  /* DAT_/PTR_0044705c (标量) */
    int32_t g_datatbl_7064;  /* DAT_/PTR_00447064 (标量) */
    int32_t g_datatbl_706c;  /* DAT_/PTR_0044706c (标量) */
    int32_t g_datatbl_7070;  /* DAT_/PTR_00447070 (标量) */
    int32_t g_datatbl_707c;  /* DAT_/PTR_0044707c (标量) */
    int32_t g_datatbl_7088;  /* DAT_/PTR_00447088 (标量) */
    int32_t g_datatbl_7094;  /* DAT_/PTR_00447094 (标量) */
    int32_t g_datatbl_70a0;  /* DAT_/PTR_004470a0 (标量) */
    int32_t g_datatbl_70ac;  /* DAT_/PTR_004470ac (标量) */
    int32_t g_datatbl_70b4;  /* DAT_/PTR_004470b4 (标量) */
    int32_t g_datatbl_70bc;  /* DAT_/PTR_004470bc (标量) */
    int32_t g_datatbl_70c4;  /* DAT_/PTR_004470c4 (标量) */
    int32_t g_datatbl_70cc;  /* DAT_/PTR_004470cc (标量) */
    int32_t g_datatbl_70d4;  /* DAT_/PTR_004470d4 (标量) */
    int32_t g_datatbl_70d8;  /* DAT_/PTR_004470d8 (标量) */
    int32_t g_datatbl_70e4;  /* DAT_/PTR_004470e4 (标量) */
    int32_t g_datatbl_70f0;  /* DAT_/PTR_004470f0 (标量) */
    int32_t g_datatbl_70fc;  /* DAT_/PTR_004470fc (标量) */
    int32_t g_datatbl_710c;  /* DAT_/PTR_0044710c (标量) */
    int32_t g_datatbl_711c;  /* DAT_/PTR_0044711c (标量) */
    int32_t g_datatbl_7128[30];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_datatbl_71a0;  /* DAT_/PTR_004471a0 (标量) */
    int32_t g_str_anm_71b4;  /* DAT_/PTR_004471b4 (标量) */
    int32_t g_datatbl_71bc;  /* DAT_/PTR_004471bc (标量) */
    int32_t g_datatbl_71c0;  /* DAT_/PTR_004471c0 (标量) */
    int32_t g_datatbl_71d0;  /* DAT_/PTR_004471d0 (标量) */
    int32_t g_pow2_32;  /* DAT_/PTR_004471d4 (标量) */
    int32_t g_datatbl_71d8;  /* DAT_/PTR_004471d8 (标量) */
    int32_t g_datatbl_71dc;  /* DAT_/PTR_004471dc (标量) */
    int32_t g_datatbl_71e0;  /* DAT_/PTR_004471e0 (标量) */
    int32_t g_datatbl_71e4;  /* DAT_/PTR_004471e4 (标量) */
    int32_t g_datatbl_7200;  /* DAT_/PTR_00447200 (标量) */
    int32_t g_datatbl_7208;  /* DAT_/PTR_00447208 (标量) */
    int32_t g_datatbl_720c;  /* DAT_/PTR_0044720c (标量) */
    int32_t g_datatbl_7214;  /* DAT_/PTR_00447214 (标量) */
    int32_t g_datatbl_721c;  /* DAT_/PTR_0044721c (标量) */
    int32_t g_datatbl_7224;  /* DAT_/PTR_00447224 (标量) */
    int32_t g_datatbl_722c;  /* DAT_/PTR_0044722c (标量) */
    int32_t g_datatbl_7234;  /* DAT_/PTR_00447234 (标量) */
    int32_t g_datatbl_723c;  /* DAT_/PTR_0044723c (标量) */
    int32_t g_datatbl_7244;  /* DAT_/PTR_00447244 (标量) */
    int32_t g_datatbl_724c;  /* DAT_/PTR_0044724c (标量) */
    int32_t g_datatbl_7254;  /* DAT_/PTR_00447254 (标量) */
    int32_t g_datatbl_725c;  /* DAT_/PTR_0044725c (标量) */
    int32_t g_datatbl_7264;  /* DAT_/PTR_00447264 (标量) */
    int32_t g_datatbl_726c;  /* DAT_/PTR_0044726c (标量) */
    int32_t g_datatbl_7274;  /* DAT_/PTR_00447274 (标量) */
    int32_t g_skill_level_thresholds[10];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_datatbl_72a8[20];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_datatbl_72f8[1];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_datatbl_72fc[37];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_datatbl_7390[1];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_datatbl_7394[95];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_datatbl_7510;  /* DAT_/PTR_00447510 (标量) */
    int32_t g_datatbl_7524;  /* DAT_/PTR_00447524 (标量) */
    int32_t g_datatbl_7528;  /* DAT_/PTR_00447528 (标量) */
    int32_t g_datatbl_752c;  /* DAT_/PTR_0044752c (标量) */
    int32_t g_datatbl_7538;  /* DAT_/PTR_00447538 (标量) */
    int32_t g_datatbl_753c;  /* DAT_/PTR_0044753c (标量) */
    int32_t g_datatbl_7540;  /* DAT_/PTR_00447540 (标量) */
    int32_t g_datatbl_7544;  /* DAT_/PTR_00447544 (标量) */
    int32_t g_datatbl_7548;  /* DAT_/PTR_00447548 (标量) */
    int32_t g_datatbl_7600;  /* DAT_/PTR_00447600 (标量) */
    int32_t g_datatbl_7614;  /* DAT_/PTR_00447614 (标量) */
    int32_t g_datatbl_7628;  /* DAT_/PTR_00447628 (标量) */
    int32_t g_datatbl_7648;  /* DAT_/PTR_00447648 (标量) */
    int32_t g_datatbl_7668;  /* DAT_/PTR_00447668 (标量) */
    int32_t g_datatbl_7680;  /* DAT_/PTR_00447680 (标量) */
    int32_t g_datatbl_7684;  /* DAT_/PTR_00447684 (标量) */
    int32_t g_datatbl_7688;  /* DAT_/PTR_00447688 (标量) */
    int32_t g_datatbl_7690;  /* DAT_/PTR_00447690 (标量) */
    int32_t g_datatbl_7694;  /* DAT_/PTR_00447694 (标量) */
    int32_t g_datatbl_7698;  /* DAT_/PTR_00447698 (标量) */
    int32_t g_datatbl_769c;  /* DAT_/PTR_0044769c (标量) */
    int32_t g_datatbl_76a0;  /* DAT_/PTR_004476a0 (标量) */
    int32_t g_datatbl_76a4;  /* DAT_/PTR_004476a4 (标量) */
    int32_t g_datatbl_76a8;  /* DAT_/PTR_004476a8 (标量) */
    int32_t g_pow2_1[48];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_pow2_1_447798[78];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_datatbl_78d0;  /* DAT_/PTR_004478d0 (标量) */
    int32_t g_datatbl_78d4;  /* DAT_/PTR_004478d4 (标量) */
    int32_t g_datatbl_78d8;  /* DAT_/PTR_004478d8 (标量) */
    int32_t g_datatbl_78dc;  /* DAT_/PTR_004478dc (标量) */
    int32_t g_str_save_78ec;  /* DAT_/PTR_004478ec (标量) */
    int32_t g_str_dat_78f4;  /* DAT_/PTR_004478f4 (标量) */
    int32_t g_datatbl_78fc;  /* DAT_/PTR_004478fc (标量) */
    int32_t g_datatbl_7900;  /* DAT_/PTR_00447900 (标量) */
    int32_t g_datatbl_7904;  /* DAT_/PTR_00447904 (标量) */
    int32_t g_datatbl_7908;  /* DAT_/PTR_00447908 (标量) */
    int32_t g_datatbl_790c;  /* DAT_/PTR_0044790c (标量) */
    int32_t g_datatbl_7910;  /* DAT_/PTR_00447910 (标量) */
    int32_t g_datatbl_7914;  /* DAT_/PTR_00447914 (标量) */
    int32_t g_datatbl_7918;  /* DAT_/PTR_00447918 (标量) */
    int32_t g_datatbl_791c;  /* DAT_/PTR_0044791c (标量) */
    int32_t g_datatbl_7920;  /* DAT_/PTR_00447920 (标量) */
    int32_t g_datatbl_7924;  /* DAT_/PTR_00447924 (标量) */
    int32_t g_datatbl_792c;  /* DAT_/PTR_0044792c (标量) */
    int32_t g_datatbl_7934;  /* DAT_/PTR_00447934 (标量) */
    int32_t g_datatbl_7948;  /* DAT_/PTR_00447948 (标量) */
    int32_t g_datatbl_7950;  /* DAT_/PTR_00447950 (标量) */
    int32_t g_datatbl_7958;  /* DAT_/PTR_00447958 (标量) */
    int32_t g_datatbl_795c;  /* DAT_/PTR_0044795c (标量) */
    int32_t g_datatbl_7964;  /* DAT_/PTR_00447964 (标量) */
    int32_t g_datatbl_796c;  /* DAT_/PTR_0044796c (标量) */
    int32_t g_datatbl_7978;  /* DAT_/PTR_00447978 (标量) */
    int32_t g_datatbl_7984;  /* DAT_/PTR_00447984 (标量) */
    int32_t g_datatbl_798c;  /* DAT_/PTR_0044798c (标量) */
    int32_t g_datatbl_7994;  /* DAT_/PTR_00447994 (标量) */
    int32_t g_datatbl_7998;  /* DAT_/PTR_00447998 (标量) */
    int32_t g_datatbl_799c;  /* DAT_/PTR_0044799c (标量) */
    int32_t g_datatbl_79a4;  /* DAT_/PTR_004479a4 (标量) */
    int32_t g_datatbl_79ac;  /* DAT_/PTR_004479ac (标量) */
    int32_t g_datatbl_79b4;  /* DAT_/PTR_004479b4 (标量) */
    int32_t g_datatbl_79c4;  /* DAT_/PTR_004479c4 (标量) */
    int32_t g_datatbl_79e8;  /* DAT_/PTR_004479e8 (标量) */
    int32_t g_datatbl_79ec;  /* DAT_/PTR_004479ec (标量) */
    int32_t g_datatbl_79f0;  /* DAT_/PTR_004479f0 (标量) */
    int32_t g_str_mpg_79f4;  /* DAT_/PTR_004479f4 (标量) */
    int32_t g_datatbl_79fc;  /* DAT_/PTR_004479fc (标量) */
    int32_t g_datatbl_7a0c;  /* DAT_/PTR_00447a0c (标量) */
    int32_t g_datatbl_7a1c;  /* DAT_/PTR_00447a1c (标量) */
    int32_t g_datatbl_7a24;  /* DAT_/PTR_00447a24 (标量) */
    int32_t g_datatbl_7a30;  /* DAT_/PTR_00447a30 (标量) */
    int32_t g_datatbl_7a3c;  /* DAT_/PTR_00447a3c (标量) */
    int32_t g_datatbl_7a48;  /* DAT_/PTR_00447a48 (标量) */
    int32_t g_datatbl_7a54;  /* DAT_/PTR_00447a54 (标量) */
    int32_t g_datatbl_7a6c;  /* DAT_/PTR_00447a6c (标量) */
    int32_t g_datatbl_7a7c;  /* DAT_/PTR_00447a7c (标量) */
    int32_t g_datatbl_7a88;  /* DAT_/PTR_00447a88 (标量) */
    int32_t g_datatbl_7a90;  /* DAT_/PTR_00447a90 (标量) */
    int32_t g_datatbl_7a9c;  /* DAT_/PTR_00447a9c (标量) */
    int32_t g_datatbl_7ac4;  /* DAT_/PTR_00447ac4 (标量) */
    int32_t g_datatbl_7ac8;  /* DAT_/PTR_00447ac8 (标量) */
    int32_t g_datatbl_7acc;  /* DAT_/PTR_00447acc (标量) */
    int32_t g_datatbl_7af8;  /* DAT_/PTR_00447af8 (标量) */
    int32_t g_datatbl_7b24;  /* DAT_/PTR_00447b24 (标量) */
    int32_t g_datatbl_7b64;  /* DAT_/PTR_00447b64 (标量) */
    int32_t g_pow2_1_447b78[4];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_datatbl_7b88[4];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_datatbl_7b98[53];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_datatbl_7d00;  /* DAT_/PTR_00447d00 (标量) */
    int32_t g_datatbl_7d08;  /* DAT_/PTR_00447d08 (标量) */
    int32_t g_datatbl_7d24;  /* DAT_/PTR_00447d24 (标量) */
    int32_t g_datatbl_7d38;  /* DAT_/PTR_00447d38 (标量) */
    int32_t g_datatbl_7d40;  /* DAT_/PTR_00447d40 (标量) */
    int32_t g_datatbl_7d50;  /* DAT_/PTR_00447d50 (标量) */
    int32_t g_datatbl_7d98;  /* DAT_/PTR_00447d98 (标量) */
    int32_t g_datatbl_7da0;  /* DAT_/PTR_00447da0 (标量) */
    int32_t g_datatbl_7da8;  /* DAT_/PTR_00447da8 (标量) */
    int32_t g_datatbl_7ea0;  /* DAT_/PTR_00447ea0 (标量) */
    int32_t g_datatbl_7ea8;  /* DAT_/PTR_00447ea8 (标量) */
    int32_t g_datatbl_7eb0;  /* DAT_/PTR_00447eb0 (标量) */
    int32_t g_datatbl_7eb8;  /* DAT_/PTR_00447eb8 (标量) */
    int32_t g_datatbl_7ec0;  /* DAT_/PTR_00447ec0 (标量) */
    int32_t g_datatbl_7ed4;  /* DAT_/PTR_00447ed4 (标量) */
    int32_t g_datatbl_7edc;  /* DAT_/PTR_00447edc (标量) */
    int32_t g_datatbl_7ee4;  /* DAT_/PTR_00447ee4 (标量) */
    int32_t g_datatbl_7eec;  /* DAT_/PTR_00447eec (标量) */
    int32_t g_datatbl_7f00;  /* DAT_/PTR_00447f00 (标量) */
    int32_t g_datatbl_7f08;  /* DAT_/PTR_00447f08 (标量) */
    int32_t g_datatbl_7f10;  /* DAT_/PTR_00447f10 (标量) */
    int32_t g_datatbl_7f18;  /* DAT_/PTR_00447f18 (标量) */
    int32_t g_datatbl_7f20;  /* DAT_/PTR_00447f20 (标量) */
    int32_t g_datatbl_7f4c;  /* DAT_/PTR_00447f4c (标量) */
    int32_t g_datatbl_7f54;  /* DAT_/PTR_00447f54 (标量) */
    int32_t g_datatbl_7f5c;  /* DAT_/PTR_00447f5c (标量) */
    int32_t g_datatbl_7f64;  /* DAT_/PTR_00447f64 (标量) */
    int32_t g_datatbl_7f6c;  /* DAT_/PTR_00447f6c (标量) */
    int32_t g_datatbl_7f74;  /* DAT_/PTR_00447f74 (标量) */
    int32_t g_datatbl_7f94;  /* DAT_/PTR_00447f94 (标量) */
    int32_t g_datatbl_7f9c;  /* DAT_/PTR_00447f9c (标量) */
    int32_t g_datatbl_7fa4;  /* DAT_/PTR_00447fa4 (标量) */
    int32_t g_datatbl_7fac;  /* DAT_/PTR_00447fac (标量) */
    int32_t g_datatbl_7fb4;  /* DAT_/PTR_00447fb4 (标量) */
    int32_t g_datatbl_7fbc;  /* DAT_/PTR_00447fbc (标量) */
    int32_t g_datatbl_7fc8;  /* DAT_/PTR_00447fc8 (标量) */
    int32_t g_datatbl_7fd4;  /* DAT_/PTR_00447fd4 (标量) */
    int32_t g_datatbl_7fec;  /* DAT_/PTR_00447fec (标量) */
    int32_t g_datatbl_8000;  /* DAT_/PTR_00448000 (标量) */
    int32_t g_datatbl_8004;  /* DAT_/PTR_00448004 (标量) */
    int32_t g_datatbl_8008;  /* DAT_/PTR_00448008 (标量) */
    int32_t g_datatbl_800c;  /* DAT_/PTR_0044800c (标量) */
    int32_t g_datatbl_8010;  /* DAT_/PTR_00448010 (标量) */
    int32_t g_str_lv_8040;  /* DAT_/PTR_00448040 (标量) */
    int32_t g_datatbl_80c0;  /* DAT_/PTR_004480c0 (标量) */
    int32_t g_datatbl_80c8;  /* DAT_/PTR_004480c8 (标量) */
    int32_t g_datatbl_80d0;  /* DAT_/PTR_004480d0 (标量) */
    int32_t g_datatbl_80d8;  /* DAT_/PTR_004480d8 (标量) */
    int32_t g_datatbl_80e0;  /* DAT_/PTR_004480e0 (标量) */
    int32_t g_datatbl_80e4;  /* DAT_/PTR_004480e4 (标量) */
    int32_t g_datatbl_80e8;  /* DAT_/PTR_004480e8 (标量) */
    int32_t g_datatbl_80ec;  /* DAT_/PTR_004480ec (标量) */
    int32_t g_datatbl_80f0;  /* DAT_/PTR_004480f0 (标量) */
    int32_t g_datatbl_80f8;  /* DAT_/PTR_004480f8 (标量) */
    int32_t g_datatbl_8100;  /* DAT_/PTR_00448100 (标量) */
    int32_t g_datatbl_8108;  /* DAT_/PTR_00448108 (标量) */
    int32_t g_datatbl_8110;  /* DAT_/PTR_00448110 (标量) */
    int32_t g_datatbl_8118;  /* DAT_/PTR_00448118 (标量) */
    int32_t g_datatbl_8120;  /* DAT_/PTR_00448120 (标量) */
    int32_t g_datatbl_8128;  /* DAT_/PTR_00448128 (标量) */
    int32_t g_datatbl_8130;  /* DAT_/PTR_00448130 (标量) */
    int32_t g_datatbl_8138;  /* DAT_/PTR_00448138 (标量) */
    int32_t g_datatbl_8140;  /* DAT_/PTR_00448140 (标量) */
    int32_t g_datatbl_8148;  /* DAT_/PTR_00448148 (标量) */
    int32_t g_datatbl_8150;  /* DAT_/PTR_00448150 (标量) */
    int32_t g_datatbl_815c;  /* DAT_/PTR_0044815c (标量) */
    int32_t g_datatbl_8168;  /* DAT_/PTR_00448168 (标量) */
    int32_t g_datatbl_8178;  /* DAT_/PTR_00448178 (标量) */
    int32_t g_datatbl_8188;  /* DAT_/PTR_00448188 (标量) */
    int32_t g_datatbl_8198;  /* DAT_/PTR_00448198 (标量) */
    int32_t g_datatbl_81a0;  /* DAT_/PTR_004481a0 (标量) */
    int32_t g_datatbl_81a8;  /* DAT_/PTR_004481a8 (标量) */
    int32_t g_datatbl_81b0;  /* DAT_/PTR_004481b0 (标量) */
    int32_t g_datatbl_81b8;  /* DAT_/PTR_004481b8 (标量) */
    int32_t g_datatbl_824c;  /* DAT_/PTR_0044824c (标量) */
    int32_t g_datatbl_8254;  /* DAT_/PTR_00448254 (标量) */
    int32_t g_datatbl_825c;  /* DAT_/PTR_0044825c (标量) */
    int32_t g_datatbl_8264;  /* DAT_/PTR_00448264 (标量) */
    int32_t g_datatbl_826c;  /* DAT_/PTR_0044826c (标量) */
    int32_t g_datatbl_8274;  /* DAT_/PTR_00448274 (标量) */
    int32_t g_datatbl_8278;  /* DAT_/PTR_00448278 (标量) */
    int32_t g_datatbl_827c;  /* DAT_/PTR_0044827c (标量) */
    int32_t g_datatbl_8280;  /* DAT_/PTR_00448280 (标量) */
    int32_t g_datatbl_8284;  /* DAT_/PTR_00448284 (标量) */
    int32_t g_datatbl_828c;  /* DAT_/PTR_0044828c (标量) */
    int32_t g_datatbl_8294;  /* DAT_/PTR_00448294 (标量) */
    int32_t g_datatbl_829c;  /* DAT_/PTR_0044829c (标量) */
    int32_t g_datatbl_82a4;  /* DAT_/PTR_004482a4 (标量) */
    int32_t g_datatbl_82ac;  /* DAT_/PTR_004482ac (标量) */
    int32_t g_datatbl_82b4;  /* DAT_/PTR_004482b4 (标量) */
    int32_t g_datatbl_82bc;  /* DAT_/PTR_004482bc (标量) */
    int32_t g_datatbl_82c4;  /* DAT_/PTR_004482c4 (标量) */
    int32_t g_datatbl_82cc;  /* DAT_/PTR_004482cc (标量) */
    int32_t g_datatbl_82d4;  /* DAT_/PTR_004482d4 (标量) */
    int32_t g_datatbl_82dc;  /* DAT_/PTR_004482dc (标量) */
    int32_t g_datatbl_82e4;  /* DAT_/PTR_004482e4 (标量) */
    int32_t g_datatbl_82ec;  /* DAT_/PTR_004482ec (标量) */
    int32_t g_datatbl_82f4;  /* DAT_/PTR_004482f4 (标量) */
    int32_t g_datatbl_82fc;  /* DAT_/PTR_004482fc (标量) */
    int32_t g_datatbl_8304;  /* DAT_/PTR_00448304 (标量) */
    int32_t g_datatbl_830c;  /* DAT_/PTR_0044830c (标量) */
    int32_t g_datatbl_8314;  /* DAT_/PTR_00448314 (标量) */
    int32_t g_datatbl_831c;  /* DAT_/PTR_0044831c (标量) */
    int32_t g_datatbl_8328;  /* DAT_/PTR_00448328 (标量) */
    int32_t g_datatbl_8334;  /* DAT_/PTR_00448334 (标量) */
    int32_t g_datatbl_8344;  /* DAT_/PTR_00448344 (标量) */
    int32_t g_datatbl_8354;  /* DAT_/PTR_00448354 (标量) */
    int32_t g_datatbl_835c;  /* DAT_/PTR_0044835c (标量) */
    int32_t g_datatbl_8368;  /* DAT_/PTR_00448368 (标量) */
    int32_t g_datatbl_8374;  /* DAT_/PTR_00448374 (标量) */
    int32_t g_datatbl_837c;  /* DAT_/PTR_0044837c (标量) */
    int32_t g_datatbl_8458;  /* DAT_/PTR_00448458 (标量) */
    int32_t g_datatbl_8460;  /* DAT_/PTR_00448460 (标量) */
    int32_t g_datatbl_8468;  /* DAT_/PTR_00448468 (标量) */
    int32_t g_datatbl_8470;  /* DAT_/PTR_00448470 (标量) */
    int32_t g_datatbl_8478;  /* DAT_/PTR_00448478 (标量) */
    int32_t g_datatbl_8480;  /* DAT_/PTR_00448480 (标量) */
    int32_t g_datatbl_8488;  /* DAT_/PTR_00448488 (标量) */
    int32_t g_datatbl_8490;  /* DAT_/PTR_00448490 (标量) */
    int32_t g_datatbl_8498;  /* DAT_/PTR_00448498 (标量) */
    int32_t g_datatbl_84a0;  /* DAT_/PTR_004484a0 (标量) */
    int32_t g_datatbl_84ac;  /* DAT_/PTR_004484ac (标量) */
    int32_t g_datatbl_84b8;  /* DAT_/PTR_004484b8 (标量) */
    int32_t g_datatbl_84c0;  /* DAT_/PTR_004484c0 (标量) */
    int32_t g_datatbl_84c8;  /* DAT_/PTR_004484c8 (标量) */
    int32_t g_datatbl_84d0;  /* DAT_/PTR_004484d0 (标量) */
    int32_t g_datatbl_84d8;  /* DAT_/PTR_004484d8 (标量) */
    int32_t g_datatbl_84e0;  /* DAT_/PTR_004484e0 (标量) */
    int32_t g_datatbl_84e8;  /* DAT_/PTR_004484e8 (标量) */
    int32_t g_datatbl_84f0;  /* DAT_/PTR_004484f0 (标量) */
    int32_t g_datatbl_84f8;  /* DAT_/PTR_004484f8 (标量) */
    int32_t g_datatbl_8500;  /* DAT_/PTR_00448500 (标量) */
    int32_t g_datatbl_8508;  /* DAT_/PTR_00448508 (标量) */
    int32_t g_datatbl_8510;  /* DAT_/PTR_00448510 (标量) */
    int32_t g_datatbl_8518;  /* DAT_/PTR_00448518 (标量) */
    int32_t g_datatbl_8520;  /* DAT_/PTR_00448520 (标量) */
    int32_t g_datatbl_8528;  /* DAT_/PTR_00448528 (标量) */
    int32_t g_str_lv_8530;  /* DAT_/PTR_00448530 (标量) */
    int32_t g_datatbl_8598;  /* DAT_/PTR_00448598 (标量) */
    int32_t g_datatbl_85a0;  /* DAT_/PTR_004485a0 (标量) */
    int32_t g_datatbl_85a4;  /* DAT_/PTR_004485a4 (标量) */
    int32_t g_datatbl_85a8;  /* DAT_/PTR_004485a8 (标量) */
    int32_t g_datatbl_85ac;  /* DAT_/PTR_004485ac (标量) */
    int32_t g_datatbl_85bc;  /* DAT_/PTR_004485bc (标量) */
    int32_t g_datatbl_85c4;  /* DAT_/PTR_004485c4 (标量) */
    int32_t g_datatbl_85cc;  /* DAT_/PTR_004485cc (标量) */
    int32_t g_datatbl_85d4;  /* DAT_/PTR_004485d4 (标量) */
    int32_t g_datatbl_85dc;  /* DAT_/PTR_004485dc (标量) */
    int32_t g_datatbl_85e4;  /* DAT_/PTR_004485e4 (标量) */
    int32_t g_datatbl_85ec;  /* DAT_/PTR_004485ec (标量) */
    int32_t g_datatbl_85f4;  /* DAT_/PTR_004485f4 (标量) */
    int32_t g_datatbl_85fc;  /* DAT_/PTR_004485fc (标量) */
    int32_t g_datatbl_8604;  /* DAT_/PTR_00448604 (标量) */
    int32_t g_datatbl_860c;  /* DAT_/PTR_0044860c (标量) */
    int32_t g_datatbl_8620;  /* DAT_/PTR_00448620 (标量) */
    int32_t g_datatbl_862c;  /* DAT_/PTR_0044862c (标量) */
    int32_t g_datatbl_8638;  /* DAT_/PTR_00448638 (标量) */
    int32_t g_datatbl_8644;  /* DAT_/PTR_00448644 (标量) */
    int32_t g_datatbl_8650;  /* DAT_/PTR_00448650 (标量) */
    int32_t g_datatbl_865c;  /* DAT_/PTR_0044865c (标量) */
    int32_t g_datatbl_8668;  /* DAT_/PTR_00448668 (标量) */
    int32_t g_str_lv_8674;  /* DAT_/PTR_00448674 (标量) */
    int32_t g_str_lv_867c;  /* DAT_/PTR_0044867c (标量) */
    int32_t g_datatbl_8684;  /* DAT_/PTR_00448684 (标量) */
    int32_t g_datatbl_868c;  /* DAT_/PTR_0044868c (标量) */
    int32_t g_datatbl_8694;  /* DAT_/PTR_00448694 (标量) */
    int32_t g_datatbl_869c;  /* DAT_/PTR_0044869c (标量) */
    int32_t g_datatbl_86a4;  /* DAT_/PTR_004486a4 (标量) */
    int32_t g_datatbl_86c8;  /* DAT_/PTR_004486c8 (标量) */
    int32_t g_datatbl_86d0;  /* DAT_/PTR_004486d0 (标量) */
    int32_t g_str_lv_86d8;  /* DAT_/PTR_004486d8 (标量) */
    int32_t g_str_lv_86e0;  /* DAT_/PTR_004486e0 (标量) */
    int32_t g_datatbl_86e8;  /* DAT_/PTR_004486e8 (标量) */
    int32_t g_datatbl_86f0;  /* DAT_/PTR_004486f0 (标量) */
    int32_t g_datatbl_86f8;  /* DAT_/PTR_004486f8 (标量) */
    int32_t g_datatbl_8720[20];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_datatbl_8770;  /* DAT_/PTR_00448770 (标量) */
    int32_t g_datatbl_8771[2];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_datatbl_877c;  /* DAT_/PTR_0044877c (标量) */
    int32_t g_datatbl_8780;  /* DAT_/PTR_00448780 (标量) */
    int32_t g_pow2_1_448790;  /* DAT_/PTR_00448790 (标量) */
    int32_t g_pow2_2;  /* DAT_/PTR_004487a4 (标量) */
    int32_t g_datatbl_87a8;  /* DAT_/PTR_004487a8 (标量) */
    int32_t g_datatbl_87ac;  /* DAT_/PTR_004487ac (标量) */
    int32_t g_datatbl_87b8;  /* DAT_/PTR_004487b8 (标量) */
    int32_t* g_datatbl_87c0;  /* DAT_/PTR_004487c0 (指针/缓冲区，按实际用法判定) */
    int32_t g_datatbl_87ca[128];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_pow2_1_4489cc;  /* DAT_/PTR_004489cc (标量) */
    int32_t g_datatbl_89d0;  /* DAT_/PTR_004489d0 (标量) */
    int32_t g_datatbl_89d8;  /* DAT_/PTR_004489d8 (标量) */
    int32_t g_datatbl_8a68[2];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_datatbl_8a70;  /* DAT_/PTR_00448a70 (标量) */
    int32_t g_datatbl_8a74[1];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_datatbl_8a78[1];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_datatbl_8a7c[1];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_datatbl_8a80[56];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_pow2_4096;  /* DAT_/PTR_00448b60 (标量) */
    int32_t g_datatbl_8b68;  /* DAT_/PTR_00448b68 (标量) */
    int32_t g_datatbl_8b6c;  /* DAT_/PTR_00448b6c (标量) */
    int32_t g_datatbl_8b70;  /* DAT_/PTR_00448b70 (标量) */
    int32_t g_pow2_2_448b78;  /* DAT_/PTR_00448b78 (标量) */
    int32_t g_datatbl_8c08;  /* DAT_/PTR_00448c08 (标量) */
    int32_t g_datatbl_8c28;  /* DAT_/PTR_00448c28 (标量) */
    int32_t g_datatbl_8c48;  /* DAT_/PTR_00448c48 (标量) */
    int32_t g_pow2_1_448e88;  /* DAT_/PTR_00448e88 (标量) */
    int32_t g_datatbl_8e8c[89];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_datatbl_8ff0;  /* DAT_/PTR_00448ff0 (标量) */
    int32_t g_datatbl_8ff4;  /* DAT_/PTR_00448ff4 (标量) */
    int32_t g_datatbl_9000;  /* DAT_/PTR_00449000 (标量) */
    int32_t g_datatbl_b010;  /* DAT_/PTR_0044b010 (标量) */
    int32_t g_datatbl_b014;  /* DAT_/PTR_0044b014 (标量) */
    int32_t g_datatbl_b018;  /* DAT_/PTR_0044b018 (标量) */
    int32_t g_datatbl_b01c;  /* DAT_/PTR_0044b01c (标量) */
    int32_t g_datatbl_b020;  /* DAT_/PTR_0044b020 (标量) */
    int32_t g_datatbl_b024;  /* DAT_/PTR_0044b024 (标量) */
    int32_t g_datatbl_b030;  /* DAT_/PTR_0044b030 (标量) */
    int32_t g_init_count;  /* DAT_/PTR_0044b230 (标量) */
    int32_t g_rtstate_b234;  /* DAT_/PTR_0044b234 (标量) */
    int32_t g_rtstate_b238;  /* DAT_/PTR_0044b238 (标量) */
    int32_t g_rtstate_b23c;  /* DAT_/PTR_0044b23c (标量) */
    int32_t g_rtstate_b240;  /* DAT_/PTR_0044b240 (标量) */
    int32_t g_rtstate_b244;  /* DAT_/PTR_0044b244 (标量) */
    int32_t g_rtstate_b248;  /* DAT_/PTR_0044b248 (标量) */
    int32_t g_rtstate_b250[2560];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_rtstate_bc50;  /* DAT_/PTR_0044bc50 (标量) */
    int32_t g_rtstate_fc50;  /* DAT_/PTR_0044fc50 (标量) */
    int32_t g_rtstate_fc54;  /* DAT_/PTR_0044fc54 (标量) */
    int32_t g_rtstate_fc58;  /* DAT_/PTR_0044fc58 (标量) */
    int32_t g_rtstate_fc78;  /* DAT_/PTR_0044fc78 (标量) */
    int32_t g_rtstate_fc88;  /* DAT_/PTR_0044fc88 (标量) */
    int32_t g_rtstate_fc90;  /* DAT_/PTR_0044fc90 (标量) */
    int32_t g_rtstate_fda0;  /* DAT_/PTR_0044fda0 (标量) */
    int32_t g_rtstate_fda4;  /* DAT_/PTR_0044fda4 (标量) */
    int32_t g_rtstate_fda8;  /* DAT_/PTR_0044fda8 (标量) */
    int32_t g_rtstate_fdac;  /* DAT_/PTR_0044fdac (标量) */
    int32_t g_rtstate_fdb0;  /* DAT_/PTR_0044fdb0 (标量) */
    int32_t g_rtstate_fdb4;  /* DAT_/PTR_0044fdb4 (标量) */
    int32_t g_rtstate_fdb8;  /* DAT_/PTR_0044fdb8 (标量) */
    int32_t g_rtstate_fdbc;  /* DAT_/PTR_0044fdbc (标量) */
    int32_t g_rtstate_fdc0;  /* DAT_/PTR_0044fdc0 (标量) */
    int32_t g_rtstate_fdc4;  /* DAT_/PTR_0044fdc4 (标量) */
    int32_t g_rtstate_fdc8[49];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_mci_device;  /* DAT_/PTR_0044fe8c (标量) */
    int32_t g_rtstate_fe90;  /* DAT_/PTR_0044fe90 (标量) */
    int32_t g_rtstate_fe94;  /* DAT_/PTR_0044fe94 (标量) */
    int32_t g_hwnd_fe98;  /* DAT_/PTR_0044fe98 (标量) */
    int32_t g_rtstate_fe9c;  /* DAT_/PTR_0044fe9c (标量) */
    int32_t g_rtstate_fea0;  /* DAT_/PTR_0044fea0 (标量) */
    ghidra_func_t* g_rtstate_00a8;  /* DAT_/PTR_004500a8 (函数指针，(*gs)(...) 调用) */
    int32_t g_rtstate_00ac;  /* DAT_/PTR_004500ac (标量) */
    int32_t g_rtstate_00b0;  /* DAT_/PTR_004500b0 (标量) */
    int32_t g_rtstate_00b8;  /* DAT_/PTR_004500b8 (标量) */
    int32_t g_rtstate_03b0;  /* DAT_/PTR_004503b0 (标量) */
    int32_t g_rtstate_03b4;  /* DAT_/PTR_004503b4 (标量) */
    int32_t g_rtstate_03b8;  /* DAT_/PTR_004503b8 (标量) */
    int32_t g_rtstate_03bc;  /* DAT_/PTR_004503bc (标量) */
    int32_t g_rtstate_03c0;  /* DAT_/PTR_004503c0 (标量) */
    int32_t g_rtstate_03d0[2816];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_rtstate_2fd0[3456];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_rtstate_65d0[131];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_rtstate_67dc;  /* DAT_/PTR_004567dc (标量) */
    int32_t g_rtstate_67e0[4];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_rtstate_67f0[1];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_rtstate_67f4[1];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_rtstate_67f8[1];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_rtstate_67fc[3];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_rtstate_6808[1];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_rtstate_680c[109];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_rtstate_69c0;  /* DAT_/PTR_004569c0 (标量) */
    int32_t g_rtstate_69c1[28399];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_accum_a;  /* DAT_/PTR_00472580 (标量) */
    int32_t g_dialogue_script_000[1];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_dialogue_script_001[3];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_dialogue_script;  /* DAT_/PTR_00472598 (标量) */
    int32_t g_dialogue_script_006;  /* DAT_/PTR_004725a0 (标量) */
    int32_t g_dialogue_script_007;  /* DAT_/PTR_004725a4 (标量) */
    int32_t g_dialogue_script_008;  /* DAT_/PTR_004725a8 (标量) */
    int32_t g_dialogue_script_009;  /* DAT_/PTR_004725ac (标量) */
    int32_t g_dialogue_script_010;  /* DAT_/PTR_004725b0 (标量) */
    int32_t g_dialogue_script_011;  /* DAT_/PTR_004725b4 (标量) */
    int32_t g_dialogue_script_012;  /* DAT_/PTR_004725b8 (标量) */
    int32_t g_dialogue_script_016;  /* DAT_/PTR_004725c8 (标量) */
    int32_t g_dialogue_script_017;  /* DAT_/PTR_004725cc (标量) */
    int32_t g_dialogue_script_018;  /* DAT_/PTR_004725d0 (标量) */
    int32_t g_dialogue_script_019;  /* DAT_/PTR_004725d4 (标量) */
    int32_t g_dialogue_script_020;  /* DAT_/PTR_004725d8 (标量) */
    int32_t g_dialogue_script_021;  /* DAT_/PTR_004725dc (标量) */
    int32_t g_dialogue_script_022;  /* DAT_/PTR_004725e0 (标量) */
    int32_t g_dialogue_script_023;  /* DAT_/PTR_004725e4 (标量) */
    int32_t g_dialogue_script_024;  /* DAT_/PTR_004725e8 (标量) */
    int32_t g_dialogue_script_025;  /* DAT_/PTR_004725ec (标量) */
    int32_t g_dialogue_script_026;  /* DAT_/PTR_004725f0 (标量) */
    int32_t g_dialogue_script_027;  /* DAT_/PTR_004725f4 (标量) */
    int32_t g_dialogue_script_028;  /* DAT_/PTR_004725f8 (标量) */
    int32_t g_dialogue_script_029;  /* DAT_/PTR_004725fc (标量) */
    int32_t g_dialogue_script_030;  /* DAT_/PTR_00472600 (标量) */
    int32_t g_dialogue_script_031;  /* DAT_/PTR_00472604 (标量) */
    int32_t g_dialogue_script_032;  /* DAT_/PTR_00472608 (标量) */
    int32_t g_dialogue_script_033;  /* DAT_/PTR_0047260c (标量) */
    int32_t g_dialogue_script_034;  /* DAT_/PTR_00472610 (标量) */
    int32_t g_dialogue_script_035;  /* DAT_/PTR_00472614 (标量) */
    int32_t g_dialogue_script_036;  /* DAT_/PTR_00472618 (标量) */
    int32_t g_dialogue_script_037;  /* DAT_/PTR_0047261c (标量) */
    int32_t g_dialogue_script_038;  /* DAT_/PTR_00472620 (标量) */
    int32_t g_dialogue_script_039;  /* DAT_/PTR_00472624 (标量) */
    int32_t g_dialogue_script_040;  /* DAT_/PTR_00472628 (标量) */
    int32_t g_dialogue_script_041;  /* DAT_/PTR_0047262c (标量) */
    int32_t g_dialogue_script_042;  /* DAT_/PTR_00472630 (标量) */
    int32_t g_dialogue_script_043;  /* DAT_/PTR_00472634 (标量) */
    int32_t g_dialogue_script_045;  /* DAT_/PTR_0047263c (标量) */
    int32_t g_dialogue_script_046;  /* DAT_/PTR_00472640 (标量) */
    int32_t g_dialogue_script_047;  /* DAT_/PTR_00472644 (标量) */
    int32_t g_dialogue_script_048;  /* DAT_/PTR_00472648 (标量) */
    int32_t g_dialogue_script_049;  /* DAT_/PTR_0047264c (标量) */
    int32_t g_dialogue_script_050;  /* DAT_/PTR_00472650 (标量) */
    int32_t g_dialogue_script_051;  /* DAT_/PTR_00472654 (标量) */
    int32_t g_dialogue_script_052;  /* DAT_/PTR_00472658 (标量) */
    int32_t g_dialogue_script_053;  /* DAT_/PTR_0047265c (标量) */
    int32_t g_dialogue_script_054;  /* DAT_/PTR_00472660 (标量) */
    int32_t g_dialogue_script_055;  /* DAT_/PTR_00472664 (标量) */
    int32_t g_dialogue_script_056;  /* DAT_/PTR_00472668 (标量) */
    int32_t g_dialogue_script_057;  /* DAT_/PTR_0047266c (标量) */
    int32_t g_dialogue_script_058;  /* DAT_/PTR_00472670 (标量) */
    int32_t g_dialogue_script_059;  /* DAT_/PTR_00472674 (标量) */
    int32_t g_dialogue_script_060;  /* DAT_/PTR_00472678 (标量) */
    int32_t g_dialogue_script_061;  /* DAT_/PTR_0047267c (标量) */
    int32_t g_dialogue_script_062;  /* DAT_/PTR_00472680 (标量) */
    int32_t g_dialogue_script_063;  /* DAT_/PTR_00472684 (标量) */
    int32_t g_dialogue_script_064;  /* DAT_/PTR_00472688 (标量) */
    int32_t g_dialogue_script_065;  /* DAT_/PTR_0047268c (标量) */
    int32_t g_dialogue_script_066;  /* DAT_/PTR_00472690 (标量) */
    int32_t g_dialogue_script_067;  /* DAT_/PTR_00472694 (标量) */
    int32_t g_dialogue_script_068;  /* DAT_/PTR_00472698 (标量) */
    int32_t g_dialogue_script_071;  /* DAT_/PTR_004726a4 (标量) */
    int32_t g_dialogue_script_072;  /* DAT_/PTR_004726a8 (标量) */
    int32_t g_dialogue_script_073;  /* DAT_/PTR_004726ac (标量) */
    int32_t g_dialogue_script_076;  /* DAT_/PTR_004726b8 (标量) */
    int32_t g_dialogue_script_077;  /* DAT_/PTR_004726bc (标量) */
    int32_t g_dialogue_script_078;  /* DAT_/PTR_004726c0 (标量) */
    int32_t g_dialogue_script_079;  /* DAT_/PTR_004726c4 (标量) */
    int32_t g_dialogue_script_080;  /* DAT_/PTR_004726c8 (标量) */
    int32_t g_dialogue_script_091;  /* DAT_/PTR_004726f4 (标量) */
    int32_t g_dialogue_script_093;  /* DAT_/PTR_004726fc (标量) */
    int32_t g_dialogue_script_094;  /* DAT_/PTR_00472700 (标量) */
    int32_t g_dialogue_script_095;  /* DAT_/PTR_00472704 (标量) */
    int32_t g_dialogue_script_096;  /* DAT_/PTR_00472708 (标量) */
    int32_t g_dialogue_script_097;  /* DAT_/PTR_0047270c (标量) */
    int32_t g_dialogue_script_098;  /* DAT_/PTR_00472710 (标量) */
    int32_t g_dialogue_script_099;  /* DAT_/PTR_00472714 (标量) */
    int32_t g_dialogue_script_100;  /* DAT_/PTR_00472718 (标量) */
    int32_t g_dialogue_script_101;  /* DAT_/PTR_0047271c (标量) */
    int32_t g_dialogue_script_102;  /* DAT_/PTR_00472720 (标量) */
    int32_t g_dialogue_script_103;  /* DAT_/PTR_00472724 (标量) */
    int32_t g_dialogue_script_104;  /* DAT_/PTR_00472728 (标量) */
    int32_t g_dialogue_script_105;  /* DAT_/PTR_0047272c (标量) */
    int32_t g_dialogue_script_106;  /* DAT_/PTR_00472730 (标量) */
    int32_t g_dialogue_script_107;  /* DAT_/PTR_00472734 (标量) */
    int32_t g_dialogue_script_108;  /* DAT_/PTR_00472738 (标量) */
    int32_t g_dialogue_script_109;  /* DAT_/PTR_0047273c (标量) */
    int32_t g_dialogue_script_110;  /* DAT_/PTR_00472740 (标量) */
    int32_t g_dialogue_script_111;  /* DAT_/PTR_00472744 (标量) */
    int32_t g_dialogue_script_112;  /* DAT_/PTR_00472748 (标量) */
    int32_t g_dialogue_script_113;  /* DAT_/PTR_0047274c (标量) */
    int32_t g_dialogue_script_114;  /* DAT_/PTR_00472750 (标量) */
    int32_t g_dialogue_script_116;  /* DAT_/PTR_00472758 (标量) */
    int32_t g_dialogue_script_117;  /* DAT_/PTR_0047275c (标量) */
    int32_t g_dialogue_script_118;  /* DAT_/PTR_00472760 (标量) */
    int32_t g_dialogue_script_119;  /* DAT_/PTR_00472764 (标量) */
    int32_t g_dialogue_script_120;  /* DAT_/PTR_00472768 (标量) */
    int32_t g_dialogue_script_121;  /* DAT_/PTR_0047276c (标量) */
    int32_t g_dialogue_script_122;  /* DAT_/PTR_00472770 (标量) */
    int32_t g_dialogue_script_123;  /* DAT_/PTR_00472774 (标量) */
    int32_t g_dialogue_script_124;  /* DAT_/PTR_00472778 (标量) */
    int32_t g_dialogue_script_125;  /* DAT_/PTR_0047277c (标量) */
    int32_t g_dialogue_script_126;  /* DAT_/PTR_00472780 (标量) */
    int32_t g_dialogue_script_127;  /* DAT_/PTR_00472784 (标量) */
    int32_t g_dialogue_script_128;  /* DAT_/PTR_00472788 (标量) */
    int32_t g_dialogue_script_129;  /* DAT_/PTR_0047278c (标量) */
    int32_t g_dialogue_script_130;  /* DAT_/PTR_00472790 (标量) */
    int32_t g_dialogue_script_131;  /* DAT_/PTR_00472794 (标量) */
    int32_t g_dialogue_script_132;  /* DAT_/PTR_00472798 (标量) */
    int32_t g_dialogue_script_133;  /* DAT_/PTR_0047279c (标量) */
    int32_t g_dialogue_script_135;  /* DAT_/PTR_004727a4 (标量) */
    int32_t g_dialogue_script_136;  /* DAT_/PTR_004727a8 (标量) */
    int32_t g_dialogue_script_137;  /* DAT_/PTR_004727ac (标量) */
    int32_t g_dialogue_script_138;  /* DAT_/PTR_004727b0 (标量) */
    int32_t g_dialogue_script_141;  /* DAT_/PTR_004727bc (标量) */
    int32_t g_dialogue_script_142;  /* DAT_/PTR_004727c0 (标量) */
    int32_t g_dialogue_script_143;  /* DAT_/PTR_004727c4 (标量) */
    int32_t g_dialogue_script_144;  /* DAT_/PTR_004727c8 (标量) */
    int32_t g_dialogue_script_145;  /* DAT_/PTR_004727cc (标量) */
    int32_t g_dialogue_script_146;  /* DAT_/PTR_004727d0 (标量) */
    int32_t g_dialogue_script_147;  /* DAT_/PTR_004727d4 (标量) */
    int32_t g_dialogue_script_148;  /* DAT_/PTR_004727d8 (标量) */
    int32_t g_dialogue_script_149;  /* DAT_/PTR_004727dc (标量) */
    int32_t g_dialogue_script_150;  /* DAT_/PTR_004727e0 (标量) */
    int32_t g_dialogue_script_151;  /* DAT_/PTR_004727e4 (标量) */
    int32_t g_dialogue_script_159;  /* DAT_/PTR_00472804 (标量) */
    int32_t g_dialogue_script_160;  /* DAT_/PTR_00472808 (标量) */
    int32_t g_dialogue_script_161;  /* DAT_/PTR_0047280c (标量) */
    int32_t g_dialogue_script_162;  /* DAT_/PTR_00472810 (标量) */
    int32_t g_dialogue_script_163;  /* DAT_/PTR_00472814 (标量) */
    int32_t g_dialogue_script_164;  /* DAT_/PTR_00472818 (标量) */
    int32_t g_dialogue_script_165;  /* DAT_/PTR_0047281c (标量) */
    int32_t g_dialogue_script_166;  /* DAT_/PTR_00472820 (标量) */
    int32_t g_dialogue_script_167;  /* DAT_/PTR_00472824 (标量) */
    int32_t g_dialogue_script_168;  /* DAT_/PTR_00472828 (标量) */
    int32_t g_dialogue_script_169;  /* DAT_/PTR_0047282c (标量) */
    int32_t g_dialogue_script_171;  /* DAT_/PTR_00472834 (标量) */
    int32_t g_dialogue_script_172;  /* DAT_/PTR_00472838 (标量) */
    int32_t g_dialogue_script_173;  /* DAT_/PTR_0047283c (标量) */
    int32_t g_dialogue_script_174;  /* DAT_/PTR_00472840 (标量) */
    int32_t g_dialogue_script_175;  /* DAT_/PTR_00472844 (标量) */
    int32_t g_dialogue_script_176;  /* DAT_/PTR_00472848 (标量) */
    int32_t g_dialogue_script_177;  /* DAT_/PTR_0047284c (标量) */
    int32_t g_dialogue_script_178;  /* DAT_/PTR_00472850 (标量) */
    int32_t g_dialogue_script_181;  /* DAT_/PTR_0047285c (标量) */
    int32_t g_dialogue_script_182;  /* DAT_/PTR_00472860 (标量) */
    int32_t g_dialogue_script_183;  /* DAT_/PTR_00472864 (标量) */
    int32_t g_dialogue_script_184;  /* DAT_/PTR_00472868 (标量) */
    int32_t g_dialogue_script_185;  /* DAT_/PTR_0047286c (标量) */
    int32_t g_dialogue_script_186;  /* DAT_/PTR_00472870 (标量) */
    int32_t g_dialogue_script_187;  /* DAT_/PTR_00472874 (标量) */
    int32_t g_dialogue_script_188;  /* DAT_/PTR_00472878 (标量) */
    int32_t g_dialogue_script_189;  /* DAT_/PTR_0047287c (标量) */
    int32_t g_dialogue_script_190;  /* DAT_/PTR_00472880 (标量) */
    int32_t g_dialogue_script_191;  /* DAT_/PTR_00472884 (标量) */
    int32_t g_dialogue_script_192;  /* DAT_/PTR_00472888 (标量) */
    int32_t g_dialogue_script_193;  /* DAT_/PTR_0047288c (标量) */
    int32_t g_dialogue_script_194;  /* DAT_/PTR_00472890 (标量) */
    int32_t g_dialogue_script_195;  /* DAT_/PTR_00472894 (标量) */
    int32_t g_dialogue_script_196;  /* DAT_/PTR_00472898 (标量) */
    int32_t g_dialogue_script_197;  /* DAT_/PTR_0047289c (标量) */
    int32_t g_dialogue_script_198;  /* DAT_/PTR_004728a0 (标量) */
    int32_t g_dialogue_script_201;  /* DAT_/PTR_004728ac (标量) */
    int32_t g_dialogue_script_202;  /* DAT_/PTR_004728b0 (标量) */
    int32_t g_dialogue_script_203;  /* DAT_/PTR_004728b4 (标量) */
    int32_t g_dialogue_script_205;  /* DAT_/PTR_004728bc (标量) */
    int32_t g_dialogue_script_206;  /* DAT_/PTR_004728c0 (标量) */
    int32_t g_dialogue_script_207;  /* DAT_/PTR_004728c4 (标量) */
    int32_t g_dialogue_script_208;  /* DAT_/PTR_004728c8 (标量) */
    int32_t g_dialogue_script_209;  /* DAT_/PTR_004728cc (标量) */
    int32_t g_dialogue_script_210;  /* DAT_/PTR_004728d0 (标量) */
    int32_t g_dialogue_script_211;  /* DAT_/PTR_004728d4 (标量) */
    int32_t g_dialogue_script_212;  /* DAT_/PTR_004728d8 (标量) */
    int32_t g_dialogue_script_213;  /* DAT_/PTR_004728dc (标量) */
    int32_t g_dialogue_script_214;  /* DAT_/PTR_004728e0 (标量) */
    int32_t g_dialogue_script_215;  /* DAT_/PTR_004728e4 (标量) */
    int32_t g_dialogue_script_216;  /* DAT_/PTR_004728e8 (标量) */
    int32_t g_dialogue_script_217;  /* DAT_/PTR_004728ec (标量) */
    int32_t g_dialogue_script_218;  /* DAT_/PTR_004728f0 (标量) */
    int32_t g_dialogue_script_219;  /* DAT_/PTR_004728f4 (标量) */
    int32_t g_dialogue_script_220;  /* DAT_/PTR_004728f8 (标量) */
    int32_t g_dialogue_script_221;  /* DAT_/PTR_004728fc (标量) */
    int32_t g_dialogue_script_222;  /* DAT_/PTR_00472900 (标量) */
    int32_t g_dialogue_script_223;  /* DAT_/PTR_00472904 (标量) */
    int32_t g_dialogue_script_224;  /* DAT_/PTR_00472908 (标量) */
    int32_t g_dialogue_script_225;  /* DAT_/PTR_0047290c (标量) */
    int32_t g_dialogue_script_226;  /* DAT_/PTR_00472910 (标量) */
    int32_t g_dialogue_script_227;  /* DAT_/PTR_00472914 (标量) */
    int32_t g_dialogue_script_228;  /* DAT_/PTR_00472918 (标量) */
    int32_t g_dialogue_script_229;  /* DAT_/PTR_0047291c (标量) */
    int32_t g_dialogue_script_234;  /* DAT_/PTR_00472930 (标量) */
    int32_t g_dialogue_script_235;  /* DAT_/PTR_00472934 (标量) */
    int32_t g_dialogue_script_236;  /* DAT_/PTR_00472938 (标量) */
    int32_t g_dialogue_script_237;  /* DAT_/PTR_0047293c (标量) */
    int32_t g_dialogue_script_238;  /* DAT_/PTR_00472940 (标量) */
    int32_t g_dialogue_script_239;  /* DAT_/PTR_00472944 (标量) */
    int32_t g_dialogue_script_241;  /* DAT_/PTR_0047294c (标量) */
    int32_t g_dialogue_script_242;  /* DAT_/PTR_00472950 (标量) */
    int32_t g_dialogue_script_243;  /* DAT_/PTR_00472954 (标量) */
    int32_t g_dialogue_script_244;  /* DAT_/PTR_00472958 (标量) */
    int32_t g_dialogue_script_245;  /* DAT_/PTR_0047295c (标量) */
    int32_t g_dialogue_script_246;  /* DAT_/PTR_00472960 (标量) */
    int32_t g_dialogue_script_247;  /* DAT_/PTR_00472964 (标量) */
    int32_t g_dialogue_script_248;  /* DAT_/PTR_00472968 (标量) */
    int32_t g_dialogue_script_249;  /* DAT_/PTR_0047296c (标量) */
    int32_t g_dialogue_script_250;  /* DAT_/PTR_00472970 (标量) */
    int32_t g_dialogue_script_251;  /* DAT_/PTR_00472974 (标量) */
    int32_t g_dialogue_script_252;  /* DAT_/PTR_00472978 (标量) */
    int32_t g_dialogue_script_255;  /* DAT_/PTR_00472984 (标量) */
    int32_t g_dialogue_script_256;  /* DAT_/PTR_00472988 (标量) */
    int32_t g_dialogue_script_257;  /* DAT_/PTR_0047298c (标量) */
    int32_t g_dialogue_script_258;  /* DAT_/PTR_00472990 (标量) */
    int32_t g_dialogue_script_261;  /* DAT_/PTR_0047299c (标量) */
    int32_t g_dialogue_script_262;  /* DAT_/PTR_004729a0 (标量) */
    int32_t g_dialogue_script_263;  /* DAT_/PTR_004729a4 (标量) */
    int32_t g_dialogue_script_264;  /* DAT_/PTR_004729a8 (标量) */
    int32_t g_dialogue_script_265;  /* DAT_/PTR_004729ac (标量) */
    int32_t g_dialogue_script_266;  /* DAT_/PTR_004729b0 (标量) */
    int32_t g_dialogue_script_267;  /* DAT_/PTR_004729b4 (标量) */
    int32_t g_dialogue_script_268;  /* DAT_/PTR_004729b8 (标量) */
    int32_t g_dialogue_script_269;  /* DAT_/PTR_004729bc (标量) */
    int32_t g_dialogue_script_270;  /* DAT_/PTR_004729c0 (标量) */
    int32_t g_dialogue_script_271;  /* DAT_/PTR_004729c4 (标量) */
    int32_t g_dialogue_script_272;  /* DAT_/PTR_004729c8 (标量) */
    int32_t g_dialogue_script_273;  /* DAT_/PTR_004729cc (标量) */
    int32_t g_dialogue_script_276;  /* DAT_/PTR_004729d8 (标量) */
    int32_t g_dialogue_script_277;  /* DAT_/PTR_004729dc (标量) */
    int32_t g_dialogue_script_278;  /* DAT_/PTR_004729e0 (标量) */
    int32_t g_dialogue_script_279;  /* DAT_/PTR_004729e4 (标量) */
    int32_t g_dialogue_script_280;  /* DAT_/PTR_004729e8 (标量) */
    int32_t g_dialogue_script_281;  /* DAT_/PTR_004729ec (标量) */
    int32_t g_dialogue_script_282;  /* DAT_/PTR_004729f0 (标量) */
    int32_t g_dialogue_script_283;  /* DAT_/PTR_004729f4 (标量) */
    int32_t g_dialogue_script_285;  /* DAT_/PTR_004729fc (标量) */
    int32_t g_dialogue_script_286;  /* DAT_/PTR_00472a00 (标量) */
    int32_t g_dialogue_script_287;  /* DAT_/PTR_00472a04 (标量) */
    int32_t g_dialogue_script_288;  /* DAT_/PTR_00472a08 (标量) */
    int32_t g_dialogue_script_289;  /* DAT_/PTR_00472a0c (标量) */
    int32_t g_dialogue_script_290;  /* DAT_/PTR_00472a10 (标量) */
    int32_t g_dialogue_script_291;  /* DAT_/PTR_00472a14 (标量) */
    int32_t g_dialogue_script_292;  /* DAT_/PTR_00472a18 (标量) */
    int32_t g_dialogue_script_293;  /* DAT_/PTR_00472a1c (标量) */
    int32_t g_dialogue_script_295;  /* DAT_/PTR_00472a24 (标量) */
    int32_t g_dialogue_script_296;  /* DAT_/PTR_00472a28 (标量) */
    int32_t g_dialogue_script_297;  /* DAT_/PTR_00472a2c (标量) */
    int32_t g_dialogue_script_298;  /* DAT_/PTR_00472a30 (标量) */
    int32_t g_dialogue_script_299;  /* DAT_/PTR_00472a34 (标量) */
    int32_t g_dialogue_script_300;  /* DAT_/PTR_00472a38 (标量) */
    int32_t g_dialogue_script_301;  /* DAT_/PTR_00472a3c (标量) */
    int32_t g_dialogue_script_302;  /* DAT_/PTR_00472a40 (标量) */
    int32_t g_dialogue_script_303;  /* DAT_/PTR_00472a44 (标量) */
    int32_t g_dialogue_script_306;  /* DAT_/PTR_00472a50 (标量) */
    int32_t g_dialogue_script_307;  /* DAT_/PTR_00472a54 (标量) */
    int32_t g_dialogue_script_308;  /* DAT_/PTR_00472a58 (标量) */
    int32_t g_dialogue_script_309;  /* DAT_/PTR_00472a5c (标量) */
    int32_t g_dialogue_script_310;  /* DAT_/PTR_00472a60 (标量) */
    int32_t g_dialogue_script_311;  /* DAT_/PTR_00472a64 (标量) */
    int32_t g_dialogue_script_312;  /* DAT_/PTR_00472a68 (标量) */
    int32_t g_dialogue_script_315;  /* DAT_/PTR_00472a74 (标量) */
    int32_t g_dialogue_script_316;  /* DAT_/PTR_00472a78 (标量) */
    int32_t g_dialogue_script_317;  /* DAT_/PTR_00472a7c (标量) */
    int32_t g_dialogue_script_320;  /* DAT_/PTR_00472a88 (标量) */
    int32_t g_dialogue_script_321;  /* DAT_/PTR_00472a8c (标量) */
    int32_t g_dialogue_script_322;  /* DAT_/PTR_00472a90 (标量) */
    int32_t g_dialogue_script_331;  /* DAT_/PTR_00472ab4 (标量) */
    int32_t g_dialogue_script_332;  /* DAT_/PTR_00472ab8 (标量) */
    int32_t g_dialogue_script_333;  /* DAT_/PTR_00472abc (标量) */
    int32_t g_dialogue_script_334;  /* DAT_/PTR_00472ac0 (标量) */
    int32_t g_dialogue_script_335;  /* DAT_/PTR_00472ac4 (标量) */
    int32_t g_dialogue_script_336;  /* DAT_/PTR_00472ac8 (标量) */
    int32_t g_dialogue_script_337;  /* DAT_/PTR_00472acc (标量) */
    int32_t g_dialogue_script_338;  /* DAT_/PTR_00472ad0 (标量) */
    int32_t g_dialogue_script_339;  /* DAT_/PTR_00472ad4 (标量) */
    int32_t g_dialogue_script_341;  /* DAT_/PTR_00472adc (标量) */
    int32_t g_dialogue_script_342;  /* DAT_/PTR_00472ae0 (标量) */
    int32_t g_dialogue_script_343;  /* DAT_/PTR_00472ae4 (标量) */
    int32_t g_dialogue_script_344;  /* DAT_/PTR_00472ae8 (标量) */
    int32_t g_dialogue_script_345;  /* DAT_/PTR_00472aec (标量) */
    int32_t g_dialogue_script_346;  /* DAT_/PTR_00472af0 (标量) */
    int32_t g_dialogue_script_347;  /* DAT_/PTR_00472af4 (标量) */
    int32_t g_dialogue_script_348;  /* DAT_/PTR_00472af8 (标量) */
    int32_t g_dialogue_script_349;  /* DAT_/PTR_00472afc (标量) */
    int32_t g_dialogue_script_350;  /* DAT_/PTR_00472b00 (标量) */
    int32_t g_dialogue_script_351;  /* DAT_/PTR_00472b04 (标量) */
    int32_t g_dialogue_script_352;  /* DAT_/PTR_00472b08 (标量) */
    int32_t g_dialogue_script_353;  /* DAT_/PTR_00472b0c (标量) */
    int32_t g_dialogue_script_355;  /* DAT_/PTR_00472b14 (标量) */
    int32_t g_dialogue_script_356;  /* DAT_/PTR_00472b18 (标量) */
    int32_t g_dialogue_script_357;  /* DAT_/PTR_00472b1c (标量) */
    int32_t g_dialogue_script_363;  /* DAT_/PTR_00472b34 (标量) */
    int32_t g_dialogue_script_364;  /* DAT_/PTR_00472b38 (标量) */
    int32_t g_dialogue_script_365;  /* DAT_/PTR_00472b3c (标量) */
    int32_t g_dialogue_script_366;  /* DAT_/PTR_00472b40 (标量) */
    int32_t g_dialogue_script_367;  /* DAT_/PTR_00472b44 (标量) */
    int32_t g_dialogue_script_368;  /* DAT_/PTR_00472b48 (标量) */
    int32_t g_dialogue_script_369;  /* DAT_/PTR_00472b4c (标量) */
    int32_t g_dialogue_script_371;  /* DAT_/PTR_00472b54 (标量) */
    int32_t g_dialogue_script_372;  /* DAT_/PTR_00472b58 (标量) */
    int32_t g_dialogue_script_373;  /* DAT_/PTR_00472b5c (标量) */
    int32_t g_dialogue_script_374;  /* DAT_/PTR_00472b60 (标量) */
    int32_t g_dialogue_script_375;  /* DAT_/PTR_00472b64 (标量) */
    int32_t g_dialogue_script_376;  /* DAT_/PTR_00472b68 (标量) */
    int32_t g_dialogue_script_377;  /* DAT_/PTR_00472b6c (标量) */
    int32_t g_dialogue_script_378;  /* DAT_/PTR_00472b70 (标量) */
    int32_t g_dialogue_script_380[16];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_dialogue_script_396;  /* DAT_/PTR_00472bb8 (标量) */
    int32_t g_dialogue_script_397;  /* DAT_/PTR_00472bbc (标量) */
    int32_t g_dialogue_script_398;  /* DAT_/PTR_00472bc0 (标量) */
    int32_t g_dialogue_script_401;  /* DAT_/PTR_00472bcc (标量) */
    int32_t g_dialogue_script_402;  /* DAT_/PTR_00472bd0 (标量) */
    int32_t g_dialogue_script_403;  /* DAT_/PTR_00472bd4 (标量) */
    int32_t g_dialogue_script_404;  /* DAT_/PTR_00472bd8 (标量) */
    int32_t g_dialogue_script_405;  /* DAT_/PTR_00472bdc (标量) */
    int32_t g_dialogue_script_406;  /* DAT_/PTR_00472be0 (标量) */
    int32_t g_dialogue_script_407;  /* DAT_/PTR_00472be4 (标量) */
    int32_t g_dialogue_script_411;  /* DAT_/PTR_00472bf4 (标量) */
    int32_t g_dialogue_script_412;  /* DAT_/PTR_00472bf8 (标量) */
    int32_t g_dialogue_script_413;  /* DAT_/PTR_00472bfc (标量) */
    int32_t g_dialogue_script_415;  /* DAT_/PTR_00472c04 (标量) */
    int32_t g_dialogue_script_416;  /* DAT_/PTR_00472c08 (标量) */
    int32_t g_dialogue_script_417;  /* DAT_/PTR_00472c0c (标量) */
    int32_t g_dialogue_script_418;  /* DAT_/PTR_00472c10 (标量) */
    int32_t g_dialogue_script_421;  /* DAT_/PTR_00472c1c (标量) */
    int32_t g_dialogue_script_422;  /* DAT_/PTR_00472c20 (标量) */
    int32_t g_dialogue_script_423;  /* DAT_/PTR_00472c24 (标量) */
    int32_t g_dialogue_script_425;  /* DAT_/PTR_00472c2c (标量) */
    int32_t g_dialogue_script_426;  /* DAT_/PTR_00472c30 (标量) */
    int32_t g_dialogue_script_427;  /* DAT_/PTR_00472c34 (标量) */
    int32_t g_dialogue_script_428;  /* DAT_/PTR_00472c38 (标量) */
    int32_t g_dialogue_script_429;  /* DAT_/PTR_00472c3c (标量) */
    int32_t g_dialogue_script_430;  /* DAT_/PTR_00472c40 (标量) */
    int32_t g_dialogue_script_431;  /* DAT_/PTR_00472c44 (标量) */
    int32_t g_dialogue_script_432;  /* DAT_/PTR_00472c48 (标量) */
    int32_t g_dialogue_script_433;  /* DAT_/PTR_00472c4c (标量) */
    int32_t g_dialogue_script_434;  /* DAT_/PTR_00472c50 (标量) */
    int32_t g_dialogue_script_435;  /* DAT_/PTR_00472c54 (标量) */
    int32_t g_dialogue_script_436;  /* DAT_/PTR_00472c58 (标量) */
    int32_t g_dialogue_script_437;  /* DAT_/PTR_00472c5c (标量) */
    int32_t g_dialogue_script_438;  /* DAT_/PTR_00472c60 (标量) */
    int32_t g_dialogue_script_439;  /* DAT_/PTR_00472c64 (标量) */
    int32_t g_dialogue_script_440;  /* DAT_/PTR_00472c68 (标量) */
    int32_t g_dialogue_script_443;  /* DAT_/PTR_00472c74 (标量) */
    int32_t g_dialogue_script_444;  /* DAT_/PTR_00472c78 (标量) */
    int32_t g_dialogue_script_445;  /* DAT_/PTR_00472c7c (标量) */
    int32_t g_dialogue_script_446;  /* DAT_/PTR_00472c80 (标量) */
    int32_t g_dialogue_script_447;  /* DAT_/PTR_00472c84 (标量) */
    int32_t g_dialogue_script_448;  /* DAT_/PTR_00472c88 (标量) */
    int32_t g_dialogue_script_449;  /* DAT_/PTR_00472c8c (标量) */
    int32_t g_dialogue_script_450;  /* DAT_/PTR_00472c90 (标量) */
    int32_t g_dialogue_script_451;  /* DAT_/PTR_00472c94 (标量) */
    int32_t g_dialogue_script_453;  /* DAT_/PTR_00472c9c (标量) */
    int32_t g_dialogue_script_454;  /* DAT_/PTR_00472ca0 (标量) */
    int32_t g_dialogue_script_455;  /* DAT_/PTR_00472ca4 (标量) */
    int32_t g_dialogue_script_456;  /* DAT_/PTR_00472ca8 (标量) */
    int32_t g_dialogue_script_457;  /* DAT_/PTR_00472cac (标量) */
    int32_t g_dialogue_script_458;  /* DAT_/PTR_00472cb0 (标量) */
    int32_t g_dialogue_script_459;  /* DAT_/PTR_00472cb4 (标量) */
    int32_t g_dialogue_script_460;  /* DAT_/PTR_00472cb8 (标量) */
    int32_t g_dialogue_script_461;  /* DAT_/PTR_00472cbc (标量) */
    int32_t g_dialogue_script_462;  /* DAT_/PTR_00472cc0 (标量) */
    int32_t g_dialogue_script_463;  /* DAT_/PTR_00472cc4 (标量) */
    int32_t g_dialogue_script_464;  /* DAT_/PTR_00472cc8 (标量) */
    int32_t g_dialogue_script_465;  /* DAT_/PTR_00472ccc (标量) */
    int32_t g_dialogue_script_466;  /* DAT_/PTR_00472cd0 (标量) */
    int32_t g_dialogue_script_468;  /* DAT_/PTR_00472cd8 (标量) */
    int32_t g_dialogue_script_469;  /* DAT_/PTR_00472cdc (标量) */
    int32_t g_dialogue_script_470;  /* DAT_/PTR_00472ce0 (标量) */
    int32_t g_dialogue_script_471;  /* DAT_/PTR_00472ce4 (标量) */
    int32_t g_dialogue_script_472;  /* DAT_/PTR_00472ce8 (标量) */
    int32_t g_dialogue_script_473;  /* DAT_/PTR_00472cec (标量) */
    int32_t g_dialogue_script_474;  /* DAT_/PTR_00472cf0 (标量) */
    int32_t g_dialogue_script_475;  /* DAT_/PTR_00472cf4 (标量) */
    int32_t g_dialogue_script_476;  /* DAT_/PTR_00472cf8 (标量) */
    int32_t g_dialogue_script_477;  /* DAT_/PTR_00472cfc (标量) */
    int32_t g_dialogue_script_478;  /* DAT_/PTR_00472d00 (标量) */
    int32_t g_dialogue_script_479;  /* DAT_/PTR_00472d04 (标量) */
    int32_t g_dialogue_script_480;  /* DAT_/PTR_00472d08 (标量) */
    int32_t g_dialogue_script_481;  /* DAT_/PTR_00472d0c (标量) */
    int32_t g_dialogue_script_482;  /* DAT_/PTR_00472d10 (标量) */
    int32_t g_dialogue_script_483;  /* DAT_/PTR_00472d14 (标量) */
    int32_t g_dialogue_script_484;  /* DAT_/PTR_00472d18 (标量) */
    int32_t g_dialogue_script_485;  /* DAT_/PTR_00472d1c (标量) */
    int32_t g_dialogue_script_486;  /* DAT_/PTR_00472d20 (标量) */
    int32_t g_dialogue_script_487;  /* DAT_/PTR_00472d24 (标量) */
    int32_t g_dialogue_script_488;  /* DAT_/PTR_00472d28 (标量) */
    int32_t g_dialogue_script_489;  /* DAT_/PTR_00472d2c (标量) */
    int32_t g_dialogue_script_490;  /* DAT_/PTR_00472d30 (标量) */
    int32_t g_dialogue_script_491;  /* DAT_/PTR_00472d34 (标量) */
    int32_t g_dialogue_script_492;  /* DAT_/PTR_00472d38 (标量) */
    int32_t g_dialogue_script_493;  /* DAT_/PTR_00472d3c (标量) */
    int32_t g_dialogue_script_494;  /* DAT_/PTR_00472d40 (标量) */
    int32_t g_dialogue_script_495;  /* DAT_/PTR_00472d44 (标量) */
    int32_t g_dialogue_script_496;  /* DAT_/PTR_00472d48 (标量) */
    int32_t g_dialogue_script_497;  /* DAT_/PTR_00472d4c (标量) */
    int32_t g_dialogue_script_498;  /* DAT_/PTR_00472d50 (标量) */
    int32_t g_dialogue_script_499;  /* DAT_/PTR_00472d54 (标量) */
    int32_t g_dialogue_script_501;  /* DAT_/PTR_00472d5c (标量) */
    int32_t g_dialogue_script_502;  /* DAT_/PTR_00472d60 (标量) */
    int32_t g_dialogue_script_503;  /* DAT_/PTR_00472d64 (标量) */
    int32_t g_dialogue_script_505;  /* DAT_/PTR_00472d6c (标量) */
    int32_t g_dialogue_script_506;  /* DAT_/PTR_00472d70 (标量) */
    int32_t g_dialogue_script_507;  /* DAT_/PTR_00472d74 (标量) */
    int32_t g_dialogue_script_508;  /* DAT_/PTR_00472d78 (标量) */
    int32_t g_dialogue_script_509;  /* DAT_/PTR_00472d7c (标量) */
    int32_t g_dialogue_script_510;  /* DAT_/PTR_00472d80 (标量) */
    int32_t g_dialogue_script_511;  /* DAT_/PTR_00472d84 (标量) */
    int32_t g_dialogue_script_512;  /* DAT_/PTR_00472d88 (标量) */
    int32_t g_dialogue_script_513;  /* DAT_/PTR_00472d8c (标量) */
    int32_t g_dialogue_script_514;  /* DAT_/PTR_00472d90 (标量) */
    int32_t g_dialogue_script_515;  /* DAT_/PTR_00472d94 (标量) */
    int32_t g_dialogue_script_520[1];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_dialogue_script_521[119];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_dialogue_script_640[32];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_misc_blk_3008;  /* DAT_/PTR_00473008 (标量) */
    int32_t g_hud_stat_values[10];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_misc_blk_3040[1];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_misc_blk_3046[2];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_misc_blk_3050[2];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_misc_blk_3058[64];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_misc_blk_3158;  /* DAT_/PTR_00473158 (标量) */
    int32_t g_misc_blk_3160;  /* DAT_/PTR_00473160 (标量) */
    int32_t g_misc_blk_3c60;  /* DAT_/PTR_00473c60 (标量) */
    uint8_t g_surf_b4f4[0x20000];  /* DAT_/PTR_004744a0 (对应原版 0x4744a0 surface 缓冲; asset_mine_img 把 g_state_blk_b4f4 指向此处。证据 FUN_00422889.c 解引用 + PE .data) */
    uint8_t g_surf_7418[0x20000];  /* DAT_/PTR_00474fa0 (对应原版 0x474fa0 surface 缓冲; asset_mine_img 把 g_buf_blk_7418 指向此处。证据 FUN_00422889.c 解引用 + PE .data) */
    int32_t* g_ddraw;  /* DAT_/PTR_00475aa0 (指针/缓冲区，按实际用法判定) */
    int32_t g_battle_frame;  /* DAT_/PTR_00475aa4 (标量) */
    int32_t g_misc_blk_5aa8;  /* DAT_/PTR_00475aa8 (标量) */
    int32_t g_misc_blk_5aac;  /* DAT_/PTR_00475aac (标量) */
    int32_t g_misc_blk_5ab0;  /* DAT_/PTR_00475ab0 (标量) */
    int32_t g_misc_blk_5ab4[1];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_misc_blk_5ab8[400];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    uint8_t g_misc_blk_5be8[0x510];  /* DAT_/PTR_00475be8 (字节掩码缓冲: 移动/碰撞 9x9 掩码源; 0x510=16*0x51(每 skill.movement_index 0x51 宽)。fread 0x510 实抽证据 FUN_*.c:18808; 索引 iVar1*0x51+9x9。证据 build_re_map classify_global + 地址区间) */
    int32_t g_misc_blk_60f8;  /* DAT_/PTR_004760f8 (标量) */
    int32_t g_sprite_sheet_buf[40390];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_misc_blk_d910;  /* DAT_/PTR_0049d910 (标量) */
    int32_t g_misc_blk_00b8;  /* DAT_/PTR_005200b8 (标量) */
    int32_t g_misc_blk_1ff8;  /* DAT_/PTR_00521ff8 (标量) */
    int32_t g_misc_blk_f4b8;  /* DAT_/PTR_0053f4b8 (标量) */
    int32_t g_map_blk_01f8;  /* DAT_/PTR_005701f8 (标量) */
    int32_t g_map_blk_8d18[1];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_map_blk_8d1e[4];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_map_blk_8d30[1];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_map_blk_8d34[1];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_map_blk_8d38[1];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_map_blk_8d3c[1];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_item_attack_bonus_table[1];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_item_defense_bonus_table[1];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_map_blk_8d48[1];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_map_blk_8d4c[3];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_map_blk_8d58[1];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_map_blk_8d5c[2864];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_map_blk_ba1c;  /* DAT_/PTR_0057ba1c (标量) */
    int32_t g_map_grid_width;  /* DAT_/PTR_0057ba20 (标量) */
    int32_t g_map_grid_height;  /* DAT_/PTR_0057ba22 (标量) */
    int32_t g_map_blk_ba28;  /* DAT_/PTR_0057ba28 (标量) */
    int32_t g_map_blk_ba29;  /* DAT_/PTR_0057ba29 (标量) */
    int32_t* g_map_blk_bd40;  /* DAT_/PTR_0057bd40 (指针/缓冲区，按实际用法判定) */
    int32_t g_map_blk_bd44;  /* DAT_/PTR_0057bd44 (标量) */
    int32_t g_active_unit_index;  /* DAT_/PTR_0057bd48 (标量) */
    int32_t g_map_blk_bd4e;  /* DAT_/PTR_0057bd4e (标量) */
    int32_t g_warehouse_items[6];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_map_blk_bd68;  /* DAT_/PTR_0057bd68 (标量) */
    int32_t g_map_blk_bd8a;  /* DAT_/PTR_0057bd8a (标量) */
    int32_t g_map_blk_bdc0;  /* DAT_/PTR_0057bdc0 (标量) */
    int32_t g_map_blk_bdf0[40];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_map_blk_be90;  /* DAT_/PTR_0057be90 (标量) */
    int32_t g_map_blk_bf0e;  /* DAT_/PTR_0057bf0e (标量) */
    uint8_t g_tile_grid[65536];  /* DAT_/PTR_0057bf10 (地图数据块：每格2字节网格 uint8_t[65536]，RE 实证消除 int32 4× 过跨距) */
    uint8_t g_tile_grid_hi[65536];  /* DAT_/PTR_0057bf11 (地图数据块：每格2字节网格 uint8_t[65536]，RE 实证消除 int32 4× 过跨距) */
    int32_t g_map_blk_bf14[3];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_map_blk_bf20[1];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_map_blk_bf24[11];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_tile_count;  /* DAT_/PTR_0057bf50 (标量) */
    int32_t g_map_blk_bf90[96];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_map_blk_c111[39];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_map_blk_c1b0;  /* DAT_/PTR_0057c1b0 (标量) */
    int32_t* g_ddraw_surface_3f10;  /* DAT_/PTR_00583f10 (指针/缓冲区，按实际用法判定) */
    int32_t g_battle_status_flags;  /* DAT_/PTR_00583f18 (标量) */
    int32_t g_combat_gate_0;  /* DAT_/PTR_00583f1c (标量) */
    int32_t g_combat_gate_1;  /* DAT_/PTR_00583f20 (标量) */
    int32_t g_combat_gate_2;  /* DAT_/PTR_00583f24 (标量) */
    int32_t g_combat_gate_3;  /* DAT_/PTR_00583f28 (标量) */
    int32_t g_battle_intro_msg5_shown;  /* DAT_/PTR_00583f2c (标量) */
    int32_t g_battle_intro_msg6_shown;  /* DAT_/PTR_00583f30 (标量) */
    int32_t g_battle_intro_msg7_shown;  /* DAT_/PTR_00583f34 (标量) */
    int32_t g_battle_phase_frame;  /* DAT_/PTR_00583f4c (标量) */
    int32_t g_battle_start_frame;  /* DAT_/PTR_00583f50 (标量) */
    int32_t g_battle_firstframe_latch;  /* DAT_/PTR_00583f54 (标量) */
    int32_t g_tile_marker_sprite;  /* DAT_/PTR_00583f58 (标量) */
    int32_t g_anim_step;  /* DAT_/PTR_00584358 (标量) */
    int32_t g_msg_init_guard;  /* DAT_/PTR_0058435c (标量) */
    int32_t g_army_sprite_cache[10];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_bgm_track_id;  /* DAT_/PTR_00584388 (标量) */
    int32_t g_pathfind_cost_a;  /* DAT_/PTR_0058438e (标量) */
    int32_t g_terrain_grid_base;  /* DAT_/PTR_00584390 (标量) */
    int32_t g_pathfind_cost_b[8191];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_town_dat_record;  /* DAT_/PTR_0058c390 (标量) */
    int32_t g_periodic_tick_gate;  /* DAT_/PTR_0058c392 (标量) */
    int32_t g_surface_buffer;  /* DAT_/PTR_0058c3a8 (标量) */
    int32_t g_ui_surface_menu;  /* DAT_/PTR_0058cca8 (标量) */
    int32_t g_menu_icon_sprite_a;  /* DAT_/PTR_0058eba8 (标量) */
    int32_t g_menu51_img_buf;  /* DAT_/PTR_0058f668 (标量) */
    int32_t g_misc_blk_03a8;  /* DAT_/PTR_005903a8 (标量) */
    int32_t g_misc_blk_13a8;  /* DAT_/PTR_005913a8 (标量) */
    int32_t g_misc_blk_2928;  /* DAT_/PTR_00592928 (标量) */
    int32_t g_misc_blk_40a8;  /* DAT_/PTR_005940a8 (标量) */
    int32_t g_misc_blk_4228;  /* DAT_/PTR_00594228 (标量) */
    int32_t g_misc_blk_5be8_595be8;  /* DAT_/PTR_00595be8 (标量) */
    int32_t g_misc_blk_8ea8;  /* DAT_/PTR_00598ea8 (标量) */
    int32_t g_sprite_buffer[312];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_icon_buffer_a;  /* DAT_/PTR_005d788a (标量) */
    int32_t g_misc_blk_7ca8;  /* DAT_/PTR_005d7ca8 (标量) */
    int32_t g_icon_buffer_b;  /* DAT_/PTR_005d7d6c (标量) */
    int32_t g_misc_blk_7ea8;  /* DAT_/PTR_005d7ea8 (标量) */
    int32_t g_misc_blk_8005;  /* DAT_/PTR_005d8005 (标量) */
    int32_t g_misc_blk_829e;  /* DAT_/PTR_005d829e (标量) */
    int32_t g_misc_blk_8537;  /* DAT_/PTR_005d8537 (标量) */
    int32_t g_misc_blk_85a8;  /* DAT_/PTR_005d85a8 (标量) */
    int32_t g_misc_blk_85ac;  /* DAT_/PTR_005d85ac (标量) */
    int32_t g_misc_blk_89a8;  /* DAT_/PTR_005d89a8 (标量) */
    int32_t g_misc_blk_8dec;  /* DAT_/PTR_005d8dec (标量) */
    int32_t g_misc_blk_962c;  /* DAT_/PTR_005d962c (标量) */
    int32_t g_misc_blk_9ba8;  /* DAT_/PTR_005d9ba8 (标量) */
    int32_t g_misc_blk_9e6c;  /* DAT_/PTR_005d9e6c (标量) */
    int32_t g_misc_blk_a6ac;  /* DAT_/PTR_005da6ac (标量) */
    int32_t g_misc_blk_aeec;  /* DAT_/PTR_005daeec (标量) */
    int32_t g_misc_blk_b72c;  /* DAT_/PTR_005db72c (标量) */
    int32_t g_misc_blk_bf6c;  /* DAT_/PTR_005dbf6c (标量) */
    int32_t g_misc_blk_c205;  /* DAT_/PTR_005dc205 (标量) */
    int32_t g_misc_blk_c49e;  /* DAT_/PTR_005dc49e (标量) */
    int32_t g_misc_blk_c737;  /* DAT_/PTR_005dc737 (标量) */
    int32_t g_misc_blk_f3a8;  /* DAT_/PTR_005df3a8 (标量) */
    int32_t g_misc_blk_9fa8;  /* DAT_/PTR_005e9fa8 (标量) */
    int32_t g_misc_blk_a5e8;  /* DAT_/PTR_005ea5e8 (标量) */
    int32_t g_misc_blk_bc0e;  /* DAT_/PTR_005ebc0e (标量) */
    int32_t g_misc_blk_cba8;  /* DAT_/PTR_005fcba8 (标量) */
    int32_t g_level_step_count;  /* DAT_/PTR_006223a8 (标量) */
    int32_t g_misc_blk_23b0;  /* DAT_/PTR_006223b0 (标量) */
    int32_t g_sprite_frame_buf[18432];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_misc_blk_44b0[92160];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_misc_blk_e4b0[327168];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_flag_block_a[19];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_state_blk_dd00[2352];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_state_blk_01c0;  /* DAT_/PTR_007d01c0 (标量) */
    int32_t* g_palette_fade_rate;  /* DAT_/PTR_007d01c4 (指针/缓冲区，按实际用法判定) */
    int32_t g_save_state_block;  /* DAT_/PTR_007d01c8 (标量) */
    int32_t g_state_blk_01c9[7];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_state_blk_01e8;  /* DAT_/PTR_007d01e8 (标量) */
    int32_t g_state_blk_01ec;  /* DAT_/PTR_007d01ec (标量) */
    int32_t g_state_blk_01f0;  /* DAT_/PTR_007d01f0 (标量) */
    int32_t g_state_blk_01fc;  /* DAT_/PTR_007d01fc (标量) */
    int32_t g_state_blk_0230;  /* DAT_/PTR_007d0230 (标量) */
    int32_t g_state_blk_0270[1];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_state_blk_0274[1];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_state_blk_0278[1];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_state_blk_027c[1];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_state_blk_0280[1];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_state_blk_0284[1];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_state_blk_0288[22];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_state_blk_02e0;  /* DAT_/PTR_007d02e0 (标量) */
    int32_t g_state_blk_02e4[4];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_state_blk_02f4[4];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_state_blk_0304;  /* DAT_/PTR_007d0304 (标量) */
    int32_t g_state_blk_0308;  /* DAT_/PTR_007d0308 (标量) */
    int32_t g_state_blk_030c;  /* DAT_/PTR_007d030c (标量) */
    int32_t g_state_blk_047c[2];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_score;  /* DAT_/PTR_007d0484 (标量) */
    int32_t g_state_blk_0488[130];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_cur_route_x;  /* DAT_/PTR_007d0690 (标量) */
    int32_t g_cur_route_y;  /* DAT_/PTR_007d0694 (标量) */
    int32_t g_state_blk_0698;  /* DAT_/PTR_007d0698 (标量) */
    int32_t g_accum_b;  /* DAT_/PTR_007d069c (标量) */
    int32_t g_fight_index;  /* DAT_/PTR_007d06a0 (标量) */
    int32_t g_side_toggle;  /* DAT_/PTR_007d06a8 (标量) */
    int32_t g_state_blk_06ac;  /* DAT_/PTR_007d06ac (标量) */
    int32_t g_sprite_render_flag;  /* DAT_/PTR_007d06b8 (标量) */
    int32_t g_state_blk_06bc;  /* DAT_/PTR_007d06bc (标量) */
    int32_t g_delay_counter;  /* DAT_/PTR_007d06c0 (标量) */
    int32_t g_level_grid_block;  /* DAT_/PTR_007d06c8 (标量) */
    int32_t g_state_blk_06d0;  /* DAT_/PTR_007d06d0 (标量) */
    int32_t g_state_blk_06d4;  /* DAT_/PTR_007d06d4 (标量) */
    int32_t g_state_blk_06d8;  /* DAT_/PTR_007d06d8 (标量) */
    int32_t g_state_blk_06dc;  /* DAT_/PTR_007d06dc (标量) */
    int32_t g_state_blk_06e0;  /* DAT_/PTR_007d06e0 (标量) */
    int32_t g_state_blk_06ec;  /* DAT_/PTR_007d06ec (标量) */
    int32_t g_state_blk_06f0;  /* DAT_/PTR_007d06f0 (标量) */
    int32_t g_state_blk_06f4;  /* DAT_/PTR_007d06f4 (标量) */
    int32_t g_state_blk_0704;  /* DAT_/PTR_007d0704 (标量) */
    int32_t g_event_state;  /* DAT_/PTR_007d0708 (标量) */
    int32_t g_state_blk_0718;  /* DAT_/PTR_007d0718 (标量) */
    int32_t g_state_blk_071c;  /* DAT_/PTR_007d071c (标量) */
    int32_t g_state_blk_072c;  /* DAT_/PTR_007d072c (标量) */
    int32_t g_state_blk_0730;  /* DAT_/PTR_007d0730 (标量) */
    int32_t g_state_blk_0740;  /* DAT_/PTR_007d0740 (标量) */
    int32_t g_state_blk_0744;  /* DAT_/PTR_007d0744 (标量) */
    int32_t g_state_blk_0754;  /* DAT_/PTR_007d0754 (标量) */
    int32_t g_state_blk_0758;  /* DAT_/PTR_007d0758 (标量) */
    int32_t g_state_blk_0768;  /* DAT_/PTR_007d0768 (标量) */
    int32_t g_state_blk_076c;  /* DAT_/PTR_007d076c (标量) */
    int32_t g_state_blk_077c;  /* DAT_/PTR_007d077c (标量) */
    int32_t g_state_blk_0780;  /* DAT_/PTR_007d0780 (标量) */
    int32_t g_state_blk_0790;  /* DAT_/PTR_007d0790 (标量) */
    int32_t g_state_blk_0794;  /* DAT_/PTR_007d0794 (标量) */
    int32_t g_state_blk_07a4;  /* DAT_/PTR_007d07a4 (标量) */
    int32_t g_state_blk_07a8;  /* DAT_/PTR_007d07a8 (标量) */
    int32_t g_state_blk_07b8;  /* DAT_/PTR_007d07b8 (标量) */
    int32_t g_state_blk_07bc;  /* DAT_/PTR_007d07bc (标量) */
    int32_t g_state_blk_07cc;  /* DAT_/PTR_007d07cc (标量) */
    int32_t g_state_blk_07d0;  /* DAT_/PTR_007d07d0 (标量) */
    int32_t g_state_blk_07e0;  /* DAT_/PTR_007d07e0 (标量) */
    int32_t g_state_blk_07e4;  /* DAT_/PTR_007d07e4 (标量) */
    int32_t g_state_blk_07f4;  /* DAT_/PTR_007d07f4 (标量) */
    int32_t g_state_blk_07f8;  /* DAT_/PTR_007d07f8 (标量) */
    int32_t g_state_blk_0804;  /* DAT_/PTR_007d0804 (标量) */
    int32_t g_state_blk_0808;  /* DAT_/PTR_007d0808 (标量) */
    int32_t g_state_blk_080c;  /* DAT_/PTR_007d080c (标量) */
    int32_t g_state_blk_081c;  /* DAT_/PTR_007d081c (标量) */
    int32_t g_state_blk_0820;  /* DAT_/PTR_007d0820 (标量) */
    int32_t g_state_blk_0830;  /* DAT_/PTR_007d0830 (标量) */
    int32_t g_fight_index_saved;  /* DAT_/PTR_007d0834 (标量) */
    int32_t g_palette_scratch;  /* DAT_/PTR_007d0848 (标量) */
    int32_t g_state_blk_0849;  /* DAT_/PTR_007d0849 (标量) */
    int32_t g_state_blk_084a;  /* DAT_/PTR_007d084a (标量) */
    int32_t g_state_blk_084b[255];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_camera_x;  /* DAT_/PTR_007d0c48 (标量) */
    int32_t g_camera_y;  /* DAT_/PTR_007d0c4c (标量) */
    int32_t g_state_blk_0c50[40];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_substate;  /* DAT_/PTR_007d0cf0 (标量) */
    int32_t g_anim_timer;  /* DAT_/PTR_007d0cf4 (标量) */
    int32_t g_state_blk_0cf8;  /* DAT_/PTR_007d0cf8 (标量) */
    int32_t g_state_blk_0cfa;  /* DAT_/PTR_007d0cfa (标量) */
    int32_t g_state_blk_0cfc;  /* DAT_/PTR_007d0cfc (标量) */
    int32_t g_state_blk_0d00;  /* DAT_/PTR_007d0d00 (标量) */
    int32_t g_state_blk_0d04;  /* DAT_/PTR_007d0d04 (标量) */
    int32_t g_state_blk_0d06;  /* DAT_/PTR_007d0d06 (标量) */
    int32_t g_state_blk_0d08;  /* DAT_/PTR_007d0d08 (标量) */
    int32_t g_state_blk_0d10;  /* DAT_/PTR_007d0d10 (标量) */
    int32_t g_state_blk_0d14;  /* DAT_/PTR_007d0d14 (标量) */
    int32_t g_state_blk_0d18;  /* DAT_/PTR_007d0d18 (标量) */
    int32_t g_state_blk_0d1c;  /* DAT_/PTR_007d0d1c (标量) */
    int32_t g_state_blk_0d20;  /* DAT_/PTR_007d0d20 (标量) */
    int32_t g_state_blk_0d24;  /* DAT_/PTR_007d0d24 (标量) */
    int32_t g_state_blk_0d28;  /* DAT_/PTR_007d0d28 (标量) */
    int32_t g_state_blk_0d2c;  /* DAT_/PTR_007d0d2c (标量) */
    int32_t g_state_blk_0d30;  /* DAT_/PTR_007d0d30 (标量) */
    int32_t g_state_blk_0d34;  /* DAT_/PTR_007d0d34 (标量) */
    int32_t g_state_blk_0d38;  /* DAT_/PTR_007d0d38 (标量) */
    int32_t g_state_blk_0d3c;  /* DAT_/PTR_007d0d3c (标量) */
    int32_t g_state_blk_0d40;  /* DAT_/PTR_007d0d40 (标量) */
    int32_t g_state_blk_0d44;  /* DAT_/PTR_007d0d44 (标量) */
    int32_t g_state_blk_0d48;  /* DAT_/PTR_007d0d48 (标量) */
    int32_t g_state_blk_0d4c;  /* DAT_/PTR_007d0d4c (标量) */
    int32_t g_state_blk_0d50;  /* DAT_/PTR_007d0d50 (标量) */
    int32_t g_state_blk_0d54;  /* DAT_/PTR_007d0d54 (标量) */
    int32_t g_state_blk_0d58;  /* DAT_/PTR_007d0d58 (标量) */
    int32_t g_state_blk_0d5c;  /* DAT_/PTR_007d0d5c (标量) */
    int32_t g_state_blk_0d60;  /* DAT_/PTR_007d0d60 (标量) */
    int32_t g_state_blk_0d64;  /* DAT_/PTR_007d0d64 (标量) */
    int32_t g_state_blk_0d68;  /* DAT_/PTR_007d0d68 (标量) */
    int32_t g_state_blk_0d6c;  /* DAT_/PTR_007d0d6c (标量) */
    int32_t g_state_blk_0d70;  /* DAT_/PTR_007d0d70 (标量) */
    int32_t g_state_blk_0d74;  /* DAT_/PTR_007d0d74 (标量) */
    int32_t g_state_blk_0d78;  /* DAT_/PTR_007d0d78 (标量) */
    int32_t g_state_blk_0d7c;  /* DAT_/PTR_007d0d7c (标量) */
    int32_t g_state_blk_0d80;  /* DAT_/PTR_007d0d80 (标量) */
    int32_t g_state_blk_0d84;  /* DAT_/PTR_007d0d84 (标量) */
    int32_t g_state_blk_0d88;  /* DAT_/PTR_007d0d88 (标量) */
    int32_t g_state_blk_0d8c;  /* DAT_/PTR_007d0d8c (标量) */
    int32_t g_state_blk_0d90;  /* DAT_/PTR_007d0d90 (标量) */
    int32_t g_state_blk_0dbc[12];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_state_blk_0dec[12];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_state_blk_0e1c[13];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_state_blk_0e50;  /* DAT_/PTR_007d0e50 (标量) */
    int32_t g_state_blk_0e54;  /* DAT_/PTR_007d0e54 (标量) */
    int32_t g_state_blk_0e58;  /* DAT_/PTR_007d0e58 (标量) */
    int32_t g_state_blk_0e60;  /* DAT_/PTR_007d0e60 (标量) */
    int32_t* g_ddraw_surface_0e64;  /* DAT_/PTR_007d0e64 (指针/缓冲区，按实际用法判定) */
    int32_t g_state_blk_0e68;  /* DAT_/PTR_007d0e68 (标量) */
    int32_t g_state_blk_0e6c;  /* DAT_/PTR_007d0e6c (标量) */
    int32_t g_state_blk_0e70[26];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_state_blk_0ed8[4232];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_battle_end_flag;  /* DAT_/PTR_007d50f8 (标量) */
    int32_t* g_ddraw_surface_50fc;  /* DAT_/PTR_007d50fc (指针/缓冲区，按实际用法判定) */
    int32_t g_state_blk_5100;  /* DAT_/PTR_007d5100 (标量) */
    int32_t g_state_blk_5104;  /* DAT_/PTR_007d5104 (标量) */
    int32_t g_flag_c;  /* DAT_/PTR_007d5108 (标量) */
    int32_t g_state_blk_510c;  /* DAT_/PTR_007d510c (标量) */
    int32_t g_state_blk_5110;  /* DAT_/PTR_007d5110 (标量) */
    int32_t g_state_blk_5114;  /* DAT_/PTR_007d5114 (标量) */
    int32_t g_state_blk_5118;  /* DAT_/PTR_007d5118 (标量) */
    int32_t g_state_blk_5120[32];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t* g_state_blk_51a0;  /* DAT_/PTR_007d51a0 (指针/缓冲区，按实际用法判定) */
    int32_t g_people_129_class_type;  /* DAT_/PTR_007d51a8 (标量) */
    int32_t g_people_129_skill_huogong[1];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_people_129_skill_gunmu[1];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_people_129_skill_leishi[1];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_people_129_skill_weishe[1];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_people_129_skill_saorao[1];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_people_129_skill_pohuai[1];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_people_129_skill_lueduo[1];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_people_129_skill_fanjian[1];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_people_129_skill_yibing[16];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_people_130_strength[2];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_people_130_level[4193];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_fighter_array_back;  /* DAT_/PTR_007d93a8 (标量) */
    int32_t g_state_blk_b3a6;  /* DAT_/PTR_007db3a6 (标量) */
    int32_t g_warehouse_items_battle[80];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_current_mode;  /* DAT_/PTR_007db4e8 (标量) */
    int32_t g_main_wnd;  /* DAT_/PTR_007db4ec (标量) */
    int32_t* g_ddraw_surface_b4f0;  /* DAT_/PTR_007db4f0 (指针/缓冲区，按实际用法判定) */
    uint8_t* g_state_blk_b4f4;  /* DAT_/PTR_007db4f4 (surface 指针; 原版指向 0x004744a0, replica 指向 g_surf_b4f4) */
    int32_t g_state_blk_b4f8[32];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_flow_phase;  /* DAT_/PTR_007db578 (标量) */
    int32_t g_state_blk_b580;  /* DAT_/PTR_007db580 (标量) */
    int32_t g_offscreen_buffer;  /* DAT_/PTR_007e7df0 (标量) */
    int32_t g_misc_blk_a9f0;  /* DAT_/PTR_007ea9f0 (标量) */
    int32_t g_misc_blk_8b80;  /* DAT_/PTR_007f8b80 (标量) */
    int32_t g_misc_blk_1248;  /* DAT_/PTR_00801248 (标量) */
    int32_t g_misc_blk_9910;  /* DAT_/PTR_00809910 (标量) */
    int32_t g_misc_blk_d5f0;  /* DAT_/PTR_0080d5f0 (标量) */
    int32_t g_misc_blk_20f0;  /* DAT_/PTR_008120f0 (标量) */
    int32_t g_aux_counter_a;  /* DAT_/PTR_008331f0 (标量) */
    int32_t g_buf_blk_31f4;  /* DAT_/PTR_008331f4 (标量) */
    int32_t g_aux_counter_b;  /* DAT_/PTR_008331f8 (标量) */
    int32_t g_view_offset;  /* DAT_/PTR_008331fc (标量) */
    int32_t g_tile_index;  /* DAT_/PTR_00833200 (标量) */
    int32_t g_buf_blk_3208;  /* DAT_/PTR_00833208 (标量) */
    int32_t g_buf_blk_320c[126];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_buf_blk_3404;  /* DAT_/PTR_00833404 (标量) */
    int32_t g_scan_row;  /* DAT_/PTR_00833408 (标量) */
    int32_t g_buf_blk_340c;  /* DAT_/PTR_0083340c (标量) */
    int32_t g_scan_col;  /* DAT_/PTR_00833410 (标量) */
    uint8_t g_grid_833414[MAP_GRID_CAP];  /* DAT_/PTR_00833414 (9x9 通行/碰撞掩码网格(字节单格), stamp_class_movemask_8970/stamp_preset_movemask_8a70/stamp_collision_9x9 写入; 容量见 MAP_GRID_CAP。证据 FUN_00418970.c:19/FUN_00418a70.c:19/FUN_00418b79.c(原版裸地址 0x833414, 收口为 &g_grid_833414[0])) */
    int32_t g_pathfind_grid[4096];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    uint8_t* g_buf_blk_7418;  /* DAT_/PTR_00837418 (surface 指针; 原版指向 0x00474fa0, replica 指向 g_surf_7418) */
    int32_t g_buf_blk_741c;  /* DAT_/PTR_0083741c (标量) */
    int32_t g_buf_blk_7420[294912];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    intptr_t* g_block_ptr_table;  /* DAT_/PTR_00957420 (指针表：独立缓冲指针，脱离扁平布局) */
    int32_t g_aux_blk_7424;  /* DAT_/PTR_00957424 (标量) */
    int32_t g_aux_blk_7428;  /* DAT_/PTR_00957428 (标量) */
    int32_t g_aux_blk_742c;  /* DAT_/PTR_0095742c (标量) */
    int32_t g_aux_blk_7430;  /* DAT_/PTR_00957430 (标量) */
    int32_t g_aux_blk_7434;  /* DAT_/PTR_00957434 (标量) */
    int32_t g_aux_blk_7460;  /* DAT_/PTR_00957460 (标量) */
    int32_t g_aux_blk_7464;  /* DAT_/PTR_00957464 (标量) */
    int32_t g_aux_blk_7468;  /* DAT_/PTR_00957468 (标量) */
    int32_t g_aux_blk_7470[3600];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_aux_blk_acb0;  /* DAT_/PTR_0095acb0 (标量) */
    int32_t g_aux_blk_e4f0;  /* DAT_/PTR_0095e4f0 (标量) */
    int32_t g_aux_blk_e4f4;  /* DAT_/PTR_0095e4f4 (标量) */
    int32_t g_aux_blk_e4f8[384];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_flag_block_b;  /* DAT_/PTR_0095eaf8 (标量) */
    int32_t g_aux_blk_eafc;  /* DAT_/PTR_0095eafc (标量) */
    int32_t g_aux_blk_eb00;  /* DAT_/PTR_0095eb00 (标量) */
    int32_t g_aux_blk_eb04;  /* DAT_/PTR_0095eb04 (标量) */
    int32_t g_aux_blk_eb08;  /* DAT_/PTR_0095eb08 (标量) */
    int32_t g_aux_blk_eb0c;  /* DAT_/PTR_0095eb0c (标量) */
    int32_t g_aux_blk_eb10;  /* DAT_/PTR_0095eb10 (标量) */
    int32_t g_aux_blk_eb14;  /* DAT_/PTR_0095eb14 (标量) */
    int32_t g_aux_blk_eb18;  /* DAT_/PTR_0095eb18 (标量) */
    int32_t g_aux_blk_eb1c;  /* DAT_/PTR_0095eb1c (标量) */
    int32_t g_aux_blk_eb20;  /* DAT_/PTR_0095eb20 (标量) */
    int32_t g_aux_blk_eb24;  /* DAT_/PTR_0095eb24 (标量) */
    int32_t g_aux_blk_eb28;  /* DAT_/PTR_0095eb28 (标量) */
    int32_t g_aux_blk_eb2c;  /* DAT_/PTR_0095eb2c (标量) */
    int32_t g_aux_blk_eb2e;  /* DAT_/PTR_0095eb2e (标量) */
    int32_t g_viewport_num_buf[20];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_flow_state;  /* DAT_/PTR_0095eb80 (标量) */
    int32_t g_class_flag_table[3840];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_audio_phase;  /* DAT_/PTR_00962788 (标量) */
    int32_t g_tbl_blk_278e;  /* DAT_/PTR_0096278e (标量) */
    uint8_t g_tile_number_grid[65536];  /* DAT_/PTR_00962790 (地图数据块：每格2字节网格 uint8_t[65536]，RE 实证消除 int32 4× 过跨距) */
    uint8_t g_tile_overlay_grid[65536];  /* DAT_/PTR_00962791 (地图数据块：每格2字节网格 uint8_t[65536]，RE 实证消除 int32 4× 过跨距) */
    int32_t g_tbl_blk_2792[5];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_tbl_blk_27a7[10];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_tbl_blk_27d0;  /* DAT_/PTR_009627d0 (标量) */
    int32_t g_tbl_blk_2811[8159];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_palette_buf;  /* DAT_/PTR_0096a790 (标量) */
    int32_t g_tbl_blk_a791;  /* DAT_/PTR_0096a791 (标量) */
    int32_t g_tbl_blk_a792;  /* DAT_/PTR_0096a792 (标量) */
    int32_t g_tbl_blk_a793[255];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_audio_handle;  /* DAT_/PTR_0096cb90 (标量) */
    int32_t g_tbl_blk_cb98[4];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_tbl_blk_cba8[5884];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_misc_blk_2798[672];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_misc_blk_3218[168];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_misc_blk_34b8[672];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_misc_blk_3f38[672];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_misc_blk_49b8;  /* DAT_/PTR_009749b8 (标量) */
    int32_t g_misc_blk_6bd8;  /* DAT_/PTR_00976bd8 (标量) */
    int32_t g_misc_blk_6be0[23040];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_cursor_d3e0;  /* DAT_/PTR_0098d3e0 (标量) */
    int32_t g_ui_blk_d3e4;  /* DAT_/PTR_0098d3e4 (标量) */
    int32_t g_ui_blk_d3e8;  /* DAT_/PTR_0098d3e8 (标量) */
    int32_t g_ui_blk_d3f0[2];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_ui_blk_d3f8;  /* DAT_/PTR_0098d3f8 (标量) */
    int32_t g_ui_blk_d3fa;  /* DAT_/PTR_0098d3fa (标量) */
    int32_t g_ui_blk_d3fc;  /* DAT_/PTR_0098d3fc (标量) */
    int32_t g_ui_blk_d3fe;  /* DAT_/PTR_0098d3fe (标量) */
    int32_t g_ui_blk_d400;  /* DAT_/PTR_0098d400 (标量) */
    int32_t g_ui_blk_d404;  /* DAT_/PTR_0098d404 (标量) */
    int32_t g_play_time;  /* DAT_/PTR_0098d408 (标量) */
    int32_t g_play_time_tick;  /* DAT_/PTR_0098d40c (标量) */
    int32_t g_ui_blk_d410;  /* DAT_/PTR_0098d410 (标量) */
    int32_t g_ui_blk_d640;  /* DAT_/PTR_0098d640 (标量) */
    int32_t g_mci_device_d644;  /* DAT_/PTR_0098d644 (标量) */
    int32_t g_ui_blk_d648;  /* DAT_/PTR_0098d648 (标量) */
    int32_t g_ui_blk_d74c;  /* DAT_/PTR_0098d74c (标量) */
    int32_t g_ui_blk_d750;  /* DAT_/PTR_0098d750 (标量) */
    int32_t g_hwnd_d754;  /* DAT_/PTR_0098d754 (标量) */
    int32_t g_ui_blk_d758;  /* DAT_/PTR_0098d758 (标量) */
    int32_t g_ui_blk_d75e;  /* DAT_/PTR_0098d75e (标量) */
    int32_t g_ui_blk_d760;  /* DAT_/PTR_0098d760 (标量) */
    int32_t g_ui_blk_d770;  /* DAT_/PTR_0098d770 (标量) */
    int32_t g_ui_blk_d776;  /* DAT_/PTR_0098d776 (标量) */
    int32_t g_ui_blk_d7f4;  /* DAT_/PTR_0098d7f4 (标量) */
    int32_t g_ui_blk_d7f8;  /* DAT_/PTR_0098d7f8 (标量) */
    int32_t g_ui_blk_d7fc;  /* DAT_/PTR_0098d7fc (标量) */
    int32_t g_ui_blk_d800;  /* DAT_/PTR_0098d800 (标量) */
    int32_t g_ui_blk_d808;  /* DAT_/PTR_0098d808 (标量) */
    int32_t g_ui_blk_d80c;  /* DAT_/PTR_0098d80c (标量) */
    int32_t g_ui_blk_d810;  /* DAT_/PTR_0098d810 (标量) */
    int32_t g_ui_blk_d814;  /* DAT_/PTR_0098d814 (标量) */
    int32_t* g_ui_blk_d818;  /* DAT_/PTR_0098d818 (指针/缓冲区，按实际用法判定) */
    int32_t g_ui_blk_d820;  /* DAT_/PTR_0098d820 (标量) */
    int32_t g_ui_blk_d824;  /* DAT_/PTR_0098d824 (标量) */
    int32_t g_scratch_int_a;  /* DAT_/PTR_0098d828 (标量) */
    int32_t g_ui_blk_d82c;  /* DAT_/PTR_0098d82c (标量) */
    int32_t g_ui_blk_d840;  /* DAT_/PTR_0098d840 (标量) */
    int32_t g_ui_blk_d844;  /* DAT_/PTR_0098d844 (标量) */
    int32_t g_ui_blk_d84c;  /* DAT_/PTR_0098d84c (标量) */
    int32_t g_ui_blk_d85c;  /* DAT_/PTR_0098d85c (标量) */
    int32_t g_ui_blk_d864;  /* DAT_/PTR_0098d864 (标量) */
    int32_t g_ui_blk_d868;  /* DAT_/PTR_0098d868 (标量) */
    int32_t g_ui_blk_d86c;  /* DAT_/PTR_0098d86c (标量) */
    int32_t g_ui_blk_d878;  /* DAT_/PTR_0098d878 (标量) */
    int32_t g_ui_blk_d980;  /* DAT_/PTR_0098d980 (标量) */
    int32_t g_ui_blk_d988;  /* DAT_/PTR_0098d988 (标量) */
    int32_t g_ui_blk_d989[64];  /* 正确建模：int32 数组（RE 定尺寸/gap 尺寸，消除 &+i*stride UB） */
    int32_t g_ui_blk_da8c;  /* DAT_/PTR_0098da8c (标量) */
    int32_t g_ui_blk_da90;  /* DAT_/PTR_0098da90 (标量) */
    int32_t g_ui_blk_da98;  /* DAT_/PTR_0098da98 (标量) */
    int32_t g_ui_blk_da9c;  /* DAT_/PTR_0098da9c (标量) */
    int32_t g_ui_blk_daa0;  /* DAT_/PTR_0098daa0 (标量) */
    int32_t g_ui_blk_daa4;  /* DAT_/PTR_0098daa4 (标量) */
    ghidra_func_t* g_ui_blk_daa8;  /* DAT_/PTR_0098daa8 (函数指针，(*gs)(...) 调用) */
    int32_t g_ui_blk_daac;  /* DAT_/PTR_0098daac (标量) */
    int32_t g_ui_blk_dab0;  /* DAT_/PTR_0098dab0 (标量) */
    int32_t g_ui_blk_dab4;  /* DAT_/PTR_0098dab4 (标量) */
    int32_t g_ui_blk_dabc;  /* DAT_/PTR_0098dabc (标量) */
    int32_t g_ui_blk_dac8;  /* DAT_/PTR_0098dac8 (标量) */
    int32_t g_ui_blk_dad8;  /* DAT_/PTR_0098dad8 (标量) */
    ghidra_func_t* g_ui_blk_dadc;  /* DAT_/PTR_0098dadc (函数指针，(*gs)(...) 调用) */
    ghidra_func_t* g_ui_blk_dae0;  /* DAT_/PTR_0098dae0 (函数指针，(*gs)(...) 调用) */
    ghidra_func_t* g_ui_blk_dae4;  /* DAT_/PTR_0098dae4 (函数指针，(*gs)(...) 调用) */
    int32_t g_ui_blk_dae8;  /* DAT_/PTR_0098dae8 (标量) */
    int32_t g_ui_blk_daec;  /* DAT_/PTR_0098daec (标量) */
    ghidra_func_t* g_ui_blk_daf0;  /* DAT_/PTR_0098daf0 (函数指针，(*gs)(...) 调用) */
    int32_t* g_ui_blk_daf4;  /* DAT_/PTR_0098daf4 (指针/缓冲区，按实际用法判定) */
    int32_t g_ui_blk_eb00;  /* DAT_/PTR_0098eb00 (标量) */
    int32_t g_ui_blk_eb04;  /* DAT_/PTR_0098eb04 (标量) */
    int32_t* g_flag_bitmap;  /* DAT_/PTR_0098eb20 (指针/缓冲区，按实际用法判定) */
    int32_t g_ui_blk_eb24;  /* DAT_/PTR_0098eb24 (标量) */
    int32_t g_block_cursor;  /* DAT_/PTR_0098ec20 (标量) */
    int32_t g_ui_blk_ec24;  /* DAT_/PTR_0098ec24 (标量) */
    int32_t g_ui_blk_ec28;  /* DAT_/PTR_0098ec28 (标量) */
    ghidra_func_t* g_ui_blk_ec2c;  /* DAT_/PTR_0098ec2c (函数指针，(*gs)(...) 调用) */
    int32_t* g_ui_blk_ec30;  /* DAT_/PTR_0098ec30 (指针/缓冲区，按实际用法判定) */
    Fighter fighter_array[128];  /* 结构化区域 Fighter（1:1 数组） */
    People people_array[128];  /* 结构化区域 People（1:1 数组） */
    ClassType classtype_array[96];  /* 结构化区域 ClassType（1:1 数组） */
    Skill skill_array[16];  /* 结构化区域 Skill（1:1 数组） */
} GameState;
#define g_rtstate_b254 g_rtstate_b250[1]  /* 正确建模：字段别名 -> 主数组[slot] */
#define g_rtstate_b258 g_rtstate_b250[2]  /* 正确建模：字段别名 -> 主数组[slot] */
#define g_rtstate_b260 g_rtstate_b250[3]  /* 正确建模：字段别名 -> 主数组[slot] */
extern GameState g_game_state;  /* 全局状态单一实例（收口 owner） */
extern intptr_t g_block_ptr_table_heap[16];  /* 任务 B 完整版：g_block_ptr_table 的独立后备缓冲（脱离扁平布局，RE: FUN_0041f6b9 最大下标 15） */

/* 字符串/数据标签：独立 const char[]（C 不允许零长数组成员） */
extern const char g_str_ascf16_6440[];  /* 字符串/数据标签 00446440 (转录占位，待接真实数据) */
extern const char g_str_hzk16_6448[];  /* 字符串/数据标签 00446448 (转录占位，待接真实数据) */
extern const char g_str_asc16_6450[];  /* 字符串/数据标签 00446450 (转录占位，待接真实数据) */
extern const char g_str_cdaudio_6464[];  /* 字符串/数据标签 00446464 (转录占位，待接真实数据) */
extern const char g_str_train_flc_69f4[];  /* 字符串/数据标签 004469f4 (转录占位，待接真实数据) */
extern const char g_str_hiscore_dat_6a0c[];  /* 字符串/数据标签 00446a0c (转录占位，待接真实数据) */
extern const char g_str_hiscore_dat_6a1c[];  /* 字符串/数据标签 00446a1c (转录占位，待接真实数据) */
extern const char g_str_end00_img_6a28[];  /* 字符串/数据标签 00446a28 (转录占位，待接真实数据) */
extern const char g_str_end01_img_6a34[];  /* 字符串/数据标签 00446a34 (转录占位，待接真实数据) */
extern const char g_str_end02_img_6a40[];  /* 字符串/数据标签 00446a40 (转录占位，待接真实数据) */
extern const char g_str_end03_img_6a4c[];  /* 字符串/数据标签 00446a4c (转录占位，待接真实数据) */
extern const char g_str_end04_img_6a58[];  /* 字符串/数据标签 00446a58 (转录占位，待接真实数据) */
extern const char g_str_end05_img_6a64[];  /* 字符串/数据标签 00446a64 (转录占位，待接真实数据) */
extern const char g_str_jz_1_img_6a70[];  /* 字符串/数据标签 00446a70 (转录占位，待接真实数据) */
extern const char g_str_jz_2_img_6a7c[];  /* 字符串/数据标签 00446a7c (转录占位，待接真实数据) */
extern const char g_str_face_dat_6a98[];  /* 字符串/数据标签 00446a98 (转录占位，待接真实数据) */
extern const char g_str_face_dat_6aa4[];  /* 字符串/数据标签 00446aa4 (转录占位，待接真实数据) */
extern const char g_str_circle_tga_6afc[];  /* 字符串/数据标签 00446afc (转录占位，待接真实数据) */
extern const char g_str_proc_break_6b08[];  /* 字符串/数据标签 00446b08 (转录占位，待接真实数据) */
extern const char g_str_break_it_6b18[];  /* 字符串/数据标签 00446b18 (转录占位，待接真实数据) */
extern const char g_str_thread_created_6b28[];  /* 字符串/数据标签 00446b28 (转录占位，待接真实数据) */
extern const char g_str_mine00_anm_6b38[];  /* 字符串/数据标签 00446b38 (转录占位，待接真实数据) */
extern const char g_str_mine01_anm_6b44[];  /* 字符串/数据标签 00446b44 (转录占位，待接真实数据) */
extern const char g_str_mine02_anm_6b50[];  /* 字符串/数据标签 00446b50 (转录占位，待接真实数据) */
extern const char g_str_mine03_anm_6b5c[];  /* 字符串/数据标签 00446b5c (转录占位，待接真实数据) */
extern const char g_str_mine04_anm_6b68[];  /* 字符串/数据标签 00446b68 (转录占位，待接真实数据) */
extern const char g_str_mine05_anm_6b74[];  /* 字符串/数据标签 00446b74 (转录占位，待接真实数据) */
extern const char g_str_mine06_anm_6b80[];  /* 字符串/数据标签 00446b80 (转录占位，待接真实数据) */
extern const char g_str_mine07_anm_6b8c[];  /* 字符串/数据标签 00446b8c (转录占位，待接真实数据) */
extern const char g_str_mine08_anm_6b98[];  /* 字符串/数据标签 00446b98 (转录占位，待接真实数据) */
extern const char g_str_mine09_anm_6ba4[];  /* 字符串/数据标签 00446ba4 (转录占位，待接真实数据) */
extern const char g_str_map_dat_6bb4[];  /* 字符串/数据标签 00446bb4 (转录占位，待接真实数据) */
extern const char g_str_blk_dat_6bc0[];  /* 字符串/数据标签 00446bc0 (转录占位，待接真实数据) */
extern const char g_str_goods_dat_6bcc[];  /* 字符串/数据标签 00446bcc (转录占位，待接真实数据) */
extern const char g_str_menu02_img_6bd8[];  /* 字符串/数据标签 00446bd8 (转录占位，待接真实数据) */
extern const char g_str_goods_dat_6c38[];  /* 字符串/数据标签 00446c38 (转录占位，待接真实数据) */
extern const char g_str_menu02_img_6c48[];  /* 字符串/数据标签 00446c48 (转录占位，待接真实数据) */
extern const char g_str_goods_dat_6c54[];  /* 字符串/数据标签 00446c54 (转录占位，待接真实数据) */
extern const char g_str_menu02_img_6de4[];  /* 字符串/数据标签 00446de4 (转录占位，待接真实数据) */
extern const char g_datatbl_6e64[];  /* 字符串/数据标签 00446e64 (转录占位，待接真实数据) */
extern const char g_str_army_dat_6f08[];  /* 字符串/数据标签 00446f08 (转录占位，待接真实数据) */
extern const char g_str_face_dat_6f14[];  /* 字符串/数据标签 00446f14 (转录占位，待接真实数据) */
extern const char g_str_kuang04_img_6f20[];  /* 字符串/数据标签 00446f20 (转录占位，待接真实数据) */
extern const char g_str_flm_dat_71a4[];  /* 字符串/数据标签 004471a4 (转录占位，待接真实数据) */
extern const char g_str_ffpact_71ac[];  /* 字符串/数据标签 004471ac (转录占位，待接真实数据) */
extern const char g_str_menu08_img_71e8[];  /* 字符串/数据标签 004471e8 (转录占位，待接真实数据) */
extern const char g_str_goods_dat_71f4[];  /* 字符串/数据标签 004471f4 (转录占位，待接真实数据) */
extern const char g_str_mpeg_d01_mpg_74f0[];  /* 字符串/数据标签 004474f0 (转录占位，待接真实数据) */
extern const char g_str_mpeg_d36_mpg_7500[];  /* 字符串/数据标签 00447500 (转录占位，待接真实数据) */
extern const char g_str_a_anti_jap_7514[];  /* 字符串/数据标签 00447514 (转录占位，待接真实数据) */
extern const char g_str_dlz_ini_7530[];  /* 字符串/数据标签 00447530 (转录占位，待接真实数据) */
extern const char g_str_game_dat_754c[];  /* 字符串/数据标签 0044754c (转录占位，待接真实数据) */
extern const char g_str_txt_dat_7558[];  /* 字符串/数据标签 00447558 (转录占位，待接真实数据) */
extern const char g_str_mine_img_7560[];  /* 字符串/数据标签 00447560 (转录占位，待接真实数据) */
extern const char g_str_box_img_756c[];  /* 字符串/数据标签 0044756c (转录占位，待接真实数据) */
extern const char g_str_face_dat_7574[];  /* 字符串/数据标签 00447574 (转录占位，待接真实数据) */
extern const char g_str_mine_dat_7580[];  /* 字符串/数据标签 00447580 (转录占位，待接真实数据) */
extern const char g_str_menu03_img_758c[];  /* 字符串/数据标签 0044758c (转录占位，待接真实数据) */
extern const char g_str_button00_tga_7598[];  /* 字符串/数据标签 00447598 (转录占位，待接真实数据) */
extern const char g_str_button01_tga_75a8[];  /* 字符串/数据标签 004475a8 (转录占位，待接真实数据) */
extern const char g_str_frame00_tga_75b8[];  /* 字符串/数据标签 004475b8 (转录占位，待接真实数据) */
extern const char g_str_menu_tga_75c4[];  /* 字符串/数据标签 004475c4 (转录占位，待接真实数据) */
extern const char g_str_dlz00_tga_75d0[];  /* 字符串/数据标签 004475d0 (转录占位，待接真实数据) */
extern const char g_str_dlz01_tga_75dc[];  /* 字符串/数据标签 004475dc (转录占位，待接真实数据) */
extern const char g_str_wave_dat_75e8[];  /* 字符串/数据标签 004475e8 (转录占位，待接真实数据) */
extern const char g_str_kuang04_img_75f4[];  /* 字符串/数据标签 004475f4 (转录占位，待接真实数据) */
extern const char g_str_disk_error_7604[];  /* 字符串/数据标签 00447604 (转录占位，待接真实数据) */
extern const char g_str_font_init_fail_7618[];  /* 字符串/数据标签 00447618 (转录占位，待接真实数据) */
extern const char g_str_memory_alloced_i_762c[];  /* 字符串/数据标签 0044762c (转录占位，待接真实数据) */
extern const char g_str_timer_alloced_in_764c[];  /* 字符串/数据标签 0044764c (转录占位，待接真实数据) */
extern const char g_str_graphics_init_fa_766c[];  /* 字符串/数据标签 0044766c (转录占位，待接真实数据) */
extern const char g_datatbl_76b0[];  /* 字符串/数据标签 004476b0 (转录占位，待接真实数据) */
extern const char g_str_face_dat_78e0[];  /* 字符串/数据标签 004478e0 (转录占位，待接真实数据) */
extern const char g_str_menu09_img_793c[];  /* 字符串/数据标签 0044793c (转录占位，待接真实数据) */
extern const char g_str_town_dat_79b8[];  /* 字符串/数据标签 004479b8 (转录占位，待接真实数据) */
extern const char g_str_fight_dat_79c8[];  /* 字符串/数据标签 004479c8 (转录占位，待接真实数据) */
extern const char g_str_face_dat_79d4[];  /* 字符串/数据标签 004479d4 (转录占位，待接真实数据) */
extern const char g_str_mpeg_d_79e0[];  /* 字符串/数据标签 004479e0 (转录占位，待接真实数据) */
extern const char g_str_kuang04_img_7a00[];  /* 字符串/数据标签 00447a00 (转录占位，待接真实数据) */
extern const char g_str_menu10_img_7a60[];  /* 字符串/数据标签 00447a60 (转录占位，待接真实数据) */
extern const char g_str_mpeg_d01_mpg_7aa4[];  /* 字符串/数据标签 00447aa4 (转录占位，待接真实数据) */
extern const char g_str_mpeg_d36_mpg_7ab4[];  /* 字符串/数据标签 00447ab4 (转录占位，待接真实数据) */
extern const char g_str_mpeg_play_window_7b00[];  /* 字符串/数据标签 00447b00 (转录占位，待接真实数据) */
extern const char g_str_system_ini_7b18[];  /* 字符串/数据标签 00447b18 (转录占位，待接真实数据) */
extern const char g_str_mpegvideo_7b28[];  /* 字符串/数据标签 00447b28 (转录占位，待接真实数据) */
extern const char g_str_error_7b38[];  /* 字符串/数据标签 00447b38 (转录占位，待接真实数据) */
extern const char g_str_mciqtz_drv_7b40[];  /* 字符串/数据标签 00447b40 (转录占位，待接真实数据) */
extern const char g_str_mpegvideo_7b68[];  /* 字符串/数据标签 00447b68 (转录占位，待接真实数据) */
extern const char g_str_kingsoft_flc_7c6c[];  /* 字符串/数据标签 00447c6c (转录占位，待接真实数据) */
extern const char g_str_test01_flc_7c7c[];  /* 字符串/数据标签 00447c7c (转录占位，待接真实数据) */
extern const char g_str_kr_1_flc_7c88[];  /* 字符串/数据标签 00447c88 (转录占位，待接真实数据) */
extern const char g_str_kr_1_flc_7c94[];  /* 字符串/数据标签 00447c94 (转录占位，待接真实数据) */
extern const char g_str_kr_1_flc_7ca0[];  /* 字符串/数据标签 00447ca0 (转录占位，待接真实数据) */
extern const char g_str_bg03_img_7cac[];  /* 字符串/数据标签 00447cac (转录占位，待接真实数据) */
extern const char g_str_mine02_img_7cb8[];  /* 字符串/数据标签 00447cb8 (转录占位，待接真实数据) */
extern const char g_str_menu50_img_7cc4[];  /* 字符串/数据标签 00447cc4 (转录占位，待接真实数据) */
extern const char g_str_menu51_img_7cd0[];  /* 字符串/数据标签 00447cd0 (转录占位，待接真实数据) */
extern const char g_str_menu52_img_7cdc[];  /* 字符串/数据标签 00447cdc (转录占位，待接真实数据) */
extern const char g_str_menu53_img_7ce8[];  /* 字符串/数据标签 00447ce8 (转录占位，待接真实数据) */
extern const char g_str_menu54_img_7cf4[];  /* 字符串/数据标签 00447cf4 (转录占位，待接真实数据) */
extern const char g_str_hiscore_dat_7d0c[];  /* 字符串/数据标签 00447d0c (转录占位，待接真实数据) */
extern const char g_str_bg01_img_7d18[];  /* 字符串/数据标签 00447d18 (转录占位，待接真实数据) */
extern const char g_str_kuang04_img_7d2c[];  /* 字符串/数据标签 00447d2c (转录占位，待接真实数据) */
extern const char g_str_people_dat_7d44[];  /* 字符串/数据标签 00447d44 (转录占位，待接真实数据) */
extern const char g_str_army_dat_7d58[];  /* 字符串/数据标签 00447d58 (转录占位，待接真实数据) */
extern const char g_str_face_dat_7d64[];  /* 字符串/数据标签 00447d64 (转录占位，待接真实数据) */
extern const char g_str_menu01_img_7d70[];  /* 字符串/数据标签 00447d70 (转录占位，待接真实数据) */
extern const char g_str_button22_img_7d7c[];  /* 字符串/数据标签 00447d7c (转录占位，待接真实数据) */
extern const char g_str_menu30_img_7d8c[];  /* 字符串/数据标签 00447d8c (转录占位，待接真实数据) */
extern const char g_str_918_2_img_7db0[];  /* 字符串/数据标签 00447db0 (转录占位，待接真实数据) */
extern const char g_str_918_3_img_7dbc[];  /* 字符串/数据标签 00447dbc (转录占位，待接真实数据) */
extern const char g_str_77_1_img_7dc8[];  /* 字符串/数据标签 00447dc8 (转录占位，待接真实数据) */
extern const char g_str_77_2_img_7dd4[];  /* 字符串/数据标签 00447dd4 (转录占位，待接真实数据) */
extern const char g_str_npic18_img_7de0[];  /* 字符串/数据标签 00447de0 (转录占位，待接真实数据) */
extern const char g_str_npic19_img_7dec[];  /* 字符串/数据标签 00447dec (转录占位，待接真实数据) */
extern const char g_str_npic20_img_7df8[];  /* 字符串/数据标签 00447df8 (转录占位，待接真实数据) */
extern const char g_str_npic21_img_7e04[];  /* 字符串/数据标签 00447e04 (转录占位，待接真实数据) */
extern const char g_str_npic22_img_7e10[];  /* 字符串/数据标签 00447e10 (转录占位，待接真实数据) */
extern const char g_str_npic29_img_7e1c[];  /* 字符串/数据标签 00447e1c (转录占位，待接真实数据) */
extern const char g_str_npic08_img_7e28[];  /* 字符串/数据标签 00447e28 (转录占位，待接真实数据) */
extern const char g_str_npic07_img_7e34[];  /* 字符串/数据标签 00447e34 (转录占位，待接真实数据) */
extern const char g_str_npic06_img_7e40[];  /* 字符串/数据标签 00447e40 (转录占位，待接真实数据) */
extern const char g_str_npic05_img_7e4c[];  /* 字符串/数据标签 00447e4c (转录占位，待接真实数据) */
extern const char g_str_npic04_img_7e58[];  /* 字符串/数据标签 00447e58 (转录占位，待接真实数据) */
extern const char g_str_npic03_img_7e64[];  /* 字符串/数据标签 00447e64 (转录占位，待接真实数据) */
extern const char g_str_npic02_img_7e70[];  /* 字符串/数据标签 00447e70 (转录占位，待接真实数据) */
extern const char g_str_npic01_img_7e7c[];  /* 字符串/数据标签 00447e7c (转录占位，待接真实数据) */
extern const char g_str_npic25_img_7e88[];  /* 字符串/数据标签 00447e88 (转录占位，待接真实数据) */
extern const char g_str_npic25_img_7e94[];  /* 字符串/数据标签 00447e94 (转录占位，待接真实数据) */
extern const char g_str_face_dat_7ec8[];  /* 字符串/数据标签 00447ec8 (转录占位，待接真实数据) */
extern const char g_str_kuang04_img_7ef4[];  /* 字符串/数据标签 00447ef4 (转录占位，待接真实数据) */
extern const char g_str_bg01_img_7f28[];  /* 字符串/数据标签 00447f28 (转录占位，待接真实数据) */
extern const char g_str_zyb_img_7f34[];  /* 字符串/数据标签 00447f34 (转录占位，待接真实数据) */
extern const char g_str_zyb_img_7f3c[];  /* 字符串/数据标签 00447f3c (转录占位，待接真实数据) */
extern const char g_str_zyb_img_7f44[];  /* 字符串/数据标签 00447f44 (转录占位，待接真实数据) */
extern const char g_str_place_dat_7f7c[];  /* 字符串/数据标签 00447f7c (转录占位，待接真实数据) */
extern const char g_str_place_dat_7f88[];  /* 字符串/数据标签 00447f88 (转录占位，待接真实数据) */
extern const char g_str_bg02_img_7fe0[];  /* 字符串/数据标签 00447fe0 (转录占位，待接真实数据) */
extern const char g_str_back19_img_7ff4[];  /* 字符串/数据标签 00447ff4 (转录占位，待接真实数据) */
extern const char g_str_bigmap_img_8014[];  /* 字符串/数据标签 00448014 (转录占位，待接真实数据) */
extern const char g_str_bigmine0_img_8020[];  /* 字符串/数据标签 00448020 (转录占位，待接真实数据) */
extern const char g_str_bigmine1_img_8030[];  /* 字符串/数据标签 00448030 (转录占位，待接真实数据) */
extern const char g_str_k01_img_8048[];  /* 字符串/数据标签 00448048 (转录占位，待接真实数据) */
extern const char g_str_button0_img_8050[];  /* 字符串/数据标签 00448050 (转录占位，待接真实数据) */
extern const char g_str_button1_img_805c[];  /* 字符串/数据标签 0044805c (转录占位，待接真实数据) */
extern const char g_str_button10_img_8068[];  /* 字符串/数据标签 00448068 (转录占位，待接真实数据) */
extern const char g_str_button11_img_8078[];  /* 字符串/数据标签 00448078 (转录占位，待接真实数据) */
extern const char g_str_button12_img_8088[];  /* 字符串/数据标签 00448088 (转录占位，待接真实数据) */
extern const char g_str_button13_img_8098[];  /* 字符串/数据标签 00448098 (转录占位，待接真实数据) */
extern const char g_str_army_dat_80a8[];  /* 字符串/数据标签 004480a8 (转录占位，待接真实数据) */
extern const char g_str_goods_dat_80b4[];  /* 字符串/数据标签 004480b4 (转录占位，待接真实数据) */
extern const char g_str_k01_img_81c4[];  /* 字符串/数据标签 004481c4 (转录占位，待接真实数据) */
extern const char g_str_button0_img_81cc[];  /* 字符串/数据标签 004481cc (转录占位，待接真实数据) */
extern const char g_str_button1_img_81d8[];  /* 字符串/数据标签 004481d8 (转录占位，待接真实数据) */
extern const char g_str_button10_img_81e4[];  /* 字符串/数据标签 004481e4 (转录占位，待接真实数据) */
extern const char g_str_button11_img_81f4[];  /* 字符串/数据标签 004481f4 (转录占位，待接真实数据) */
extern const char g_str_button12_img_8204[];  /* 字符串/数据标签 00448204 (转录占位，待接真实数据) */
extern const char g_str_button13_img_8214[];  /* 字符串/数据标签 00448214 (转录占位，待接真实数据) */
extern const char g_str_army_dat_8224[];  /* 字符串/数据标签 00448224 (转录占位，待接真实数据) */
extern const char g_str_goods_dat_8230[];  /* 字符串/数据标签 00448230 (转录占位，待接真实数据) */
extern const char g_str_material_dat_823c[];  /* 字符串/数据标签 0044823c (转录占位，待接真实数据) */
extern const char g_str_k03_img_8388[];  /* 字符串/数据标签 00448388 (转录占位，待接真实数据) */
extern const char g_str_material_dat_8390[];  /* 字符串/数据标签 00448390 (转录占位，待接真实数据) */
extern const char g_str_button0_img_83a0[];  /* 字符串/数据标签 004483a0 (转录占位，待接真实数据) */
extern const char g_str_button1_img_83ac[];  /* 字符串/数据标签 004483ac (转录占位，待接真实数据) */
extern const char g_str_button2_img_83b8[];  /* 字符串/数据标签 004483b8 (转录占位，待接真实数据) */
extern const char g_str_button3_img_83c4[];  /* 字符串/数据标签 004483c4 (转录占位，待接真实数据) */
extern const char g_str_button4_img_83d0[];  /* 字符串/数据标签 004483d0 (转录占位，待接真实数据) */
extern const char g_str_button5_img_83dc[];  /* 字符串/数据标签 004483dc (转录占位，待接真实数据) */
extern const char g_str_button6_img_83e8[];  /* 字符串/数据标签 004483e8 (转录占位，待接真实数据) */
extern const char g_str_button7_img_83f4[];  /* 字符串/数据标签 004483f4 (转录占位，待接真实数据) */
extern const char g_str_button8_img_8400[];  /* 字符串/数据标签 00448400 (转录占位，待接真实数据) */
extern const char g_str_button9_img_840c[];  /* 字符串/数据标签 0044840c (转录占位，待接真实数据) */
extern const char g_str_button10_img_8418[];  /* 字符串/数据标签 00448418 (转录占位，待接真实数据) */
extern const char g_str_button11_img_8428[];  /* 字符串/数据标签 00448428 (转录占位，待接真实数据) */
extern const char g_str_button12_img_8438[];  /* 字符串/数据标签 00448438 (转录占位，待接真实数据) */
extern const char g_str_button13_img_8448[];  /* 字符串/数据标签 00448448 (转录占位，待接真实数据) */
extern const char g_str_k04_img_8538[];  /* 字符串/数据标签 00448538 (转录占位，待接真实数据) */
extern const char g_str_button0_img_8540[];  /* 字符串/数据标签 00448540 (转录占位，待接真实数据) */
extern const char g_str_button1_img_854c[];  /* 字符串/数据标签 0044854c (转录占位，待接真实数据) */
extern const char g_str_button10_img_8558[];  /* 字符串/数据标签 00448558 (转录占位，待接真实数据) */
extern const char g_str_button11_img_8568[];  /* 字符串/数据标签 00448568 (转录占位，待接真实数据) */
extern const char g_str_button12_img_8578[];  /* 字符串/数据标签 00448578 (转录占位，待接真实数据) */
extern const char g_str_button13_img_8588[];  /* 字符串/数据标签 00448588 (转录占位，待接真实数据) */
extern const char g_str_army_dat_85b0[];  /* 字符串/数据标签 004485b0 (转录占位，待接真实数据) */
extern const char g_str_army_dat_8614[];  /* 字符串/数据标签 00448614 (转录占位，待接真实数据) */
extern const char g_str_k00_img_86a8[];  /* 字符串/数据标签 004486a8 (转录占位，待接真实数据) */
extern const char g_str_button0_img_86b0[];  /* 字符串/数据标签 004486b0 (转录占位，待接真实数据) */
extern const char g_str_button1_img_86bc[];  /* 字符串/数据标签 004486bc (转录占位，待接真实数据) */
extern const char g_str_dileizan_flc_8700[];  /* 字符串/数据标签 00448700 (转录占位，待接真实数据) */
extern const char g_str_dileizan_flc_8710[];  /* 字符串/数据标签 00448710 (转录占位，待接真实数据) */
extern const char g_str_wave_dat_8760[];  /* 字符串/数据标签 00448760 (转录占位，待接真实数据) */
extern const char g_datatbl_87b0[];  /* 字符串/数据标签 004487b0 (转录占位，待接真实数据) */

/* === 复刻函数声明 === */
int32_t zero_128x20_structs();
int32_t ai_score_distance(int32_t param_1,int32_t param_2,int32_t param_3,int32_t param_4);
int32_t ai_score_power(int32_t param_1,int32_t param_2,int32_t param_3,int32_t param_4,int32_t param_5);
int32_t ai_pick_best_candidate();
int32_t find_nearest_free_cell(int32_t param_1,int32_t param_2,int32_t param_3,uint32_t *param_4,uint32_t *param_5);
int32_t find_best_cell_terrain(int32_t param_1,uint32_t *param_2,uint32_t *param_3);
int32_t people_cond_a(int32_t param_1);
int32_t people_cond_b(int32_t param_1);
uint32_t ai_find_enemy_radius(int32_t param_1,int32_t param_2,int32_t param_3,int32_t param_4);
int32_t ai_find_enemy_unit_wrap(int32_t param_1,int32_t param_2);
int32_t ai_pick_attack_position(int32_t param_1,int32_t param_2);
uint32_t ai_find_enemy_radius_b(int32_t param_1,int32_t param_2,int32_t param_3,int32_t param_4);
int32_t ai_find_enemy_unit_wrap_b(int32_t param_1,int32_t param_2);
int32_t ai_pick_attack_position_b(int32_t param_1,int32_t param_2);
uint32_t ai_find_weaker_enemy_radius(int32_t param_1,int32_t param_2,int32_t param_3,int32_t param_4);
int32_t ai_find_weaker_enemy_wrap(int32_t param_1,int32_t param_2);
int32_t ai_pick_weaker_enemy_pos(int32_t param_1,int32_t param_2);
uint32_t ai_find_friendly_radius(int32_t param_1,int32_t param_2,int32_t param_3);
int32_t ai_find_friendly_wrap(int32_t param_1);
int32_t ai_pick_friendly_pos(int32_t param_1);
int32_t ai_score_enemies_radius(int32_t param_1,int32_t param_2,int32_t param_3);
int32_t ai_score_enemies_power(int32_t param_1,int32_t param_2,int32_t param_3);
int32_t ai_find_unit_radius(int32_t param_1,uint32_t param_2,int32_t param_3,int32_t param_4);
int32_t ai_score_and_pick(int32_t param_1);
int32_t ai_score_pick_wrap(int32_t param_1);
int32_t ai_score_pick_power_wrap(int32_t param_1);
int32_t ai_pick_unit_position(int32_t param_1,int32_t param_2);
int32_t ai_any_unit_flagged();
int32_t collect_active_slot_idx();
int32_t ai_action_dispatch(int32_t param_1,int32_t param_2);
int32_t ai_try_candidate_actions(int32_t param_1);
int32_t ai_unit_state_machine(int32_t param_1);
int32_t ai_apply_action(int32_t param_1);
int32_t draw_box_sized(int32_t param_1);
int32_t ai_process_turn();
int32_t battle_advance_turn();
int32_t file_read_whole(LPCSTR param_1,int32_t *param_2);
int32_t datafile_read_record(int32_t *param_1,int32_t param_2);
int32_t read_record_wrap(int32_t param_1,int32_t param_2);
int32_t decompress_stream(int16_t *param_1,void *param_2);
int32_t handle_close(int32_t *param_1);
int32_t file_read_whole_5070();
int32_t global_alloc();
int32_t glyph_bitmap_load(int32_t param_1);
int32_t glyph_entry_load(int32_t param_1);
int32_t draw_masked_sprite(int32_t param_1, int32_t param_2, char param_3, uint32_t param_4, int32_t param_5, int32_t param_6);
int32_t draw_masked_sprite_16x16(int32_t param_1, int32_t param_2, char param_3, uint32_t param_4, int32_t param_5, int32_t param_6);
int32_t set_text_colors(uint32_t param_1,uint32_t param_2,uint32_t param_3);
int32_t get_state_triple(int32_t *param_1,int32_t *param_2,int32_t *param_3);
int32_t set_global_4fdb4(int32_t param_1);
int32_t get_global_4fdb4(int32_t *param_1);
int32_t set_rect_state(int32_t param_1,int32_t param_2,int32_t param_3,int32_t param_4);
int32_t get_rect_state(int32_t *param_1,int32_t *param_2,int32_t *param_3,int32_t *param_4);
int32_t draw_rect_state(int32_t param_1,int32_t param_2);
int32_t draw_text_string(int32_t param_1,int32_t param_2,char *param_3,int32_t param_4);
int32_t draw_text_color_codes(int32_t param_1,int32_t param_2,char *param_3,int32_t param_4);
int32_t mci_set();
int32_t mci_play(int32_t param_1,int32_t param_2);
int32_t mci_seek();
int32_t mci_close();
int32_t mci_get_status();
int32_t mci_stop();
int32_t mci_send_cmd_wait();
int32_t mci_build_cue_table();
int32_t * get_global_ptr_4fdc0();
int32_t any_unit_flag_loaded();
int32_t any_unit_flag_unloaded();
int32_t battle_intro_sequence();
int32_t dialogue_show_step_0();
int32_t dialogue_show_step_1();
int32_t dialogue_show_step_2();
int32_t dialogue_show_step_3();
int32_t dialogue_show_step_4();
int32_t dialogue_show_step_5();
int32_t dialogue_show_step_6();
int32_t dialogue_show_step_7();
int32_t dialogue_show_step_8();
int32_t dialogue_show_step_9();
int32_t dialogue_show_step_10();
int32_t dialogue_show_step_11();
int32_t dialogue_show_step_12();
int32_t dialogue_show_step_13();
int32_t dialogue_show_step_14();
int32_t dialogue_show_step_15();
int32_t dialogue_show_step_16();
int32_t noop_stub2();
int32_t noop_stub5();
int32_t noop_stub6();
int32_t enter_stage_load_46818();
int32_t enter_stage_load_46830();
int32_t enter_stage_load_4683c();
int32_t enter_stage_load_46854();
int32_t enter_stage_load_46860();
int32_t enter_stage_load_46878();
int32_t enter_stage_load_46890();
int32_t enter_stage_load_468a8();
int32_t enter_stage_load_468cc();
int32_t enter_stage_load_468d8();
int32_t enter_stage_load_468e4();
int32_t enter_stage_load_468f0();
int32_t enter_stage_load_468fc();
int32_t enter_stage_load_46914();
int32_t battle_intro_sequence_2();
int32_t enter_stage_load_46954();
int32_t enter_stage_load_46984();
int32_t noop_stub3();
int32_t noop_stub4();
int32_t battle_event_8d28();
int32_t battle_event_8f67();
int32_t battle_event_907b();
int32_t battle_event_9107();
int32_t battle_event_91da();
int32_t battle_event_9232();
int32_t battle_event_9279();
int32_t battle_event_9307();
int32_t battle_event_9445();
int32_t battle_event_9533();
int32_t battle_event_95bf();
int32_t battle_event_9860();
int32_t battle_event_995b();
int32_t battle_event_9a49();
int32_t battle_event_9ae6();
int32_t battle_event_9ba2();
int32_t battle_event_9c99();
uint32_t battle_event_9ee6();
int32_t battle_event_a0ba();
int32_t battle_event_a209();
int32_t battle_event_a325();
int32_t battle_event_a427();
int32_t battle_event_a4af();
int32_t battle_event_a679();
int32_t battle_event_a79c();
int32_t battle_event_a8b3();
int32_t battle_event_a95c();
int32_t battle_event_a9b9();
int32_t battle_event_aac2();
int32_t battle_event_ac08();
int32_t battle_event_ad9d();
int32_t battle_event_aecf();
int32_t battle_event_af8b();
int32_t battle_event_b017();
int32_t battle_event_b0a3();
int32_t battle_event_b18d();
int32_t battle_event_b2cf();
int32_t count_units_flag_loaded();
int32_t battle_event_b46c();
int32_t battle_event_b695();
int32_t battle_event_bc2f();
int32_t phase1_event_be07();
int32_t phase1_event_be5d();
int32_t phase1_event_beb3();
int32_t phase1_event_c15b();
int32_t phase1_event_c1b1();
int32_t phase1_event_c207();
int32_t phase1_event_c2e1();
int32_t phase1_event_c3e3();
int32_t phase1_event_c439();
int32_t phase1_event_c48f();
int32_t phase1_event_c529();
int32_t phase1_event_c5f0();
int32_t phase1_event_c646();
int32_t phase1_event_c686();
int32_t phase1_event_c717();
int32_t img_train_flc();
int32_t phase1_event_c850();
int32_t phase1_event_c89a();
int32_t phase1_event_c8da();
int32_t phase1_event_c930();
int32_t phase1_event_c970();
int32_t phase1_event_c9bf();
int32_t phase1_event_c9ff();
int32_t phase1_event_ca3f();
int32_t phase1_event_cb09();
int32_t phase1_event_cb49();
int32_t phase1_event_cd78();
int32_t phase1_event_cdb8();
int32_t phase1_event_cdf8();
int32_t phase1_event_ce51();
int32_t phase1_event_ce91();
int32_t phase1_event_cee0();
int32_t phase1_event_cf20();
int32_t phase1_event_cf60();
int32_t phase1_event_cfa0();
int32_t phase1_event_cfe0();
int32_t phase1_event_d020();
int32_t phase1_event_d07e();
int32_t phase1_event_d207();
int32_t phase1_event_d251();
int32_t battle_start_init();
int32_t exit_battle_init();
int32_t exit_battle_init_d418();
int32_t exit_battle_init_d4a5();
int32_t exit_battle_init_d51e();
int32_t exit_battle_init_d5a1();
int32_t exit_battle_init_d624();
int32_t exit_battle_init_d72b();
int32_t exit_battle_init_d7ae();
int32_t exit_battle_init_d827();
int32_t exit_battle_init_d8aa();
int32_t exit_battle_init_d92d();
int32_t exit_battle_init_d9b0();
int32_t battle_reinit_da33();
int32_t exit_battle_init_daa9();
int32_t exit_battle_init_db2c();
int32_t exit_battle_init_dbaf();
int32_t exit_battle_init_dc32();
int32_t exit_battle_init_dcb5();
int32_t exit_battle_init_dd42();
int32_t exit_battle_init_ddc5();
int32_t exit_battle_init_de48();
int32_t exit_battle_init_decb();
int32_t exit_battle_init_df4e();
int32_t exit_battle_init_dfdb();
int32_t exit_battle_init_e05e();
int32_t exit_battle_init_e0eb();
int32_t exit_battle_init_e16e();
int32_t exit_battle_init_e1f1();
int32_t exit_battle_init_e274();
int32_t exit_battle_init_e301();
int32_t exit_battle_init_e384();
int32_t mode37_init_e407();
int32_t mode37_init_e42f();
int32_t exit_battle_init_e457();
int32_t asset_hiscore_dat();
int32_t asset_end00_img();
int32_t victory_ending_show();
int32_t asset_face_dat();
int32_t victory_transition_init_edad();
int32_t victory_transition_init_ee2b();
int32_t victory_transition_init_eea9();
int32_t call_0ea9e_wrapper();
int32_t img_circle_tga(int32_t param_1,int32_t param_2,uint32_t param_3);
int32_t deploy_unit_event_f14b();
int32_t battle_event_f24d();
int32_t battle_event_f364();
int32_t set_endflag_over30();
int32_t set_endflag_over20();
int32_t battle_event_f459();
int32_t battle_event_f4dc();
int32_t set_endflag_over20b();
int32_t set_endflag_over40();
int32_t set_endflag_over30b();
int32_t set_endflag_over30c();
int32_t stage_counter_dispatch();
int32_t dialogue_script_f66b();
int32_t dialogue_script_f687();
int32_t dialogue_script_f788();
int32_t set_endflag_over30d();
int32_t dialogue_script_f7f2();
int32_t dialogue_script_f84b();
int32_t set_endflag_over30e();
int32_t dialogue_script_f966();
int32_t noop_stub7();
int32_t dialogue_script_f9dc();
int32_t dialogue_script_fa0d();
int32_t set_endflag_over30f();
int32_t set_endflag_over30g();
int32_t set_endflag_over30h();
int32_t dialogue_script_fa84();
int32_t set_endflag_over20c();
int32_t set_endflag_over40b();
int32_t dialogue_script_fb2a();
int32_t dialogue_script_fbe8();
int32_t set_endflag_over40c();
int32_t dialogue_script_fc1c();
int32_t set_endflag_over50();
int32_t dialogue_script_fc96();
int32_t dialogue_script_fcc5();
int32_t battle_event_fcf4();
int32_t battle_event_fd23();
int32_t battle_event_fee6();
int32_t battle_event_ffff();
int32_t battle_event_1018e();
int32_t battle_end_eval_10243();
int32_t phase1_event_103e5();
int32_t victory_transition_init_10434();
int32_t battle_event_104ac();
int32_t battle_event_1061f();
int32_t phase1_event_107c6();
int32_t victory_transition_init_1095d();
int32_t battle_event_109d5();
int32_t battle_end_eval_10b99();
int32_t phase1_event_10fdf();
int32_t victory_transition_init_102e();
int32_t battle_phase_event_10a6();
int32_t battle_event_1134f();
int32_t phase1_event_1557();
int32_t call_0ea9e_wrapperb();
int32_t battle_phase_event_15ba();
int32_t select_endflag_value();
int32_t stage1_memset_init();
int32_t exit_battle_init_16e8();
int32_t noop_stub8();
int32_t select_endflag_value2();
int32_t stage1_memset_initb();
int32_t exit_battle_init_17e8();
int32_t noop_stub9();
int32_t count_check_endflag();
int32_t phase1_event_18d9();
int32_t exit_battle_init_1927();
int32_t set_stage2_flag();
int32_t img_proc_break(HWND param_1);
int32_t pulse_sync_event();
int32_t com_uninitialize();
LRESULT main_wnd_proc(HWND param_1,UINT param_2,WPARAM param_3,LPARAM param_4);
int32_t unit_placement_check_11b14(int32_t param_1,int32_t param_2);
int32_t unit_placement_commit_11b6a(int32_t param_1);
int32_t trigger_once_4500ac();
int32_t create_thread(HWND param_1,LPCSTR param_2);
WPARAM message_pump_peek();
int32_t create_window(HINSTANCE param_1);
int32_t send_message();
int32_t fire_pending_callback_wrap();
int32_t surface_blit_11ecd();
int32_t fire_pending_callback();
int32_t input_poll_event();
int32_t wait_single_object();
int32_t draw_unit_sprite_anim_12030(int32_t param_1,int32_t param_2,int32_t param_3,int32_t param_4);
int32_t draw_unit_sprite_anim_123f9(int32_t param_1,int32_t param_2,int32_t param_3,int32_t param_4);
int32_t draw_unit_sprite_anim_127f5(int32_t param_1,int32_t param_2,int32_t param_3,int32_t param_4);
int32_t asset_mine00_anm(int32_t param_1);
int32_t battle_damage_apply_1307f(int32_t param_1);
void battle_damage_sim_138ea(uint32_t param_1);
int32_t battle_damage_sim_15c35(int32_t param_1);
int32_t battle_damage_sim_165cc(int32_t param_1);
int32_t map_dat_load();
int32_t load_map_tiles_16e3c();
int32_t draw_grid_cell_170aa(int32_t param_1,int32_t param_2,int32_t param_3);
int32_t redraw_grid_17547();
int32_t check_box_4corners(int32_t param_1,int32_t param_2,int32_t param_3,int32_t param_4,int32_t param_5,int32_t param_6);
int32_t blit_4edge_17690(int32_t param_1,int32_t param_2,int32_t param_3,int32_t param_4,int32_t param_5,int32_t param_6);
int32_t grid_scroll_1775e(int16_t param_1);
int32_t draw_grid_overlay_17b47();
int32_t file_read_at_417e40(LPCSTR param_1,LPVOID param_2);
int32_t struct_close_handle(int32_t param_1);
int32_t archive_read_17ef0(int32_t param_1,uint32_t *param_2,void *param_3,int32_t param_4);
int32_t archive_read_18102(int32_t param_1,uint32_t *param_2,int32_t param_3,int32_t param_4);
int32_t rle_rgb_expand(char *param_1,int32_t param_2);
int32_t rle_rgb_copy(char *param_1,int32_t param_2);
int32_t rle_byte_expand(uint8_t *param_1,uint8_t *param_2,uint32_t param_3);
int32_t rle_expand_u16(uint16_t *param_1,int32_t param_2,uint32_t param_3);
int32_t rle_expand_block(uint16_t *param_1,int32_t param_2,uint32_t param_3);
int32_t stamp_class_movemask_8970(int32_t param_1,int32_t param_2,int32_t param_3);
int32_t stamp_preset_movemask_8a70(uint32_t param_1,int32_t param_2,int32_t param_3);
int32_t stamp_collision_9x9(int32_t param_1,int32_t param_2,int32_t param_3);
int32_t tile_border_test_8c7c(uint32_t param_1,uint32_t param_2);
int32_t floodfill_move_8e29(uint32_t param_1,uint32_t param_2,uint32_t param_3,int32_t param_4,int32_t param_5,int32_t param_6);
int32_t clear_passable_cells(int32_t param_1,int32_t param_2,int32_t param_3);
int32_t mark_occupancy_cell(int32_t param_1,int32_t param_2,int32_t param_3);
int32_t clear_grid_square(int32_t param_1,int32_t param_2,int32_t param_3);
int32_t mark_occupancy_diamond(int32_t param_1,int32_t param_2,int32_t param_3);
uint32_t tile_movecost_94c0(int32_t param_1,int32_t param_2,int32_t param_3);
int32_t encode_route_directions(uint32_t param_1,int32_t param_2,int32_t param_3);
int32_t route_cost_sum_9742(int32_t param_1,int32_t param_2,int32_t param_3,int32_t param_4);
uint32_t find_min_route_record(uint32_t param_1);
int32_t eval_routes_98e5(int32_t param_1,int32_t param_2,int32_t param_3,int32_t param_4,int32_t param_5);
int32_t grid_cell_table_reset_199f9();
int32_t grid_cell_register_19a2c(int32_t param_1,int32_t param_2);
int32_t restore_grid_cell_table();
int32_t backup_unit_status_19ad3();
int32_t restore_unit_status_19b0d();
int32_t wait_message(uint32_t param_1);
uint32_t threshold_bucket_19ca4(uint32_t param_1);
int32_t draw_unit_panel_19cdf(uint32_t param_1);
int32_t img_menu02_img(uint32_t param_1);
int32_t img_menu02_img_b00e(uint32_t param_1);
int32_t asset_army_dat(uint32_t param_1,int32_t param_2);
int32_t battle_init_deployment();
int32_t img_kuang04_img();
int32_t enter_level_1b8e9();
int32_t tick_count_inc();
int32_t battle_cmd_handler_b9b3(int32_t param_1);
int32_t enqueue_arrow_key(int32_t param_1);
int32_t blit_hud_frame_bb9c();
int32_t grid_scroll_handler_bbfc(int32_t param_1,int32_t param_2);
int32_t unit_anim_tick_bcb8();
int32_t draw_unit_info_panel_be02(int32_t param_1,int32_t param_2);
int32_t clear_unit_info_panel_c5b0();
int32_t draw_logo_screen_c65c();
int32_t draw_unit_stat_panel_c773(int32_t param_1,int32_t param_2,int32_t param_3);
int32_t list_player_units_ca3e();
int32_t list_enemy_units_cc90();
int32_t update_unit_state_cf4e(int32_t param_1);
int32_t draw_help_screen_d03e();
int32_t menu_cmd_handler_d206(int32_t param_1,int32_t param_2);
int32_t grid_viewport_pan_d8d8(int32_t param_1,int32_t param_2);
int32_t system_menu_dispatch(int32_t param_1,int32_t param_2);
int32_t game_tick(uint32_t param_1,uint32_t param_2,uint32_t param_3);
int32_t draw_tile_clear_occ(int32_t param_1,int32_t param_2);
int32_t animate_unit_move(int32_t param_1,int32_t param_2,int32_t param_3,uint32_t param_4);
int32_t update_viewport_scroll(uint32_t param_1,uint32_t param_2);
int32_t move_unit_along_path(int32_t param_1,int32_t *param_2,int32_t *param_3);
int32_t blink_unit_highlight(int32_t param_1);
int32_t load_flm_block(int32_t param_1);
int32_t img_ffpact(int32_t param_1,int32_t param_2,int32_t param_3);
int32_t process_map_cell_events();
int32_t grid_click_select_unit(int32_t param_1,int32_t param_2);
uint32_t build_item_pick_menu(int32_t param_1);
int32_t build_skill_pick_menu(int32_t param_1);
int32_t wait_message_f935(int32_t param_1);
int32_t unit_command_dispatch(int32_t param_1,int32_t param_2);
int32_t strategic_click_move(int32_t param_1,int32_t param_2);
int32_t strategic_click_place_item(int32_t param_1,int32_t param_2);
int32_t cell_clear_feature_20afc(int32_t param_1,int32_t param_2);
int32_t cell_record_terrain_20cc9(int32_t param_1,int32_t param_2);
int32_t cell_set_terrain_20e94(int32_t param_1,int32_t param_2);
int32_t cell_deploy_group_21087(int32_t param_1,int32_t param_2);
int32_t deploy_unit_212a7(int32_t param_1,int32_t param_2);
int32_t deploy_unit_214aa(int32_t param_1,int32_t param_2);
int32_t deploy_unit_21677(int32_t param_1,int32_t param_2);
int32_t deploy_unit_to_feature_2183c(int32_t param_1,int32_t param_2);
int32_t run_bounds_1d(int32_t param_1, int32_t param_2, int32_t param_3, int32_t *param_4, int32_t *param_5, int32_t param_6);
int32_t scan_draw_runs(int32_t *param_1, int32_t *param_2, int32_t param_3, int32_t param_4, int32_t param_5, int32_t param_6);
int32_t scan_extent_bidir(int32_t param_1,int32_t param_2,int32_t param_3,int32_t param_4);
int32_t nearest_color_match_21d10(uint32_t param_1, const uint8_t* param_2);
int32_t palette_find_nearest_ex(uint32_t param_1,int32_t param_2,int32_t param_3);
int32_t remap_bytes_lut(uint8_t* param_1, uint8_t* param_2, uint32_t param_3, uint8_t* param_4);
int32_t translate_bytes_cond(uint8_t *param_1,uint8_t *param_2,char *param_3,uint32_t param_4,int32_t param_5);
int32_t blend_color_to_palette(uint8_t *param_1,uint32_t param_2,int32_t param_3,uint32_t param_4);
int32_t copy_skip_char(char *param_1,char *param_2,int32_t param_3,char param_4);
int32_t copy_int_block(int32_t *param_1,int32_t *param_2,int32_t param_3);
int32_t surface_write_byte_20bb(int32_t param_1,int32_t param_2,uint8_t param_3,int32_t param_4,int32_t *param_5);
int32_t blit_locked_surface(int32_t param_1,int32_t param_2,int32_t *param_3);
int32_t surface_span_rop();
int32_t surface_span_rop2(int32_t param_1,uint32_t param_2,uint32_t param_3,uint8_t param_4,int32_t param_5,int32_t *param_6);
int32_t draw_line_2241f(uint32_t param_1, uint32_t param_2, uint32_t param_3, uint32_t param_4, int32_t param_5, int32_t param_6, int32_t param_7);
int32_t swap_ints(int32_t *param_1,int32_t *param_2);
int32_t rect_test_22571(int32_t param_1, int32_t param_2, int32_t param_3, int32_t param_4, int32_t param_5, int32_t param_6, int32_t param_7);
int32_t ddraw_blt();
int32_t ddraw_blt_4226b6(int32_t param_1, int32_t param_2, int32_t param_3, int32_t param_4, int32_t param_5, int32_t param_6, int32_t param_7, int32_t param_8, int32_t *param_9);
int32_t hit_test_rect_sprite();
int32_t read_surface_pixels(int32_t param_1,int32_t param_2,int32_t param_3,int32_t param_4,uint8_t *param_5,int32_t *param_6);
int32_t surface_blit_22889(int32_t param_1,int32_t param_2,int32_t param_3,int32_t param_4,uint8_t *param_5,int32_t *param_6);
int32_t blit_locked_surface_2951(int32_t param_1,int32_t param_2,int32_t param_3,int32_t param_4,int32_t *param_5,int32_t *param_6);
int32_t blit_locked_surface_29da(int32_t param_1,int32_t param_2,int32_t param_3,int32_t param_4,int32_t *param_5,int32_t *param_6);
int32_t blit_surface_sprite(int32_t param_1, int32_t param_2, int32_t param_3, int32_t param_4, char *param_5, char param_6, int32_t *param_7);
int32_t blit_locked_surface_2b07(int32_t param_1,int32_t param_2,int16_t *param_3,int32_t *param_4);
int32_t blit_locked_surface_2c35(int32_t param_1,int32_t param_2,int16_t *param_3,int32_t *param_4);
int32_t blit_locked_surface_2d03(int32_t param_1,int32_t param_2,int16_t *param_3,int32_t *param_4);
int32_t blit_locked_surface_2daf(int32_t param_1,int32_t param_2,int16_t *param_3,int32_t *param_4);
int32_t blit_locked_surface_2e3a(int32_t param_1,int32_t param_2,int16_t *param_3,int32_t *param_4);
int32_t surface_scale_blit_22f68(int32_t param_1,int32_t param_2,int32_t param_3,int32_t param_4,int32_t param_5);
int32_t blit_wipe_v_edges_309b(int32_t param_1,int32_t param_2,int32_t param_3,uint32_t param_4,int32_t param_5,int32_t param_6);
int32_t blit_wipe_v_center_317a(int32_t param_1,int32_t param_2,int32_t param_3,uint32_t param_4,int32_t param_5,int32_t param_6);
int32_t blit_wipe_h_edges_3269(int32_t param_1, int32_t param_2, uint32_t param_3, uint32_t param_4, int32_t param_5, int32_t param_6);
int32_t blit_wipe_h_center_3438(int32_t param_1, int32_t param_2, uint32_t param_3, uint32_t param_4, int32_t param_5, int32_t param_6);
int32_t screen_wipe_3615(uint32_t param_1,int32_t param_2);
int32_t wait_message_37a8();
int32_t surface_blit_rect_3bd7(int32_t param_1, int32_t param_2, int32_t param_3, int32_t param_4, int32_t param_5, int32_t param_6, int32_t param_7, int32_t param_8, int32_t param_9, int32_t *param_10);
int32_t surface_blit_colorkey_3cb1(int32_t param_1, int32_t param_2, int32_t param_3, int32_t param_4, int32_t param_5, int32_t param_6, int32_t param_7, int32_t param_8, int32_t param_9, char param_10, int32_t *param_11);
int32_t clip_blit_3d96(int32_t param_1, int32_t param_2, int32_t param_3, int32_t param_4, int32_t param_5, int32_t param_6, int32_t param_7, int32_t param_8, int32_t param_9, int32_t param_10);
int32_t image_crossfade_40b9(int32_t param_1, int32_t param_2, int32_t param_3, int32_t param_4, uint8_t *param_5, uint8_t *param_6, uint8_t *param_7, int32_t param_8);
int32_t blit_locked_surface_42b0(int32_t param_1,int32_t param_2,uint32_t param_3,int32_t param_4,uint8_t param_5,int32_t *param_6);
int32_t img_mpeg_d36_mpg();
int32_t asset_dlz_ini();
int32_t asset_game_dat();
int32_t get_tick_count();
int32_t asset_mine_img();
int32_t set_timer();
int32_t directsound_init_49d0();
int32_t mci_check_4ac1();
int32_t start_mm_timer_4ae2();
int32_t ddraw_create();
int32_t show_cursor(int32_t param_1,int32_t param_2);
int32_t create_window_4eb6(HINSTANCE param_1,int32_t param_2);
int32_t slot8_destroy_all();
int32_t audio_stop_5008();
int32_t directx_teardown_5020();
int32_t kill_timer();
int32_t mru_queue4_reset();
int16_t mru_queue4_head();
int32_t mru_queue4_update(int16_t param_1,int16_t param_2);
int32_t mode_msg_dispatcher_25340(int32_t param_1,int32_t param_2,int32_t param_3);
int32_t wndproc_main_25641(HWND param_1,uint32_t param_2,uint32_t param_3,LPARAM param_4);
WPARAM wait_message_58b9(int32_t param_1,int32_t param_2,int32_t param_3,int32_t param_4);
int32_t message_peek();
int32_t message_peek_5948();
int32_t message_peek_5980();
int32_t clear_screen_layers_259b8();
int32_t delay_ms(uint32_t param_1);
int32_t modal_msg_pump_25a08();
int32_t wait_message_5a63(uint32_t param_1);
int32_t fade_palette_out_25ad4(int32_t param_1,uint32_t param_2,int32_t *param_3);
int32_t check_span_room(uint8_t param_1,uint8_t param_2,uint32_t param_3);
int32_t fade_palette_to_25d7c(int32_t param_1,uint32_t param_2,int32_t *param_3);
int32_t file_read_at(LPCSTR param_1,LPVOID param_2,LONG param_3,DWORD param_4);
int32_t load_palette_file(LPCSTR param_1,LPVOID param_2,int32_t param_3);
int32_t file_read_at_6202(LPCSTR param_1,LPVOID param_2,DWORD param_3,uint32_t param_4);
int32_t show_cursor_62b6(int32_t param_1,int32_t param_2);
int32_t play_anim_frames_26451(int32_t param_1,int32_t param_2,int32_t param_3,int32_t param_4);
int32_t play_anim_frames_2661b(int32_t param_1,int32_t param_2,int32_t param_3,int32_t param_4);
int32_t rect_overlap(int32_t param_1,int32_t param_2,int32_t param_3,int32_t param_4,int32_t param_5,int32_t param_6);
int32_t wait_message_6844(uint32_t param_1,uint32_t param_2);
uint32_t menu_select_26a05(uint32_t param_1,uint32_t param_2,uint32_t param_3);
uint32_t set_cursor_pos(uint32_t param_1,uint32_t param_2,uint32_t param_3);
int32_t draw_number_2728b();
int32_t draw_number_fmt_27313(uint32_t param_1, uint32_t param_2, int32_t param_3, int32_t param_4, int32_t param_5);
uint32_t draw_text(uint32_t param_1,uint32_t param_2,char *param_3);
int32_t show_message_box_27726(char *param_1);
int32_t clamp_stat_delta(int32_t param_1, int32_t param_2, uint32_t param_3);
uint32_t battle_damage_resolve_2784b(uint32_t param_1, uint32_t param_2, uint32_t param_3);
int32_t draw_countup_27873(int32_t param_1,int32_t param_2,uint32_t param_3);
int32_t find_non_cdrom_drive();
int32_t delta_direction_code(int32_t param_1,int32_t param_2,int32_t param_3,int32_t param_4);
int32_t matrix2d_transform(int32_t param_1,int32_t param_2,uint32_t param_3,uint32_t param_4);
int32_t int_array_rotate_left(int32_t param_1,uint32_t param_2,int32_t param_3);
int32_t save_game(int32_t param_1);
int32_t get_tick_count_7fef(int32_t param_1);
int32_t wait_message_82f9(int32_t param_1);
uint32_t wait_message_8b8f(uint32_t param_1,int32_t param_2,int32_t param_3);
int32_t compact_slot_array_74(int32_t param_1);
int32_t slot_remove_match_74(int32_t param_1,uint32_t param_2);
int32_t slot_add_first_empty_74(int32_t param_1,uint32_t param_2);
int32_t compact_slot_array_6c(int32_t param_1);
int32_t slot_remove_match_6c(int32_t param_1,uint32_t param_2);
int32_t slot_add_first_empty_6c(int32_t param_1,uint32_t param_2);
int32_t town_dat_load();
int32_t fight_dat_load();
int32_t noop_stub10();
int32_t set_cursor();
int32_t play_mpeg_clip(int32_t param_1);
int32_t play_tile_flm_animation(int32_t param_1,int32_t param_2,int32_t param_3);
int32_t slot_is_adjacent(int32_t param_1,int32_t param_2);
int32_t item_skill_pickup_anim(int32_t param_1);
int32_t unit_undeploy(int32_t param_1);
int32_t deploy_unit_on_grid(int32_t param_1);
int32_t max_hp_getter_9e31(int32_t param_1);
int32_t unit_gain_exp_9e92(int32_t param_1, int32_t param_2);
int32_t reset_deploy_slots_2a012();
int32_t update_grid_occupancy_2a0d1();
int32_t calc_level_bonus_a199();
int32_t battle_init_units_2a279();
int32_t turn_regen_tick_2a395();
int32_t calc_unit_power_2a82a(int32_t param_1);
uint16_t terrain_grid_get_2a95b(int32_t param_1, int32_t param_2);
int32_t calc_unit_defense_2a98e(int32_t param_1);
int32_t battle_exp_from_attack_aa92(int32_t param_1, int32_t param_2);
int32_t battle_exp_from_kill_ab23(int32_t param_1, int32_t param_2);
bool hit_chance_check_abb0(int param_1, int param_2);
bool hit_chance_check_weapon_ac0b(int param_1, int param_2);
int32_t backup_state_2ac86();
int32_t init_people_and_state();
int32_t count_unit_skills(int32_t param_1);
int32_t skill_table_max_40(int32_t param_1);
int32_t skill_table_max_44(int32_t param_1);
uint32_t unit_stat_max(int32_t param_1);
int32_t move_skill_slot(int32_t param_1,int32_t param_2);
int32_t commit_frame_deltas();
int32_t auto_train_units(int32_t param_1);
int32_t img_menu10_img();
int32_t unit_set_class_or_skill(uint32_t param_1);
int32_t img_mpeg_d01_mpg();
int32_t bgm_set_track();
int32_t ini_get_string(int32_t param_1,int32_t param_2);
int32_t mci_close_be8e(int32_t param_1);
int32_t mci_play_bef2(int32_t param_1);
int32_t mci_status_query(int32_t param_1,int32_t param_2);
int32_t create_window_bf88(HWND param_1);
LRESULT wndproc_char_esc(HWND param_1,UINT param_2,WPARAM param_3,LPARAM param_4);
int32_t mci_seek_42c0c7();
int32_t show_cursor_c100();
int32_t enter_mode_transition_a(int32_t param_1);
int32_t show_cursor_c215();
int32_t show_cursor_c3a5(int32_t param_1);
int32_t img_bg03_img();
int32_t screen_wndproc_tabbed(int32_t param_1,int32_t param_2,uint32_t param_3);
int32_t asset_hiscore_dat_cfea();
int32_t enter_mode_transition_b(int32_t param_1);
int32_t img_kuang04_img_d217();
int32_t post_message(int32_t param_1);
int32_t get_tick_count_d36b();
int32_t asset_army_dat_d686();
int32_t img_918_3_img(uint32_t param_1);
int32_t wait_message_e0f1();
int32_t asset_face_dat_e2d6(int32_t param_1,uint32_t param_2,uint32_t param_3);
int32_t draw_atlas_glyph_f841(int32_t param_1,int32_t param_2,uint32_t param_3);
int32_t draw_glyph_grid_2f891();
int32_t img_kuang04_img_f938();
uint32_t ui_dialog_handler_faca(uint32_t param_1,int32_t param_2,uint32_t param_3);
int32_t img_bg01_img();
int32_t ui_timer_transition_00e1(int32_t param_1);
int32_t asset_zyb_img();
int32_t asset_zyb_img_03b8(int32_t param_1);
int32_t asset_place_dat();
int32_t ui_menu_handler_0658(int32_t param_1,int32_t param_2);
uint32_t ui_button_menu_08f2(uint32_t param_1,int32_t param_2,uint32_t param_3);
int32_t img_bg02_img();
int32_t ui_button_0d3c(int32_t param_1,int32_t param_2,uint32_t param_3);
int32_t img_back19_img();
int32_t ui_button_0e39(int32_t param_1,int32_t param_2,uint32_t param_3);
int32_t get_tick_count_0ed9();
int32_t asset_bigmap_img();
int32_t hit_test_18_boxes(int32_t param_1,int32_t param_2);
uint32_t ui_scene_select_1273(uint32_t param_1,int32_t param_2,uint32_t param_3);
int32_t rect_overlap_14b6(int32_t param_1,int32_t param_2,int32_t param_3,int32_t param_4,int32_t *param_5);
int32_t draw_value_box_16df();
int32_t gather_index_list_tagged();
int32_t draw_list_panel_17c2();
int32_t count_slots_1d22(int32_t param_1);
int32_t draw_bar_meter_1d72(uint32_t param_1);
int32_t draw_unit_grid_2x8();
int32_t tally_weighted_counts_hi();
int32_t tally_weighted_counts_lo();
int32_t img_k01_img();
int32_t wndproc_mouse(uint32_t param_1,int32_t param_2,uint32_t param_3);
int32_t gather_index_list();
int32_t draw_roster_panels();
int32_t img_k01_img_4b9b();
int32_t wndproc_mousemove(uint32_t param_1,int32_t param_2,uint32_t param_3);
int32_t draw_tab_panels();
int32_t draw_tab_list();
int32_t draw_detail_list();
int32_t draw_summary_panel();
int32_t draw_item_icon_row();
int32_t img_k03_img();
uint32_t min_ratio_limit(int32_t param_1,int32_t param_2);
uint32_t screen_msg_handler_tabbed(int32_t param_1,int32_t param_2,uint32_t param_3);
int32_t draw_roster_list();
int32_t draw_skill_bars(uint32_t param_1);
int32_t draw_unit_grid_4x7();
int32_t count_slots_74(int32_t param_1);
int32_t draw_value_box();
int32_t img_k04_img();
int32_t recruit_screen_msg_handler(uint32_t param_1,int32_t param_2,uint32_t param_3);
int32_t count_nonzero_shorts();
int32_t remove_list_entry_by_tag(uint32_t param_1);
int32_t alloc_list_slot_20(int32_t param_1);
int32_t asset_army_dat_a903();
int32_t draw_unit_card_aa5c(int32_t param_1);
int32_t draw_deploy_screen_acd3();
int32_t img_k00_img();
int32_t screen_to_grid_index(int32_t param_1,int32_t param_2);
int32_t screen_to_grid_index_alt(int32_t param_1,int32_t param_2);
int32_t alloc_list_slot_20_flag(uint8_t param_1);
int32_t place_unit_at_free_cell(int32_t param_1,int32_t param_2,int32_t param_3,int32_t param_4);
int32_t begin_battle_deploy_b4f1();
int32_t deploy_screen_handler_b680(int32_t param_1,int32_t param_2,uint32_t param_3);
int32_t img_dileizan_flc();
int32_t ui_button_c222(int32_t param_1);
int32_t img_dileizan_flc_c273();
int32_t ui_button_c287(int32_t param_1);
int32_t noop_empty();
int32_t scene_render_dispatch();
int32_t mci_music_callback_c440();
int32_t bgm_select_track(int32_t param_1);
int32_t file_read_at_c4e4(int32_t param_1,uint32_t param_2);
int32_t slot8_destroy_one(int32_t param_1);
int32_t bgm_play();
int32_t audio_stop_if_playing();
int32_t ds_buffer_release_c78e(uint32_t param_1);
int32_t slot8_active_query(uint32_t param_1);
int32_t query_surface_status();
uint32_t slot8_find_free();
int32_t play_sfx_c954(int32_t param_1);
int32_t count_space_or_hi(char *param_1);
int32_t decrypt_text_c9ec(int32_t param_1);
int32_t dialogue_read_token(int32_t param_1,uint8_t *param_2,int32_t param_3);
int32_t wait_message_cfe1(int32_t param_1,char *param_2,int32_t param_3);
int32_t dialogue_show_box(int32_t param_1,int32_t param_2,int32_t param_3);
int32_t draw_text_box_d56b(char *param_1);
int32_t dialogue_render_line(int32_t param_1,char *param_2);
int32_t dialogue_play_script(int32_t param_1);
int32_t abs_int_de40(int32_t param_1);
int32_t * memmove_impl(void* a_dst, void* a_src, uint32_t param_3);
uint32_t * strcpy_impl(uint32_t *param_1,uint32_t *param_2);
uint32_t * strcat_impl(uint32_t *param_1,uint32_t *param_2);
int32_t store_global_448790(int32_t param_1);
uint32_t rand_impl();
uint32_t stream_read_e340(char *param_1,uint32_t param_2,uint32_t param_3,int32_t *param_4);
int32_t stream_close_e490(int32_t param_1);
uint32_t fread_impl(int32_t *param_1, uint32_t param_2, uint32_t param_3, int32_t *param_4);
int32_t fopen_sized(int32_t param_1,int32_t param_2,int32_t param_3);
int32_t fopen_impl(int32_t param_1,int32_t param_2);
int32_t fseek_impl(int32_t param_1,int32_t param_2,int32_t param_3);
int32_t sprintf_impl(uint8_t *param_1,int32_t param_2);
int32_t fscanf_impl(int32_t param_1,int32_t param_2);
uint32_t fgetc_impl(int32_t *param_1);
uint32_t glyph_attr4_query(int32_t param_1);
uint32_t glyph_attr8_query(int32_t param_1);
uint32_t stream_refill_read(int32_t *param_1);
uint32_t flsbuf_impl(uint32_t param_1,int32_t *param_2);
int32_t fwrite_impl(uint32_t param_1,char *param_2,uint32_t param_3);
int32_t fflush_impl(int32_t *param_1);
int32_t heap_free_impl(LPVOID param_1);
int32_t close_indexed_handle(uint32_t param_1);
int32_t stream_buffer_free(int32_t *param_1);
int32_t fread_impl_43f1c0(uint32_t param_1,char *param_2,DWORD param_3);
int32_t * fopen_core(int32_t param_1,char *param_2,int32_t param_3,int32_t *param_4);
int32_t * stream_pool_alloc();
int32_t cleanup_hook_pair();
int32_t call_f6f0_wrapper(int32_t param_1);
int32_t process_exit_cleanup(UINT param_1,int32_t param_2,int32_t param_3);
int32_t invoke_function_array(int32_t *param_1,int32_t *param_2);
DWORD vfile_seek(uint32_t param_1,LONG param_2,DWORD param_3);
int32_t vfile_read(int32_t *param_1);
int32_t vsprintf_engine(int32_t param_1,char *param_2,int32_t *param_3);
int32_t fmt_putc(uint32_t param_1,int32_t *param_2,int32_t *param_3);
int32_t fmt_fill_n(int32_t param_1,int32_t param_2,int32_t param_3,int32_t *param_4);
int32_t fmt_puts_n(char *param_1,int32_t param_2,int32_t param_3,int32_t *param_4);
int32_t ptr_read_u32(intptr_t* param_1);
uint64_t ptr_read_u64(intptr_t* param_1);
int32_t ptr_read_u16(intptr_t* param_1);
int32_t scanf_impl(int32_t param_1,uint8_t *param_2,int32_t *param_3);
uint32_t glyph_attr4_adjust(uint32_t param_1);
uint32_t stream_getchar(int32_t *param_1);
int32_t call_436b0_if_valid();
int32_t stream_skip_ws(int32_t *param_1,int32_t param_2);
uint32_t glyph_metric_get(int32_t param_1,uint32_t param_2);
int32_t vfile_handle_table_init();
int32_t stream_buffer_init(int32_t *param_1);
int32_t call_417c0_wrapper(int32_t param_1);
int32_t char_flag_test(uint32_t param_1,uint32_t param_2,uint8_t param_3);
int32_t split_on_eq_to_ptrs();
int32_t get_module_filename();
int32_t parse_quoted_string(uint8_t *param_1,int32_t *param_2,uint8_t *param_3,int32_t *param_4,int32_t *param_5);
LPSTR env_strings_to_ansi();
int32_t set_codepage(int32_t param_1);
int32_t resolve_codepage(int32_t param_1);
int32_t codepage_to_charset(int32_t param_1);
int32_t reset_codepage_state();
int32_t call_41d00_wrapper();
int32_t heap_create_with_init();
int32_t save_call_frame();
int32_t seh_local_unwind(int32_t param_1);
int32_t exit_cleanup_hook();
int32_t get_module_filename_2230(int32_t* param_1);
uint8_t vfile_flag40_query(uint32_t param_1);
int32_t vfile_report_error(int32_t *param_1);
int32_t ** heap_alloc();
int32_t heap_region_free(int32_t **param_1);
int32_t heap_trim_free_pages(int32_t param_1);
int32_t heap_locate_page(int32_t *param_1,int32_t *param_2,uint32_t *param_3);
int32_t heap_page_commit(int32_t param_1,int32_t param_2,uint8_t *param_3);
int32_t * heap_alloc_block(uint32_t param_1);
int32_t heap_block_alloc(int32_t *param_1,uint32_t param_2,uint32_t param_3);
int32_t vfile_open_slot();
int32_t vfile_assign_handle(uint32_t param_1,HANDLE param_2);
int32_t vfile_free_handle_slot(uint32_t param_1);
int32_t vfile_get_handle(uint32_t param_1);
uint32_t file_createfile(LPCSTR param_1,uint32_t param_2,uint32_t param_3,int32_t param_4);
int32_t forward_to_43350(int32_t param_1);
int32_t alloc_with_fallback(uint32_t param_1,int32_t param_2);
int32_t heap_alloc_33a0(int32_t param_1);
LPSTR wchar_to_mb_char(LPSTR param_1,WCHAR param_2);
uint32_t mb_to_wchar_char(LPWSTR param_1,uint8_t *param_2,uint32_t param_3);
uint32_t stream_put_char(uint32_t param_1,int32_t *param_2);
BOOL get_string_type(DWORD param_1,LPCSTR param_2,int32_t param_3,LPWORD param_4,UINT param_5,LCID param_6);
int32_t load_library(int32_t param_1,int32_t param_2,int32_t param_3);
int32_t * heap_alloc_3a00(int32_t param_1,int32_t param_2);
int32_t file_truncate_eof();
int32_t optional_callback_dispatch(int32_t param_1);
int32_t amsg_exit_2();
int32_t * memmove_impl2(int32_t *param_1,int32_t *param_2,uint32_t param_3);
int32_t vfile_seek_mode_set(uint32_t param_1,int32_t param_2);
int32_t chkstk_probe();

#ifdef __cplusplus
}
#endif
#endif /* DLZR_REPLICA_STUBS_H */
