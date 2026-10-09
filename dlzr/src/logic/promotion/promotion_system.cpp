// promotion_system.cpp — 转职系统实现
// Class promotion system implementation

#include "logic/promotion/promotion_system.hpp"

#include "core/data_model/constants.hpp"

namespace dlzr::logic {

void PromotionSystem::init(core::GameData& game_data) {
    game_data_ = &game_data;
}

bool PromotionSystem::can_promote(int32_t people_idx) const {
    if (!game_data_ || people_idx < 0 || people_idx >= core::MAX_PEOPLE) {
        return false;
    }

    const auto& person = game_data_->people[people_idx];
    if (person.enemy != 0) return false;

    int32_t class_idx = person.class_type;
    if (class_idx < 0 || class_idx >= core::MAX_CLASS_TYPES) return false;

    const auto& cls = game_data_->class_types[class_idx];
    if (cls.promotion_class < 0) return false;

    if (person.level < MIN_PROMOTION_LEVEL) return false;

    int32_t required_item = cls.promotion_item;
    if (required_item < 0) return false;

    for (int i = 0; i < 8; ++i) {
        if (person.items[i] == static_cast<uint8_t>(required_item)) {
            return true;
        }
    }

    return false;
}

void PromotionSystem::execute_promotion(int32_t people_idx) {
    if (!game_data_ || people_idx < 0 || people_idx >= core::MAX_PEOPLE) return;
    if (!can_promote(people_idx)) return;

    auto& person = game_data_->people[people_idx];
    int32_t class_idx = person.class_type;
    const auto& cls = game_data_->class_types[class_idx];

    person.class_type = cls.promotion_class;

    int32_t required_item = cls.promotion_item;
    for (int i = 0; i < 8; ++i) {
        if (person.items[i] == static_cast<uint8_t>(required_item)) {
            person.items[i] = 0;
            break;
        }
    }
}

int32_t PromotionSystem::get_promotion_target(int32_t people_idx) const {
    if (!game_data_ || people_idx < 0 || people_idx >= core::MAX_PEOPLE) return -1;

    const auto& person = game_data_->people[people_idx];
    int32_t class_idx = person.class_type;
    if (class_idx < 0 || class_idx >= core::MAX_CLASS_TYPES) return -1;

    return game_data_->class_types[class_idx].promotion_class;
}

int32_t PromotionSystem::get_required_item(int32_t people_idx) const {
    if (!game_data_ || people_idx < 0 || people_idx >= core::MAX_PEOPLE) return -1;

    const auto& person = game_data_->people[people_idx];
    int32_t class_idx = person.class_type;
    if (class_idx < 0 || class_idx >= core::MAX_CLASS_TYPES) return -1;

    return game_data_->class_types[class_idx].promotion_item;
}

}  // namespace dlzr::logic
