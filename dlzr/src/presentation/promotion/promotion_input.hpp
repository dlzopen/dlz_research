#pragma once

// promotion_input.hpp — 转职界面输入处理
// Class promotion screen input handler

#include "platform/platform.hpp"

#include <cstdint>

namespace dlzr::presentation {

enum class PromotionAction {
    NONE,
    BACK,
    SELECT,
    PROMOTE,
};

class PromotionInput {
public:
    PromotionAction process_event(const platform::Event& event,
                                   int32_t& selected_char);
};

}  // namespace dlzr::presentation
