
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
#include "provider_data.h"


#if defined XL_TEMPLATE_LOG_ENABLE
#define XL_TEMPLATE_LOG(format_string, ...) \
    xl::templates::log.info(format_string, ##__VA_ARGS__);
#else
#define XL_TEMPLATE_LOG(format_string, ...)
#endif



namespace xl::templates {



struct ProviderData;
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
    explicit Template(std::string const & tmpl) : _tmpl(tmpl) {}
    explicit Template(std::string && tmpl = "") : _tmpl(std::move(tmpl)) {}

    inline char const * c_str() const { return this->_tmpl.c_str(); }


    template<class ProviderContainer = void, class T = std::string>
    std::string fill(T && source = T{}, ProviderData && = {}) const;

    // compiles the template for faster processing
    // benchmarks showed ~200x improvement when re-using templates 1000 times
    inline void compile() const;

    inline bool is_compiled() const;
};

using TemplateMap = std::map<std::string, Template>;

} // end namespace xl

// need to call directly to this .h file because circular dependencies make things tricky
#include "../log/log.h"

#include "provider.h"
#include "directory_loader.h"

namespace xl::templates {

class provider_data;

template<typename ProviderContainer, class T>
std::string Template::fill(T && source, ProviderData && input_data) const {

    XL_TEMPLATE_LOG("Filling template: '{}'", this->c_str());

    if (!this->is_compiled()) {
        this->compile();
    }

    // used for storing the provider if necessary
    std::unique_ptr<Provider_Interface> provider_interface_unique_pointer;

    // used for consistent interface for assigning to reference later
    Provider_Interface * provider_interface_pointer;
    if constexpr(std::is_base_of_v<Provider_Interface, std::decay_t<T>>) {
        provider_interface_pointer = & source;

    } else {

        // need to store the unique_ptr to maintain object lifetime then assign to normal pointer
        //   so there is a common way to get the object below for assignment to reference type
        provider_interface_unique_pointer = DefaultProviders<ProviderContainer>::template make_provider(source);
        XL_TEMPLATE_LOG("**** got unique ptr at {}", (void*)provider_interface_unique_pointer.get());
        provider_interface_pointer = provider_interface_unique_pointer.get();
        XL_TEMPLATE_LOG("**** set provider interface pointer to {}", (void*)provider_interface_pointer);
    }

    XL_TEMPLATE_LOG("outside: provider interface pointer to {}", (void*)provider_interface_pointer);

    if (!provider_interface_pointer->is_fillable_provider()) {
        auto fillable_provider = provider_interface_pointer->get_fillable_provider();
        provider_interface_unique_pointer = std::move(fillable_provider);
        provider_interface_pointer = provider_interface_unique_pointer.get();
    }


    // whichever way the object was provided, get a reference to the object for convenience here
    Provider_Interface & provider = *provider_interface_pointer;

    if (provider.is_template_passthrough()) {
        return provider(input_data);
    }


    std::string result{""};
    XL_TEMPLATE_LOG("just created variable 'result': '{}'", result);
    result.reserve(this->minimum_result_length);

    for(int i = 0; i < this->compiled_static_strings.size(); i++) {

        result.insert(result.end(), this->compiled_static_strings[i].begin(), this->compiled_static_strings[i].end());
        XL_TEMPLATE_LOG("fill: just added static section {}: '{}'", i, this->compiled_static_strings[i]);

        if (this->compiled_substitutions.size() > i) {
            ProviderData data(this->compiled_substitutions[i]);
            XL_TEMPLATE_LOG("grabbed data for compiled_subsitution {} - it has name {} and inline template: {}",
                            i, data.name, (void*)data.inline_template.get());
            data.templates = input_data.templates;
            data.current_template = this;
            XL_TEMPLATE_LOG("substitution instantiation data.name: '{}'", data.name);

            // substituting another template in
            if (data.template_name != "") {
                if (input_data.templates->empty()) {
                    throw TemplateException("Cannot refer to another template if no other templates specified: " + data.template_name);
                }
                auto template_iterator = input_data.templates->find(data.template_name);
                if (template_iterator == input_data.templates->end()) {
                    throw TemplateException("No template found named: {}", data.template_name);
                }
                auto inline_template_result = template_iterator->second.fill<ProviderContainer>(provider, input_data.templates);
                if (!inline_template_result.empty()) {
                    result.insert(result.end(), data.contingent_leading_content.begin(), data.contingent_leading_content.end());
                }
                result.insert(result.end(), begin(inline_template_result), end(inline_template_result));
                if (!inline_template_result.empty()) {
                    result.insert(result.end(), data.contingent_trailing_content.begin(), data.contingent_trailing_content.end());
                }
            }
            // filling a template
            else {

                if (data.comment) {
                    XL_TEMPLATE_LOG("Handled comment");
                } else {

                    XL_TEMPLATE_LOG("created ProviderData data on the stack at {}", (void *) &data);

                    XL_TEMPLATE_LOG("about to call provider() named '{}' at {} and inline_template: {}",
                                    provider.get_name(), (void *) &provider, (void *) data.inline_template.get());
                    auto substitution_result = provider(data);
                    XL_TEMPLATE_LOG("provider() named {} returned: '{}'", provider.get_name(), substitution_result);
                    if (!data.contingent_leading_content.empty() && !substitution_result.empty()) {
                        XL_TEMPLATE_LOG("adding contingent data: {}", data.contingent_leading_content);

                        result.insert(result.end(), data.contingent_leading_content.begin(),
                                      data.contingent_leading_content.end());
                    }
                    result.insert(result.end(), substitution_result.begin(), substitution_result.end());
                    if (!data.contingent_trailing_content.empty() && !substitution_result.empty()) {
                        XL_TEMPLATE_LOG("inserting trailing content: {}", data.contingent_trailing_content);
                        result.insert(result.end(), data.contingent_trailing_content.begin(),
                                      data.contingent_trailing_content.end());
                    }
                }
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
(?(DEFINE)(?<UntilDoubleBrace>(?:\s|\\(?&OpenDelimiterHead)|\\(?&CloseDelimiterHead)|[^{}>]|[{](?!\{)|[}](?!\})|>(?!(\}\}|>\}\})))*))
(?(DEFINE)(?<UntilEndOfLine>[^\n]*\n))



(?&NotEmptyAssertion)
(?<Literal>(?&UntilDoubleBrace))

# Start Substitution
(?:(?<Substitution>
    (?<OpenDelimiterHere>\{\{)
    \s*
    (?:
        (?<Comment>\#.*?)|
        (?<IgnoreEmptyBeforeMarker><<?)?
        \s*

        # If there's a leading !, then another template is inserted here instead of a value from a provider
        (?<TemplateInsertionMarker>!)?

        # Replacement name
        (?:(?<SubstitutionName>(?:\\\}|\\\{|[^|%>](?!{{)|>(?!}}))*?)\s*(?=(?&OpenDelimiter)|(?&CloseDelimiter)|\||\%|>|$))

        # Join string, starting with %, if specified
        (?:(?<JoinStringMarker>%)(?<LeadingJoinStringMarker>%?)(?<JoinString>(?:\\\||>(?!}})|[^|>])*))?

        # Everything between | and }}
        (?:[|]
            (?<InlineTemplateMarker>!)?
            (?<IgnoreWhitespaceTilEndOfLine>!(?&UntilEndOfLine))?
            (?<SubstitutionData>((?&UntilDoubleBrace)(?&Substitution)?)*)


        )? # end PIPE
        (?<IgnoreEmptyAfterMarker>>{0,2})
    )

    (?<CloseDelimiterHere>\}\})
) # end Substitution
| (?<UnmatchedOpen>\{\{) | (?<UnmatchedClose>\}\}) | $)


)",
        xl::OPTIMIZE | xl::EXTENDED | xl::DOTALL);


    // find all escaped characters - anything following a backslash
    static xl::RegexPcre post_process_regex("(?:\\\\(.))", xl::OPTIMIZE);


    // the portion of the template string which hasn't yet been parsed by the main regex
    std::string remaining_template = this->c_str();
    log.info(TemplateSubjects::Subjects::Compile, std::string("compiling template: '") + this->_tmpl.c_str() + "'");

    // 0 - no contingent data
    // 1 - same line contingent data
    // 2 - same line and all subsequent empty lines
    uint8_t first_line_belongs_to_last_substitution = 0;

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



        remaining_template = matches.suffix();

        std::string literal_string = std::string(matches["Literal"]);
        log.info(TemplateSubjects::Subjects::Compile, "postprocessing: '" + literal_string + "'");

        literal_string = post_process_regex.replace(literal_string, "$1");
        log.info(TemplateSubjects::Subjects::Compile, std::string("into: ") + literal_string);


        bool ignore_empty_replacements_before = matches.has("IgnoreEmptyBeforeMarker");
        log.info(TemplateSubjects::Subjects::Compile, std::string("ignoring empty replacements? ") + (ignore_empty_replacements_before ? "true" : "false"));
        std::string contingent_leading_content = "";


        // if the current literal string has contingent data for the previous substitution, grab it off now
        if (first_line_belongs_to_last_substitution) {
//            std::cerr << fmt::format("trailing contingent ({}) on: '{}'", first_line_belongs_to_last_substitution, literal_string) << std::endl;
            static Regex first_line_regex(R"(^([^\n]*)(.*)$)", xl::DOTALL | xl::DOLLAR_END_ONLY);
            static Regex first_line_and_empty_lines_regex(R"(^([^\n]*\n*)(.*)$)", xl::DOTALL | xl::DOLLAR_END_ONLY);

            auto & regex = first_line_belongs_to_last_substitution == 1 ?
                           first_line_regex : first_line_and_empty_lines_regex;
            if (auto results = regex.match(literal_string)) {

//                std::cerr << fmt::format("got '{}' and '{}'", results[1], results[2]) << std::endl;

                // get previous substitution
                this->compiled_substitutions.back().contingent_trailing_content = results[1];

                literal_string = results[2];

            } else {
                assert(false);
            }
        }

        if (matches.has("IgnoreEmptyBeforeMarker")) {
            // trim off everything after the last newline on the static and put it in the template
            static Regex last_line_regex(R"(^(.*?)(\n?[^\n]*)$)", xl::DOTALL | xl::DOLLAR_END_ONLY);
            static Regex last_line_and_blank_lines_regex(R"(^(.+?\n?)?(\n*[^\n]*)$)", xl::DOTALL | xl::DOLLAR_END_ONLY);
            auto & regex = matches.length("IgnoreEmptyBeforeMarker") == 1 ?
                           last_line_regex : last_line_and_blank_lines_regex;

//            std::cerr << fmt::format("Running ignore empty before marker on '{}'", literal_string) << std::endl;
            auto results = regex.match(literal_string);
            if (results) {
//                std::cerr << fmt::format("got '{}' and '{}'", results[1], results[2]) << std::endl;
                literal_string = results[1];
                contingent_leading_content = results[2];
            }
        }

        first_line_belongs_to_last_substitution = matches.length("IgnoreEmptyAfterMarker");
