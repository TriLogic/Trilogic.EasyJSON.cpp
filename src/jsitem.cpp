#include <easyjson/easyjson.hpp>

#include <charconv>
#include <cmath>
#include <limits>
#include <sstream>
#include "numeric.hpp"

namespace easyjson {

JSItem::~JSItem() = default;

std::shared_ptr<JSItem> JSItem::operator[](std::size_t) { throw JSException("Invalid JSArray"); }
std::shared_ptr<const JSItem> JSItem::operator[](std::size_t) const { throw JSException("Invalid JSArray"); }
std::shared_ptr<JSItem> JSItem::operator[](std::string_view) { throw JSException("Invalid JSObject"); }
std::shared_ptr<const JSItem> JSItem::operator[](std::string_view) const { throw JSException("Invalid JSObject"); }
bool JSItem::contains(std::string_view) const { throw JSException("Invalid JSObject"); }
std::shared_ptr<JSItem> JSItem::add(std::shared_ptr<JSItem>) { throw JSException("Invalid JSArray"); }
std::shared_ptr<JSItem> JSItem::add(std::string, std::shared_ptr<JSItem>) { throw JSException("Invalid JSObject"); }
std::shared_ptr<JSItem> JSItem::remove(std::size_t) { throw JSException("Invalid JSArray"); }
std::shared_ptr<JSItem> JSItem::remove(std::string_view) { throw JSException("Invalid JSObject"); }
void JSItem::clear() { throw JSException("Invalid Container"); }
bool JSItem::get_boolean() const { throw JSException("Invalid boolean"); }
double JSItem::get_number() const { throw JSException("Invalid number"); }
std::optional<std::string> JSItem::get_string() const {
    if (is_null()) return std::nullopt;
    return to_string();
}

JSValue JSItem::get_value() const {
    if (is_null()) return nullptr;
    if (is_boolean()) return get_boolean();
    if (is_number()) return get_number();
    if (is_string()) return *get_string();
    if (is_array()) return std::cref(get_list());
    return std::cref(get_dictionary());
}
void JSItem::set_value(const JSValue&) {
    throw JSException(is_null() ? "Attempt to set null value" : "Disallowed for container");
}
const JSList& JSItem::get_list() const { throw JSException("Invalid JSArray"); }
const JSDictionary& JSItem::get_dictionary() const { throw JSException("Invalid JSObject"); }
JSList JSItem::to_list() const { return get_list(); }
JSDictionary JSItem::to_dictionary() const { return get_dictionary(); }
bool JSItem::exists(std::string_view key) const { return contains(key); }
std::vector<JSEntry> JSItem::get_enumerator() const {
    std::vector<JSEntry> entries;
    entries.reserve(size());
    if (is_array()) {
        for (const auto& item : get_list()) entries.push_back({std::nullopt, item});
    } else if (is_object()) {
        for (const auto& [key, item] : get_dictionary()) entries.push_back({key, item});
    } else { throw JSException("Invalid container"); }
    return entries;
}

std::string JSItem::to_string() const { return JSFormatter(JSOutputFormat::OutputCompact).format(*this); }
std::string JSItem::to_string(JSOutputFormat format, std::string_view indent, bool expand_empty) const {
    return JSFormatter(format, indent, expand_empty).format(*this);
}
void JSItem::write(std::ostream& stream) const { JSFormatter(JSOutputFormat::OutputCompact).write(stream, *this); }
void JSItem::write(std::string& buffer) const { buffer += to_string(); }
void JSItem::write(std::ostream& stream, JSOutputFormat format, std::string_view indent, bool expand_empty) const {
    JSFormatter(format, indent, expand_empty).write(stream, *this);
}
void JSItem::write(std::ostream& stream, const JSFormatter& formatter) const {
    formatter.write(stream, *this);
    stream.flush();
    if (!stream) throw JSException("JSON stream write failed");
}

bool JSItem::has_numeric_content() const noexcept {
    if (is_number()) return true;
    if (!is_string()) return false;
    try { (void)get_number(); return true; }
    catch (...) { return false; }
}

namespace {
template<class T> T integer_value(const JSItem& item) {
    if (item.is_string()) {
        const auto owned = *item.get_string();
        auto text = detail::trim_numeric(owned);
        if (!text.empty() && text.front() == '+') {
            text.remove_prefix(1);
            if (!text.empty() && (text.front() == '-' || text.front() == '+')) throw JSException("Invalid integer");
        }
        if (text.empty()) throw JSException("Invalid integer");
        T result;
        const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), result);
        if (error == std::errc{} && end == text.data() + text.size()) return result;
        throw JSException("Invalid integer");
    }
    const double value = std::trunc(item.get_number());
    // Use an exclusive upper bound to avoid rounded integer limits in double.
    const double upper = std::ldexp(1.0, std::numeric_limits<T>::digits);
    const double lower = std::numeric_limits<T>::is_signed ? -upper : 0.0;
    if (!std::isfinite(value) || value < lower || value >= upper) {
        throw JSException("Integer out of range");
    }
    return static_cast<T>(value);
}
}

