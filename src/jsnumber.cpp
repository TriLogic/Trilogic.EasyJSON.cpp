#include <easyjson/jsnumber.hpp>
#include <format>

namespace easyjson {

std::optional<std::string> Number::get_string() const { return std::format("{}", value_); }

Number::Number(double value) noexcept : value_(value) {}
ValueType Number::type() const noexcept { return ValueType::Number; }
double Number::value() const noexcept { return value_; }
void Number::set_value(double value) noexcept { value_ = value; }

}
