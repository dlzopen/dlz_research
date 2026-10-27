#include "logic/dialogue/dialogue_text_util.hpp"

#include <cstdint>
#include <string>

namespace dlzr::logic {

std::string wrap_per_gbk_char(const std::string& s, const std::string& marker) {
    std::string out;
    size_t i = 0;
    while (i < s.size()) {
        out += marker;
        uint8_t b = static_cast<uint8_t>(s[i]);
        int n = (b < 0x80) ? 1 : 2;
        if (i + n > s.size()) n = 1;
        out.append(s, i, n);
        i += n;
    }
    return out;
}

std::string format_dialogue_text(const std::string& raw,
                                 const std::string& protagonist_name,
                                 const std::string& t_name) {
    std::string out = raw;
    if (!protagonist_name.empty()) {
        // 原版 FUN_0043cc96.c:83-93：GG 展开为 "AA"+name（每 GBK 字前置 "AA"），
        // 使 dialogue_build_segments 按 FUN_004061df.c:39-42 着红(0xb7)。
        const std::string rep = wrap_per_gbk_char(protagonist_name, "AA");
        size_t pos = 0;
        while ((pos = out.find("GG", pos)) != std::string::npos) {
            out.replace(pos, 2, rep);
            pos += rep.size();
        }
    }
    if (!t_name.empty()) {
        // TT = 战斗/地名令牌：原版 FUN_0043cc96.c:71-82 的 T 分支展开 DAT_007d01c8，
        //   与 GG 同构——"AA"+t_name（每 GBK 字前置 "AA"），dialogue_build_segments
        //   按 FUN_004061df.c:39-42 着红(0xb7)。t_name = 运行时战斗/地名（对应
        //   DAT_007d01c8），由场景装载；PE 抽不到，但展开格式已逐指令钉死。
        const std::string rep = wrap_per_gbk_char(t_name, "AA");
        size_t pos = 0;
        while ((pos = out.find("TT", pos)) != std::string::npos) {
            out.replace(pos, 2, rep);
            pos += rep.size();
        }
    } else {
        // t_name 未装载（空）→ 原版 :74 _strlen 得 0，T 循环 0 次不发射任何字节，
        // 而 "TT" 自身已在 :72-73 被消费；等价于把 TT 剔除。与二进制行为一致。
        size_t tpos = 0;
        while ((tpos = out.find("TT", tpos)) != std::string::npos) {
            out.replace(tpos, 2, "");
        }
    }
    return out;
}

}  // namespace dlzr::logic
