#pragma once

#include <regex>

#include "zstring_view.h"


namespace xl {

class RegexResult {
    std::smatch _matches;
    std::string _string_copy;

public:
    RegexResult(zstring_view string, std::regex const & regex) :
        _string_copy(string)
    {
        std::regex_match(this->_string_copy, this->_matches, regex);
    }

    RegexResult(zstring_view regex_string, zstring_view string) :
        RegexResult(string, std::regex(regex_string.c_str()))
    {}

    std::smatch const & matches() {
        return this->_matches;
    }

    auto operator[](size_t n) const {
        return this->_matches[n];
    }

    auto size() const {
        return this->_matches.size();
    }

    bool empty() const {
        return this->_matches.empty();
    }

    /**
     * Returns true if the regex matched anything
     * @return whether the regex matched anything
     */
    operator bool() const {
        return !this->empty();
    }
};


inline RegexResult regexer(zstring_view string, std::regex const & regex) {
    return RegexResult(string, regex);
}

inline RegexResult regexer(zstring_view string, zstring_view regex_string) {
    return regexer(string, std::regex(regex_string.c_str()));
}

inline std::regex operator"" _re(char const * regex_string, unsigned long) {
    return std::regex(regex_string, std::regex_constants::ECMAScript);
}

inline std::regex operator"" _rei(char const * regex_string, unsigned long) {
    return std::regex(regex_string, std::regex_constants::ECMAScript | std::regex_constants::icase);
}


}