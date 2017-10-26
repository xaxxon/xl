#pragma once

#include <fmt/format.h>
#include <iostream>

#include <regex>
#include "../zstring_view.h"


namespace xl {


class RegexResultStd {
    std::smatch _matches;
    std::string _string_copy;

public:
    RegexResultStd(std::string_view string, std::regex const & regex) :
        _string_copy(string)
    {
        std::regex_search(this->_string_copy, this->_matches, regex);
    }

    RegexResultStd(zstring_view regex_string, zstring_view string) :
        RegexResultStd(string, std::regex(regex_string.c_str()))
    {}

    // everything after the match
    char const * suffix() {
        return &*(this->_matches.suffix().first);
    }

    std::string operator[](size_t n) const {
        return this->_matches[n].str();
    }

    auto size() const {
        return this->_matches.size();
    }

    bool empty() const {
        return this->_matches.empty();
    }

    bool length(size_t n = 0) const {
        return this->_matches.length(n);
    }

    /**
     * Returns true if the regex matched anything
     * @return whether the regex matched anything
     */
    operator bool() const {
        return !this->empty();
    }
};

class RegexStd {
    std::regex regex;
    std::string regex_string = "UNKNOWN";

    std::regex_constants::syntax_option_type make_std_regex_flags(xl::RegexFlags flags) {

        std::underlying_type_t<std::regex_constants::syntax_option_type> result = std::regex_constants::ECMAScript;

        if (flags & EXTENDED) {
            throw RegexException("std::regex doesn't support extended regexes");
        } else if (flags & DOTALL) {
            throw RegexException("std::regex doesn't support DOTALL");
        } else if (flags & MULTILINE) {
            throw RegexException("std::regex for my compiler hasn't yet implemented std::regex_constants::multiline");
        } else if (flags & DOLLAR_END_ONLY) {
            throw RegexException("Not sure if this is even a change in behavior for std::regex");
        }

        result |= (flags & OPTIMIZE ? std::regex_constants::optimize : 0);

        // not supported in clang 5
//        result |= flags & MULTILINE ? std::regex_constants::multiline : 0;
//        std::cout << fmt::format("final flags: {}", (int)result) << std::endl;
        return static_cast<std::regex_constants::syntax_option_type>(result);
    }

public:
    RegexStd(xl::zstring_view regex_string, xl::RegexFlags flags = NONE) try :
//        regex(regex_string.c_str(), make_std_regex_flags(flags)),
        regex_string(regex_string)
    {
//        std::cerr << fmt::format("flags are {} => std::regex flag {}", (int)flags, make_std_regex_flags(flags)) << std::endl;
//        std::cout << fmt::format("Creating regex with '{}'", regex_string.c_str()) << std::endl;
        this->regex = std::regex(regex_string.c_str(), make_std_regex_flags(flags));
    } catch (std::regex_error const & e) {
        std::cerr << fmt::format("caught error creating std::regex for '{}'", regex_string.c_str()) << std::endl;
        throw xl::RegexException(e.what());
    }

    RegexStd(std::regex regex) :
        regex(std::move(regex)) {}

    RegexResultStd match(std::string_view source) const {
//        std::cout << fmt::format("about to match with {}", regex_string) << std::endl;
        return RegexResultStd(source, this->regex);
    }


    std::string replace(xl::zstring_view source, xl::zstring_view result) {
        return std::regex_replace(source.c_str(), this->regex, result.c_str());
    }
};

} // end namespace xl