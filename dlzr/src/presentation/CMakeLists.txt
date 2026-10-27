# dlzr_presentation — 表现层：渲染、音频、UI
# Presentation layer: rendering, audio, UI

add_library(dlzr_presentation STATIC
    font/font_renderer.cpp
    image/image_cache.cpp
    audio/audio_manager.cpp
    animation/animation_player.cpp
    battle/battle_renderer.cpp
    battle/battle_input_handler.cpp
    battle/battle_result_renderer.cpp
    battle/battle_animation_overlay.cpp
    battle/battle_animation_controller.cpp
    forging/forging_renderer.cpp
    forging/forging_input.cpp
    promotion/promotion_renderer.cpp
    promotion/promotion_input.cpp
    strategic/strategic_map_renderer.cpp
    strategic/strategic_map_input.cpp
    strategic/town_renderer.cpp
    strategic/town_input.cpp
    strategic/shop_renderer.cpp
    strategic/equipment_renderer.cpp
    strategic/warehouse_renderer.cpp
    strategic/save_load_renderer.cpp
    strategic/level_select_renderer.cpp
    strategic/level_select_input.cpp
    title/title_renderer.cpp
    title/title_input.cpp
    character_select/character_select_renderer.cpp
    character_select/character_select_input.cpp
    dialogue/dialogue_renderer.cpp
    dialogue/dialogue_input.cpp
    ending/ending_renderer.cpp
    opening/opening_renderer.cpp
)
target_include_directories(dlzr_presentation PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
target_link_libraries(dlzr_presentation PUBLIC
    dlzr_core
    dlzr_logic
    dlzr_platform
)
