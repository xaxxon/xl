#pragma once


#include <regex>

#include "zstring_view.h"

namespace xl {

class RegexStd {
    std::regex regex;

    RegexStd(xl::zstring_view regex_string, xl::RegexFlags flags) :
        regex(regex_string.c_str(), flags | OPTIMIZE ? std::regex_constants::optimize : 0) {}

    RegexStd(std::regex regex) :
        regex(std::move(regex)) {}
};

} // end namespcae xl