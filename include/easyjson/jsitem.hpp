#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <iosfwd>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <utility>
#include <concepts>

namespace easyjson {

enum class ValueType { Null, Bool, Number, String, Array, Object };

class JSItem;
class Array;
class Object;
class JSFormatter;
enum class JSOutputFormat { OutputCompact, OutputLinear, OutputKNR, OutputAllman, OutputWhitesmith };
using JSList = std::vector<std::shared_ptr<JSItem>>;
using JSDictionary = std::vector<std::pair<std::string, std::shared_ptr<JSItem>>>;
using JSValue = std::variant<std::nullptr_t, bool, double, std::string,
    std::reference_wrapper<const JSList>, std::reference_wrapper<const JSDictionary>>;

struct JSEntry {
    std::optional<std::string> key;
    std::shared_ptr<JSItem> item;
};

class JSException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class JSItem : public std::enable_shared_from_this<JSItem> {
public:
    virtual ~JSItem();
    JSItem(const JSItem&) = delete;
    JSItem& operator=(const JSItem&) = delete;
    JSItem(JSItem&&) = delete;
    JSItem& operator=(JSItem&&) = delete;

    [[nodiscard]] virtual ValueType type() const noexcept = 0;
    [[nodiscard]] bool is_container() const noexcept {
        return type() == ValueType::Array || type() == ValueType::Object;
    }
    [[nodiscard]] std::shared_ptr<JSItem> parent() noexcept { return parent_.lock(); }
    [[nodiscard]] std::shared_ptr<const JSItem> parent() const noexcept { return parent_.lock(); }
    [[nodiscard]] std::shared_ptr<JSItem> root();
    [[nodiscard]] std::shared_ptr<const JSItem> root() const;
    [[nodiscard]] virtual std::size_t size() const noexcept { return 0; }
    [[nodiscard]] virtual JSValue get_value() const;
    virtual void set_value(const JSValue& value);
    void set_value(bool value) { set_value(JSValue(value)); }
    void set_value(double value) { set_value(JSValue(value)); }
    template<std::integral T> requires (!std::same_as<T, bool>)
    void set_value(T value) { set_value(static_cast<double>(value)); }
    void set_value(std::string value) { set_value(JSValue(std::move(value))); }
    void set_value(const char* value) { set_value(JSValue(std::string(value ? value : ""))); }
    [[nodiscard]] virtual const JSList& get_list() const;
    [[nodiscard]] virtual const JSDictionary& get_dictionary() const;
    [[nodiscard]] virtual JSList to_list() const;
    [[nodiscard]] virtual JSDictionary to_dictionary() const;
    [[nodiscard]] virtual bool exists(std::string_view key) const;
    [[nodiscard]] std::vector<JSEntry> get_enumerator() const;

    [[nodiscard]] virtual std::string to_string() const;
    [[nodiscard]] virtual std::string to_string(JSOutputFormat format,
        std::string_view indent = "\t", bool expand_empty = false) const;
    virtual void write(std::ostream& stream) const;
    virtual void write(std::string& buffer) const;
    virtual void write(std::ostream& stream, JSOutputFormat format,
        std::string_view indent = "\t", bool expand_empty = false) const;
    virtual void write(std::ostream& stream, const JSFormatter& formatter) const;
    [[nodiscard]] static std::shared_ptr<JSItem> parse(std::string_view json);
    [[nodiscard]] static std::shared_ptr<JSItem> parse(std::istream& stream);
    [[nodiscard]] static std::shared_ptr<Array> parse_array(std::string_view json);
    [[nodiscard]] static std::shared_ptr<Array> parse_array(std::istream& stream);
    [[nodiscard]] static std::shared_ptr<Object> parse_object(std::string_view json);
    [[nodiscard]] static std::shared_ptr<Object> parse_object(std::istream& stream);

    [[nodiscard]] bool is_null() const noexcept { return type() == ValueType::Null; }
    [[nodiscard]] bool is_boolean() const noexcept { return type() == ValueType::Bool; }
    [[nodiscard]] bool is_number() const noexcept { return type() == ValueType::Number; }
    [[nodiscard]] bool is_string() const noexcept { return type() == ValueType::String; }
    [[nodiscard]] bool is_array() const noexcept { return type() == ValueType::Array; }
    [[nodiscard]] bool is_object() const noexcept { return type() == ValueType::Object; }

