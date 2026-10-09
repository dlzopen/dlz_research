cmake_minimum_required(VERSION 3.20)
project(dlzr LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# MSVC: UTF-8 源文件 / UTF-8 source files
if(MSVC)
    add_compile_options(/utf-8)
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
add_executable(dlzr src/main.cpp)
target_link_libraries(dlzr PRIVATE
    dlzr_app
    dlzr_logic
    dlzr_presentation
    dlzr_platform
)

# Emscripten 特定配置 / Emscripten-specific configuration
if(EMSCRIPTEN)
    # 内存配置 / Memory configuration
    target_link_options(dlzr PRIVATE
        "SHELL:-s USE_SDL=0"
        "SHELL:-s ALLOW_MEMORY_GROWTH=1"
        "SHELL:-s INITIAL_MEMORY=67108864"
        "SHELL:-s MAX_WEB_IMAGE_SIZE=16777216"
    )

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

    FetchContent_Declare(googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(googletest)

    add_subdirectory(tests)
endif()
