#pragma once

#include <easyjson/jsitem.hpp>
#include <cstddef>

namespace easyjson {

class Null final : public JSItem {
public:
    [[nodiscard]] ValueType type() const noexcept override;
    [[nodiscard]] std::nullptr_t value() const noexcept;
};


}
