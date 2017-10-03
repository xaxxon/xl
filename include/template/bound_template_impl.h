
#pragma once

#include "bound_template.h"

namespace xl {

template<class T>
BoundTemplate::BoundTemplate(Template tmpl, T && source) :
    tmpl(tmpl)
{
    this->providers.push_back(Provider(MakeProvider<T>()(std::forward<T>(source))));
}


BoundTemplate::~BoundTemplate()
{}


/**
 * Fills out `this` template and returns the result
 *
 * @tparam T Source type - must either be a Provider or something convertible to a Provider
 * @param source object to be used to populate `this` template
 * @return populated template result
 */
std::string BoundTemplate::operator()() {
    std::stringstream result;
    std::cerr << fmt::format("providers vector size: {}", this->providers.size()) << std::endl;
    for (auto & provider : this->providers) {

        // must match:
        //   a string with no replacement
        //   a replacement with no other string
        //   a string followed by a replacement
        //   BUT NOT neither. (leading positive lookahead assertion makes sure string isn't empty)
        static std::regex r(
            // forward lookahead that the string has at least one character (not empty)
            "^(?=(?:.|\\n))"

                // grab everything up until a first lone curly brace
                "((?:[^{}]|[{]{2}|[}]{2})*)"

                // grab a brace (potentially the wrong one which will be checked for later) and then a submatch of
                //   everything inside the braces excluding leading and trailing whitespace
                //   followed by a closing curly brace or potentially end-of-line in case of a no-closing-brace error
                // negative forward assertion to make sure the closing brace isn't escaped as }}
                "(?:([{}]?)\\s*((?:[^}{]|[}]{2}|[{]{2})*?)(?::((?:[^{}]|[}]{2}|[{]{2})*))?\\s*([}]|$)(?!\\}))?",

            std::regex::optimize
        );

        // submatch positions for the above regex
        enum RegexMatchIndex {
            WHOLE_STRiNG_INDEX = 0,
            LITERAL_STRING_INDEX,
            OPEN_BRACE_INDEX,
            REPLACEMENT_NAME_INDEX,
            REPLACEMENT_OPTIONS_INDEX,
            CLOSE_BRACE_INDEX,
        };


        std::cmatch matches;
        char const * remaining_template = this->tmpl.c_str();
        std::cerr << fmt::format("filling template: '{}'", this->tmpl.c_str()) << std::endl;

        while (std::regex_search(remaining_template, matches, r)) {

            std::cerr << fmt::format("matching against: '{}'", remaining_template) << std::endl;
            for (int i = 0; i < matches.size(); i++) {
                std::cerr << fmt::format("match[{}]: {}", i, matches[i].str()) << std::endl;
            }

            // if no substitution found, everything was a literal and is handled as a "trailing literal" outside
            //   this loop
            if (matches[OPEN_BRACE_INDEX].str() == "" && matches[CLOSE_BRACE_INDEX].str() == "") {
                break;
            }

            // check for open but no close or incorrect brace type
            if (matches[OPEN_BRACE_INDEX].str() != "{" || matches[CLOSE_BRACE_INDEX].str() != "}") {
                throw TemplateException("Found mismatched braces");
            }

            remaining_template = matches.suffix().first;
            result << matches[1];


            if (!provider.provides(matches[REPLACEMENT_NAME_INDEX])) {
                throw TemplateException(
                    fmt::format("Provider doesn't provide value for name: '{}'", matches[REPLACEMENT_NAME_INDEX]));
            }

            if (matches[REPLACEMENT_OPTIONS_INDEX].str() != "") {
                std::cerr << fmt::format("GOT REPLACEMENT OPTIONS: {}", matches[REPLACEMENT_OPTIONS_INDEX].str())
                          << std::endl;

            }
            std::string provider_data = provider(matches[REPLACEMENT_NAME_INDEX].str(),
                                                 ProviderOptions(matches[REPLACEMENT_OPTIONS_INDEX].str()));
            result << provider_data;

        }
        std::cerr << fmt::format("putting on remaining template after loop exits: {}", remaining_template) << std::endl;
        result << remaining_template;

        std::cerr << fmt::format("fill result: '{}'", result.str()) << std::endl;
    }

    return result.str();
}




} // end namespace xl


