#include <easyjson/jsbool.hpp>

namespace easyjson {

Bool::Bool(bool value) noexcept : value_(value) {}
ValueType Bool::type() const noexcept { return ValueType::Bool; }
bool Bool::value() const noexcept { return value_; }
void Bool::set_value(bool value) noexcept { value_ = value; }

}
