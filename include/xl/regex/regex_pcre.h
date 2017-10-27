#pragma once

#if defined XL_USE_PCRE

#include <pcre.h>
#include "../zstring_view.h"

#include <fmt/format.h>

namespace xl {

class RegexPcre;

using pcre_ptr = std::shared_ptr<pcre>;

inline pcre_ptr make_pcre_shared_ptr(pcre * compiled_pattern) {
    return pcre_ptr(compiled_pattern, [](pcre * compiled_pattern){pcre_free(compiled_pattern);});
}

class RegexResultPcre {
    friend class RegexPcre;
private:

    pcre_ptr compiled_pattern;

    /// original regex string
    std::string const source;

    /// number of actual captures from running the regex
    int results = 0;

    /// buffer for storing submatch offsets (3 for each capture)
    std::unique_ptr<int[]> const captures;

    /// number of captures space has been allocated for (int * 3)
    int const capture_count;


public:
    RegexResultPcre(pcre_ptr compiled_pattern,
                    std::string source,
                    int results,
                    std::unique_ptr<int[]> captures,
                    size_t capture_count) :
        compiled_pattern(compiled_pattern),
        source(source),
        results(results),
        captures(std::move(captures)),
        capture_count(capture_count)
    {}


    operator bool() const {
        return results > 0;
    }


    size_t size() const {
        return this->results;
    }


    std::string_view prefix() const {
        size_t length = this->captures[0];
        return std::string_view(&this->source[0], length);

    }

    char const * suffix() const {
//        std::cerr << fmt::format("string length: {}, captures[1]: {}", this->source.length(), this->captures[1]) << std::endl;
//        std::cerr << fmt::format("suffix: '{}'", this->source.data() + this->captures[1]) << std::endl;
        char const * result = this->source.data() + this->captures[1];
//        std::cerr << fmt::format("returning suffix address {} vs base {}", (void*)result, (void*)this->source.c_str()) << std::endl;
        return result;
    }


    size_t length(xl::zstring_view name) const {
        auto index = pcre_get_stringnumber(this->compiled_pattern.get(), name.c_str());
        // std::cerr << fmt::format("Looked up named capture '{}' => {}", name.c_str(), index) << std::endl;
        return this->length(index);
    }


    size_t length(size_t index) const {
        if (index > this->results) {
            return 0;
        }
        auto length = this->captures[index * 2 + 1] - this->captures[index * 2];
        // std::cerr << fmt::format("return length of substring {}: {}", index, length) << std::endl;
        return length;
    }


    std::string operator[](char const * name) const {
        auto index = pcre_get_stringnumber(this->compiled_pattern.get(), name);
        // std::cerr << fmt::format("Looked up named capture '{}' => {}", name, index) << std::endl;
        return this->operator[](index);
    }


    std::string operator[](xl::zstring_view name) const {
        return this->operator[](name.c_str());
    }


    std::string operator[](int index) const {
        if (index > results) {
//            throw RegexException(fmt::format("index out of range: {} vs {}", index, results));
           // std::cerr << fmt::format("index might be out of range: {} vs {}", index, results) << std::endl;
            return "";
        }

        auto substring_buffer_length = (this->captures[(index * 2) + 1] - this->captures[(index * 2)]);
        std::string substring(substring_buffer_length, '\0');
        pcre_copy_substring(source.c_str(), this->captures.get(), this->results, index, substring.data(), substring_buffer_length + 1);
        // std::cerr << fmt::format("returning substring {}: '{}'", index, substring) << std::endl;
        return substring;
    }


    std::vector<std::string> get_all_matches() const {
        std::vector<std::string> results;
        for (int i = 0; i < this->results; i++) {
            results.push_back(this->operator[](i));
        }
        return results;
    }

};


class RegexPcre {

    pcre_ptr compiled_regex;
    pcre_extra * extra = nullptr;
    int capture_count;

