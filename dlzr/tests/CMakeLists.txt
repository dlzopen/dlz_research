# dlzr_tests — 单元测试
# Unit tests (Google Test)

add_executable(dlzr_tests
    test_main.cpp
    test_combat_formulas.cpp
    test_game_state_manager.cpp
    test_battle_settlement.cpp
    test_first_level.cpp
    test_heap_character_select.cpp
    ${CMAKE_SOURCE_DIR}/src/heartbeat.cpp
)
target_link_libraries(dlzr_tests PRIVATE
    dlzr_core
    dlzr_app
    dlzr_logic
    dlzr_presentation
    GTest::gtest
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(dlzr_tests)

# 真实 Sdl3Platform 下的堆越界实证驱动（独立 main，无 GTest SEH 包裹）
add_executable(dlzr_asan_cs
    asan_cs_driver.cpp
    ${CMAKE_SOURCE_DIR}/src/heartbeat.cpp
)
target_link_libraries(dlzr_asan_cs PRIVATE
    dlzr_core
    dlzr_app
    dlzr_logic
    dlzr_presentation
    dlzr_platform
    dbghelp
)
