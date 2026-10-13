// crash_handler.cpp — Windows 崩溃堆栈打印
// Uses SetUnhandledExceptionFilter + CaptureStackBackTrace + DbgHelp symbols.
// Writes crash.log to working directory and prints to stderr.

#include "platform/crash_handler.hpp"

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <dbghelp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#pragma comment(lib, "dbghelp.lib")

namespace dlzr::platform {

namespace {

constexpr int kMaxFrames = 64;

FILE* g_crash_log = nullptr;

void crash_printf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    if (g_crash_log) {
        va_start(args, fmt);
        vfprintf(g_crash_log, fmt, args);
        va_end(args);
        fflush(g_crash_log);
    }
}

const char* exception_code_name(DWORD code) {
    switch (code) {
    case EXCEPTION_ACCESS_VIOLATION:      return "EXCEPTION_ACCESS_VIOLATION";
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:  return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
    case EXCEPTION_BREAKPOINT:             return "EXCEPTION_BREAKPOINT";
    case EXCEPTION_DATATYPE_MISALIGNMENT:  return "EXCEPTION_DATATYPE_MISALIGNMENT";
    case EXCEPTION_FLT_DENORMAL_OPERAND:   return "EXCEPTION_FLT_DENORMAL_OPERAND";
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:     return "EXCEPTION_FLT_DIVIDE_BY_ZERO";
    case EXCEPTION_FLT_INEXACT_RESULT:     return "EXCEPTION_FLT_INEXACT_RESULT";
    case EXCEPTION_FLT_INVALID_OPERATION:  return "EXCEPTION_FLT_INVALID_OPERATION";
    case EXCEPTION_FLT_OVERFLOW:           return "EXCEPTION_FLT_OVERFLOW";
    case EXCEPTION_FLT_STACK_CHECK:        return "EXCEPTION_FLT_STACK_CHECK";
    case EXCEPTION_FLT_UNDERFLOW:          return "EXCEPTION_FLT_UNDERFLOW";
    case EXCEPTION_ILLEGAL_INSTRUCTION:    return "EXCEPTION_ILLEGAL_INSTRUCTION";
    case EXCEPTION_IN_PAGE_ERROR:          return "EXCEPTION_IN_PAGE_ERROR";
    case EXCEPTION_INT_DIVIDE_BY_ZERO:     return "EXCEPTION_INT_DIVIDE_BY_ZERO";
    case EXCEPTION_INT_OVERFLOW:           return "EXCEPTION_INT_OVERFLOW";
    case EXCEPTION_INVALID_DISPOSITION:    return "EXCEPTION_INVALID_DISPOSITION";
    case EXCEPTION_NONCONTINUABLE_EXCEPTION: return "EXCEPTION_NONCONTINUABLE_EXCEPTION";
    case EXCEPTION_PRIV_INSTRUCTION:       return "EXCEPTION_PRIV_INSTRUCTION";
    case EXCEPTION_SINGLE_STEP:            return "EXCEPTION_SINGLE_STEP";
    case EXCEPTION_STACK_OVERFLOW:         return "EXCEPTION_STACK_OVERFLOW";
    default:                               return "UNKNOWN_EXCEPTION";
    }
}

void print_stack_trace() {
    void* frames[kMaxFrames];
    USHORT captured = CaptureStackBackTrace(0, kMaxFrames, frames, nullptr);

    HANDLE process = GetCurrentProcess();
    if (!SymInitialize(process, nullptr, TRUE)) {
        crash_printf("  (SymInitialize failed, raw addresses only)\n");
        for (USHORT i = 0; i < captured; ++i) {
            crash_printf("  #%u  %p\n", i, frames[i]);
        }
        return;
    }

    SYMBOL_INFO* symbol = static_cast<SYMBOL_INFO*>(
        calloc(sizeof(SYMBOL_INFO) + 256, 1));
    if (!symbol) {
        SymCleanup(process);
        for (USHORT i = 0; i < captured; ++i) {
            crash_printf("  #%u  %p\n", i, frames[i]);
        }
        return;
    }
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    DWORD64 displacement = 0;
    IMAGEHLP_LINE64 line;
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

    for (USHORT i = 0; i < captured; ++i) {
        DWORD64 addr = reinterpret_cast<DWORD64>(frames[i]);

        BOOL has_symbol = SymFromAddr(process, addr, &displacement, symbol);
        DWORD line_disp = 0;
        BOOL has_line = SymGetLineFromAddr64(process, addr, &line_disp, &line);

        if (has_symbol && has_line) {
            crash_printf("  #%u  %s  (%s:%lu)\n",
                         i, symbol->Name, line.FileName, line.LineNumber);
        } else if (has_symbol) {
            crash_printf("  #%u  %s+0x%llx  (%p)\n",
                         i, symbol->Name, displacement, frames[i]);
        } else {
            crash_printf("  #%u  %p\n", i, frames[i]);
        }
    }

    free(symbol);
    SymCleanup(process);
}

void write_crash_header(const char* context) {
    time_t now = time(nullptr);
    char time_buf[64];
    struct tm local_time;
    localtime_s(&local_time, &now);
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &local_time);

