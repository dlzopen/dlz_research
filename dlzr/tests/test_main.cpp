// dlzr_tests — Google Test 入口
// Google Test entry point

#include <gtest/gtest.h>

// 基础验证：结构体大小 / Basic verification: struct sizes
#include "core/data_model/types.hpp"
#include "core/data_model/constants.hpp"

TEST(DataModel, StructSizes) {
    EXPECT_EQ(sizeof(dlzr::core::People), 0x84);
    EXPECT_EQ(sizeof(dlzr::core::Fighter), 0x40);
    EXPECT_EQ(sizeof(dlzr::core::ClassType), 0xA0);
    EXPECT_EQ(sizeof(dlzr::core::JiNeng), 48);
    EXPECT_EQ(sizeof(dlzr::core::ImgHeader), 16);
}

TEST(DataModel, Constants) {
    EXPECT_EQ(dlzr::core::SCREEN_WIDTH, 640);
    EXPECT_EQ(dlzr::core::SCREEN_HEIGHT, 480);
    EXPECT_EQ(dlzr::core::ANM_WIDTH, 320);
    EXPECT_EQ(dlzr::core::ANM_HEIGHT, 200);
    EXPECT_EQ(dlzr::core::SAVE_SLOT_COUNT, 10);
    EXPECT_EQ(dlzr::core::SAVE_DATA_SIZE, 85072);
}
