# dlzr_platform — 平台抽象层 / Platform abstraction layer
# 支持多后端：SDL3（生产）、Mock（测试）
# Multi-backend support: SDL3 (production), Mock (testing)

set(PLATFORM_SOURCES
    sdl3/sdl3_platform.cpp
    crash_handler.cpp
)

set(PLATFORM_LINK_LIBS
    dlzr_core
    SDL3::SDL3
)

if(WIN32)
    list(APPEND PLATFORM_LINK_LIBS dbghelp)
endif()

# SDL3_mixer temporarily disabled due to version mismatch
# if(TARGET SDL3_mixer::SDL3_mixer)
#     list(APPEND PLATFORM_LINK_LIBS SDL3_mixer::SDL3_mixer)
#     target_compile_definitions(dlzr_platform PRIVATE DLZR_HAS_SDL3_MIXER)
# endif()

# 测试构建时添加 Mock 后端 / Add Mock backend for test builds
if(BUILD_TESTING)
    list(APPEND PLATFORM_SOURCES mock/mock_platform.cpp)
endif()

add_library(dlzr_platform STATIC ${PLATFORM_SOURCES})
target_include_directories(dlzr_platform PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
target_link_libraries(dlzr_platform PUBLIC ${PLATFORM_LINK_LIBS})
