// crash_handler.hpp — 崩溃时打印堆栈 / Print stack trace on crash
// Windows only (SetUnhandledExceptionFilter + DbgHelp)

#pragma once

namespace dlzr::platform {

void install_crash_handler();

}  // namespace dlzr::platform