    auto make_pcre_regex_flags(RegexFlagsT flags) {
        decltype(PCRE_CASELESS) result = 0;
        result |= flags & ICASE ? PCRE_CASELESS : 0;
        result |= flags & EXTENDED ? PCRE_EXTENDED : 0;
        result |= flags & DOTALL ? PCRE_DOTALL : 0;
        result |= flags & MULTILINE ? PCRE_MULTILINE : 0;
        result |= flags & DOLLAR_END_ONLY ? PCRE_DOLLAR_ENDONLY : 0;
        return result;
    }

public:

    RegexPcre(xl::zstring_view regex_string, std::underlying_type_t<xl::RegexFlags> flags = NONE)
    {
        const char *error_string;
        int error_offset;

        compiled_regex = make_pcre_shared_ptr(pcre_compile(regex_string.c_str(),
                                      make_pcre_regex_flags(flags), // options
                                      &error_string,
                                      &error_offset,
                                      NULL // table pointer ??
        ));

        if (compiled_regex == nullptr) {
            throw RegexException(std::string("Invalid regex: ") + error_string);
        }

        if (flags & OPTIMIZE) {
            char const * pcre_study_error_message;
            this->extra = pcre_study(compiled_regex.get(), 0, &pcre_study_error_message);
//            std::cerr << fmt::format("study result: {}", (void*)this->extra) << std::endl;
            if (this->extra == nullptr) {
                // this is ok
            }
        }

        // inspect the regex to find out how much space to allocate for results
        pcre_fullinfo(this->compiled_regex.get(), this->extra, PCRE_INFO_CAPTURECOUNT, &this->capture_count);
        this->capture_count++; // plus one for full match
    }


    static std::string info(){
        return fmt::format("PCRE Version: {}.{}", PCRE_MAJOR, PCRE_MINOR);
    }


    RegexResultPcre match(xl::zstring_view data) const {
        auto buffer_length = this->capture_count * 3;
        auto buffer = std::make_unique<int[]>(buffer_length);
        auto results = pcre_exec(this->compiled_regex.get(),
                                         this->extra,
                                         data.c_str(),
                                         data.length(),  // length of string
                                         0,              // Start looking at this point
                                         0,              // OPTIONS
                                         buffer.get(),
                                         buffer_length); // Length of subStrVec

        return RegexResultPcre(this->compiled_regex, data, results, std::move(buffer), buffer_length);
    }


    ~RegexPcre() {
        pcre_free(this->extra);

        // this is only used with the JIT compiler
        // pcre_free_study(this->extra);
    }


    std::string replace(xl::zstring_view source, xl::zstring_view format) {

        auto matches = this->match(source);
        if (!matches) {
            return source;
        }


        std::stringstream result;
        result << matches.prefix();
        bool found_escape = false;
        for(int i = 0; format.c_str()[i] != '\0'; i++) {
            char c = format.c_str()[i];
//            std::cerr << fmt::format("looking at '{}'", c) << std::endl;
            if (c == '$') {
                if (found_escape) {
//                    std::cerr << fmt::format("adding backslash") << std::endl;
                    result << '$';
                } else {
//                    std::cerr << fmt::format("found escape char.. not doing anything yet") << std::endl;
                    found_escape = true;
                }
            } else {
                if (found_escape) {
                    if (c >= '0' && c <= '9') {
                        int index = c - '0';
//                        std::cerr << fmt::format("index {} matches.results {}", index, matches.results) << std::endl;
                        if (index >= matches.results) {
                            throw RegexException(
                                std::string("Regex doesn't contain match index") + std::to_string(index));
                        }
//                        std::cerr << fmt::format("adding match {}: '{}'", index, matches[index]) << std::endl;

                        result << matches[index];
                    } else {
//                        std::cerr << fmt::format("adding escaped '{}'", c) << std::endl;
                        result << c;
                    }
                } else {
//                    std::cerr << fmt::format("adding '{}'", c) << std::endl;
                    result << c;
                }
                found_escape = false;
            }
        }

        result << matches.suffix();
        return result.str();
    }

};

#endif

} // end namespace xl