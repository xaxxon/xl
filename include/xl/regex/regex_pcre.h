#pragma once

#if defined XL_USE_PCRE

#include <sstream>
#include <string_view>
#include <string>

#include <pcre.h>
#include <fmt/format.h>

#include "regexer.h"
#include "../zstring_view.h"

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
    std::string source = "";

    /// number of actual captures from running the regex
    size_t results = 0;

    /// buffer for storing submatch offsets (3 for each capture)
    std::vector<int> captures;

    RegexPcre const * regex = nullptr;

public:
    RegexResultPcre(pcre_ptr compiled_pattern,
                    std::string source,
                    size_t results,
                    std::vector<int> captures,
                    RegexPcre const & regex) :
        compiled_pattern(compiled_pattern),
        source(source),
        results(results),
        captures(std::move(captures)),
        regex(&regex)
    {
        assert(results < 10000000);
    }

    RegexResultPcre() = default;

    RegexResultPcre(RegexResultPcre&&) = default;

    // Not defined anywhere but declaration needed for NRVO to function
    RegexResultPcre(RegexResultPcre const &) = default;

    RegexResultPcre & operator=(RegexResultPcre &&) = default;
    RegexResultPcre & operator=(RegexResultPcre const &) = default;


    /**
     * Was this object generated from a successful regex match or not
     * @return
     */
    operator bool() const {
        return results > 0;
    }


    /**
     * Number of results present in this match object
     * @return
     */
    size_t size() const {
        return this->results;
    }


    /**
     * Part of source string (if any) from before the regex matched
     * @return the portion of the string from before the match
     */
    xl::string_view prefix() const {
        size_t length = this->captures[0];
        return xl::string_view(&this->source[0], length);
    }


    /**
     * Part of the source string (if any) from after the regex matched
     * @return
     */
    char const * suffix() const {
        if (*this) {
//        std::cerr << fmt::format("string length: {}, captures[1]: {}", this->source.length(), this->captures[1]) << std::endl;
//        std::cerr << fmt::format("suffix: '{}'", this->source.data() + this->captures[1]) << std::endl;
            char const * result = this->source.data() + this->captures[1];
//            std::cerr << fmt::format("returning suffix address {} vs base {}", (void*)result, (void*)this->source.c_str()) << std::endl;
            return result;
        } else {
            return source.c_str();
        }
    }

    // first 2 bytes in the table are a 16 bit integer with the most significant byte first
    static uint16_t get_index_from_stringtable_at(char * table_location) {
        auto byte_pointer = reinterpret_cast<uint8_t*>(table_location);
        uint16_t index = (byte_pointer[0] << 8) | byte_pointer[1];
        return index;
    }

    /**
     * Returns true if the specified named capture has a non-zero length
     * @param name named capture to check for non-zero length
     * @return whether the specified named capture has a non-zero length
     */
    bool has(xl::zstring_view name) const {
        if (!*this) {
            return false;
        }

        char * begin = nullptr;
        char * end = nullptr;
        auto size = pcre_get_stringtable_entries(this->compiled_pattern.get(), name.c_str(), &begin, &end);
//        std::cerr << fmt::format("strintable entries return value: {}", size) << std::endl;
//        std::cerr << fmt::format("begin {} end {}", (void*)begin, (void*)end) << std::endl;
        while (begin != end + size) {
            auto index = get_index_from_stringtable_at(begin);
            begin += size;
            if (this->has(index)) {
                return true;
            }
        }
//        auto index = pcre_get_stringnumber(this->compiled_pattern.get(), name);
//        std::cerr << fmt::format("Looked up named capture '{}' => {}", name, index) << std::endl;
        return false;
    }

    bool has(int position) const {
        return this->length(position) > 0;
    }


    /**
     * Returns the length of the named capturing pattern
     * @param name name of the capturing pattern
     * @return length of the named capturing pattern
     */
    size_t length(xl::zstring_view name) const {
        if (!*this) {
            return 0;
        }

        auto index = pcre_get_stringnumber(this->compiled_pattern.get(), name.c_str());
        // std::cerr << fmt::format("Looked up named capture '{}' => {}", name.c_str(), index) << std::endl;
        return this->length(index);
    }


    /**
     * Length of the match at the specified position
     * @param index position to get length for
     * @return length of the match at the specified position
     */
    size_t length(size_t index) const {
        if (index > this->results) {
            return 0;
        }
        auto length = this->captures[index * 2 + 1] - this->captures[index * 2];
        // std::cerr << fmt::format("return length of substring {}: {}", index, length) << std::endl;
        return length;
    }

    xl::string_view operator[](int index) const {
        return this->operator[](static_cast<size_t>(index));
    }


        /**
         * Returns the string captured by the named capturing pattern
         * @param name name of the pattern to return the value for
         * @return captured string for the specified pattern
         */
    xl::string_view operator[](char const * const name) const {
        if (!*this) {
            return xl::string_view();
        }
        char * begin = nullptr;
        char * end = nullptr;
        auto size = pcre_get_stringtable_entries(this->compiled_pattern.get(), name, &begin, &end);
//        std::cerr << fmt::format("strintable entries return value: {}", size) << std::endl;
//        std::cerr << fmt::format("begin {} end {}", (void*)begin, (void*)end) << std::endl;
        while (begin != end + size) {
            auto index = get_index_from_stringtable_at(begin);
//            std::cerr << fmt::format("operator[] stringtable: {} => {}", name, index) << std::endl;
            begin += size;
            auto possible_result = this->operator[](index);
            if (!possible_result.empty()) {
                return possible_result;
            }
        }
//        auto index = pcre_get_stringnumber(this->compiled_pattern.get(), name);
//         std::cerr << fmt::format("Looked up named capture '{}' => {}", name, index) << std::endl;
        return {};
    }


    /**
     * Returns the string captured by the named capturing pattern
     * @param name name of the pattern to return the value for
     * @return captured string for the specified pattern
     */
    xl::string_view operator[](xl::zstring_view name) const {
        return this->operator[](name.c_str());
    }


    /**
     * Returns the string captured by the capturing pattern at the specified index
     * @param index index to return captured string for pattern
     * @return string captured by capture pattern at specified index
     */
    xl::string_view operator[](size_t index) const {
        if (index > results) {
//            throw RegexException(fmt::format("index out of range: {} vs {}", index, results));
           // std::cerr << fmt::format("index might be out of range: {} vs {}", index, results) << std::endl;
            return "";
        }

        auto substring_buffer_length = (this->captures[(index * 2) + 1] - this->captures[(index * 2)]);
        if (substring_buffer_length == 0) {
            return xl::string_view();
        } else {
            return xl::string_view(&this->source[this->captures[(index * 2)]], substring_buffer_length);
        }
    }


    /**
     * Returns the string for the entire match as well as each capturing pattern
     * @return vector of each captured result
     */
    std::vector<xl::string_view> get_all_matches() const {
        std::vector<xl::string_view> results;
        for (size_t i = 0; i < this->results; i++) {
            results.push_back(this->operator[](i));
        }
        return results;
    }


    /**
     * Returns the match from running the same regex over the suffix of this match
     * @return next match on the same string
     */
    RegexResultPcre next() const;
    RegexResultPcre & operator*() {
        return *this;
    }

    RegexResultPcre const & operator*() const {
        return *this;
    }



    RegexResultPcre & begin() {
        return *this;
    }


    RegexResultPcre const & begin() const {
        return *this;
    }



    bool end() {
        return false;
    }


    RegexResultPcre & operator++() {
        *this = this->next();
        return *this;
    }


    bool operator!=(bool) const {
        // if this returns true, then the range is not complete because this does not equal the end
        return *this;
    }
};



