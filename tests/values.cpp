#include <easyjson/easyjson.hpp>

#include <stdexcept>
#include <type_traits>

using namespace easyjson;

void check(bool condition) {
    if (!condition) throw std::runtime_error("Value model check failed");
}

template<class Exception, class Function>
void throws(Function function) {
    try { function(); }
    catch (const Exception&) { return; }
    throw std::runtime_error("Expected exception");
}

int main() {
    static_assert(std::is_abstract_v<JSItem>);
    static_assert(!std::is_move_constructible_v<Object>);
    Null null;
    check(null.type() == ValueType::Null && null.value() == nullptr);
    Bool boolean(true);
    boolean.set_value(false);
    check(boolean.type() == ValueType::Bool && !boolean.value());
    Number number(3.5);
    number.set_value(-2);
    check(number.type() == ValueType::Number && number.value() == -2);
    JSItem& numeric = number;
    check(numeric.get_integer() == -2 && numeric.get_long() == -2);
    check(numeric.get_double() == -2 && numeric.get_float() == -2);
    check(numeric.has_numeric_content());
    throws<JSException>([&] { (void)numeric.get_byte(); });
    String digits("255");
    check(digits.get_byte() == 255 && digits.has_numeric_content());
    String fraction("1.5");
    check(fraction.get_number() == 1.5);
    throws<JSException>([&] { (void)fraction.get_integer(); });
    String invalid("text");
    check(!invalid.has_numeric_content());
    String string;
    string.set_value(std::string("a\0b", 3));
    check(string.type() == ValueType::String && string.value().size() == 3);
    throws<JSException>([&] { (void)number.get_boolean(); });

    auto document = std::make_shared<Object>();
    auto array = document->emplace<Array>("items");
    auto child = array->emplace<Object>();
    child->emplace<String>("name", "example");
    child->emplace<Bool>("enabled", true);
    array->emplace<Null>();
    document->emplace<Number>("", 42);
    check(document->is_container() && document->size() == 2);
    check(array->type() == ValueType::Array && array->size() == 2);
    check(child->parent() == array && child->root() == document);
    check((*document)[""]->get_number() == 42);
    const Object& view = *document;
    check((*(*view["items"])[0])["name"]->get_string() == "example");
    check(std::as_const(*child).root() == document);
    throws<std::invalid_argument>([&] { document->emplace<Null>("items"); });
    throws<std::invalid_argument>([&] { array->add(nullptr); });
    throws<std::out_of_range>([&] { (void)(*document)["missing"]; });
    throws<std::out_of_range>([&] { (void)array->remove(10); });
    throws<std::out_of_range>([&] { (void)document->remove("missing"); });

    auto detached = array->remove(0);
    check(detached == child && detached->parent() == nullptr && detached->root() == detached);
    check((*detached)["name"]->parent() == detached);
    auto other = JSItem::create_object();
    other->add("moved", detached);
    check((*other)["moved"]->root() == other);
    auto removed = document->remove("items");
    check(!document->contains("items") && removed->parent() == nullptr);
    other->clear();
    check(other->size() == 0 && child->parent() == nullptr);

    auto base = JSItem::create_object();
    auto values = base->add_array("values");
    check(values->add_number(1)->add_boolean(true)->add_string("text") == values);
    check(values->get_number(0) == 1 && values->get_boolean(1));
    check(values->get_integer(0) == 1 && values->get_byte(0) == 1);
    check((*values)[2]->get_string() == "text");
    throws<JSException>([&] { (void)values->remove("wrong"); });
    throws<JSException>([&] { (void)base->remove(0); });
    throws<JSException>([&] { (void)values->contains("wrong"); });
    throws<JSException>([&] { null.clear(); });
    throws<JSException>([&] { (void)null[0]; });
    throws<JSException>([&] { (void)base->add_null(); });
    check(base->size() == 1 && values->size() == 3);
    const JSItem& const_base = *base;
    check((*const_base["values"])[0]->get_number() == 1);
    auto base_removed = values->remove(0);
    check(base_removed->parent() == nullptr && base_removed->get_number() == 1);
    auto retained = (*values)[0];
    values->clear();
    check(values->size() == 0 && retained->parent() == nullptr && retained->get_boolean());

    auto owner = JSItem::create_object();
    auto nested = owner->add_array("nested");
    throws<std::invalid_argument>([&] { (void)nested->add(owner); });
    throws<std::invalid_argument>([&] { (void)owner->add("self", owner); });
    throws<std::invalid_argument>([&] { (void)other->add("attached", nested); });
    throws<std::invalid_argument>([&] { (void)owner->add("duplicate-child", nested); });
    check(owner != nullptr && nested->size() == 0 && nested->parent() == owner);
    Object stack_container;
    throws<JSException>([&] { (void)stack_container.add_null("child"); });
    check(stack_container.size() == 0);

    // Parents do not survive solely because an external child handle exists.
    for (bool object_parent : {false, true}) {
        std::shared_ptr<JSItem> survivor;
        std::weak_ptr<JSItem> parent_watch;
        std::weak_ptr<JSItem> unretained_watch;
        std::weak_ptr<JSItem> descendant_watch;
        {
            auto parent = object_parent ? JSItem::create_object() : JSItem::create_array();
            parent_watch = parent;
            parent->add_number(123, object_parent ? "unretained" : "");
            unretained_watch = object_parent ? (*parent)["unretained"] : (*parent)[0];
            parent->add_object(object_parent ? "retained" : "")->add_string("alive", "name");
            survivor = object_parent ? (*parent)["retained"] : (*parent)[1];
            descendant_watch = (*survivor)["name"];
            check(survivor->parent() == parent && survivor->root() == parent);
        }
        check(parent_watch.expired() && unretained_watch.expired());
        check(survivor->parent() == nullptr && std::as_const(*survivor).parent() == nullptr);
        check(survivor->root() == survivor && survivor->get_string("name") == "alive");
        check(!descendant_watch.expired() && (*survivor)["name"]->parent() == survivor);
        auto adopter = JSItem::create_array();
        adopter->add(survivor);
        check(survivor->parent() == adopter);
        adopter->clear();
        check(survivor->parent() == nullptr);
        survivor.reset();
        check(descendant_watch.expired());
    }

    // A descendant can survive the destruction of every ancestor.
    std::shared_ptr<JSItem> leaf;
    std::weak_ptr<JSItem> middle_watch;
    {
        auto top = JSItem::create_object();
        auto middle = top->add_array("middle");
        middle->add_number(7);
        leaf = (*middle)[0];
        middle_watch = middle;
    }
    check(middle_watch.expired() && leaf->parent() == nullptr && leaf->get_number() == 7);

    // Failed insertions preserve both ownership and parent metadata.
    auto duplicate_parent = JSItem::create_object();
    duplicate_parent->add_null("key");
    auto candidate = std::make_shared<String>("kept");
    throws<std::invalid_argument>([&] { (void)duplicate_parent->add("key", candidate); });
    check(candidate->parent() == nullptr && candidate->value() == "kept");
}
