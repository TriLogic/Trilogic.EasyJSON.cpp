# Trilogic.EasyJSON.cpp

Trilogic.EasyJSON.cpp is a C++ port of
[Trilogic.EasyJSON](https://github.com/TriLogic/Trilogic.EasyJSON), a C# library
for reading, writing, and formatting JSON. The library and CMake package name is
`EasyJSON`, and the C++ namespace is `easyjson`.
The port currently provides `JSItem` and its `Null`, `Bool`, `Number`, `String`,
`Array`, and `Object` value types, plus parsing, serialization, and configurable
formatting through `JSFormatter`.
Containers retain children through `std::shared_ptr`. Factories, builders,
indexing, `remove`, and `emplace<T>` return shared pointers, so retaining a returned
handle keeps that node alive. Parent links use `std::weak_ptr`: a child does not keep
its parent alive, and `parent()` returns an empty pointer when the parent expires.
`remove` and `clear` detach children immediately. Destroying a container releases
its children; externally retained children survive with a null parent.

Create containers with `JSItem::create_object()`, `JSItem::create_array()`, or
`std::make_shared<Object/Array>()`. Adding children to a stack-allocated container
throws `JSException`. A child may have only one parent; remove it before reattaching
it elsewhere. Ownership cycles are rejected. Object keys are unique, may be empty,
and retain insertion order. Numbers use `double`, matching upstream. Nodes cannot
be copied or moved directly; shared handles can be copied freely.

`JSItem` exposes indexing, removal, clearing, type checks, value retrieval, and
fluent builders through the base type, following the upstream API. Unsupported
operations throw `JSException`. Adding a primitive returns its parent; adding a
container returns the new child. Builder keys follow upstream: an omitted or empty
key selects array insertion. The explicit keyed `add` overload supports empty object
keys. Cast helpers are not exposed.

```cpp
auto document = easyjson::JSItem::create_object();
document->add_array("items")->add_string("hello");
document->add_boolean(true, "enabled");
bool enabled = document->get_boolean("enabled");

auto child = (*document)["items"];
document.reset();
bool no_parent = child->parent() == nullptr; // Child remains alive.
```

## JSON interface

`JSItem::parse`, `parse_array`, and `parse_object` accept text (`std::string_view`)
or `std::istream`. Like upstream, parsing requires an array or object root, allows
line/block comments and trailing commas, and rejects duplicate keys. Unlike
upstream's permissive reader, the port rejects trailing garbage and malformed JSON
tokens. Empty keys and escaped keys work. Nesting is limited to 256 levels.

`to_string()` emits compact JSON. Its format overload and `JSFormatter` support
`OutputCompact`, `OutputLinear`, `OutputKNR`, `OutputAllman`, and `OutputWhitesmith`,
custom indentation, and expanded empty containers. The formatter also supports
spacing before colons/commas and indented KNR closures. `write` accepts an output
stream or appends to a string; `JSFormatter::write` also accepts a filesystem path.
Streams remain owned by the caller. Strings and keys are escaped, Unicode is
validated, and non-finite numbers cannot be serialized as JSON. Upstream's invalid
empty-object and Whitesmith array delimiters are corrected.

`get_value()` returns `JSValue`, a variant of null, bool, double, string, or a
borrowed read-only collection view. `set_value()` works through `JSItem` for
primitives; null values and containers reject assignment. Assigning null to a
String produces an empty string, following upstream. `get_string()` returns
`std::optional<std::string>`: JSON null produces `std::nullopt`, booleans produce
`True`/`False` as in C#, and containers produce compact JSON.

`get_list()` and `get_dictionary()` expose borrowed read-only collections; retain
the owning node while using a view. Dictionaries are insertion-ordered key/value
pairs. `to_list()` and `to_dictionary()` make shallow copies whose shared handles
keep children alive. `get_enumerator()` returns a snapshot usable in a range-for
loop, with an optional key and a shared item handle for each entry. Object entries
have keys; array entries do not. Direct collection mutation is excluded to preserve
parent links and prevent ownership cycles. Use builders, `add`, `remove`, and `clear`
to change containers. `exists()` is the upstream object-key lookup alias.

Numeric conversions use a fixed, locale-independent convention: decimal point,
optional leading sign/whitespace, and commas for numeric string grouping.
Integer strings must contain an integer; numeric nodes truncate toward zero for
integer retrieval. Out-of-range integer conversions throw `JSException` rather
than relying on C# unchecked casts. An array has numeric content only when nonempty
and every child has numeric content. Cast helpers remain excluded by design.

The parser and string escaping use a vendored, pinned nlohmann/json 3.12.0 header.
This dependency is private to the implementation and requires no download at build
time. See `third_party/nlohmann/LICENSE.MIT` and `THIRD_PARTY_NOTICES.md`.

Development uses GCC 16.2 and its bundled libstdc++ inside a Debian container.
C++26 mode is the default, with GNU extensions disabled. C++26 support is
experimental and incomplete; see the [GCC language support table](https://gcc.gnu.org/projects/cxx-status.html)
and [standard-library status](https://gcc.gnu.org/onlinedocs/libstdc++/manual/status.html).
The `cpp23` preset provides a fallback. STL containers, algorithms, strings,
and ranges come with the compiler; no separate STL dependency is needed.

## Getting started

The host needs Docker or Podman running. All compiler, debugger, and build-tool
installation occurs in the image. The repository is mounted into the container;
build artifacts stay under `build/`. The command wrapper uses your host UID/GID.

```sh
./scripts/container build-image
./scripts/container cmake --preset dev
./scripts/container cmake --build --preset dev
./scripts/container ctest --preset dev
```

Use `release` or `cpp23` in all three preset commands to select those builds.
Run `./scripts/container` for an interactive shell. Set `CONTAINER_RUNTIME=podman`
to choose Podman explicitly. Re-run `build-image` after changing the Dockerfile.
The compiler image version is fixed; Debian package updates are resolved at image
build time.

VS Code users can open this folder with the Dev Containers extension and choose
**Reopen in Container**, then run the same CMake commands in its terminal.
The development container uses its default root account; use the command wrapper
if host file ownership must match your user.

## Library integration

Within a CMake project, use `add_subdirectory` and link `EasyJSON::easyjson`.
For an installed package:

```sh
./scripts/container cmake --install build/release --prefix /workspace/build/install
```

Consumers can then use `find_package(EasyJSON CONFIG REQUIRED)` and
`target_link_libraries(my_app PRIVATE EasyJSON::easyjson)`, with that install prefix in
`CMAKE_PREFIX_PATH`. The selected C++ standard propagates to consumers.

Public headers live in `include/easyjson`, implementation in `src`, and tests in
`tests`. The toolchain smoke test checks library linking plus containers,
algorithms, ranges, `std::format`, and `std::expected` in Debug and Release.
