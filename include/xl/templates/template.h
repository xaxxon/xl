
#pragma once

#include <string_view>
#include <map>
#include <string>
#include <variant>
#include <sstream>
#include <type_traits>
#include <functional>

#include <fmt/ostream.h>

#include "../regex/regexer.h"
#include "../library_extensions.h"


namespace xl::templates {

class ProviderData;
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

    inline char const * c_str() const { return this->_tmpl.c_str(); }

//    std::string fill(Provider_Interface const & interface = EmptyProvider{}, std::map<std::string, Template> const & templates = {});

    template<class ProviderContainer = void, class T = std::string>
    std::string fill(T && source = T{}, std::map<std::string, Template> const & templates = {}) const;

    // compiles the template for faster processing
    // benchmarks showed ~200x improvement when re-using templates 1000 times
    inline void compile() const;

    inline bool is_compiled() const;
};

using TemplateMap = std::map<std::string, Template>;

} // end namespace xl

#include "provider_data.h"
#include "provider.h"
#include "directory_loader.h"

namespace xl::templates {


template<typename ProviderContainer, class T>
std::string Template::fill(T && source, TemplateMap const & templates) const {
    if (!this->is_compiled()) {
        this->compile();
    }

    if constexpr(is_passthrough_provider_v<T>) {
        std::cerr << fmt::format("fill got passthrough provider {}, recursively calling fill with underlying provider", source.get_name()) << std::endl;
        return fill(source.get_underlying_provider(), templates);
    }



    // used for storing the provider if necessary
    std::unique_ptr<Provider_Interface> provider_interface_unique_pointer;

    // used for consistent interface for assigning to reference later
    Provider_Interface * provider_interface_pointer;
    if constexpr(std::is_base_of_v<Provider_Interface, std::decay_t<T>>) {
        std::cerr << fmt::format("**** already had provider interface") << std::endl;
        provider_interface_pointer = & source;
    } else {

        // need to store the unique_ptr to maintain object lifetime then assign to normal pointer
        //   so there is a common way to get the object below for assignment to reference type
        provider_interface_unique_pointer = DefaultProviders<ProviderContainer>::template make_provider(source);
//        std::cerr << fmt::format("**** got unique ptr at {}", (void*)provider_interface_unique_pointer.get()) << std::endl;
        provider_interface_pointer = provider_interface_unique_pointer.get();
//        std::cerr << fmt::format("**** set provider interface pointer to {}", (void*)provider_interface_pointer) << std::endl;
    }

    std::cerr << fmt::format("outside: provider interface pointer to {}", (void*)provider_interface_pointer) << std::endl;



    // whichever way the object was provided, get a reference to the object for convenience here
    Provider_Interface & provider = *provider_interface_pointer;

    std::string result{""};
    std::cerr << fmt::format("just created variable 'result': '{}'", result) << std::endl;
//    result.reserve(this->minimum_result_length);


    for(int i = 0; i < this->compiled_static_strings.size(); i++) {

        result.insert(result.end(), this->compiled_static_strings[i].begin(), this->compiled_static_strings[i].end());
        std::cerr << fmt::format("fill: just added static section {}: '{}'", i, result) << std::endl;

        if (this->compiled_substitutions.size() > i) {
            ProviderData data(this->compiled_substitutions[i]);
            std::cerr << fmt::format("grabbed data for compiled_subsitution {} - it has name {}", i, data.name) << std::endl;
            data.templates = &templates;
            data.current_template = this;
            std::cerr << fmt::format("substitution instantiation data.name: '{}'", data.name) << std::endl;

            // substituting another template in
            if (data.template_name != "") {
                if (templates.empty()) {
                    throw TemplateException("Cannot refer to another template if no other templates specified: " + data.template_name);
                }
                auto template_iterator = templates.find(data.template_name);
                if (template_iterator == templates.end()) {
                    throw TemplateException("No template found named: " + data.template_name);
                }
                auto inline_template_result = template_iterator->second.fill(provider, templates);
                if (!inline_template_result.empty()) {
                    result.insert(result.end(), data.contingent_leading_content.begin(), data.contingent_leading_content.end());
                }
                result.insert(result.end(), begin(inline_template_result), end(inline_template_result));

            }
            // filling a template
            else {

                std::cerr << fmt::format("created ProviderData data on the stack at {}", (void*) &data) << std::endl;

                std::cerr << fmt::format("about to call provider() at {}", (void*)&provider) << std::endl;
                auto substitution_result = provider(data);
                std::cerr << fmt::format("got substitution result: '{}'", substitution_result) << std::endl;
                if (!data.contingent_leading_content.empty()) {
                    if (!substitution_result.empty()) {
//                        std::cerr << fmt::format("adding contingent data: {}", data.contingent_leading_content)
//                                  << std::endl;
                        result.insert(result.end(), data.contingent_leading_content.begin(),
                                      data.contingent_leading_content.end());
                    } else {
//                        std::cerr << fmt::format("skipping contingent data: {}", data.contingent_leading_content)
//                                  << std::endl;
                    }
                }
                result.insert(result.end(), substitution_result.begin(), substitution_result.end());
            }
        }
    }
    return result;
}


bool Template::is_compiled() const {
    return !this->compiled_static_strings.empty() || !this->compiled_substitutions.empty();
}


void Template::compile() const {

    // regex used to parse sections of a template into 0 or more pairs of leading string literal (may be empty)
    //   and a following substitution (optional)
    // Development and testing of this regex can be done at regex101.com
    static xl::RegexPcre r(R"(

(?(DEFINE)(?<NotEmptyAssertion>(?=(?:.|\n))))
(?(DEFINE)(?<OpenDelimiterHead>\{))
(?(DEFINE)(?<CloseDelimiterHead>\}))
(?(DEFINE)(?<OpenDelimiterTail>\{))
(?(DEFINE)(?<CloseDelimiterTail>\}))

(?(DEFINE)(?<OpenDelimiter>(?&OpenDelimiterHead)(?&OpenDelimiterTail)))
(?(DEFINE)(?<CloseDelimiter>(?&CloseDelimiterHead)(?&CloseDelimiterTail)))
(?(DEFINE)(?<EitherDelimiter>( (?&OpenDelimiter) | (?&CloseDelimiter))))
(?(DEFINE)(?<UntilDoubleBrace>(?:\s|\\(?&OpenDelimiterHead)|\\(?&CloseDelimiterHead)|[^{}]|[{](?!\{)|[}](?!\}))*))
(?(DEFINE)(?<UntilEndOfLine>[^\n]*\n))



(?&NotEmptyAssertion)
(?<Literal>(?&UntilDoubleBrace))

# Start Substitution
(?:(?<Substitution>
    (?<OpenDelimiterHere>\{\{)
    \s*
    (?<IgnoreEmptyMarker><)?
    \s*

    # If there's a leading !, then another template is inserted here instead of a value from a provider
    (?<TemplateInsertionMarker>!)?

    # Replacement name
    (?:(?<SubstitutionName>(?:\\\}|\\\{|[^|%])*?)\s*(?=(?&OpenDelimiter)|(?&CloseDelimiter)|\||\%|$))

    # Join string, starting with %, if specified
    (?:(?<JoinStringMarker>%)(?<JoinString>(?:\\\||[^|])*))?

    # Everything after the | before the }}
    (?:[|]
        (?<InlineTemplateMarker>!)?
        (?<IgnoreWhitespaceTilEndOfLine>!(?&UntilEndOfLine))?
        (?<SubstitutionData>((?&UntilDoubleBrace)(?&Substitution)?)*)


    )? # end PIPE

    (?<CloseDelimiterHere>\}\})
) # end Substition
| (?<UnmatchedOpen>\{\{) | (?<UnmatchedClose>\}\}) | $)

)",
        xl::OPTIMIZE | xl::EXTENDED | xl::DOTALL);


