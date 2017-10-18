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

    // original regex string
    std::string const source;

    // number of actual captures from running the regex
    int results = 0;

    // buffer for storing submatch offsets (3 for each capture)
    std::unique_ptr<int[]> const captures;

    // number of captures space has been allocated for (int * 3)
    int const capture_count;

    std::vector<std::pair<std::string, int>> name_map;

public:
    RegexResultPcre(pcre_ptr compiled_pattern, std::string source, int results, std::unique_ptr<int[]> captures, size_t capture_count) :
        compiled_pattern(compiled_pattern),
        source(source),
        results(results),
        captures(std::move(captures)),
        capture_count(capture_count)
    {
    }

    operator bool() const {
        return results > 0;
    }

    size_t size() const {
        return this->results;
    }

    char const * suffix() const {
        std::cerr << fmt::format("string length: {}, captures[1]: {}", this->source.length(), this->captures[1]) << std::endl;
        std::cerr << fmt::format("suffix: '{}'", this->source.data() + this->captures[1]) << std::endl;
        char const * result = this->source.data() + this->captures[1];
        std::cerr << fmt::format("returning suffix address {} vs base {}", (void*)result, (void*)this->source.c_str()) << std::endl;
        return result;
    }


    size_t length(xl::zstring_view name) const {
        auto index = pcre_get_stringnumber(this->compiled_pattern.get(), name.c_str());
        std::cerr << fmt::format("Looked up named capture '{}' => {}", name.c_str(), index) << std::endl;
        return this->length(index);
    }

    size_t length(size_t index) const {
        if (index > this->results) {
            return 0;
        }
        auto length = this->captures[index * 2 + 1] - this->captures[index * 2];
        std::cerr << fmt::format("return length of substring {}: {}", index, length) << std::endl;
        return length;
    }

    std::string operator[](char const * name) const {
        auto index = pcre_get_stringnumber(this->compiled_pattern.get(), name);
        std::cerr << fmt::format("Looked up named capture '{}' => {}", name, index) << std::endl;
        return this->operator[](index);
    }

    std::string operator[](xl::zstring_view name) const {
        return this->operator[](name.c_str());
    }

    std::string operator[](int index) const {
        if (index > results) {
//            throw RegexException(fmt::format("index out of range: {} vs {}", index, results));
            std::cerr << fmt::format("index might be out of range: {} vs {}", index, results) << std::endl;
            return "";
        }

        auto substring_buffer_length = (this->captures[(index * 2) + 1] - this->captures[(index * 2)]);
        std::string substring(substring_buffer_length, '\0');
        pcre_copy_substring(source.c_str(), this->captures.get(), this->results, index, substring.data(), substring_buffer_length + 1);
std::cerr << fmt::format("returning substring {}: '{}'", index, substring) << std::endl;
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
            // TODO: improve error message
            throw RegexException("Invalid regex");
        }

        if (flags & OPTIMIZE) {
            char const * pcre_study_error_message;
            this->extra = pcre_study(compiled_regex.get(), 0, &pcre_study_error_message);
            if (this->extra == nullptr) {
                // TODO: ERROR
            }
        }

        // inspect the regex to find out how much space to allocate for results
        pcre_fullinfo(this->compiled_regex.get(), this->extra, PCRE_INFO_CAPTURECOUNT, &this->capture_count);
        this->capture_count++; // plus one for full match

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
        //pcre_free_study(this->extra);
    }

};

#endif

} // end namespace xl