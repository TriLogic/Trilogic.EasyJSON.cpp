#include <easyjson/easyjson.hpp>

#include <algorithm>
#include <expected>
#include <format>
#include <ranges>
#include <string>
#include <vector>

int main() {
    std::vector<int> values{3, 1, 2};
    std::ranges::sort(values);
    auto doubled = values | std::views::transform([](int value) { return value * 2; });
    std::expected<std::string, std::string> result =
        std::format("EasyJSON {}: {}", easyjson::version(), doubled.front());
    return result && *result == "EasyJSON 0.1.0: 2" && values.back() == 3 ? 0 : 1;
}
