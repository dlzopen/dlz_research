// asan_cs_driver.cpp — 角色选择屏堆损坏实证驱动（独立 main，无 GTest SEH 包裹）
// 不依赖 AddressSanitizer（本机 MSVC ASan 动态运行时无法初始化），改用：
//   1) Vectored Exception Handler 抓取 AV 时的调用栈返回地址；
//   2) DbgHelp (PDB) 把返回地址解析为 module!function (file.cpp:line)，直接定位"谁写的 bug"；
//   3) 看门狗：若单轮 render 卡死/超时，主动 RaiseException 触发栈转储。
//
// 构建（普通配置即可，无需 DLZR_ASAN）：cmake 后构建 dlzr_asan_cs。
// 运行：SDL_VIDEODRIVER=dummy ./dlzr_asan_cs.exe

#include <cstdio>
#include <cstring>
#include <string>
#include <windows.h>
#include <DbgHelp.h>
#ifdef _DEBUG
#include <crtdbg.h>
#endif

#include "platform/platform.hpp"
#include "app/resource/face_loader.hpp"
#include "app/resource/army_loader.hpp"
#include "presentation/font/font_renderer.hpp"
#include "presentation/image/image_cache.hpp"
#include "presentation/character_select/character_select_renderer.hpp"

static void init_symbols() {
    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
    if (!SymInitialize(GetCurrentProcess(), NULL, TRUE)) {
        fprintf(stderr, "[DRV] SymInitialize failed (0x%lX) — 将仅有 RVA\n",
                (unsigned long)GetLastError());
        fflush(stderr);
    }
}

// 把单个返回地址解析为 "module!function (file:line)"，失败退回 RVA。
static void print_frame(int idx, uintptr_t addr, HMODULE base) {
    char module[MAX_PATH] = {0};
    DWORD modlen = 0;
    const char* modname = "?";
    HMODULE hmod = NULL;
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                           (LPCSTR)(uintptr_t)addr, &hmod)) {
        if (GetModuleFileNameA(hmod, module, MAX_PATH))
            modname = module; // 仅打印末段更清爽，但保留全路径以便核对
        // 取末段文件名
        const char* slash = strrchr(module, '\\');
        if (slash) modname = slash + 1;
    }

    char symbuf[sizeof(SYMBOL_INFO) + 512] = {0};
    SYMBOL_INFO* si = (SYMBOL_INFO*)symbuf;
    si->SizeOfStruct = sizeof(SYMBOL_INFO);
    si->MaxNameLen = 512;
    DWORD64 disp = 0;
    const char* func = "??";
    if (SymFromAddr(GetCurrentProcess(), (DWORD64)addr, &disp, si)) {
        func = si->Name;
    }

    IMAGEHLP_LINE64 line = {0};
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
    DWORD lineDisp = 0;
    const char* file = "";
    int lineno = 0;
    if (SymGetLineFromAddr64(GetCurrentProcess(), (DWORD64)addr, &lineDisp, &line)) {
        file = line.FileName;
        lineno = (int)line.LineNumber;
    }

    fprintf(stderr, "  #%02d  %s!%s", idx, modname, func);
    if (lineno) fprintf(stderr, "  (%s:%d)", file, lineno);
    fprintf(stderr, "  RVA=0x%llX  (%p)\n",
            (unsigned long long)(addr - (uintptr_t)base), (void*)addr);
}

static LONG WINAPI veh_handler(EXCEPTION_POINTERS* ep) {
    DWORD code = ep->ExceptionRecord->ExceptionCode;
    // 忽略 SDL 等抛出的良性调试异常（线程命名等），避免刷屏/误处理。
    if (code == 0x40010005 || code == 0x40010006 || code == 0x4001000A)
        return EXCEPTION_CONTINUE_SEARCH;

    bool is_av = (code == 0xC0000005);
    bool is_watchdog = (code == 0xE0000001);
    const char* kind = is_av ? "ACCESS VIOLATION" :
                       (is_watchdog ? "WATCHDOG TIMEOUT" : "EXCEPTION");
    fprintf(stderr, "\n=== VEH: %s 0x%08lX at RIP=0x%p ===\n",
            kind, (unsigned long)code, (void*)ep->ContextRecord->Rip);
    if (is_av) {
        fprintf(stderr, "  attempt=%s  fault_addr=0x%p\n",
                (ep->ExceptionRecord->ExceptionInformation[0] ? "WRITE" : "READ"),
                (void*)ep->ExceptionRecord->ExceptionInformation[1]);
    }

    HMODULE base = GetModuleHandle(NULL);
    fprintf(stderr, "module base = 0x%p\n", (void*)base);

    void* frames[64];
    USHORT n = CaptureStackBackTrace(0, 64, frames, NULL);
    fprintf(stderr, "stack frames (%u):\n", (unsigned)n);
    for (USHORT i = 0; i < n; ++i) {
        print_frame((int)i, (uintptr_t)frames[i], base);
    }
    fflush(stderr);
    ExitProcess(1);
    return EXCEPTION_EXECUTE_HANDLER;
}