    virtual std::shared_ptr<JSItem> operator[](std::size_t index);
    virtual std::shared_ptr<const JSItem> operator[](std::size_t index) const;
    virtual std::shared_ptr<JSItem> operator[](std::string_view key);
    virtual std::shared_ptr<const JSItem> operator[](std::string_view key) const;
    [[nodiscard]] virtual bool contains(std::string_view key) const;
    virtual std::shared_ptr<JSItem> add(std::shared_ptr<JSItem> item);
    virtual std::shared_ptr<JSItem> add(std::string key, std::shared_ptr<JSItem> item);
    std::shared_ptr<JSItem> add(std::shared_ptr<JSItem> item, std::string_view key) {
        return key.empty() ? add(std::move(item)) : add(std::string(key), std::move(item));
    }
    [[nodiscard]] virtual std::shared_ptr<JSItem> remove(std::size_t index);
    [[nodiscard]] virtual std::shared_ptr<JSItem> remove(std::string_view key);
    virtual void clear();

    std::shared_ptr<JSItem> add_object(std::string_view key = {});
    std::shared_ptr<JSItem> add_array(std::string_view key = {});
    std::shared_ptr<JSItem> add_null(std::string_view key = {});
    std::shared_ptr<JSItem> add_boolean(bool value, std::string_view key = {});
    std::shared_ptr<JSItem> add_true(std::string_view key = {}) { return add_boolean(true, key); }
    std::shared_ptr<JSItem> add_false(std::string_view key = {}) { return add_boolean(false, key); }
    std::shared_ptr<JSItem> add_number(double value, std::string_view key = {});
    std::shared_ptr<JSItem> add_string(std::string value, std::string_view key = {});
    [[nodiscard]] static std::shared_ptr<JSItem> create_object();
    [[nodiscard]] static std::shared_ptr<JSItem> create_array();

    [[nodiscard]] virtual bool get_boolean() const;
    [[nodiscard]] virtual double get_number() const;
    [[nodiscard]] virtual std::optional<std::string> get_string() const;
    [[nodiscard]] virtual bool has_numeric_content() const noexcept;
    [[nodiscard]] virtual std::uint8_t get_byte() const;
    [[nodiscard]] virtual std::int32_t get_integer() const;
    [[nodiscard]] virtual std::int64_t get_long() const;
    [[nodiscard]] virtual float get_float() const;
    [[nodiscard]] virtual double get_double() const;
    std::uint8_t get_byte(std::size_t index) const { return operator[](index)->get_byte(); }
    std::uint8_t get_byte(std::string_view key) const { return operator[](key)->get_byte(); }
    std::int32_t get_integer(std::size_t index) const { return operator[](index)->get_integer(); }
    std::int32_t get_integer(std::string_view key) const { return operator[](key)->get_integer(); }
    std::int64_t get_long(std::size_t index) const { return operator[](index)->get_long(); }
    std::int64_t get_long(std::string_view key) const { return operator[](key)->get_long(); }
    float get_float(std::size_t index) const { return operator[](index)->get_float(); }
    float get_float(std::string_view key) const { return operator[](key)->get_float(); }
    double get_double(std::size_t index) const { return operator[](index)->get_double(); }
    double get_double(std::string_view key) const { return operator[](key)->get_double(); }
    bool get_boolean(std::size_t index) const { return operator[](index)->get_boolean(); }
    bool get_boolean(std::string_view key) const { return operator[](key)->get_boolean(); }
    double get_number(std::size_t index) const { return operator[](index)->get_number(); }
    double get_number(std::string_view key) const { return operator[](key)->get_number(); }
    std::optional<std::string> get_string(std::size_t index) const { return operator[](index)->get_string(); }
    std::optional<std::string> get_string(std::string_view key) const { return operator[](key)->get_string(); }

protected:
    JSItem() = default;
    std::shared_ptr<JSItem> validate_child(const std::shared_ptr<JSItem>& item);

private:
    std::shared_ptr<JSItem> add_value(std::shared_ptr<JSItem> item, std::string_view key);
    friend class Array;
    friend class Object;
    std::weak_ptr<JSItem> parent_;
};

}
