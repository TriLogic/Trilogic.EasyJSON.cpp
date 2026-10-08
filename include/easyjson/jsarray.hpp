#pragma once

#include <easyjson/jsitem.hpp>
#include <memory>
#include <utility>
#include <vector>

namespace easyjson {

class Array final : public JSItem {
public:
    using JSItem::operator[];
    using JSItem::add;
    using JSItem::remove;
    [[nodiscard]] ValueType type() const noexcept override;
    [[nodiscard]] std::size_t size() const noexcept override;
    const JSList& get_list() const override { return items_; }
    bool has_numeric_content() const noexcept override;
    static std::shared_ptr<Array> parse(std::string_view json) { return JSItem::parse_array(json); }
    static std::shared_ptr<Array> parse(std::istream& stream) { return JSItem::parse_array(stream); }
    std::shared_ptr<JSItem> operator[](std::size_t index) override;
    std::shared_ptr<const JSItem> operator[](std::size_t index) const override;
    std::shared_ptr<JSItem> add(std::shared_ptr<JSItem> item) override;
    template<class T, class... Args> std::shared_ptr<T> emplace(Args&&... args) {
        auto item = std::make_shared<T>(std::forward<Args>(args)...);
        add(item);
        return item;
    }
    [[nodiscard]] std::shared_ptr<JSItem> remove(std::size_t index) override;
    void clear() noexcept override;
private:
    std::vector<std::shared_ptr<JSItem>> items_;
};


}
