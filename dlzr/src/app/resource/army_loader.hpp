#pragma once

// army_loader.hpp — ARMY.DAT 部队图标/动画加载器
// ARMY.DAT unit icon/animation loader
//
// 91 unit entries, each with 19 animation frames (48×48 pixels, 8-bit palette).
// Total per entry: 19 × 48 × 48 = 0xAB00 bytes.
//
// 19-Frame Layout (from RE: FUN_004170aa, FUN_0041e5f1):
//   Frame  0:    Idle Left (2 frames alternating: 0,1)
//   Frame  1:    Idle Right
//   Frame  2-3:  Move North (dir=0)
//   Frame  4-5:  Move South (dir=1)
//   Frame  6-7:  Move West  (dir=2)
//   Frame  8-9:  Move East  (dir=3)
//   Frame 10-11: Attack North (dir=0)
//   Frame 12-13: Attack South (dir=1)
//   Frame 14-15: Attack West  (dir=2)
//   Frame 16-17: Attack East  (dir=3)
//   Frame    18: Hit/Death (1 frame)
//
// Frame selection formula (from FUN_004170aa):
//   action_type == 0 (idle):   frame = anim_counter % 2
//   action_type == 1 (move):   frame = (direction % 4) * 2 + 2 + anim_counter % 2
//   action_type == 2 (attack): frame = (direction % 4) * 2 + 10 + anim_counter % 2
//   action_type == 3 (hit):    frame = 18
//
// Direction encoding (Fighter+0x28): 0=N, 1=S, 2=W, 3=E

#include <cstdint>
#include <string>
#include <vector>

namespace dlzr::app {

class ArmyLoader {
public:
    static constexpr int32_t ICON_WIDTH = 48;
    static constexpr int32_t ICON_HEIGHT = 48;
    static constexpr int32_t ICON_SIZE = ICON_WIDTH * ICON_HEIGHT;
    static constexpr int32_t FRAME_COUNT = 19;
    static constexpr int32_t ENTRY_SIZE = FRAME_COUNT * ICON_SIZE;  // 0xAB00
    static constexpr int32_t ENTRY_COUNT = 91;

    // Frame indices
    static constexpr int32_t FRAME_IDLE_LEFT = 0;
    static constexpr int32_t FRAME_IDLE_RIGHT = 1;
    static constexpr int32_t FRAME_MOVE_N_0 = 2;
    static constexpr int32_t FRAME_MOVE_N_1 = 3;
    static constexpr int32_t FRAME_MOVE_S_0 = 4;
    static constexpr int32_t FRAME_MOVE_S_1 = 5;
    static constexpr int32_t FRAME_MOVE_W_0 = 6;
    static constexpr int32_t FRAME_MOVE_W_1 = 7;
    static constexpr int32_t FRAME_MOVE_E_0 = 8;
    static constexpr int32_t FRAME_MOVE_E_1 = 9;
    static constexpr int32_t FRAME_ATK_N_0 = 10;
    static constexpr int32_t FRAME_ATK_N_1 = 11;
    static constexpr int32_t FRAME_ATK_S_0 = 12;
    static constexpr int32_t FRAME_ATK_S_1 = 13;
    static constexpr int32_t FRAME_ATK_W_0 = 14;
    static constexpr int32_t FRAME_ATK_W_1 = 15;
    static constexpr int32_t FRAME_ATK_E_0 = 16;
    static constexpr int32_t FRAME_ATK_E_1 = 17;
    static constexpr int32_t FRAME_HIT = 18;

    // Direction enum for frame lookup
    enum class Direction : int32_t {
        NORTH = 0,
        SOUTH = 1,
        WEST = 2,
        EAST = 3,
    };

    bool load(const std::string& path);

    // Get first frame (idle frame 0) as RGBA — for UI icons
    const uint8_t* get_icon_rgba(int32_t entry_index) const;

    // Get specific animation frame as RGBA
    const uint8_t* get_frame_rgba(int32_t entry_index, int32_t frame_index) const;

    // Get frame by action state and direction
    const uint8_t* get_action_frame_rgba(int32_t entry_index,
                                         int32_t action_type,
                                         Direction direction,
                                         int32_t anim_counter) const;

    bool is_loaded() const { return !raw_data_.empty(); }

    // Authoritative 256×4 RGBA palette (exposed for differential tests verifying
    // byte-faithful decode against the DLZFULL/ARMY.DAT ground truth).
    static const uint8_t* palette();

private:
    std::vector<uint8_t> raw_data_;
    std::vector<std::vector<uint8_t>> rgba_cache_;  // [entry * FRAME_COUNT + frame]

    void build_rgba_cache();
};

}  // namespace dlzr::app
