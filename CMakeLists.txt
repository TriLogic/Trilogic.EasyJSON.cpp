cmake_minimum_required(VERSION 3.30)
project(EasyJSON VERSION 0.1.0 LANGUAGES CXX)

set(EASYJSON_CXX_STANDARD "26" CACHE STRING "C++ standard (23 or 26)")
set_property(CACHE EASYJSON_CXX_STANDARD PROPERTY STRINGS 23 26)
if(NOT EASYJSON_CXX_STANDARD MATCHES "^(23|26)$")
    message(FATAL_ERROR "EASYJSON_CXX_STANDARD must be 23 or 26")
endif()
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(easyjson
    src/easyjson.cpp
    src/jsitem.cpp
    src/jsreader.cpp
    src/jsformatter.cpp
    src/jsnull.cpp
    src/jsbool.cpp
    src/jsnumber.cpp
    src/jsstring.cpp
    src/jsarray.cpp
    src/jsobject.cpp
)
add_library(EasyJSON::easyjson ALIAS easyjson)
target_compile_features(easyjson PUBLIC cxx_std_${EASYJSON_CXX_STANDARD})
target_include_directories(easyjson PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(easyjson PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(easyjson_toolchain_smoke tests/toolchain_smoke.cpp)
    target_link_libraries(easyjson_toolchain_smoke PRIVATE EasyJSON::easyjson)
    add_test(NAME toolchain_smoke COMMAND easyjson_toolchain_smoke)
    add_executable(easyjson_values tests/values.cpp)
    target_link_libraries(easyjson_values PRIVATE EasyJSON::easyjson)
    add_test(NAME values COMMAND easyjson_values)
    add_executable(easyjson_interface tests/interface.cpp)
    target_link_libraries(easyjson_interface PRIVATE EasyJSON::easyjson)
    add_test(NAME interface COMMAND easyjson_interface)
    add_executable(easyjson_parsing tests/parsing.cpp)
    target_link_libraries(easyjson_parsing PRIVATE EasyJSON::easyjson)
    add_test(NAME parsing COMMAND easyjson_parsing)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
install(TARGETS easyjson EXPORT EasyJSONTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT EasyJSONTargets NAMESPACE EasyJSON:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/EasyJSON)
configure_package_config_file(cmake/EasyJSONConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/EasyJSONConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/EasyJSON)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/EasyJSONConfigVersion.cmake
    COMPATIBILITY SameMajorVersion)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/EasyJSONConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/EasyJSONConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/EasyJSON)
install(FILES LICENSE.Trilogic.EasyJSON.txt THIRD_PARTY_NOTICES.md
    DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/EasyJSON)
