#include "presentation/opening/opening_renderer.hpp"

#include "heartbeat.hpp"

namespace dlzr::presentation {

// ---------------------------------------------------------------------------
// 开场幻灯片 — 全部结论来自反编译指令流，无一处推断。
//
// 证据 1: output/ghidra_all/FUN_0042e0f1.c
//   - FUN_0043c49f(0x2e) + FUN_0043c731(): 选轨并播放 BGM（0x2e 是音轨号，
//     FUN_0040696e 里是 mciSendCommandA MCI_PLAY —— 前人误读为"载入对话"已纠）；
//   - FUN_00426059(s_npic25_img,...) 读入 NPIC25.IMG（实测 609x275）；
//   - FUN_00422627(0,0,0x280,0x1e0,底色) 铺满 640x480；
//   - FUN_00422889((640-w)/2,(480-h)/2,w,h,...) 居中贴图；
//   - 模态 GetMessageA 循环：WM_TIMER(0x113) → DAT_007d0cf4++ →
//     FUN_0042dc9e(DAT_007d0cf4)；WM_LBUTTONDOWN(0x201) → 退出循环。
//
// 证据 2: output/ghidra_all/FUN_0042dc9e.c —— WM_TIMER 计数器各阈值换图：
//     0x113(275)→918-2   0x226(550)→918-3   0x352(850)→77-1
//     0x5aa(1450)→77-2   0x753(1875)→NPIC18 2000→NPIC19
//     0x84d(2125)→NPIC20 0x8ca(2250)→NPIC21 0x947(2375)→NPIC22
//     0x9c4(2500)→NPIC29 0xb22(2850)→NPIC08 0xbd1(3025)→NPIC07
//     0xc80(3200)→NPIC06 0xd2f(3375)→NPIC05 0xdde(3550)→NPIC04
//     0xe8d(3725)→NPIC03 0xf3c(3900)→NPIC02 0xfeb(4075)→NPIC01
//   （Ghidra 标签 s_918_2_img 等中的 '_' 对应实际文件名中的 '-'，
//    磁盘实测: DLZFULL/918-2.IMG / 918-3.IMG / 77-1.IMG / 77-2.IMG）
//
// 证据 3: output/ghidra_all/FUN_004249a2.c:12
//   SetTimer(DAT_007db4ec, 1, 0x37, NULL) —— 定时器间隔 0x37 = 55ms。
//   故各屏时长 = (下一阈值 - 本阈值) * 55ms。
//
// 证据 4（原版实截图 ground truth）: screen/walk19/w19_slide_001.png
//   —— 黑底 + 居中历史照片，无任何文字，与上述 RE 一致。
//   旧日记"幻灯片仅单屏 NPIC25"结论错误，已按 FUN_0042dc9e 指令流纠正。
//
// 整个幻灯片**没有任何文字叠加**（FUN_0042dc9e 只换图不画字）；
// 旧版自拟的叙事文字与"（点击鼠标继续）"提示均属虚构，已删除。
// ---------------------------------------------------------------------------

// 定时器节拍（FUN_004249a2: 0x37 = 55ms）
static constexpr uint32_t kTimerTickMs = 55;

struct SlideMark {
    uint32_t tick;        // FUN_0042dc9e 的计数器阈值
    const char* image;    // 该阈值换入的图片
};

// FUN_0042dc9e.c 的阈值表，按先后顺序排列（首屏 NPIC25 在进循环前已画）
static constexpr SlideMark kSlides[] = {
    {0,     "NPIC25.IMG"},
    {275,   "918-2.IMG"},
    {550,   "918-3.IMG"},
    {850,   "77-1.IMG"},
    {1450,  "77-2.IMG"},
    {1875,  "NPIC18.IMG"},
    {2000,  "NPIC19.IMG"},
    {2125,  "NPIC20.IMG"},
    {2250,  "NPIC21.IMG"},
    {2375,  "NPIC22.IMG"},
    {2500,  "NPIC29.IMG"},
    {2850,  "NPIC08.IMG"},
    {3025,  "NPIC07.IMG"},
    {3200,  "NPIC06.IMG"},
    {3375,  "NPIC05.IMG"},
    {3550,  "NPIC04.IMG"},
    {3725,  "NPIC03.IMG"},
    {3900,  "NPIC02.IMG"},
    {4075,  "NPIC01.IMG"},   // 末屏：无下一阈值，等待左键结束
};
static constexpr int kSlideCount = static_cast<int>(sizeof(kSlides) / sizeof(kSlides[0]));

void OpeningRenderer::init(platform::Platform& platform, FontRenderer& font, ImageCache& images) {
    platform_ = &platform;
    font_ = &font;
    images_ = &images;
}

void OpeningRenderer::start(const logic::GameStateManager& gsm) {
    (void)gsm;
    scenes_.clear();
    current_scene_ = 0;
    scene_timer_ = 0;
    playing_ = true;
    finished_ = false;
    requested_bg_ = -1;

    build_sequence(gsm);
}

void OpeningRenderer::build_sequence(const logic::GameStateManager& gsm) {
    (void)gsm;
    for (int i = 0; i < kSlideCount; ++i) {
        OpeningScene scene;
        scene.image = kSlides[i].image;
        if (i + 1 < kSlideCount) {
            scene.duration_ms =
                static_cast<int32_t>((kSlides[i + 1].tick - kSlides[i].tick) * kTimerTickMs);
        } else {
            scene.duration_ms = 0;   // 末屏等待左键（原版仅 0x201 结束）
        }
        scenes_.push_back(std::move(scene));
    }
}

bool OpeningRenderer::update(uint32_t delta_ms) {
    if (!playing_ || finished_) return true;
    if (current_scene_ >= scenes_.size()) {
        finished_ = true;
        playing_ = false;
        return true;
    }

    scene_timer_ += delta_ms;
    const auto& scene = scenes_[current_scene_];

    // 原版靠 WM_TIMER(0x113) 自增计数器推进换图；这里按各屏时长翻页。
    if (scene.duration_ms > 0 &&
        scene_timer_ >= static_cast<uint32_t>(scene.duration_ms)) {
        advance_scene();
    }

    return finished_;
}

void OpeningRenderer::render() {
    if (!playing_ || finished_ || current_scene_ >= scenes_.size()) return;

    // 原版 FUN_0042e0f1 的绘制流程（RE 实锤）：
    //   1) FUN_00422627(0,0,0x280,0x1e0,底色)       —— 先用底色铺满 640x480
    //   2) FUN_00422889((0x280-w)/2,(0x1e0-h)/2,w,h) —— 再把当前图**居中**贴上
    // 每屏宽度不同，按各自 w/h 居中（数据驱动，走 ImageCache 实测尺寸）。
    platform_->fill_rect(0, 0, 640, 480, 0, 0, 0);

    const auto& scene = scenes_[current_scene_];
    dlzr::hb_step(scene.image.c_str());
    int32_t iw = images_->image_width(scene.image);
    int32_t ih = images_->image_height(scene.image);
    images_->draw_image(scene.image, (640 - iw) / 2, (480 - ih) / 2);
}

bool OpeningRenderer::process_input(const platform::Event& event) {
    if (!playing_ || finished_) return false;

    // 原版 FUN_0042e0f1 是模态循环，**结束条件只有 WM_LBUTTONDOWN(0x201)**，
    // 完全不响应键盘消息（换图走 WM_TIMER 0x113）。
    // 早期"任意键快进"会把 PrintScreen 等系统键当成翻页键吞掉，玩家无法截图。
    // 这里对齐原版：只认鼠标左键，键盘一律不处理。
    if (event.type == platform::Event::Type::MouseDown && event.mouse_button == 1) {
        advance_scene();
        return true;
    }

    return false;
}

void OpeningRenderer::advance_scene() {
    ++current_scene_;
    scene_timer_ = 0;

    if (current_scene_ >= scenes_.size()) {
        finished_ = true;
        playing_ = false;
    }
}

}  // namespace dlzr::presentation
