
#pragma once

#include <string_view>
#include <map>
#include <string>
#include <variant>
#include <sstream>
#include <regex>
#include <type_traits>
#include <functional>

#include <fmt/ostream.h>

#include "library_extensions.h"

#include "provider_data.h"


namespace xl {

class Provider_Interface;


class Template {
private:
    std::string _tmpl;

public:
    Template(std::string const & tmpl = "") : _tmpl(tmpl) {}

    char const * c_str() const { return this->_tmpl.c_str(); }

//    std::string fill(Provider_Interface const & interface = EmptyProvider{}, std::map<std::string, Template> const & templates = {});

    template<class T = EmptyProvider>
    std::string fill(T && source = T{}, std::map<std::string, Template> const & templates = {}) const;
};

using TemplateMap = std::map<std::string, Template>;

} // end namespace xl


#include "provider.h"
namespace xl {

//std::string Template::fill(Provider_Interface const & interface, std::map<std::string, Template> const & templates) {
//    return this->fill<>(interface, templates);
//}

template<class T>
std::string Template::fill(T && source, TemplateMap const & templates) const {
    std::unique_ptr<Provider_Interface> provider_interface_unique_pointer;
    Provider_Interface * provider_interface_pointer;
    if constexpr(std::is_base_of_v<Provider_Interface, std::decay_t<T>>) {
        provider_interface_pointer = & source;
    } else {
        provider_interface_unique_pointer = MakeProvider<std::decay_t<T>>()(source);
        provider_interface_pointer = provider_interface_unique_pointer.get();
    }
    Provider_Interface & provider = *provider_interface_pointer;
    std::stringstream result;
//    std::cerr << fmt::format("providers vector size: {}", this->providers.size()) << std::endl;

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
                "(?:([{}]?)\\s*((?:[^}{]|[}]{2}|[{]{2})*?)(?:[|]((?:[^{}]|[}]{2}|[{]{2})*?\\s*))?\\s*([}]|$)(?!\\}))?",

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
        char const * remaining_template = this->c_str();
//        std::cerr << fmt::format("filling template: '{}'", this->_tmpl.c_str()) << std::endl;

        while (std::regex_search(remaining_template, matches, r)) {

//            std::cerr << fmt::format("matching against: '{}'", remaining_template) << std::endl;
//            for (int i = 0; i < matches.size(); i++) {
//                std::cerr << fmt::format("match[{}]: '{}'", i, matches[i].str()) << std::endl;
//            }

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
            result << matches[LITERAL_STRING_INDEX];


//            if (!provider.provides(matches[REPLACEMENT_NAME_INDEX])) {
//                throw TemplateException(
//                    fmt::format("Provider doesn't provide value for name: '{}'", matches[REPLACEMENT_NAME_INDEX]));
//            }

//            if (matches[REPLACEMENT_OPTIONS_INDEX].str() != "") {
//                std::cerr << fmt::format("GOT REPLACEMENT OPTIONS: {}", matches[REPLACEMENT_OPTIONS_INDEX].str()) << std::endl;
//            }

            // look up the template to use:
            ProviderData new_data(matches[REPLACEMENT_NAME_INDEX], templates, matches[REPLACEMENT_OPTIONS_INDEX]);


            std::string provider_data = provider(new_data);
//            std::cerr << fmt::format("replacement string for '{}' is '{}'", matches[REPLACEMENT_NAME_INDEX], provider_data) << std::endl;
            result << provider_data;

        }
//        std::cerr << fmt::format("putting on remaining template after loop exits: {}", remaining_template) << std::endl;
        result << remaining_template;

//        std::cerr << fmt::format("fill result: '{}'", result.str()) << std::endl;


    return result.str();
}
} // end namespace xl