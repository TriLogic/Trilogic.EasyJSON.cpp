#include <easyjson/jsnull.hpp>

namespace easyjson {

ValueType Null::type() const noexcept { return ValueType::Null; }
std::nullptr_t Null::value() const noexcept { return nullptr; }

}
