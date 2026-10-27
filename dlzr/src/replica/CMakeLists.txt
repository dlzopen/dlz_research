# ============================================================
# 复刻转录产物（dlzr::replica）
# 由 scripts/gen_replica_stubs.py 把 output/ghidra_all/FUN_*.c 1:1 转录成
# generated/replica_stubs.c（C 编译，逐行照抄反编译）。证据制下的 1:1 转录，无 PLACEHOLDER。
# 全局变量统一 gs_<addr> 占位（待收口 GameState），类型在转录阶段按需判定。
# ============================================================
# 机器生成的转录体单独放 generated/，与手写语义模块（battle_calc.cpp / msvc32_compat.h）区分。
add_library(dlzr_replica STATIC generated/replica_stubs.c battle_calc.cpp battle_sim.cpp)
target_include_directories(dlzr_replica PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/generated)

# 原版为 Win32 程序，转录体直接调用 GDI/User/MCI/DirectSound/DirectDraw 等 API，
# 需链接相应系统库。dsound/ddraw 必须真实链接：副本运行时经 DirectDraw 的 vtable 做
# 实际表面操作（replica_stubs.c 无 NULL 检查直接解引用 g_ddraw），桩成失败会崩溃。
if(WIN32)
    target_link_libraries(dlzr_replica PUBLIC user32 gdi32 winmm dsound ddraw)
endif()
