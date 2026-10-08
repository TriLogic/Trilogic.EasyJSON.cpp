#include <easyjson/jsstring.hpp>

#include <utility>
#include <charconv>
#include "numeric.hpp"

namespace easyjson {

double String::get_number() const {
    auto text = detail::trim_numeric(value_);
    if (!text.empty() && text.front() == '+') {
        text.remove_prefix(1);
        if (!text.empty() && (text.front() == '-' || text.front() == '+')) throw JSException("Invalid Numeric");
    }
    std::string normalized;
    bool integral_part = true;
    bool digit_seen = false;
    for (char c : text) {
        if (c == ',') {
            if (!integral_part || !digit_seen) throw JSException("Invalid Numeric");
            continue;
        }
        if (c == '.' || c == 'e' || c == 'E') integral_part = false;
        if (c >= '0' && c <= '9') digit_seen = true;
        normalized += c;
    }
    if (normalized.find('(') != std::string::npos) throw JSException("Invalid Numeric");
    double result;
    const auto [end, error] = std::from_chars(normalized.data(), normalized.data() + normalized.size(), result);
    if (error != std::errc{} || end != normalized.data() + normalized.size()) {
        throw JSException("Invalid Numeric");
    }
    return result;
}

String::String(std::string value) : value_(std::move(value)) {}
ValueType String::type() const noexcept { return ValueType::String; }
const std::string& String::value() const noexcept { return value_; }
void String::set_value(std::string value) { value_ = std::move(value); }

}
