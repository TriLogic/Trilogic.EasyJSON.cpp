#include <easyjson/jsarray.hpp>

namespace easyjson {

ValueType Array::type() const noexcept { return ValueType::Array; }
std::size_t Array::size() const noexcept { return items_.size(); }

bool Array::has_numeric_content() const noexcept {
    if (items_.empty()) return false;
    for (const auto& item : items_) if (!item->has_numeric_content()) return false;
    return true;
}

void Array::clear() noexcept {
    for (const auto& item : items_) item->parent_.reset();
    items_.clear();
}

std::shared_ptr<JSItem> Array::operator[](std::size_t index) { return items_.at(index); }
std::shared_ptr<const JSItem> Array::operator[](std::size_t index) const { return items_.at(index); }

std::shared_ptr<JSItem> Array::add(std::shared_ptr<JSItem> item) {
    auto self = validate_child(item);
    items_.push_back(item);
    item->parent_ = self;
    return item->is_container() ? item : self;
}

std::shared_ptr<JSItem> Array::remove(std::size_t index) {
    auto item = items_.at(index);
    items_.erase(items_.begin() + index);
    item->parent_.reset();
    return item;
}

}
