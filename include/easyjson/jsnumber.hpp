#pragma once

#include <easyjson/jsitem.hpp>

namespace easyjson {

class Number final : public JSItem {
public:
    using JSItem::set_value;
    void set_value(const JSValue& value) override {
        if (const auto* typed = std::get_if<double>(&value)) { set_value(*typed); return; }
        throw JSException("Invalid value type");
    }
    using JSItem::get_number;
    using JSItem::get_string;
    double get_number() const override { return value_; }
    std::optional<std::string> get_string() const override;
    explicit Number(double value = 0.0) noexcept;
    [[nodiscard]] ValueType type() const noexcept override;
    [[nodiscard]] double value() const noexcept;
    void set_value(double value) noexcept;
private:
    double value_;
};


}
