#include <easyjson/easyjson.hpp>
#include <nlohmann/json.hpp>

#include <istream>
#include <iterator>
#include <map>
#include <set>

namespace easyjson {
namespace {
using Json = nlohmann::ordered_json;

// Normalize only upstream's comment/trailing-comma extensions. The JSON
// dependency handles all grammar, escapes, Unicode, and numeric validation.
std::string normalize(std::string_view input) {
    std::string text;
    bool quoted = false;
    bool escaped = false;
    for (std::size_t i = 0; i < input.size(); ++i) {
        const char c = input[i];
        if (!quoted && c == '/' && i + 1 < input.size()) {
            if (input[i + 1] == '/') {
                text += ' ';
                i += 2;
                while (i < input.size() && input[i] != '\n' && input[i] != '\r') ++i;
                if (i < input.size()) text += input[i];
                continue;
            }
            if (input[i + 1] == '*') {
                text += ' ';
                i += 2;
                while (i + 1 < input.size() && !(input[i] == '*' && input[i + 1] == '/')) {
                    text += input[i] == '\n' ? '\n' : ' ';
                    ++i;
                }
                if (i + 1 >= input.size()) throw JSException("Unterminated JSON comment");
                ++i;
                continue;
            }
        }
        text += c;
        if (quoted && escaped) escaped = false;
        else if (quoted && c == '\\') escaped = true;
        else if (c == '"') quoted = !quoted;
    }
    quoted = false;
    escaped = false;
    for (std::size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if (!quoted && c == ',') {
            const auto next = text.find_first_not_of(" \t\r\n", i + 1);
            const auto previous = text.find_last_not_of(" \t\r\n", i == 0 ? 0 : i - 1);
            if (i > 0 && previous != std::string::npos &&
                std::string_view("[{,:").find(text[previous]) == std::string_view::npos &&
                next != std::string::npos && (text[next] == ']' || text[next] == '}')) text[i] = ' ';
        }
        if (quoted && escaped) escaped = false;
        else if (quoted && c == '\\') escaped = true;
        else if (c == '"') quoted = !quoted;
    }
    return text;
}

std::shared_ptr<JSItem> convert(const Json& value) {
    if (value.is_null()) return std::make_shared<Null>();
    if (value.is_boolean()) return std::make_shared<Bool>(value.get<bool>());
    if (value.is_number()) return std::make_shared<Number>(value.get<double>());
    if (value.is_string()) return std::make_shared<String>(value.get<std::string>());
    if (value.is_array()) {
        auto result = JSItem::create_array();
        for (const auto& child : value) result->add(convert(child));
        return result;
    }
    auto result = JSItem::create_object();
    for (auto it = value.begin(); it != value.end(); ++it) result->add(it.key(), convert(it.value()));
    return result;
}

std::string read_stream(std::istream& stream) {
    if (!stream.good()) throw JSException("JSON stream is not readable");
    std::string text{std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
    if (stream.bad()) throw JSException("JSON stream read failed");
    return text;
}
}

std::shared_ptr<JSItem> JSItem::parse(std::string_view json) {
    try {
        std::map<int, std::set<std::string>> keys;
        auto callback = [&keys](int depth, Json::parse_event_t event, Json& parsed) {
            if (depth > 256) throw JSException("JSON nesting limit exceeded (256)");
            if (event == Json::parse_event_t::object_start) keys[depth].clear();
            if (event == Json::parse_event_t::key) {
                if (!keys[depth - 1].insert(parsed.get<std::string>()).second) {
                    throw JSException("JSON: Duplicate Object Key");
                }
            }
            if (event == Json::parse_event_t::object_end) keys.erase(depth);
            return true;
        };
        const auto value = Json::parse(normalize(json), callback);
        if (!value.is_array() && !value.is_object()) throw JSException("Expected Array or Object");
        return convert(value);
    } catch (const Json::exception& error) {
        throw JSException(error.what());
    }
}
std::shared_ptr<JSItem> JSItem::parse(std::istream& stream) { return parse(read_stream(stream)); }
std::shared_ptr<Array> JSItem::parse_array(std::string_view json) {
    auto result = parse(json);
    if (!result->is_array()) throw JSException("JSON: Invalid Array Syntax");
    return std::static_pointer_cast<Array>(result);
}
std::shared_ptr<Array> JSItem::parse_array(std::istream& stream) { return parse_array(read_stream(stream)); }
std::shared_ptr<Object> JSItem::parse_object(std::string_view json) {
    auto result = parse(json);
    if (!result->is_object()) throw JSException("JSON: Invalid Object Syntax");
    return std::static_pointer_cast<Object>(result);
}
std::shared_ptr<Object> JSItem::parse_object(std::istream& stream) { return parse_object(read_stream(stream)); }

}
