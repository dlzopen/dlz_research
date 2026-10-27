// test_heap_character_select.cpp — 无头堆越界实证测试
// 用真实 DLZFULL 资源跑 CharacterSelectRenderer::render()，配合 AddressSanitizer
// 编译，在越界写入/读取的那一行精确报出（谁写的 bug 一目了然）。
//
// 用法：cmake -DDLZR_ASAN=ON ... 后运行 dlzr_tests
// 数据目录：环境变量 DLZR_TEST_DATA，否则回退到已知路径。

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "platform/platform.hpp"
#include "app/resource/face_loader.hpp"
#include "app/resource/army_loader.hpp"
#include "presentation/font/font_renderer.hpp"
#include "presentation/image/image_cache.hpp"
#include "presentation/character_select/character_select_renderer.hpp"

namespace {

// 无头 Mock 平台：实现 Platform 接口，不初始化 SDL/D3D。
// 所有平台类型用全限定名，避免命名空间歧义。
class MockPlatform : public dlzr::platform::Platform {
public:
    bool init(const dlzr::platform::PlatformConfig&) override { return true; }
    void shutdown() override {}

    bool poll_event(dlzr::platform::Event&) override { return false; }

    void clear(uint8_t, uint8_t, uint8_t) override {}
    void present() override {}

    dlzr::platform::TextureHandle create_texture(int32_t, int32_t) override {
        return next_++;
    }
    void update_texture(dlzr::platform::TextureHandle,
                        const uint8_t*, int32_t) override {
        // 不拷贝：越界检测交给 ASan 对调用方缓冲（font/loader 的 std::vector）的红区。
    }
    void draw_texture(dlzr::platform::TextureHandle, int32_t, int32_t) override {}
    void draw_texture_rect(dlzr::platform::TextureHandle, int32_t, int32_t,
                           int32_t, int32_t) override {}
    void draw_texture_sub(dlzr::platform::TextureHandle, int32_t, int32_t, int32_t,
                          int32_t, int32_t, int32_t) override {}
    void destroy_texture(dlzr::platform::TextureHandle) override {}

    void draw_rect(int32_t, int32_t, int32_t, int32_t, uint8_t, uint8_t, uint8_t) override {}
    void fill_rect(int32_t, int32_t, int32_t, int32_t, uint8_t, uint8_t, uint8_t) override {}
    void draw_line(int32_t, int32_t, int32_t, int32_t, uint8_t, uint8_t, uint8_t) override {}

    dlzr::platform::Platform::AudioHandle play_sound(const uint8_t*, size_t,
                                                     int32_t) override {
        return 0;
    }
    void stop_sound(dlzr::platform::Platform::AudioHandle) override {}
    bool is_sound_playing(dlzr::platform::Platform::AudioHandle) override { return false; }

    bool play_music(const std::string&, bool) override { return true; }
    void stop_music() override {}
    void set_music_volume(int32_t) override {}

    void delay_ms(uint32_t) override {}
    uint32_t ticks_ms() override { return tick_++; }

    void start_text_input() override {}
    void stop_text_input() override {}

private:
    dlzr::platform::TextureHandle next_{1};
    uint32_t tick_{0};
};

std::string data_dir() {
    const char* e = std::getenv("DLZR_TEST_DATA");
    if (e && e[0]) return e;
    return "C:/seesaw/source/dlz_research/DLZFULL";
}

}  // namespace

TEST(CharacterSelectHeap, RenderManyFramesNoOverflow) {
    MockPlatform mp;
    fprintf(stderr, "[T] load faces\n"); fflush(stderr);
    dlzr::app::FaceLoader faces;
    ASSERT_TRUE(faces.load(data_dir() + "/FACE.DAT")) << "FACE.DAT 加载失败";

    fprintf(stderr, "[T] load army\n"); fflush(stderr);
    dlzr::app::ArmyLoader army;
    ASSERT_TRUE(army.load(data_dir() + "/ARMY.DAT")) << "ARMY.DAT 加载失败";

    fprintf(stderr, "[T] font init\n"); fflush(stderr);
    dlzr::presentation::FontRenderer font;
    ASSERT_TRUE(font.init(mp, data_dir())) << "字体初始化失败(HZK16/ASC16)";

    fprintf(stderr, "[T] imagecache init\n"); fflush(stderr);
    dlzr::presentation::ImageCache images;
    images.init(mp, data_dir());

    fprintf(stderr, "[T] csr init\n"); fflush(stderr);
    dlzr::presentation::CharacterSelectRenderer csr;
    csr.init(mp, font, images, faces, army);
    fprintf(stderr, "[T] init done\n"); fflush(stderr);

    // 真实项目 class_types[1,4,8,11].name 的 GBK 串（与原版一致）
    const char* fnames[4] = {
        "\xB2\xBD\xB2\xBD",   // 步兵
        "\xC6\xD5\xB1\xF8",   // 骑兵
        "\xB9\xE6\xB1\xF8",   // 弓兵
        "\xBE\xFC\xCA\xF8",   // 军师
    };

    std::string player_names[] = {
        "", "赵云", "abcdef", "李逍遥xyz",
        "\xD2\xBB\xB6\xFE\xC8\xFD\xCB\xC4\xCE\xE5\xC1\xF9\xC6\xDF\xCB\xC4\xCA\xAE",
    };

    for (int f = 0; f < 5000; ++f) {
        int sel_p = f % 6;
        int sel_f = f % 4;
        int slider = 60 + (f % 41);          // 60..100
        int diff = f % 3;                     // 0..2
        bool naming = (f % 7 == 0);
        int hovered = f % 3;
        int pressed = (f % 5 == 0) ? (f % 3) : -1;
        std::string pn = player_names[f % 5];

        fprintf(stderr, "[T] frame %d render enter\n", f); fflush(stderr);
        csr.render(sel_p, sel_f, slider, diff, pn, fnames,
                   hovered, pressed, naming);
        fprintf(stderr, "[T] frame %d render done\n", f); fflush(stderr);

        if (f % 50 == 0) csr.trigger_formation_anim(f % 4);
        csr.update_anim(16);

        mp.present();  // 无操作
    }
    // 若走到这里说明 5000 帧无堆越界（ASan 会在越界处直接 abort）。
    SUCCEED();
}
