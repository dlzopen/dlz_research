#pragma once

#include "platform/platform.hpp"

#include <cstdint>

namespace dlzr::presentation {

enum class DialogueAction {
    NONE,
    ADVANCE,
    DISMISS,
};

class DialogueInput {
public:
    DialogueAction process_event(const platform::Event& event);
};

}  // namespace dlzr::presentation
