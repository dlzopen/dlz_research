/* 兼容转发头（手写，gen_replica_stubs.py 只写 generated/，不会被覆盖）。
 *
 * 机器生成的转录体已移至 generated/replica_stubs.h（与手写语义模块
 * battle_calc.cpp / msvc32_compat.h 区分）。保留此转发头以维持既有
 * #include "replica/replica_stubs.h" / #include "replica_stubs.h" 的兼容。
 */
#include "generated/replica_stubs.h"
