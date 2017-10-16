#pragma once

#if defined XL_USE_PCRE

#include <pcre.h>

#include <fmt/ostream.h>

#include "zstring_view.h"

namespace xl {

class RegexPcre;

class RegexResultPcre {
    friend class RegexPcre;
private:

    // original regex string
    std::string const source;

    // number of actual captures from running the regex
    int results = 0;

    // buffer for storing submatch offsets (3 for each capture)
    std::unique_ptr<int[]> const captures;

    // number of captures space has been allocated for (int * 3)
    int const capture_count;

public:
    RegexResultPcre(std::string source, int results, std::unique_ptr<int[]> captures, size_t capture_count) :
        source(source),
        results(results),
        captures(std::move(captures)),
        capture_count(capture_count)
    {
//        std::cerr << fmt::format("created results for {} with results {} and capture count {} with buffer {}",
//            this->source, this->results, this->capture_count, (void*)this->captures.get()
//        ) << std::endl;
    }

    operator bool() const {
        return results > 0;
    }

    size_t size() const {
        return this->results;
    }

    std::string operator[](size_t index) const {
        if (index > results) {
            throw RegexException("Index out of range");
        }

        auto substring_buffer_length = (this->captures[(index * 2) + 1] - this->captures[(index * 2)]);
        std::string substring(substring_buffer_length, '\0');
        pcre_copy_substring(source.c_str(), this->captures.get(), this->results, index, substring.data(), substring_buffer_length + 1);

        return substring;
    }
};


class RegexPcre {

    pcre * compiled_regex = nullptr;
    pcre_extra * extra = nullptr;
    int capture_count;

    int make_pcre_regex_flags(RegexFlags flags) {
        int result = 0;
        result |= flags | ICASE ? PCRE_CASELESS : 0;

        return result;
    }

public:

    RegexPcre(xl::zstring_view regex_string, xl::RegexFlags flags = NONE)
    {
        const char *error_string;
        int error_offset;

        compiled_regex = pcre_compile(regex_string.c_str(),
                                      make_pcre_regex_flags(flags), // options
                                      &error_string,
                                      &error_offset,
                                      NULL // table pointer ??
        );

        if (compiled_regex == nullptr) {
            // TODO: improve error message
            throw RegexException("Invalid regex");
        }

        if (flags | OPTIMIZE) {
            char const * pcre_study_error_message;
            this->extra = pcre_study(compiled_regex, 0, &pcre_study_error_message);
            if (this->extra == nullptr) {
                // TODO: ERROR
            }
        }

        // inspect the regex to find out how much space to allocate for results
        pcre_fullinfo(this->compiled_regex, this->extra, PCRE_INFO_CAPTURECOUNT, &this->capture_count);
        this->capture_count++; // plus one for full match

    }


    RegexResultPcre match(xl::zstring_view data) const {
        auto buffer_length = this->capture_count * 3;
        auto buffer = std::make_unique<int[]>(buffer_length);
        auto results = pcre_exec(this->compiled_regex,
                                         this->extra,
                                         data.c_str(),
                                         data.length(),  // length of string
                                         0,              // Start looking at this point
                                         0,              // OPTIONS
                                         buffer.get(),
                                         buffer_length); // Length of subStrVec

      return RegexResultPcre(data, results, std::move(buffer), buffer_length);
    }

    ~RegexPcre() {
        //pcre_free_study(this->extra);
        pcre_free(this->compiled_regex);
    }

};

#endif

} // end namespace xl