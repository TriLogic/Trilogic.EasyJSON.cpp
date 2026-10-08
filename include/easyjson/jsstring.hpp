#pragma once

#include <easyjson/jsitem.hpp>
#include <string>

namespace easyjson {

class String final : public JSItem {
public:
    using JSItem::set_value;
    void set_value(const JSValue& value) override {
        if (const auto* typed = std::get_if<std::string>(&value)) { set_value(*typed); return; }
        if (std::holds_alternative<std::nullptr_t>(value)) { set_value(std::string{}); return; }
        throw JSException("Invalid value type");
    }
    using JSItem::get_number;
    using JSItem::get_string;
    double get_number() const override;
    std::optional<std::string> get_string() const override { return value_; }
    explicit String(std::string value = {});
    [[nodiscard]] ValueType type() const noexcept override;
    [[nodiscard]] const std::string& value() const noexcept;
    void set_value(std::string value);
private:
    std::string value_;
};


}