class RegexPcre : public RegexBase<RegexPcre, RegexResultPcre> {

    pcre_ptr compiled_regex;
    pcre_extra * extra = nullptr;
    int capture_count;

    auto make_pcre_regex_flags(xl::RegexFlagsT flags) {
        decltype(PCRE_CASELESS) result = 0;
        result |= flags & ICASE ? PCRE_CASELESS : 0;
        result |= flags & EXTENDED ? PCRE_EXTENDED : 0;
        result |= flags & DOTALL ? PCRE_DOTALL : 0;
        result |= flags & MULTILINE ? PCRE_MULTILINE : 0;
        result |= flags & DOLLAR_END_ONLY ? PCRE_DOLLAR_ENDONLY : 0;
        result |= flags & ALLOW_DUPLICATE_SUBPATTERN_NAMES ? PCRE_DUPNAMES : 0;
        return result;
    }

public:

    using ResultT = RegexResultPcre;

    RegexPcre() = default;

    /**
     * Creates a regular expression from the given string and flags
     * @param regex_string
     * @param flags
     */
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
            throw RegexException("Invalid regex: {} - '{}'", error_string, regex_string);
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


    /**
     * Returns data about the underlying regular expression implementation
     * @return
     */
    static std::string info(){
        return fmt::format("PCRE Version: {}.{}", PCRE_MAJOR, PCRE_MINOR);
    }


    /**
     * Attempts to match this regular expression against the given string
     * @param data string to match this regular expression against
     * @return results from attempting the match
     */
    RegexResultPcre match(xl::zstring_view data) const {
        auto buffer_length = this->capture_count * 3;
        std::vector<int> buffer;
        buffer.resize(buffer_length);
        auto results = pcre_exec(this->compiled_regex.get(),
                                         this->extra,
                                         data.c_str(),
                                         data.length(),  // length of string
                                         0,              // Start looking at this point
                                         0,              // OPTIONS
                                         buffer.data(),
                                         buffer_length); // Length of subStrVec

        return RegexResultPcre(this->compiled_regex, data, results < 0 ? 0 : results, std::move(buffer), *this);
    }


    ~RegexPcre() {
        pcre_free(this->extra);

        // this is only used with the JIT compiler
        // pcre_free_study(this->extra);
    }


    /**
     * Returns a string with the matched section of the source replaced by the format string.  If no
     * match, returns an exact copy of the source string.
     * @param source string to match against
     * @param format what to substitute for the matched section of the source string.  $1-$9 replace with thecontents
     *               of captured subpatterns.  Currently limited to just 9 matches.
     * @return copy of the new string
     * @ifnot
     */
    std::string replace(xl::zstring_view source, xl::zstring_view format, bool all = false) {


        auto matches = this->match(source);
        if (!matches) {
            return source;
        }

        std::stringstream result;


        while(matches) {
            result << matches.prefix();
            bool found_escape = false;
            for (int i = 0; format.c_str()[i] != '\0'; i++) {
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
                    
                    // Looking for \0, \1, ..., \9
                    if (found_escape) {
                        if (c >= '0' && c <= '9') {
                            size_t index = c - '0';
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
            if (!all || !matches.has(0)) {
                break;
            }
            ++matches;
        }

        result << matches.suffix();
        return result.str();
    }

    operator bool() {
        return this->compiled_regex.get() != nullptr;
    }

};

inline RegexResultPcre RegexResultPcre::next() const
{
    auto suffix = this->suffix();
    if (suffix[0] == '\0') {
        return {};
    } else {
        return this->regex->match(this->suffix());
    }

}



#endif

} // end namespace xl