    // find all escaped characters - anything following a backslash
    static xl::RegexPcre post_process_regex("(?:\\\\(.))", xl::OPTIMIZE);


    // the portion of the template string which hasn't yet been parsed by the main regex
    std::string remaining_template = this->c_str();
//    std::cerr << fmt::format("compiling template: '{}'", this->_tmpl.c_str()) << std::endl;

    while (auto matches = r.match(remaining_template)) {

//        for(auto [s,i] : each_i(matches.get_all_matches())) {
//            if (s != "") {
//                std::cerr << fmt::format("{}: '{}'", i, s) << std::endl;
//            }
//        }
//        std::cerr << fmt::format("literal: '{}', substutition: '{}'",
//                                 matches["Literal"],
//                                 matches["Substitution"]
//        ) << std::endl;

//        for (int i = 0; i < matches.size(); i++) {
//            std::cerr << fmt::format("match[{}]: '{}'", i, matches[i].str()) << std::endl;
//        }


        // check for open but no close or incorrect brace type
        if (matches.length("UnmatchedOpen")) {
            throw TemplateException("Unmatched Open");
        }
        if (matches.length("UnmatchedClose")) {
            throw TemplateException("Unmatched Close");
        }


        // if no substitution found, everything was a literal and is handled as a "trailing literal" outside
        //   this loop
        if (matches.length("Substitution") == 0) {
            break;
        }


        remaining_template = matches.suffix();

        std::string literal_string = matches["Literal"];
//        std::cerr << fmt::format("postprocessing: '{}'", literal_string) << std::endl;

        literal_string = post_process_regex.replace(literal_string, "$1");
//        std::cerr << fmt::format("into: '{}'", literal_string) << std::endl;


        bool ignore_empty_replacements = matches.has("IgnoreEmptyMarker");
//        std::cerr << fmt::format("ignoring empty replacements? {}", ignore_empty_replacements) << std::endl;
        std::string contingent_leading_content;
        if (ignore_empty_replacements) {
            // trim off everything after the last newline on the static and put it in the template
            static Regex last_line_regex(R"(^(.*?)(\n?[^\n]*)$)", xl::DOTALL | xl::DOLLAR_END_ONLY);
            auto results = last_line_regex.match(literal_string);
            if (results) {
                literal_string = results[1];
                contingent_leading_content = results[2];
            }
//            std::cerr << fmt::format("start: '{}', last line: '{}'", beginning_of_literal, contingent_leading_content) << std::endl;
        }

        this->compiled_static_strings.push_back(literal_string);
        this->minimum_result_length += this->compiled_static_strings.back().size();


        ProviderData data;

        if (!matches.has("TemplateInsertionMarker")) {
            data.name = matches["SubstitutionName"];
        }

        if (matches.length("JoinStringMarker")) {
            data.join_string = matches["JoinString"];
        }

        if (!matches.has("InlineTemplateMarker")) {
            data.parameters = matches["SubstitutionData"];
        }

        data.contingent_leading_content = contingent_leading_content;

        data.ignore_empty_replacements = ignore_empty_replacements;

        if (matches.has("InlineTemplateMarker")) {
            std::cerr << fmt::format("Template::compile - creating inline template from '{}'", matches["SubstitutionData"]) << std::endl;
            data.inline_template = Template(matches["SubstitutionData"]);
        }

        if (matches.has("TemplateInsertionMarker")) {
            data.template_name = matches["SubstitutionName"];
        }

        this->compiled_substitutions.emplace_back(std::move(data));

//
//        if (matches.length("TemplateInsertionMarker") == 1) {
//            this->compiled_substitutions.emplace_back(""                         , ""                         , std::optional<Template>()            , matches["SubstitutionName"], contingent_leading_content, join_string).set_ignore_empty_replacements(ignore_empty_replacements);
//        }else if (matches.length("InlineTemplateMarker") == 1) {
//            this->compiled_substitutions.emplace_back(matches["SubstitutionName"], ""                         , Template(matches["SubstitutionData"]), ""                         , contingent_leading_content, join_string).set_ignore_empty_replacements(ignore_empty_replacements);
//        } else {
//            this->compiled_substitutions.emplace_back(matches["SubstitutionName"], matches["SubstitutionData"], std::optional<Template>()            , ""                         , contingent_leading_content, join_string).set_ignore_empty_replacements(ignore_empty_replacements);
//        }

    }

//std::cerr << fmt::format("remaining template: '{}'", remaining_template) << std::endl;
//    std::cerr << fmt::format("pushing on remaining template: '{}'", std::regex_replace(remaining_template, post_process_regex, "$1")) << std::endl;
    this->compiled_static_strings.push_back(post_process_regex.replace(remaining_template, "$1"));
}

} // end namespace xl
