#pragma once

#include <exception>
#include <string>

#include "../exceptions.h"
#include "../zstring_view.h"


namespace xl {
    enum RegexFlags {
        NONE      = 0,
        OPTIMIZE  = 1 << 0,
        ICASE     = 1 << 1,
        EXTENDED  = 1 << 2,
        DOTALL    = 1 << 3,
        MULTILINE = 1 << 4,
        DOLLAR_END_ONLY = 1 << 5,
        ALLOW_DUPLICATE_SUBPATTERN_NAMES = 1 << 6
    };
    using RegexFlagsT = std::underlying_type_t<RegexFlags>;


    class RegexException : public xl::FormattedException {
    public:
        using xl::FormattedException::FormattedException;
    };






template<typename RegexT, typename RegexResultT>
struct RegexBase {
    using ResultT = RegexResultT;


    std::vector<ResultT> all(xl::zstring_view source) {
        std::vector<ResultT> results;

        if (auto matches = static_cast<RegexT *>(this)->match(source)) {
            results.push_back(std::move(matches));

            while (true) {
                auto next_match = results.back().next();
                if (next_match) {
                    results.push_back(std::move(next_match));
                } else {
                    break;
                }
            }
        }
        return results;
    }

};

}

#include "regex_std.h"
#if defined XL_USE_PCRE
#include "regex_pcre.h"
#endif





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


inline Regex operator"" _re(char const * regex_string, unsigned long) {
    return Regex(regex_string);
}


inline Regex operator"" _rei(char const * regex_string, unsigned long) {
    return Regex(regex_string, ICASE);
}


} // end namespace xl::regex