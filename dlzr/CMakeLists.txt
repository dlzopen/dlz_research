cmake_minimum_required(VERSION 3.20)
project(dlzr LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# MSVC: UTF-8 源文件 / UTF-8 source files
if(MSVC)
    add_compile_options(/utf-8)
    # 生成 MAP + PDB，便于崩溃时把 module!offset 离线解析到函数。
    # Generate MAP + PDB so a crash's module!offset is resolvable offline.
    add_link_options(/MAP /DEBUG)
endif()

# ============================================================
# AddressSanitizer 实证抓错（仅调试堆越界用，默认关）
# ============================================================
option(DLZR_ASAN "Build with AddressSanitizer (empirical heap-overflow detection)" OFF)
if(DLZR_ASAN)
    if(MSVC)
        # 编译期插桩（必需）；MSVC 的 link.exe 不识别 /fsanitize=address，
        # 故运行时库改为显式链接，避免 LNK4044 被忽略导致 ASan 未生效。
        add_compile_options(/fsanitize=address /Zi)
        # ASan 运行时随 VS2022 MSVC 工具链分发；本机已知路径（诊断用，默认关闭）。
        set(_vs_root "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.37.32822")
        set(ASAN_LIB_DIR "${_vs_root}/lib/x64")
        set(ASAN_DLL_DIR "${_vs_root}/bin/Hostx64/x64")
        if(EXISTS "${ASAN_LIB_DIR}/clang_rt.asan_dynamic-x86_64.lib")
            message(STATUS "ASan runtime lib dir: ${ASAN_LIB_DIR}")
            message(STATUS "ASan runtime dll dir: ${ASAN_DLL_DIR}")
            add_link_options("${ASAN_LIB_DIR}/clang_rt.asan_dynamic-x86_64.lib"
                             "${ASAN_LIB_DIR}/clang_rt.asan_dynamic_runtime_thunk-x86_64.lib")
        else()
            message(WARNING "DLZR_ASAN: clang_rt.asan runtime libs not found at ${ASAN_LIB_DIR}")
        endif()
    else()
        add_compile_options(-fsanitize=address -g)
        add_link_options(-fsanitize=address)
    endif()
endif()

# Emscripten: Web 构建 / Web build
if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    add_compile_options(-fexceptions)
    add_link_options(-fexceptions)
endif()

# 输出目录统一 / Unified output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ============================================================
# 依赖 / Dependencies (FetchContent)
# ============================================================
include(FetchContent)

# SDL3
FetchContent_Declare(SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.x
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(SDL3)

# SDL3_mixer (for BGM playback) — temporarily disabled due to version mismatch
# FetchContent_Declare(SDL3_mixer
#     GIT_REPOSITORY https://github.com/libsdl-org/SDL_mixer.git
#     GIT_TAG release-3.2.x
#     GIT_SHALLOW TRUE
# )
# set(SDL3MIXER_FLAC OFF CACHE BOOL "" FORCE)
# set(SDL3MIXER_MOD OFF CACHE BOOL "" FORCE)
# set(SDL3MIXER_OPUS OFF CACHE BOOL "" FORCE)
# set(SDL3MIXER_WAVPACK OFF CACHE BOOL "" FORCE)
# set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
# FetchContent_MakeAvailable(SDL3_mixer)
# set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)

# ============================================================
# 子模块 / Sub-modules
# ============================================================
add_subdirectory(src/core)
add_subdirectory(src/app)
add_subdirectory(src/logic)
add_subdirectory(src/presentation)
add_subdirectory(src/platform)

# ============================================================
# 主可执行文件 / Main executable
# ============================================================
add_executable(dlzr src/main.cpp src/heartbeat.cpp)
target_link_libraries(dlzr PRIVATE
    dlzr_app
    dlzr_logic
    dlzr_presentation
    dlzr_platform
)

# 部署引擎自带数据文件（对话组表/映射表，从原版 EXE 抽取）到 exe 目录，
# 供 DialogueManager::init_dialogue_groups 运行时读取。
set(_DLZR_ENGINE_DATA "${CMAKE_CURRENT_SOURCE_DIR}/src/app/resource/data")
foreach(_bin dialogue_groups.bin dialogue_map.bin)
    add_custom_command(TARGET dlzr POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${_DLZR_ENGINE_DATA}/${_bin}"
            "$<TARGET_FILE_DIR:dlzr>/${_bin}"
        COMMENT "Copy engine data ${_bin} next to dlzr.exe"
    )
endforeach()

# Emscripten 特定配置 / Emscripten-specific configuration
if(EMSCRIPTEN)
    # 内存配置 / Memory configuration
    target_link_options(dlzr PRIVATE
        "SHELL:-s USE_SDL=0"
        "SHELL:-s ALLOW_MEMORY_GROWTH=1"
        "SHELL:-s INITIAL_MEMORY=67108864"
        "SHELL:-s MAX_WEB_IMAGE_SIZE=16777216"
    )

    # 自定义 HTML 模板 / Custom HTML shell
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html")
        target_link_options(dlzr PRIVATE
            "SHELL:--shell-file ${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html"
        )
    endif()

    # 资源预加载 / Asset preloading
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data")
        target_link_options(dlzr PRIVATE
            "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/data@/data"
        )
    endif()

    # 启用异常支持 / Enable exception support
    target_link_options(dlzr PRIVATE
        "SHELL:-s DISABLE_EXCEPTION_CATCHING=0"
    )

    # 优化选项 / Optimization options
    target_link_options(dlzr PRIVATE
        "SHELL:-s ENVIRONMENT=web,worker"
        "SHELL:-s EXPORTED_RUNTIME_METHODS=['ccall','cwrap']"
    )
endif()

# ============================================================
# 测试 / Tests
# ============================================================
option(DLZR_BUILD_TESTS "Build test suite" ON)
if(DLZR_BUILD_TESTS)
    enable_testing()

    if(FETCHCONTENT_FULLY_DISCONNECTED)
        # 离线模式：直接使用已下载的源码 / Offline mode: use already downloaded sources
        if(EXISTS "${CMAKE_BINARY_DIR}/_deps/googletest-src")
            add_subdirectory(${CMAKE_BINARY_DIR}/_deps/googletest-src ${CMAKE_BINARY_DIR}/_deps/googletest-build)
        endif()
    else()
        FetchContent_Declare(googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.14.0
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(googletest)
    endif()

    add_subdirectory(tests)
endif()
