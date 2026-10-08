#pragma once

#include <easyjson/jsitem.hpp>
#include <filesystem>

namespace easyjson {

class JSFormatter {
public:
    explicit JSFormatter(JSOutputFormat format = JSOutputFormat::OutputKNR,
        std::string_view indent = "\t", bool expand_empty = false,
        bool blank_before_colon = false);
    void set_format(JSOutputFormat format, std::string_view indent = "\t",
        bool expand_empty = true, bool blank_before_colon = false,
        bool blank_before_comma = false);
    void set_format_compact();
    void set_format_linear(bool expand_empty = true, bool blank_before_colon = false,
        bool blank_before_comma = false);
    void set_format_knr(std::string_view indent = "\t", bool expand_empty = false,
        bool indent_closure = false, bool blank_before_colon = false,
        bool blank_before_comma = false);
    void set_format_allman(std::string_view indent = "\t", bool expand_empty = false,
        bool blank_before_colon = false, bool blank_before_comma = false);
    void set_format_whitesmith(std::string_view indent = "\t", bool expand_empty = false,
        bool blank_before_colon = false, bool blank_before_comma = false);
    [[nodiscard]] std::string format(const JSItem& item) const;
    void write(std::ostream& stream, const JSItem& item) const;
    void write(const std::filesystem::path& path, const JSItem& item) const;
private:
    void render(std::ostream& stream, const JSItem& item, std::size_t depth) const;
    void indent(std::ostream& stream, std::size_t depth) const;
    JSOutputFormat format_;
    std::string indent_;
    bool expand_empty_;
    bool blank_before_colon_;
    bool blank_before_comma_ = false;
    bool indent_closure_ = false;
};

}
