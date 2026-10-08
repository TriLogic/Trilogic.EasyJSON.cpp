#include <easyjson/jsobject.hpp>

#include <algorithm>

namespace easyjson {

ValueType Object::type() const noexcept { return ValueType::Object; }
std::size_t Object::size() const noexcept { return members_.size(); }

void Object::clear() noexcept {
    for (const auto& [name, item] : members_) item->parent_.reset();
    members_.clear();
}

bool Object::contains(std::string_view key) const noexcept {
    return std::ranges::any_of(members_, [key](const auto& member) { return member.first == key; });
}

std::shared_ptr<const JSItem> Object::operator[](std::string_view key) const {
    for (const auto& [name, item] : members_) {
        if (name == key) return item;
    }
    throw std::out_of_range("JSON object key not found");
}

std::shared_ptr<JSItem> Object::operator[](std::string_view key) {
    for (const auto& [name, item] : members_) {
        if (name == key) return item;
    }
    throw std::out_of_range("JSON object key not found");
}

std::shared_ptr<JSItem> Object::add(std::string key, std::shared_ptr<JSItem> item) {
    auto self = validate_child(item);
    if (contains(key)) throw std::invalid_argument("Duplicate JSON object key");
    members_.emplace_back(std::move(key), item);
    item->parent_ = self;
    return item->is_container() ? item : self;
}

std::shared_ptr<JSItem> Object::remove(std::string_view key) {
    for (auto it = members_.begin(); it != members_.end(); ++it) {
        if (it->first == key) {
            auto item = it->second;
            members_.erase(it);
            item->parent_.reset();
            return item;
        }
    }
    throw std::out_of_range("JSON object key not found");
}

}
