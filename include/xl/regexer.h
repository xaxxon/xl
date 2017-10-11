#pragma once

#include <regex>

#include "zstring_view.h"


namespace xl {

class RegexResult {
    std::smatch _matches;
    std::string _string_copy;

public:
    RegexResult(std::regex const & regex, zstring_view string) :
        _string_copy(string)
    {
        std::regex_match(this->_string_copy, this->_matches, regex);
    }

    RegexResult(zstring_view regex_string, zstring_view string) :
        RegexResult(std::regex(regex_string.c_str()), string)
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

};

RegexResult regex(std::regex const & regex, zstring_view string) {
    return RegexResult(regex, string);
}


RegexResult regex(zstring_view regex_string, zstring_view string) {
    return regex(std::regex(regex_string.c_str()), string);
}


}