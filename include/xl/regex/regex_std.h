#pragma once

#ifdef XL_USE_LIB_FMT
#include <fmt/format.h>
#endif

#include <iostream>

#include <regex>
#include "regexer.h"
#include "../zstring_view.h"


namespace xl {

class RegexStd;

class RegexResultStd {
    std::cmatch _matches;
    std::unique_ptr<char[]> _string_copy;
    xl::RegexStd const * regex;
public:
    RegexResultStd(std::string_view string, xl::RegexStd const * regex);

    RegexResultStd(RegexResultStd &&) = default;


    xl::string_view prefix() const {
        auto prefix_length = (this->_matches.prefix().second - this->_matches.prefix().first);
        return xl::string_view(&*this->_matches.prefix().first, prefix_length);
    }


    // everything after the match
    char const * suffix() const {
        return &*this->_matches.suffix().first;
    }


    xl::string_view operator[](size_t n) const {
        return xl::string_view(&*this->_matches[n].first, this->_matches.length(n));
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

    RegexResultStd next() const;

};

class RegexStd : public RegexBase<RegexStd, RegexResultStd> {
    friend class RegexResultStd;
    std::regex regex;
    std::string regex_source = "";

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

    using ResultT = RegexResultStd;

    RegexStd(xl::zstring_view regex_string, xl::RegexFlags flags = NONE) try :
//        regex(regex_string.c_str(), make_std_regex_flags(flags)),
        regex_source(regex_string)
    {
//        std::cerr << fmt::format("flags are {} => std::regex flag {}", (int)flags, make_std_regex_flags(flags)) << std::endl;
//        std::cout << fmt::format("Creating regex with '{}'", regex_string.c_str()) << std::endl;
        this->regex = std::regex(regex_string.c_str(), make_std_regex_flags(flags));
    } catch (std::regex_error const & e) {
//        std::cerr << fmt::format("caught error creating std::regex for '{}'", regex_source.c_str()) << std::endl;
        throw xl::RegexException(e.what());
    }

    RegexStd(std::regex regex) :
        regex(std::move(regex)) {}

    RegexResultStd match(std::string_view input_text) const {
//        std::cout << fmt::format("about to match with {}", input_text) << std::endl;
        return RegexResultStd(input_text, this);
    }


    std::string replace(xl::zstring_view replace_source, xl::zstring_view result) {
        return std::regex_replace(replace_source.c_str(), this->regex, result.c_str());
    }

};


inline RegexResultStd::RegexResultStd(std::string_view string, xl::RegexStd const * regex) :
_string_copy(std::make_unique<char[]>(string.length() + 1)),
regex(regex)
{
    memcpy((void*)_string_copy.get(), (void const *)string.data(), string.length());
//    _string_copy[string.length()] = 0; // should not be necessary
    std::regex_search(this->_string_copy.get(), this->_matches, this->regex->regex);
}


inline RegexResultStd RegexResultStd::next() const
{
    return this->regex->match(this->suffix());
}


} // end namespace xl