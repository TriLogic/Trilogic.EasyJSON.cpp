#pragma once

#include <easyjson/jsitem.hpp>

namespace easyjson::detail {
// C++ strings use UTF-8; the C# source uses UTF-16 code units.
inline unsigned read_utf8(std::string_view text, std::size_t& position) {
    if (position == text.size()) throw JSException("Incomplete UTF-8 character");
    const auto first = static_cast<unsigned char>(text[position++]);
    if (first < 0x80) return first;
    unsigned code;
    unsigned count;
    unsigned minimum;
    if (first >= 0xc2 && first <= 0xdf) { code = first & 0x1f; count = 1; minimum = 0x80; }
    else if (first >= 0xe0 && first <= 0xef) { code = first & 0x0f; count = 2; minimum = 0x800; }
    else if (first >= 0xf0 && first <= 0xf4) { code = first & 7; count = 3; minimum = 0x10000; }
    else throw JSException("Invalid UTF-8 character");
    for (unsigned i = 0; i < count; ++i) {
        if (position == text.size()) throw JSException("Incomplete UTF-8 character");
        const auto next = static_cast<unsigned char>(text[position++]);
        if ((next & 0xc0) != 0x80) throw JSException("Invalid UTF-8 continuation");
        code = (code << 6) | (next & 0x3f);
    }
    if (code < minimum || code > 0x10ffff || (code >= 0xd800 && code <= 0xdfff))
        throw JSException("Invalid Unicode character");
    return code;
}

inline void append_utf8(std::string& text, unsigned code) {
    if (code < 0x80) text += static_cast<char>(code);
    else if (code < 0x800) {
        text += static_cast<char>(0xc0 | (code >> 6));
        text += static_cast<char>(0x80 | (code & 0x3f));
    } else if (code < 0x10000) {
        text += static_cast<char>(0xe0 | (code >> 12));
        text += static_cast<char>(0x80 | ((code >> 6) & 0x3f));
        text += static_cast<char>(0x80 | (code & 0x3f));
    } else {
        text += static_cast<char>(0xf0 | (code >> 18));
        text += static_cast<char>(0x80 | ((code >> 12) & 0x3f));
        text += static_cast<char>(0x80 | ((code >> 6) & 0x3f));
        text += static_cast<char>(0x80 | (code & 0x3f));
    }
}

inline void append_unicode_escape(std::string& text, unsigned code) {
    constexpr char hex[] = "0123456789abcdef";
    text += "\\u";
    for (int shift = 12; shift >= 0; shift -= 4) text += hex[(code >> shift) & 15];
}

// Port of JSTools.WriteWithEscapes, with all JSON control characters escaped.
inline std::string quote(std::string_view text) {
    std::string result = "\"";
    for (std::size_t position = 0; position < text.size();) {
        const unsigned code = read_utf8(text, position);
        switch (code) {
        case '"': result += "\\\""; break;
        case '\\': result += "\\\\"; break;
        case '\b': result += "\\b"; break;
        case '\f': result += "\\f"; break;
        case '\n': result += "\\n"; break;
        case '\r': result += "\\r"; break;
        case '\t': result += "\\t"; break;
        default:
            if (code < 0x20 || code > 0x7f) {
                if (code > 0xffff) {
                    append_unicode_escape(result, 0xd800 + ((code - 0x10000) >> 10));
                    append_unicode_escape(result, 0xdc00 + ((code - 0x10000) & 0x3ff));
                } else append_unicode_escape(result, code);
            } else result += static_cast<char>(code);
        }
    }
    result += '"';
    return result;
}
}
