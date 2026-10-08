#include <easyjson/easyjson.hpp>
#include "jstokenizer.hpp"

#include <istream>
#include <iterator>

namespace easyjson {
namespace {
using detail::TokenType;

// Port of JSReader: consume tokenizer values directly into the EasyJSON tree.
class JSReader {
public:
    explicit JSReader(std::string_view input) : tokens_(input) { tokens_.next(); }
    std::shared_ptr<JSItem> parse() {
        if (tokens_.type != TokenType::ArrayLeft && tokens_.type != TokenType::ObjectLeft)
            throw JSException("Expected Array or Object");
        auto result = value(0);
        if (tokens_.type != TokenType::End) throw JSException("Unexpected trailing JSON input");
        return result;
    }
private:
    detail::JSTokenizer tokens_;
    std::shared_ptr<JSItem> value(std::size_t depth) {
        if (depth > 256) throw JSException("JSON nesting limit exceeded (256)");
        std::shared_ptr<JSItem> result;
        switch (tokens_.type) {
        case TokenType::ArrayLeft: return container(false, depth);
        case TokenType::ObjectLeft: return container(true, depth);
        case TokenType::Null: result = std::make_shared<Null>(); break;
        case TokenType::Boolean: result = std::make_shared<Bool>(tokens_.boolean); break;
        case TokenType::Number: result = std::make_shared<Number>(tokens_.number); break;
        case TokenType::String: result = std::make_shared<String>(tokens_.text); break;
        default: throw JSException("Expected JSON value");
        }
        tokens_.next();
        return result;
    }
    std::shared_ptr<JSItem> container(bool object, std::size_t depth) {
        auto result = object ? JSItem::create_object() : JSItem::create_array();
        const auto close = object ? TokenType::ObjectRight : TokenType::ArrayRight;
        const auto error = object ? "JSON: Invalid Object Syntax" : "JSON: Invalid Array Syntax";
        tokens_.next();
        while (tokens_.type != close) {
            std::string key;
            if (object) {
                if (tokens_.type != TokenType::String) throw JSException(error);
                key = tokens_.text;
                if (result->contains(key)) throw JSException("JSON: Duplicate Object Key (" + key + ")");
                tokens_.next();
                if (tokens_.type != TokenType::Colon) throw JSException(error);
                tokens_.next();
            }
            auto child = value(depth + 1);
            if (object) result->add(std::move(key), std::move(child));
            else result->add(std::move(child));
            if (tokens_.type == close) break;
            if (tokens_.type != TokenType::Comma) throw JSException(error);
            tokens_.next();
            // The upstream reader permits a trailing comma.
        }
        tokens_.next();
        return result;
    }
};

std::string read_stream(std::istream& stream) {
    if (!stream.good()) throw JSException("JSON stream is not readable");
    std::string text{std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
    if (stream.bad()) throw JSException("JSON stream read failed");
    return text;
}
}

std::shared_ptr<JSItem> JSItem::parse(std::string_view json) {
    return JSReader(json).parse();
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
