#pragma once

#include <exception>
#include <string>
#include <exceptions.h>

namespace xl {
    enum RegexFlags {
        NONE      = 0,
        OPTIMIZE  = 1 << 0,
        ICASE     = 1 << 1,
        EXTENDED  = 1 << 2,
        DOTALL    = 1 << 3,
        MULTILINE = 1 << 4,
        DOLLAR_END_ONLY = 1 << 5
    };
    using RegexFlagsT = std::underlying_type_t<RegexFlags>;


    class RegexException : public xl::FormattedException {
    public:
        using xl::FormattedException::FormattedException;
    };

} // end namespace xl

#if defined XL_USE_PCRE
#include "regex_pcre.h"
#endif

#include "regex_std.h"

#include "../zstring_view.h"




namespace xl {



#if defined XL_USE_PCRE
using Regex = xl::RegexPcre;
#else
using Regex = xl::RegexStd;
#endif


inline auto regexer(zstring_view string, Regex const & regex) {
    return regex.match(string);
}


inline auto regexer(zstring_view string, zstring_view regex_string) {
    Regex regex(regex_string);
    return regexer(string, regex);
}


inline Regex operator"" _re(char const * regex_string, unsigned long length) {
    return Regex(regex_string);
}


inline Regex operator"" _rei(char const * regex_string, unsigned long) {
    return Regex(regex_string, ICASE);
}


}