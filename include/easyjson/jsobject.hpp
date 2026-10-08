#pragma once

#include <easyjson/jsitem.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace easyjson {

class Object final : public JSItem {
public:
    using JSItem::operator[];
    using JSItem::add;
    using JSItem::remove;
    [[nodiscard]] ValueType type() const noexcept override;
    [[nodiscard]] std::size_t size() const noexcept override;
    const JSDictionary& get_dictionary() const override { return members_; }
    [[nodiscard]] bool contains(std::string_view key) const noexcept override;
    std::shared_ptr<JSItem> operator[](std::string_view key) override;
    std::shared_ptr<const JSItem> operator[](std::string_view key) const override;
    std::shared_ptr<JSItem> add(std::string key, std::shared_ptr<JSItem> item) override;
    template<class T, class... Args> std::shared_ptr<T> emplace(std::string key, Args&&... args) {
        auto item = std::make_shared<T>(std::forward<Args>(args)...);
        add(std::move(key), item);
        return item;
    }
    [[nodiscard]] std::shared_ptr<JSItem> remove(std::string_view key) override;
    void clear() noexcept override;
private:
    // Preserve insertion order for the future formatter.
    std::vector<std::pair<std::string, std::shared_ptr<JSItem>>> members_;
};


}
