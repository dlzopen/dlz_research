# dlzr_logic — 游戏逻辑层：战斗公式、状态机、AI
# Game logic layer: combat formulas, state machine, AI

add_library(dlzr_logic STATIC
    combat/combat_formulas.cpp
    battle/battle_engine.cpp
    battle/battle_settlement.cpp
    dialogue/dialogue_manager.cpp
    forging/forging_system.cpp
    level/level_event_manager.cpp
    promotion/promotion_system.cpp
    game_state_manager.cpp
)
target_include_directories(dlzr_logic PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)
target_link_libraries(dlzr_logic PUBLIC dlzr_core)
