// promotion_input.cpp — 转职界面输入处理实现
// Class promotion screen input handler implementation

#include "presentation/promotion/promotion_input.hpp"

namespace dlzr::presentation {

PromotionAction PromotionInput::process_event(const platform::Event& event,
                                               int32_t& selected_char) {
    if (event.type == platform::Event::Type::KeyDown) {
        if (event.key == 0x1B) {
            return PromotionAction::BACK;
        }
        if (event.key == 0x0D) {
            return PromotionAction::PROMOTE;
        }
        if (event.key == 0x40000052 || event.key == 'w') {
            if (selected_char >= 2) selected_char -= 2;
            return PromotionAction::SELECT;
        }
        if (event.key == 0x40000051 || event.key == 's') {
            selected_char += 2;
            return PromotionAction::SELECT;
        }
        if (event.key == 0x40000050 || event.key == 'a') {
            if (selected_char % 2 > 0) --selected_char;
            return PromotionAction::SELECT;
        }
        if (event.key == 0x4000004F || event.key == 'd') {
            if (selected_char % 2 < 1) ++selected_char;
            return PromotionAction::SELECT;
        }
    }

    if (event.type == platform::Event::Type::MouseDown) {
        if (event.mouse_button == 1) {
            return PromotionAction::SELECT;
        }
        if (event.mouse_button == 3) {
            return PromotionAction::BACK;
        }
    }

    return PromotionAction::NONE;
}

}  // namespace dlzr::presentation
