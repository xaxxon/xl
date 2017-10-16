#pragma once


#include <regex>
#include <fmt/ostream.h>
#include "zstring_view.h"


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

class RegexStd {
    std::regex regex;

    auto make_std_regex_flags(xl::RegexFlags flags) {
        if (flags | EXTENDED) {
            throw RegexException("std::regex doesn't support extended regexes");
        } else if (flags | DOTALL) {
            throw RegexException("std::regex doesn't support DOTALL");
        } else if (flags | MULTILINE) {
            throw RegexException("std::regex for my compiler hasn't yet implemented std::regex_constants::multiline");
        }

        std::underlying_type_t<std::regex_constants::syntax_option_type> result = std::regex_constants::ECMAScript;
        result |= flags | OPTIMIZE ? std::regex_constants::optimize : 0;

        // not supported in clang 5
//        result |= flags | MULTILINE ? std::regex_constants::multiline : 0;
        return result;
    }

public:
    RegexStd(xl::zstring_view regex_string, xl::RegexFlags flags = NONE) try :
        regex(regex_string.c_str(), make_std_regex_flags(flags))
    {
        std::cerr << fmt::format("tried to make regex from {}", regex_string) << std::endl;
        regex = std::regex(regex_string.c_str());
    } catch (std::regex_error const & e) {
        std::cerr << fmt::format("about to rethrow") << std::endl;
        throw xl::RegexException(e.what());
    }

    RegexStd(std::regex regex) :
        regex(std::move(regex)) {}

    RegexResultStd match(std::string_view source) const {

        return RegexResultStd(source, this->regex);

    }
};

} // end namespace xl