    crash_printf("========================================\n");
    crash_printf("CRASH: %s\n", context);
    crash_printf("Time: %s\n", time_buf);
    crash_printf("========================================\n");
    crash_printf("\nStack trace:\n");
}

LONG WINAPI unhandled_exception_filter(EXCEPTION_POINTERS* exinfo) {
    g_crash_log = fopen("crash.log", "w");

    char context[256];
    DWORD code = exinfo->ExceptionRecord->ExceptionCode;
    snprintf(context, sizeof(context), "%s (0x%08lX) at %p",
             exception_code_name(code), code,
             exinfo->ExceptionRecord->ExceptionAddress);

    if (code == EXCEPTION_ACCESS_VIOLATION) {
        ULONG_PTR type = exinfo->ExceptionRecord->ExceptionInformation[0];
        ULONG_PTR addr = exinfo->ExceptionRecord->ExceptionInformation[1];
        crash_printf("Access violation: %s address %p\n",
                     type == 0 ? "reading" : type == 1 ? "writing" : "executing",
                     reinterpret_cast<void*>(addr));
    }

    write_crash_header(context);
    print_stack_trace();

    crash_printf("\n========================================\n");
    if (g_crash_log) {
        fclose(g_crash_log);
        g_crash_log = nullptr;
        fprintf(stderr, "Crash log written to: crash.log\n");
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

void signal_handler(int sig) {
    const char* name = "UNKNOWN";
    switch (sig) {
    case SIGSEGV: name = "SIGSEGV"; break;
    case SIGABRT: name = "SIGABRT"; break;
    case SIGFPE:  name = "SIGFPE";  break;
    case SIGILL:  name = "SIGILL";  break;
    }

    g_crash_log = fopen("crash.log", "w");

    char context[128];
    snprintf(context, sizeof(context), "%s (signal %d)", name, sig);
    write_crash_header(context);
    print_stack_trace();

    crash_printf("\n========================================\n");
    if (g_crash_log) {
        fclose(g_crash_log);
        g_crash_log = nullptr;
        fprintf(stderr, "Crash log written to: crash.log\n");
    }

    _exit(1);
}

}  // namespace

void install_crash_handler() {
    SetUnhandledExceptionFilter(unhandled_exception_filter);
    signal(SIGSEGV, signal_handler);
    signal(SIGABRT, signal_handler);
    signal(SIGFPE,  signal_handler);
    signal(SIGILL,  signal_handler);
}

}  // namespace dlzr::platform

#else

namespace dlzr::platform {
void install_crash_handler() {}
}  // namespace dlzr::platform

#endif
