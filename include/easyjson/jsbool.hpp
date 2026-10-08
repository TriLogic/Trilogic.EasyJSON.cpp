#pragma once

#include <easyjson/jsitem.hpp>

namespace easyjson {

class Bool final : public JSItem {
public:
    using JSItem::set_value;
    void set_value(const JSValue& value) override {
        if (const auto* typed = std::get_if<bool>(&value)) { set_value(*typed); return; }
        throw JSException("Invalid value type");
    }
    using JSItem::get_boolean;
    using JSItem::get_string;
    bool get_boolean() const override { return value_; }
    std::optional<std::string> get_string() const override { return value_ ? "True" : "False"; }
    explicit Bool(bool value = false) noexcept;
    [[nodiscard]] ValueType type() const noexcept override;
    [[nodiscard]] bool value() const noexcept;
    void set_value(bool value) noexcept;
private:
    bool value_;
};


}
