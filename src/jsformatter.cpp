#include <easyjson/jsformatter.hpp>
#include "jsontext.hpp"

#include <cmath>
#include <fstream>
#include <sstream>

namespace easyjson {
using detail::quote;

JSFormatter::JSFormatter(JSOutputFormat format, std::string_view indent_text,
    bool expand_empty, bool blank_before_colon) {
    set_format(format, indent_text, expand_empty, blank_before_colon);
}

void JSFormatter::set_format(JSOutputFormat format, std::string_view indent_text,
    bool expand_empty, bool blank_before_colon, bool blank_before_comma) {
    switch (format) {
    case JSOutputFormat::OutputCompact: set_format_compact(); break;
    case JSOutputFormat::OutputLinear: set_format_linear(expand_empty, blank_before_colon, blank_before_comma); break;
    case JSOutputFormat::OutputKNR: set_format_knr(indent_text, expand_empty, false, blank_before_colon, blank_before_comma); break;
    case JSOutputFormat::OutputAllman: set_format_allman(indent_text, expand_empty, blank_before_colon, blank_before_comma); break;
    case JSOutputFormat::OutputWhitesmith: set_format_whitesmith(indent_text, expand_empty, blank_before_colon, blank_before_comma); break;
    default: throw JSException("Invalid JSON output format");
    }
}
void JSFormatter::set_format_compact() {
    format_ = JSOutputFormat::OutputCompact;
    indent_.clear();
    expand_empty_ = blank_before_colon_ = blank_before_comma_ = indent_closure_ = false;
}
void JSFormatter::set_format_linear(bool expand_empty, bool blank_before_colon, bool blank_before_comma) {
    set_format_knr({}, expand_empty, false, blank_before_colon, blank_before_comma);
    format_ = JSOutputFormat::OutputLinear;
}
void JSFormatter::set_format_knr(std::string_view indent_text, bool expand_empty,
    bool indent_closure, bool blank_before_colon, bool blank_before_comma) {
    format_ = JSOutputFormat::OutputKNR;
    indent_ = indent_text;
    expand_empty_ = expand_empty;
    indent_closure_ = indent_closure;
    blank_before_colon_ = blank_before_colon;
    blank_before_comma_ = blank_before_comma;
}
void JSFormatter::set_format_allman(std::string_view indent_text, bool expand_empty,
    bool blank_before_colon, bool blank_before_comma) {
    set_format_knr(indent_text, expand_empty, false, blank_before_colon, blank_before_comma);
    format_ = JSOutputFormat::OutputAllman;
}
void JSFormatter::set_format_whitesmith(std::string_view indent_text, bool expand_empty,
    bool blank_before_colon, bool blank_before_comma) {
    set_format_knr(indent_text, expand_empty, false, blank_before_colon, blank_before_comma);
    format_ = JSOutputFormat::OutputWhitesmith;
}
void JSFormatter::indent(std::ostream& stream, std::size_t depth) const {
    for (std::size_t i = 0; i < depth; ++i) stream << indent_;
}

void JSFormatter::render(std::ostream& stream, const JSItem& item, std::size_t depth) const {
    if (depth > 256) throw JSException("JSON nesting limit exceeded (256)");
    if (item.is_null()) { stream << "null"; return; }
    if (item.is_boolean()) { stream << (item.get_boolean() ? "true" : "false"); return; }
    if (item.is_number()) {
        if (!std::isfinite(item.get_number())) throw JSException("Non-finite number cannot be written as JSON");
        stream << *item.get_string();
        return;
    }
    if (item.is_string()) { stream << quote(*item.get_string()); return; }

    const bool object = item.is_object();
    const char open = object ? '{' : '[';
    const char close = object ? '}' : ']';
    if (item.size() == 0) {
        stream << open;
        if (expand_empty_ && format_ != JSOutputFormat::OutputCompact) stream << ' ';
        stream << close;
        return;
    }
    const bool compact = format_ == JSOutputFormat::OutputCompact;
    const bool linear = format_ == JSOutputFormat::OutputLinear;
    const bool whitesmith = format_ == JSOutputFormat::OutputWhitesmith;
    const bool allman = format_ == JSOutputFormat::OutputAllman;
    if (depth > 0 && (allman || whitesmith)) {
        stream << '\n';
        indent(stream, whitesmith ? depth + 1 : depth);
    }
    stream << open;
    for (std::size_t i = 0; i < item.size(); ++i) {
        if (i > 0) {
            if (blank_before_comma_ && !compact) stream << ' ';
            stream << ',';
            if (linear) stream << ' ';
        }
        if (!compact && !linear) {
            stream << '\n';
            indent(stream, depth + 1);
        }
        std::shared_ptr<const JSItem> child;
        if (object) {
            const auto& [key, value] = item.get_dictionary()[i];
            stream << quote(key);
            if (blank_before_colon_ && !compact) stream << ' ';
            stream << ':';
            if (!compact) stream << ' ';
            child = value;
        } else { child = item[i]; }
        render(stream, *child, depth + 1);
    }
    if (!compact && !linear) {
        stream << '\n';
        indent(stream, (whitesmith && depth > 0) || indent_closure_ ? depth + 1 : depth);
    }
    stream << close;
}
std::string JSFormatter::format(const JSItem& item) const {
    std::ostringstream stream;
    write(stream, item);
    return stream.str();
}
void JSFormatter::write(std::ostream& stream, const JSItem& item) const {
    render(stream, item, 0);
    if (!stream) throw JSException("JSON stream write failed");
}
void JSFormatter::write(const std::filesystem::path& path, const JSItem& item) const {
    std::ofstream stream(path, std::ios::binary);
    if (!stream) throw JSException("Cannot open JSON output file");
    write(stream, item);
    stream.flush();
    if (!stream) throw JSException("JSON file write failed");
}

}
