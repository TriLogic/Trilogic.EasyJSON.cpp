#include <easyjson/easyjson.hpp>

#include <cmath>
#include <stdexcept>
#include <string>

using namespace easyjson;
void check(bool condition) {
    if (!condition) throw std::runtime_error("Parser regression check failed");
}
template<class F> void rejects(F function) {
    try { function(); } catch (const JSException&) { return; }
    throw std::runtime_error("Expected parser rejection");
}

int main() {
    // Decode once, including backslashes followed by escape-looking text.
    auto escaped = JSItem::parse_array(R"(["\"\\\/\b\f\n\r\t","\\u0041","\u0041F","\u0000"])");
    check(escaped->get_string(0) == "\"\\/\b\f\n\r\t");
    check(escaped->get_string(1) == "\\u0041");
    check(escaped->get_string(2) == "AF");
    check(escaped->get_string(3) == std::string(1, '\0'));
    std::string controls;
    for (char c = 0; c < 32; ++c) controls += c;
    auto array = JSItem::create_array();
    array->add_string(controls);
    check(JSItem::parse(array->to_string())->get_string(0) == controls);

    auto unicode = JSItem::parse_array(R"(["\u007f\u0080\u07ff\u0800\uffff\ud800\udc00\udbff\udfff"])");
    check(JSItem::parse(unicode->to_string())->get_string(0) == unicode->get_string(0));
    auto raw = JSItem::parse_array("[\"\xc3\xa9\xf0\x9f\x98\x80\"]");
    check(raw->get_string(0) == "\xc3\xa9\xf0\x9f\x98\x80");
    check(raw->to_string() == R"(["\u00e9\ud83d\ude00"])");
    for (const auto* bytes : {"\x80", "\xc0\xaf", "\xe0\x80\x80", "\xed\xa0\x80",
                             "\xf4\x90\x80\x80", "\xf0\x9f", "\xc2x"}) {
        rejects([&] { (void)JSItem::parse(std::string("[\"") + bytes + "\"]"); });
        String invalid(bytes);
        rejects([&] { (void)invalid.to_string(); });
    }
    for (const auto* text : {R"(["\x"])", R"(["\u123"])", R"(["\u12xz"])",
        R"(["\udc00"])", R"(["\ud800\u0041"])", "[\"unterminated", "[\"\\",
        "[+1]", "[.1]", "[-]", "[1.]", "[1e+]", "[00]", "[-01]", "[1e9999]",
        "[NaN]", "[Infinity]", "[truex]", "[nullx]", "[falsex]", "[1/*unfinished]",
        "[1]/", "{/*comment*/,}", R"({"a":1,"\u0061":2})"}) {
        rejects([&] { (void)JSItem::parse(text); });
    }
    std::string embedded = "[]"; embedded += '\0';
    rejects([&] { (void)JSItem::parse(embedded); });
    auto numbers = JSItem::parse_array("[-0,0.125,-1.25e+2,1E-2,1.7976931348623157e308]");
    check(std::signbit(numbers->get_number(0)));
    check(numbers->get_number(1) == 0.125 && numbers->get_number(2) == -125);
    check(numbers->get_number(3) == 0.01 && std::isfinite(numbers->get_number(4)));

    auto document = JSItem::parse("/*before*/{\"\": [1/*middle*/,2,],\"next\":{},} //after");
    check((*document)[""]->size() == 2 && document->get_dictionary()[0].first.empty());
    check((*(*document)[""])[0]->parent() == (*document)[""]);
    check((*(*document)[""])[0]->root() == document);
    check(JSItem::parse("[1// comment ends at EOF\n]")->size() == 1);
    check(JSItem::parse("[]// EOF comment")->size() == 0);
    check(JSItem::parse(std::string(257, '[') + std::string(257, ']'))->is_array());
    rejects([] { (void)JSItem::parse(std::string(258, '[') + std::string(258, ']')); });
}
