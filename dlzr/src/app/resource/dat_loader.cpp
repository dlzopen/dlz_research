// dat_loader.cpp — DAT 文件加载实现
// DAT file loading implementation
//
// 直接读取原始二进制文件，结构体布局严格匹配原作内存布局。
// Reads original binary files directly; struct layouts match original memory.

#include "app/resource/dat_loader.hpp"

#include "core/data_model/constants.hpp"

#include <cstring>
#include <stdexcept>

namespace dlzr::app {

core::GameData DatLoader::load_game_data() const {
    core::GameData gd;

    // PEOPLE.DAT: 40 records × 132 bytes
    {
        BinaryReader reader(resolve("PEOPLE.DAT"));
        static_assert(core::PEOPLE_RECORD_SIZE == 132);
        for (int i = 0; i < core::MAX_PEOPLE; ++i) {
            size_t offset = static_cast<size_t>(i) * core::PEOPLE_RECORD_SIZE;
            reader.read_bytes(offset, &gd.people[i], sizeof(core::People));
        }
    }

    // GAME.DAT: multiple sections
    // Layout (from binary template analysis):
    //   +0x000: What0 (160 bytes)
    //   +0x0A0: DiXingModifier[16] (320 bytes)
    //   +0x1E0: BingZhong[96] (15360 bytes) — ClassType
    //   +0x3FE0: BingZhongYiDong[96] (1536 bytes)
    //   +0x43E0: WuPin[160] (11520 bytes) — Items
    //   +0x70E0: Material[10] (280 bytes)
    //   +0x71F8: DiLeiRecipe[10] (100 bytes) — actually at 29176
    //   +0x725C: DiLei[10] (480 bytes) — actually at 29276
    //   +0x743C: JiNeng[16] (640 bytes) — actually at 29756
    {
        BinaryReader reader(resolve("GAME.DAT"));

        // ClassType (BingZhong): offset 0x1E0 = 480
        constexpr size_t kClassTypeOffset = 0x1E0;
        for (int i = 0; i < core::MAX_CLASS_TYPES; ++i) {
            size_t offset = kClassTypeOffset + static_cast<size_t>(i) * sizeof(core::ClassType);
            if (offset + sizeof(core::ClassType) > reader.size()) break;
            reader.read_bytes(offset, &gd.class_types[i], sizeof(core::ClassType));
        }

        // WuPin (items): offset 0x43E0 = 17376
        constexpr size_t kWuPinOffset = 0x43E0;
        for (int i = 0; i < core::MAX_WUPIN; ++i) {
            size_t offset = kWuPinOffset + static_cast<size_t>(i) * sizeof(core::WuPin);
            if (offset + sizeof(core::WuPin) > reader.size()) break;
            reader.read_bytes(offset, &gd.items[i], sizeof(core::WuPin));
        }

        // DiLei (mines): offset after Material and DiLeiRecipe
        // Material[10] at 28896, size 280 → 29176
        // DiLeiRecipe[10] at 29176, size 100 → 29276
        // DiLei[10] at 29276
        constexpr size_t kDiLeiOffset = 29276;
        for (int i = 0; i < core::MAX_DILEI; ++i) {
            size_t offset = kDiLeiOffset + static_cast<size_t>(i) * sizeof(core::DiLei);
            if (offset + sizeof(core::DiLei) > reader.size()) break;
            reader.read_bytes(offset, &gd.mines[i], sizeof(core::DiLei));
        }

        // Material[10]: 10 × 32 bytes at offset 0x7260 (29280)
        constexpr size_t kMaterialOffset = 0x7260;
        for (int i = 0; i < 10; ++i) {
            size_t offset = kMaterialOffset + static_cast<size_t>(i) * sizeof(core::Material);
            if (offset + sizeof(core::Material) > reader.size()) break;
            reader.read_bytes(offset, &gd.materials[i], sizeof(core::Material));
        }

        // DiLeiRecipe: 10×10 bytes at offset 0x73A0 (29600)
        constexpr size_t kRecipeOffset = 0x73A0;
        if (kRecipeOffset + sizeof(core::DiLeiRecipe) <= reader.size()) {
            reader.read_bytes(kRecipeOffset, &gd.mine_recipe, sizeof(core::DiLeiRecipe));
        }

        // JiNeng (skills): offset after DiLei
        // DiLei[10] at 29276, size 480 → 29756
        constexpr size_t kJiNengOffset = 29756;
        for (int i = 0; i < core::MAX_SKILLS; ++i) {
            size_t offset = kJiNengOffset + static_cast<size_t>(i) * sizeof(core::JiNeng);
            if (offset + sizeof(core::JiNeng) > reader.size()) break;
            reader.read_bytes(offset, &gd.skills[i], sizeof(core::JiNeng));
        }
    }

    return gd;
}

void DatLoader::load_towns(core::GameData& gd) const {
    BinaryReader reader(resolve("TOWN.DAT"));
    static_assert(core::TOWN_RECORD_SIZE == 346);
    for (int i = 0; i < core::MAX_TOWNS; ++i) {
        size_t offset = static_cast<size_t>(i) * core::TOWN_RECORD_SIZE;
        reader.read_bytes(offset, &gd.towns[i], sizeof(core::Town));
    }
}

core::FightData DatLoader::load_fight(int32_t fight_index) const {
    if (fight_index < 0 || fight_index >= core::MAX_FIGHTS) {
        throw std::out_of_range("fight_index out of range");
    }

    BinaryReader reader(resolve("FIGHT.DAT"));
    core::FightData fd;

    size_t base = static_cast<size_t>(fight_index) * core::FIGHT_RECORD_SIZE;

    // Intro: 700 bytes (name + difficulty + texts + boxes + grid_terrain[92])
    reader.read_bytes(base, &fd.intro, sizeof(core::FightIntro));

    // Fighter[128]: 128 × 64 = 8192 bytes, offset 700
    constexpr size_t kFighterOffset = 700;
    for (int i = 0; i < 128; ++i) {
        size_t off = base + kFighterOffset
                   + static_cast<size_t>(i) * sizeof(core::FightFighter);
        reader.read_bytes(off, &fd.fighters[i], sizeof(core::FightFighter));
    }

    // People[88]: 88 × 132 = 11616 bytes, offset 700 + 8192 = 8892
    constexpr size_t kPeopleOffset = 700 + 8192;
    for (int i = 0; i < 88; ++i) {
        size_t off = base + kPeopleOffset
                   + static_cast<size_t>(i) * sizeof(core::FightPeople);
        reader.read_bytes(off, &fd.people[i], sizeof(core::FightPeople));
    }

    return fd;
}

std::vector<std::string> DatLoader::load_dialogues() const {
    BinaryReader reader(resolve("TXT.DAT"));
    std::vector<std::string> dialogues;

    // TXT.DAT: XOR 0x51 encrypted, variable-length records
    // After decryption: "NNN speaker_code text\r\n" format
    // Total size: ~110KB, 593 entries

    std::vector<uint8_t> decrypted(reader.size());
    for (size_t i = 0; i < reader.size(); ++i) {
        decrypted[i] = reader.data()[i] ^ core::TXT_XOR_KEY;
    }

    // Parse entries: look for pattern "NNN " (3-digit index + space)
    std::string text(reinterpret_cast<const char*>(decrypted.data()), decrypted.size());
    size_t pos = 0;

    while (pos < text.size()) {
        // Find entry start: 3 digits followed by space
        if (pos + 4 < text.size() &&
            std::isdigit(text[pos]) &&
            std::isdigit(text[pos+1]) &&
            std::isdigit(text[pos+2]) &&
            text[pos+3] == ' ') {

            // Find entry end: CR LF
            size_t end = text.find("\r\n", pos);
            if (end == std::string::npos) {
                end = text.size();
            }

            // Extract entry (skip "NNN " prefix)
            std::string entry = text.substr(pos + 4, end - pos - 4);
            dialogues.push_back(entry);

            pos = end + 2;  // Skip past CR LF
        } else {
            ++pos;
        }
    }

    return dialogues;
}

std::vector<uint8_t> DatLoader::load_wave_clip(int32_t wave_id) const {
    BinaryReader reader(resolve("WAVE.DAT"));

    // WAVE.DAT: 8KB index + raw PCM
    // 152 clips, mono 22050Hz 16-bit
    // Index at offset 0: 152 entries × {offset(4), size(4)} = 1216 bytes
    // PCM data follows after the index
    if (wave_id < 0 || wave_id >= 152) {
        throw std::out_of_range("wave_id out of range");
    }

    size_t index_offset = static_cast<size_t>(wave_id) * 8;
    uint32_t pcm_offset = reader.read_at<uint32_t>(index_offset);
    uint32_t pcm_size = reader.read_at<uint32_t>(index_offset + 4);

    return reader.read_vector(pcm_offset, pcm_size);
}

}  // namespace dlzr::app