//        std::cerr << fmt::format("ignore empty after marker: {}", matches["IgnoreEmptyAfterMarker"]) << std::endl;
//        std::cerr << fmt::format("setting first line belongs to last substitution to {} on {}", first_line_belongs_to_last_substitution, matches["Substitution"]) << std::endl;


        this->compiled_static_strings.push_back(literal_string);



        this->minimum_result_length += this->compiled_static_strings.back().size();



        // if no substitution found, everything was a literal and is handled as a "trailing literal" outside
        //   this loop
        if (!matches.has("Substitution")) {
            break;
        }


        ProviderData data;

        // if the substition is a comment, nothing else matters
        if (matches.has("Comment")) {
            data.comment = true;
        }

        if (!matches.has("TemplateInsertionMarker")) {
            data.name = matches["SubstitutionName"];
        }

        if (matches.has("JoinStringMarker")) {
            data.join_string = matches["JoinString"];
        }

        if (matches.has("LeadingJoinStringMarker")) {
            data.leading_join_string = true;
        }


        if (!matches.has("InlineTemplateMarker")) {
            data.parameters = matches["SubstitutionData"];
        }

        data.contingent_leading_content = contingent_leading_content;

        data.ignore_empty_replacements = ignore_empty_replacements_before;

        if (matches.has("InlineTemplateMarker")) {
            log.info(TemplateSubjects::Subjects::Compile, std::string("Template::compile - creating inline template from '") + std::string(matches["SubstitutionData"]) + "'");
            auto inline_template_text = matches["SubstitutionData"];
            log.info(TemplateSubjects::Subjects::Compile, "inline template text: " + std::string(inline_template_text));
            data.inline_template = std::make_shared<Template>(inline_template_text);
        }

        if (matches.has("TemplateInsertionMarker")) {
            data.template_name = matches["SubstitutionName"];
        }

        this->compiled_substitutions.emplace_back(std::move(data));
    }
    assert(remaining_template.empty());
}

} // end namespace xl