static std::string data_dir() {
    const char* e = std::getenv("DLZR_TEST_DATA");
    if (e && e[0]) return e;
    return "C:/seesaw/source/dlz_research/DLZFULL";
}

int main() {
    // 运行时开启 FULL PAGE HEAP：把每个堆分配之后的 guard page 暴露出来，
    // 这样越界 WRITE 会直接落在 guard page 上，立即 AV 并给出干净的故障栈，
    // 精确定位"谁写的 bug"（而不是在后续分配时才检测到堆损坏）。
    if (std::getenv("DLZR_PAGEHEAP")) {
        ULONG compat = 2;  // HeapCompatibilityInformation = full page heap
        if (!HeapSetInformation(GetProcessHeap(), HeapCompatibilityInformation,
                                &compat, sizeof(compat))) {
            fprintf(stderr, "[DRV] HeapSetInformation(pageheap) failed 0x%lX\n",
                    (unsigned long)GetLastError());
        } else {
            fprintf(stderr, "[DRV] FULL PAGE HEAP enabled\n");
        }
        fflush(stderr);
    }

#ifdef _DEBUG
    // CRT 调试堆：每次分配/释放都校验所有块的 no-mans-land，
    // 越界 WRITE 会立刻在"被写穿的缓冲区块"上被检出，直接报告该块的分配点。
    if (std::getenv("DLZR_CRTDBG")) {
        _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF);
        fprintf(stderr, "[DRV] CRT debug heap CHECK_ALWAYS enabled\n");
        fflush(stderr);
    }
#endif

    init_symbols();
    AddVectoredExceptionHandler(1, veh_handler);

    auto platform = dlzr::platform::create_sdl3_platform();
    dlzr::platform::PlatformConfig cfg;
    cfg.width = 640;
    cfg.height = 480;
    cfg.vsync = false;
    if (!platform->init(cfg)) {
        std::fprintf(stderr, "[DRV] platform init failed\n");
        return 2;
    }

    std::string d = data_dir();
    dlzr::app::FaceLoader faces;
    if (!faces.load(d + "/FACE.DAT")) { std::fprintf(stderr, "[DRV] FACE.DAT fail\n"); return 3; }
    dlzr::app::ArmyLoader army;
    if (!army.load(d + "/ARMY.DAT")) { std::fprintf(stderr, "[DRV] ARMY.DAT fail\n"); return 3; }
    dlzr::presentation::FontRenderer font;
    if (!font.init(*platform, d)) { std::fprintf(stderr, "[DRV] font fail\n"); return 3; }
    dlzr::presentation::ImageCache images;
    images.init(*platform, d);
    dlzr::presentation::CharacterSelectRenderer csr;
    csr.init(*platform, font, images, faces, army);
    std::fprintf(stderr, "[DRV] init done\n");
    fflush(stderr);

    const char* fnames[4] = {
        "\xB2\xBD\xB2\xBD",   // 步兵
        "\xC6\xD5\xB1\xF8",   // 骑兵
        "\xB9\xE6\xB1\xF8",   // 弓兵
        "\xBE\xFC\xCA\xF8",   // 军师
    };
    std::string pns[] = {
        "", "赵云", "abcdef", "李逍遥xyz",
        "\xD2\xBB\xB6\xFE\xC8\xFD\xCB\xC4\xCE\xE5\xC1\xF6\xC6\xDF\xCB\xC4\xCA\xAE",
    };

    const int N = 20000;
    DWORD start = GetTickCount();
    for (int f = 0; f < N; ++f) {
        int sel_p = f % 6;
        int sel_f = f % 4;
        int slider = 60 + (f % 41);
        int diff = f % 3;
        bool naming = (f % 7 == 0);
        int hovered = f % 3;
        int pressed = (f % 5 == 0) ? (f % 3) : -1;
        std::string pn = pns[f % 5];

        csr.render(sel_p, sel_f, slider, diff, pn, fnames, hovered, pressed, naming);
        if (f % 50 == 0) csr.trigger_formation_anim(f % 4);
        csr.update_anim(16);
        platform->present();

        if (f % 200 == 0) {
            DWORD now = GetTickCount();
            fprintf(stderr, "[DRV] frame %d  elapsed=%lums\n", f, (unsigned long)(now - start));
            fflush(stderr);
        }

        // 看门狗：若单轮耗时异常（卡死/无限循环），主动抛异常转储栈。
        DWORD now = GetTickCount();
        if (now - start > 30000) {
            fprintf(stderr, "[DRV] WATCHDOG: stuck near frame %d (elapsed=%lums)\n",
                    f, (unsigned long)(now - start));
            fflush(stderr);
            RaiseException(0xE0000001, 0, 0, NULL);
        }
    }
    std::fprintf(stderr, "[DRV] DONE ok (%d frames)\n", N);
    return 0;
}
