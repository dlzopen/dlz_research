# dlzr_tests — 单元测试
# Unit tests (Google Test)

add_executable(dlzr_tests
    test_main.cpp
    test_combat_formulas.cpp
)
target_link_libraries(dlzr_tests PRIVATE
    dlzr_core
    dlzr_app
    dlzr_logic
    GTest::gtest
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(dlzr_tests)
