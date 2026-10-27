// heartbeat.hpp — 卡死（无栈可取的挂起）诊断模块
//
// 用途：原版在角色选择等界面偶发"进程不退出、画面冻结"的挂起，
// 这类挂起没有异常栈可抓（多为堆损坏导致分配器自旋，或
// SDL_RenderPresent 在某帧阻塞）。标准心跳日志（每帧写一行）只能
// 告诉我们卡在哪个 Mode，但无法定位"该帧内的哪一步"。
//
// 本模块提供：
//   hb_init()    打开 dlzr_heartbeat.log
//   hb_step(s)   标记当前子步骤（会立即 fflush 落盘）
//   hb_mode(m)   标记当前 GameMode（切屏时调用）
//   hb_frame(f)  每帧 present 后上报帧号（每 30 帧落盘一行）
//   hb_watchdog() 独立线程：若主线程 3 秒未推进帧号，落盘
//                "WATCHDOG STALL: mode=.. step=.."，精确锁定挂起点。
//
// 注意：g_step/g_mode 由主线程写、看门狗线程读，存在数据竞争，
// 但仅用于诊断展示，不影响正确性（不崩溃）。
#pragma once

#include <cstdint>

namespace dlzr {

void hb_init();
void hb_step(const char* s);
void hb_mode(const char* m);
void hb_frame(uint64_t f);
void hb_watchdog();

}  // namespace dlzr
