#pragma once

#include <string_view>

namespace easyjson::detail {
inline std::string_view trim_numeric(std::string_view text) {
    constexpr std::string_view whitespace = " \t\r\n\f\v";
    const auto begin = text.find_first_not_of(whitespace);
    if (begin == std::string_view::npos) return {};
    const auto end = text.find_last_not_of(whitespace);
    return text.substr(begin, end - begin + 1);
}
}
