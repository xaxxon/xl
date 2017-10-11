
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

#include "../library_extensions.h"

#include "provider_data.h"


namespace xl {

class Provider_Interface;


class Template {
private:
    std::string _tmpl;

    // static portions of the template as separated by substitutions
    mutable std::vector<std::string> compiled_static_strings;

    // substitutions in the template
    mutable std::vector<ProviderData> compiled_substitutions;

    // sum of the length of all the fixed portions of the template
    mutable size_t minimum_result_length = 0;

public:
    Template(std::string const & tmpl) : _tmpl(tmpl) {}
    Template(std::string && tmpl = "") : _tmpl(std::move(tmpl)) {}

    char const * c_str() const { return this->_tmpl.c_str(); }

//    std::string fill(Provider_Interface const & interface = EmptyProvider{}, std::map<std::string, Template> const & templates = {});

    template<class T = EmptyProvider>
    std::string fill(T && source = T{}, std::map<std::string, Template> const & templates = {}) const;

    // compiles the template for faster processing
    // benchmarks showed ~200x improvement when re-using templates 1000 times
    void compile() const;

    bool is_compiled() const;
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
    if (!this->is_compiled()) {
        this->compile();
    }

    std::unique_ptr<Provider_Interface> provider_interface_unique_pointer;
    Provider_Interface * provider_interface_pointer;
    if constexpr(std::is_base_of_v<Provider_Interface, std::decay_t<T>>) {
        provider_interface_pointer = & source;
    } else {
        provider_interface_unique_pointer = make_provider(source);
        provider_interface_pointer = provider_interface_unique_pointer.get();
    }
    Provider_Interface & provider = *provider_interface_pointer;
//    std::cerr << fmt::format("providers vector size: {}", this->providers.size()) << std::endl;

    std::string result{};
    result.reserve(this->minimum_result_length);

    for(int i = 0; i < this->compiled_static_strings.size(); i++) {
        result.insert(result.end(), this->compiled_static_strings[i].begin(), this->compiled_static_strings[i].end());

        if (this->compiled_substitutions.size() > i) {
            auto & data = this->compiled_substitutions[i];
            auto substitution_result = provider(ProviderData(data.name, &templates, data.parameters));
            result.insert(result.end(), substitution_result.begin(), substitution_result.end());
        }
    }
    return result;
}


bool Template::is_compiled() const {
    return !this->compiled_static_strings.empty() || !this->compiled_substitutions.empty();
}


void Template::compile() const {


    // must match:
    //   a string with no replacement
    //   a replacement with no other string
    //   a string followed by a replacement
    //   BUT NOT neither. (leading positive lookahead assertion makes sure string isn't empty)
    static std::regex r(
        // forward lookahead that the string has at least one character (not empty)
        //"^(?=(?:.|\\n))"

            // grab everything up until a first lone curly brace
            //"((?:[^{}]|[{]{2}|[}]{2})*)"

            // grab a brace (potentially the wrong one which will be checked for later) and then a submatch of
            //   everything inside the braces excluding leading and trailing whitespace
            //   followed by a closing curly brace or potentially end-of-line in case of a no-closing-brace error
            // negative forward assertion to make sure the closing brace isn't escaped as }}
            //"(?:([{}]?)\\s*((?:[^}{]|[}]{2}|[{]{2})*?)(?:[|]((?:[^{}]|[}]{2}|[{]{2})*?\\s*))?\\s*([}]|$)(?!\\})\\\\?)?",
        "^(?=(?:.|\\n))((?:\\\\[{]|[^{}]|[{](?!\\{)|[}](?!\\}))*)(?:([{]{2})\\s*((?:[^{}|\\s]|\\s*(?!(?:[}][}]))|\\\\[}]|[^}|\\s]|[}](?!\\}))*)\\s*(?:[|]((?:\\\\[}]|[^}]|[}](?!\\}))*))?)?([}]{2})?",
        std::regex::optimize
    );

    // submatch positions for the above regex
    enum RegexMatchIndex {
        WHOLE_STRING_INDEX = 0,
        LITERAL_STRING_INDEX,
        OPEN_BRACE_INDEX,
        REPLACEMENT_NAME_INDEX,
        REPLACEMENT_OPTIONS_INDEX,
        CLOSE_BRACE_INDEX,
    };

    // turn double curlies {{ or }} into { or } and anything with a backslash before it into just the thing
    //   after the backslash
    static std::regex post_process_regex("(?:([{}])\\1|\\\\(.))");



    std::cmatch matches;
    char const * remaining_template = this->c_str();
//    std::cerr << fmt::format("filling template: '{}'", this->_tmpl.c_str()) << std::endl;

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
        if (matches[OPEN_BRACE_INDEX].str() != "{{" || matches[CLOSE_BRACE_INDEX].str() != "}}") {
            throw TemplateException("Found mismatched braces");
        }

        remaining_template = matches.suffix().first;

        std::string literal_string = matches[LITERAL_STRING_INDEX];
//        std::cerr << fmt::format("postprocessing: '{}'", literal_string) << std::endl;

        literal_string = std::regex_replace(literal_string, post_process_regex, "$1$2");
//        std::cerr << fmt::format("into: '{}'", literal_string) << std::endl;

        this->compiled_static_strings.push_back(literal_string);
        this->minimum_result_length += this->compiled_static_strings.back().size();


//            if (!provider.provides(matches[REPLACEMENT_NAME_INDEX])) {
//                throw TemplateException(
//                    fmt::format("Provider doesn't provide value for name: '{}'", matches[REPLACEMENT_NAME_INDEX]));
//            }

//            if (matches[REPLACEMENT_OPTIONS_INDEX].str() != "") {
//                std::cerr << fmt::format("GOT REPLACEMENT OPTIONS: {}", matches[REPLACEMENT_OPTIONS_INDEX].str()) << std::endl;
//            }

        // look up the template to use:
        this->compiled_substitutions.emplace_back(matches[REPLACEMENT_NAME_INDEX], nullptr, matches[REPLACEMENT_OPTIONS_INDEX]);

    }

//    std::cerr << fmt::format("pushing on remaining template: '{}'", std::regex_replace(remaining_template, post_process_regex, "$1")) << std::endl;
    this->compiled_static_strings.push_back(std::regex_replace(remaining_template, post_process_regex, "$1$2"));

}

} // end namespace xl