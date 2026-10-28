# dlzr_tests — 单元测试
# Unit tests (Google Test)

add_executable(dlzr_tests
    test_main.cpp
    test_combat_formulas.cpp
    test_game_state_manager.cpp
    test_battle_settlement.cpp
    test_first_level.cpp
    test_heap_character_select.cpp
    test_battle_calc.cpp
    test_cross_layer_diff.cpp
    test_skill_grid_diff.cpp
    test_map_pathfinding.cpp
    test_battle_setup.cpp
    test_dialogue.cpp
    test_dialogue_branch.cpp
    test_txt_bulk.cpp
    test_txt_loader.cpp
    test_dialogue_indexing.cpp
    test_game_dat_blocks.cpp
    test_game_dat_content.cpp
    test_combat_formula_anchors.cpp
    test_campaign_recruitment.cpp
    test_case9_item_recovery.cpp
    test_boot.cpp
    test_dialogue_render.cpp
    test_dialogue_format.cpp
    test_dialogue_playback.cpp
    test_dialogue_real_entries.cpp
    test_replica_stubs.cpp
    test_stateful_stubs.cpp
    test_dat_io.cpp
    test_movement_range.cpp
    test_bfs_destination.cpp
    test_retreat_destination.cpp
    test_enemy_ai_dispatch.cpp
    test_save_system.cpp
    test_game_flow.cpp
    test_forging_system.cpp
    generated_stub_registry.cpp
    ${CMAKE_SOURCE_DIR}/src/heartbeat.cpp
)
target_link_libraries(dlzr_tests PRIVATE
    dlzr_core
    dlzr_app
    dlzr_logic
    dlzr_presentation
    dlzr_replica
    GTest::gtest
    GTest::gtest_main
)
# 真实数据 IO 集成测试：数据目录默认指向仓库根 DLZFULL，可被 env DLZR_DATA_DIR 覆盖
target_compile_definitions(dlzr_tests PRIVATE
    DLZR_DATA_DIR_DEFAULT="${CMAKE_SOURCE_DIR}/../DLZFULL"
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
