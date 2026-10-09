#include "presentation/dialogue/dialogue_input.hpp"

namespace dlzr::presentation {

DialogueAction DialogueInput::process_event(const platform::Event& event) {
    if (event.type == platform::Event::Type::MouseDown &&
        event.mouse_button == 1) {
        return DialogueAction::ADVANCE;
    }

    if (event.type == platform::Event::Type::KeyDown) {
        if (event.key == 0x0D || event.key == ' ') {
            return DialogueAction::ADVANCE;
        }
        if (event.key == 0x1B) {
            return DialogueAction::DISMISS;
        }
    }

    return DialogueAction::NONE;
}

}  // namespace dlzr::presentation
