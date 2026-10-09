# dlzr_core — 核心数据模型（当前为 header-only）
# Core data model (currently header-only)

add_library(dlzr_core INTERFACE)
target_include_directories(dlzr_core INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
