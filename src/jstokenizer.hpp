#pragma once

#include "jsontext.hpp"
#include <charconv>
#include <cmath>

namespace easyjson::detail {
enum class TokenType { End, Null, Boolean, Number, String, Comma, Colon,
    ArrayLeft, ArrayRight, ObjectLeft, ObjectRight };

// Port of JSTokenizer: whitespace and comments are skipped by next().
class JSTokenizer {
public:
    explicit JSTokenizer(std::string_view input) : input_(input) {}
    TokenType type = TokenType::End;
    std::string text;
    double number = 0;
    bool boolean = false;

    void next() {
        skip();
        text.clear();
        if (position_ == input_.size()) { type = TokenType::End; return; }
        const char c = input_[position_++];
        switch (c) {
        case '[': type = TokenType::ArrayLeft; return;
        case ']': type = TokenType::ArrayRight; return;
        case '{': type = TokenType::ObjectLeft; return;
        case '}': type = TokenType::ObjectRight; return;
        case ',': type = TokenType::Comma; return;
        case ':': type = TokenType::Colon; return;
        case '"': parse_string(); type = TokenType::String; return;
        case 't': exact("rue"); boolean = true; type = TokenType::Boolean; return;
        case 'f': exact("alse"); boolean = false; type = TokenType::Boolean; return;
        case 'n': exact("ull"); type = TokenType::Null; return;
        default:
            if (c == '-' || digit(c)) { --position_; parse_number(); type = TokenType::Number; return; }
            fail("Invalid JSON token");
        }
    }

private:
    std::string_view input_;
    std::size_t position_ = 0;
    [[noreturn]] void fail(const char* message) const {
        throw JSException(std::string(message) + " at byte " + std::to_string(position_));
    }
    char peek() const { return position_ == input_.size() ? '\0' : input_[position_]; }
    static bool digit(char c) { return c >= '0' && c <= '9'; }
    bool take(char c) {
        if (position_ < input_.size() && input_[position_] == c) { ++position_; return true; }
        return false;
    }
    void exact(std::string_view value) {
        if (input_.substr(position_, value.size()) != value) fail("Invalid JSON constant");
        position_ += value.size();
    }
    void skip() {
        while (position_ < input_.size()) {
            const char c = peek();
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') { ++position_; continue; }
            if (c != '/' || position_ + 1 == input_.size()) return;
            const char next = input_[position_ + 1];
            if (next == '/') {
                position_ += 2;
                while (position_ < input_.size() && peek() != '\r' && peek() != '\n') ++position_;
            } else if (next == '*') {
                position_ += 2;
                const auto end = input_.find("*/", position_);
                if (end == std::string_view::npos) fail("Unterminated JSON comment");
                position_ = end + 2;
            } else return;
        }
    }
    void digits() {
        if (!digit(peek())) fail("Expected JSON number digit");
        while (digit(peek())) ++position_;
    }
    void parse_number() {
        const auto start = position_;
        take('-');
        if (!take('0')) digits();
        if (take('.')) digits();
        if (take('e') || take('E')) {
            if (!take('+')) take('-');
            digits();
        }
        const char* begin = input_.data() + start;
        const char* end = input_.data() + position_;
        const auto result = std::from_chars(begin, end, number);
        if (result.ec != std::errc{} || result.ptr != end || !std::isfinite(number))
            fail("JSON number out of range");
    }
    unsigned quad() {
        unsigned code = 0;
        for (unsigned i = 0; i < 4; ++i) {
            if (position_ == input_.size()) fail("Incomplete Unicode escape");
            const char c = input_[position_++];
            unsigned nibble;
            if (digit(c)) nibble = static_cast<unsigned>(c - '0');
            else if (c >= 'a' && c <= 'f') nibble = static_cast<unsigned>(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') nibble = static_cast<unsigned>(c - 'A' + 10);
            else fail("Invalid Unicode escape");
            code = (code << 4) | nibble;
        }
        return code;
    }
    void parse_string() {
        while (position_ < input_.size()) {
            const auto c = static_cast<unsigned char>(input_[position_++]);
            if (c == '"') return;
            if (c < 0x20) fail("Unescaped JSON control character");
            if (c >= 0x80) {
                --position_;
                append_utf8(text, read_utf8(input_, position_));
            } else if (c != '\\') text += static_cast<char>(c);
            else {
                if (position_ == input_.size()) fail("Incomplete JSON escape");
                switch (input_[position_++]) {
                case '"': text += '"'; break;
                case '\\': text += '\\'; break;
                case '/': text += '/'; break;
                case 'b': text += '\b'; break;
                case 'f': text += '\f'; break;
                case 'n': text += '\n'; break;
                case 'r': text += '\r'; break;
                case 't': text += '\t'; break;
                case 'u': {
                    unsigned code = quad();
                    if (code >= 0xd800 && code <= 0xdbff) {
                        if (!take('\\') || !take('u')) fail("Missing low Unicode surrogate");
                        const unsigned low = quad();
                        if (low < 0xdc00 || low > 0xdfff) fail("Invalid low Unicode surrogate");
                        code = 0x10000 + ((code - 0xd800) << 10) + low - 0xdc00;
                    } else if (code >= 0xdc00 && code <= 0xdfff) fail("Unpaired Unicode surrogate");
                    append_utf8(text, code);
                    break;
                }
                default: fail("Invalid JSON escape");
                }
            }
        }
        fail("Unterminated JSON string");
    }
};
}