std::uint8_t JSItem::get_byte() const { return integer_value<std::uint8_t>(*this); }
std::int32_t JSItem::get_integer() const { return integer_value<std::int32_t>(*this); }
std::int64_t JSItem::get_long() const { return integer_value<std::int64_t>(*this); }
float JSItem::get_float() const {
    const double value = get_number();
    if (std::isfinite(value) && std::abs(value) > std::numeric_limits<float>::max()) {
        throw JSException("Float out of range");
    }
    return static_cast<float>(value);
}
double JSItem::get_double() const { return get_number(); }

std::shared_ptr<JSItem> JSItem::add_value(std::shared_ptr<JSItem> item, std::string_view key) {
    const bool container = item->is_container();
    auto added = key.empty() ? add(std::move(item)) : add(std::string(key), std::move(item));
    return container ? added : shared_from_this();
}
std::shared_ptr<JSItem> JSItem::add_object(std::string_view key) { return add_value(std::make_shared<Object>(), key); }
std::shared_ptr<JSItem> JSItem::add_array(std::string_view key) { return add_value(std::make_shared<Array>(), key); }
std::shared_ptr<JSItem> JSItem::add_null(std::string_view key) { return add_value(std::make_shared<Null>(), key); }
std::shared_ptr<JSItem> JSItem::add_boolean(bool value, std::string_view key) { return add_value(std::make_shared<Bool>(value), key); }
std::shared_ptr<JSItem> JSItem::add_number(double value, std::string_view key) { return add_value(std::make_shared<Number>(value), key); }
std::shared_ptr<JSItem> JSItem::add_string(std::string value, std::string_view key) { return add_value(std::make_shared<String>(std::move(value)), key); }
std::shared_ptr<JSItem> JSItem::create_object() { return std::make_shared<Object>(); }
std::shared_ptr<JSItem> JSItem::create_array() { return std::make_shared<Array>(); }

std::shared_ptr<JSItem> JSItem::validate_child(const std::shared_ptr<JSItem>& item) {
    if (!item) throw std::invalid_argument("Use Null for a JSON null value");
    auto self = weak_from_this().lock();
    if (!self) throw JSException("Containers must be managed by std::shared_ptr");
    for (auto ancestor = self; ancestor; ancestor = ancestor->parent()) {
        if (ancestor == item) throw std::invalid_argument("JSON ownership cycle");
    }
    if (item->parent() != nullptr) throw std::invalid_argument("JSON item already has a parent");
    return self;
}

std::shared_ptr<JSItem> JSItem::root() {
    auto item = shared_from_this();
    while (auto ancestor = item->parent()) item = std::move(ancestor);
    return item;
}

std::shared_ptr<const JSItem> JSItem::root() const {
    auto item = shared_from_this();
    while (auto ancestor = item->parent()) item = std::move(ancestor);
    return item;
}

}
