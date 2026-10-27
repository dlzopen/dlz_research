/*
 * msvc32_compat.h — Ghidra 转录伪影兼容垫片（铁律零：本文件是兼容垫片，非原版语义 RE）
 * ============================================================================
 * 原版 DLZ.exe 是 32 位 MSVC 程序。Ghidra 反编译把若干 32 位 MSVC 运行时内部函数
 * 与 memcpy 内建名直接照抄进转录体，这些符号在 x64 构建环境里不存在 / 不可链接：
 *   - builtin_memcpy        : Ghidra 把 memcpy 误命名为 builtin_memcpy → 重定向到 memcpy
 *   - __aullrem / __aulldiv  : 32 位 MSVC 64 位无符号余/商内部函数（x64 编译器内联，无导出）
 *   - __allmul              : 32 位 MSVC 64 位乘法内部函数
 *   - __allshl              : 32 位 MSVC 64 位左移内部函数（本调用点 0 参，Ghidra 丢失操作数）
 *   - __amsg_exit           : CRT 致命错误退出（副本映射为 abort）
 *   - __local_unwind2       : SEH 局部展开（副本无 SEH，空操作）
 *
 * 这些函数本体属于文本渲染 / 数字格式化 / SEH 等路径，副本的战斗数值对拍用例不执行，
 * 仅用于让副本可链接。调用点参数布局由 Ghidra 寄存器恢复得出（已在各函数注释标注），
 * 其中 __allshl 的 0 参调用属反编译丢弃操作数的伪影，待逐函数 RE 正名，不声称 100% 还原。
 *
 * 注意：DirectSoundCreate / DirectDrawCreate 不在本头处理 —— 它们是真实 Win32 API，
 * 由生成器 #include <dsound.h>/<ddraw.h> 并链接 dsound.lib/ddraw.lib（见 replica/CMakeLists.txt）
 * 经系统导入库解析；副本运行时依赖 DirectDraw 的 vtable，必须真实链接，不能桩成失败。
 */
#ifndef DLZR_REPLICA_MSCV32_COMPAT_H
#define DLZR_REPLICA_MSCV32_COMPAT_H

#include <string.h>
#include <stdlib.h>
#include <stdint.h>

/* --- builtin_memcpy（Ghidra 把 memcpy 误命名为 builtin_memcpy）--- */
#ifndef builtin_memcpy
#define builtin_memcpy memcpy
#endif

/* --- 32 位 MSVC 64 位运算 / CRT 内部函数兼容桩 ---
 * 调用点参数布局由 Ghidra 寄存器恢复得出，标注为重建语义。 */

/* __aullrem(low, divisor, high): 64 位无符号余数。
 * 调用点 __aullrem(lVar13, local_22c, local_204)：
 *   arg1=lVar13(低32), arg2=local_22c(除数), arg3=local_204(高32)。 */
static inline int32_t __aullrem(uint32_t low, uint32_t divisor, uint32_t high) {
    uint64_t dividend = ((uint64_t)high << 32) | (uint64_t)low;
    if (divisor == 0) return 0; /* 原版 UB；副本防御，返回 0 */
    return (int32_t)(dividend % (uint64_t)divisor);
}

/* __aulldiv(low, divisor, high): 64 位无符号商。布局同上。 */
static inline int32_t __aulldiv(uint32_t low, uint32_t divisor, uint32_t high) {
    uint64_t dividend = ((uint64_t)high << 32) | (uint64_t)low;
    if (divisor == 0) return 0;
    return (int32_t)(dividend / (uint64_t)divisor);
}

/* __allmul(a, b_low, b_high): 64 位乘。调用点 __allmul(lVar19,10,0) → lVar19 * 10。
 * arg1=64 位被乘数(低32 显式+高32 隐式), arg2=乘数低32, arg3=乘数高32。 */
static inline int64_t __allmul(uint64_t a, uint32_t b_low, uint32_t b_high) {
    uint64_t b = ((uint64_t)b_high << 32) | (uint64_t)b_low;
    return (int64_t)(a * b);
}

/* __allshl(): 调用点 0 参（Ghidra 丢失操作数），属文本光栅路径；返回 0 占位。
 * 真实语义应为 64 位左移，参数已被反编译丢弃，待 RE。 */
static inline int64_t __allshl(void) {
    return 0;
}

/* __amsg_exit(code): 原版 CRT 致命错误退出。副本映射为 abort()。 */
static inline void __amsg_exit(int32_t code) {
    (void)code;
    abort();
}

/* __local_unwind2(frame, flag): SEH 局部展开。副本无 SEH，空操作返回 0。 */
static inline int32_t __local_unwind2(int32_t frame, int32_t flag) {
    (void)frame; (void)flag;
    return 0;
}

#endif /* DLZR_REPLICA_MSCV32_COMPAT_H */
