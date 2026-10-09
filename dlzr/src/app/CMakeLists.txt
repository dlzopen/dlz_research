# dlzr_app — 应用层：资源加载、数据管理
# Application layer: resource loading, data management

add_library(dlzr_app STATIC
    resource/dat_loader.cpp
    resource/img_loader.cpp
    resource/anm_loader.cpp
    resource/flm_loader.cpp
    resource/flc_loader.cpp
    resource/save_loader.cpp
    resource/face_loader.cpp
)
target_include_directories(dlzr_app PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
target_link_libraries(dlzr_app PUBLIC dlzr_core)
