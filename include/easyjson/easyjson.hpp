#pragma once

#include <easyjson/jsitem.hpp>
#include <easyjson/jsformatter.hpp>
#include <easyjson/jsnull.hpp>
#include <easyjson/jsbool.hpp>
#include <easyjson/jsnumber.hpp>
#include <easyjson/jsstring.hpp>
#include <easyjson/jsarray.hpp>
#include <easyjson/jsobject.hpp>

#include <string_view>

namespace easyjson {
[[nodiscard]] std::string_view version() noexcept;
}
