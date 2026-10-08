#include <easyjson/easyjson.hpp>

#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <type_traits>

using namespace easyjson;
void check(bool condition) { if (!condition) throw std::runtime_error("Interface check failed"); }
template<class F> void rejects(F function) {
    try { function(); } catch (const JSException&) { return; }
    throw std::runtime_error("Expected JSException");
}

int main() {
    auto json = JSItem::parse(R"({"name":"Bob","age":63,"values":[null,true,123.45,"text",[],{}]})");
    check(json->is_object() && json->size() == 3 && json->exists("age"));
    check(json->get_string("name") == "Bob" && json->get_integer("age") == 63);
    auto values = (*json)["values"];
    check(values->size() == 6 && values->get_string(0) == std::nullopt);
    check(values->get_boolean(1) && values->get_number(2) == 123.45);
    check(values->get_string(4) == "[]" && values->get_string(5) == "{}");
    check((*values)[1]->to_string() == "true" && (*values)[1]->get_string() == "True");
    check((*values)[0]->to_string() == "null");
    check(json->get_string() == json->to_string());
    (*json)["age"]->set_value(42);
    check(std::get<double>((*json)["age"]->get_value()) == 42);
    (*json)["name"]->set_value("Alice");
    check(json->get_string("name") == "Alice");
    (*values)[1]->set_value(false);
    check(!values->get_boolean(1));
    rejects([&] { (*json)["age"]->set_value("wrong"); });
    rejects([&] { (*values)[0]->set_value(nullptr); });
    rejects([&] { json->set_value(nullptr); });
    (*json)["name"]->set_value(nullptr);
    check(json->get_string("name") == "");

    const auto& list = values->get_list();
    auto copied_list = values->to_list();
    check(copied_list[0] == list[0]);
    copied_list.clear();
    check(values->size() == 6);
    const auto& dictionary = json->get_dictionary();
    auto copied_dictionary = json->to_dictionary();
    check(copied_dictionary[0].first == "name" && copied_dictionary[0].second == dictionary[0].second);
    copied_dictionary.clear();
    check(json->size() == 3);
    check(std::get<std::reference_wrapper<const JSList>>(values->get_value()).get().size() == 6);
    check(std::get<std::reference_wrapper<const JSDictionary>>(json->get_value()).get().size() == 3);
    std::size_t count = 0;
    for (const auto& entry : json->get_enumerator()) { check(entry.key.has_value() && entry.item != nullptr); ++count; }
    check(count == 3);
    for (const auto& entry : values->get_enumerator()) check(!entry.key.has_value());
    rejects([&] { (void)values->get_dictionary(); });
    rejects([&] { (void)json->get_list(); });
    rejects([&] { (void)(*values)[0]->get_enumerator(); });

    auto numeric = JSItem::parse_array(R"([" +255 "," 1,234.5 "," -9223372036854775808 "])" );
    check(numeric->has_numeric_content() && numeric->get_byte(0) == 255);
    check(numeric->get_number(1) == 1234.5 && numeric->get_long(2) == std::numeric_limits<std::int64_t>::min());
    numeric->add_string("bad");
    check(!numeric->has_numeric_content());
    check(!JSItem::create_array()->has_numeric_content());
    for (const auto* text : {"+-1", "++1", "1e,2", ",1", "1.2,3", "nan(payload)"}) {
        String invalid(text);
        rejects([&] { (void)invalid.get_number(); });
    }
    rejects([&] { (void)numeric->get_integer(1); });
    auto overflow = JSItem::parse_array(R"(["9223372036854775808"])" );
    rejects([&] { (void)overflow->get_long(0); });

    auto unicode = JSItem::parse_object(R"({"a\"b":"\uD83D\uDE00\n\t\u0000","":"//not a comment,]"})");
    check(unicode->exists("a\"b") && unicode->exists(""));
    check(JSItem::parse(unicode->to_string())->to_string() == unicode->to_string());
    check(JSItem::parse("/* comment */ [1, // comment\n 2,]")->size() == 2);
    check(JSItem::parse("{\"a\":1, /* comment */ }")->size() == 1);
    for (const auto* invalid : {"", "null", "1", "true", "[", "[, ]", "[1,,]", "[01]", "[1e]",
        "[true false]", "[1] trailing", "{\"a\":1,\"a\":2}", "{\"a\":{\"x\":1,\"x\":2}}",
        "{\"a\":}", "[\"\\uD800\"]", "/* unterminated", "[\"raw\nnewline\"]"}) {
        rejects([&] { (void)JSItem::parse(invalid); });
    }
    rejects([] { (void)JSItem::parse_array("{}"); });
    rejects([] { (void)Array::parse("{}"); });
    check(Array::parse("[]")->size() == 0);
    rejects([] { (void)JSItem::parse_object("[]"); });
    std::string deep(258, '['); deep += std::string(258, ']');
    rejects([&] { (void)JSItem::parse(deep); });

    const auto compact = json->to_string();
    for (auto mode : {JSOutputFormat::OutputCompact, JSOutputFormat::OutputLinear, JSOutputFormat::OutputKNR,
        JSOutputFormat::OutputAllman, JSOutputFormat::OutputWhitesmith}) {
        for (bool expand : {false, true}) {
            const auto text = json->to_string(mode, "  ", expand);
            check(JSItem::parse(text)->to_string() == compact);
            check(JSItem::parse(text)->to_string(mode, "  ", expand) == text);
            auto array = JSItem::parse_array("[{}, [], [1]]");
            check(JSItem::parse(array->to_string(mode, "  ", expand))->to_string() == array->to_string());
        }
    }
    auto sample = JSItem::parse("{\"x\":{\"y\":1}}");
    check(sample->to_string(JSOutputFormat::OutputLinear) == "{\"x\": {\"y\": 1}}");
    check(sample->to_string(JSOutputFormat::OutputKNR) == "{\n\t\"x\": {\n\t\t\"y\": 1\n\t}\n}");
    check(sample->to_string(JSOutputFormat::OutputAllman) == "{\n\t\"x\": \n\t{\n\t\t\"y\": 1\n\t}\n}");
    check(sample->to_string(JSOutputFormat::OutputWhitesmith) == "{\n\t\"x\": \n\t\t{\n\t\t\"y\": 1\n\t\t}\n}");
    JSFormatter formatter;
    formatter.set_format_linear(true, true, true);
    check(formatter.format(*JSItem::parse("{\"x\":{},\"y\":[]}")) == "{\"x\" : { } , \"y\" : [ ]}");
    formatter.set_format_knr(" ", false, true);
    check(formatter.format(*JSItem::parse("[1]")) == "[\n 1\n ]");
    formatter.set_format_allman();
    formatter.set_format_whitesmith();
    formatter.set_format_compact();
    check(formatter.format(*json) == compact);
    auto add_test = JSItem::create_object();
    auto new_child = std::make_shared<Number>(10);
    check(add_test->add(new_child, "value") == add_test && (*add_test)["value"] == new_child);

    std::ostringstream output;
    json->write(output);
    check(output.str() == compact);
    std::string buffer = "prefix:";
    json->write(buffer);
    check(buffer == "prefix:" + compact);
    output.str("");
    json->write(output, JSOutputFormat::OutputLinear);
    check(JSItem::parse(output.str())->to_string() == compact);
    output.str(""); json->write(output, formatter);
    check(output.str() == compact);
    std::istringstream input(compact);
    check(JSItem::parse(input)->to_string() == compact);
    std::istringstream array_input("[]"), object_input("{}");
    check(JSItem::parse_array(array_input)->is_array());
    check(JSItem::parse_object(object_input)->is_object());
    output.setstate(std::ios::badbit);
    rejects([&] { json->write(output); });
    std::istringstream broken; broken.setstate(std::ios::badbit);
    rejects([&] { (void)JSItem::parse(broken); });
    Number infinite(std::numeric_limits<double>::infinity());
    rejects([&] { (void)infinite.to_string(); });

    const auto path = std::filesystem::temp_directory_path() / "easyjson-interface-test.json";
    formatter.write(path, *json);
    std::ifstream file(path);
    check(JSItem::parse(file)->to_string() == compact);
    file.close();
    std::filesystem::remove(path);
}
