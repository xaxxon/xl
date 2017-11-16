
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
#include "../log.h"
#include "../library_extensions.h"
#include "provider_data.h"


#if defined XL_TEMPLATE_LOG_ENABLE
#define XL_TEMPLATE_LOG(format_string, ...) \
    xl::templates::log.info(format_string, ##__VA_ARGS__);
#else
#define XL_TEMPLATE_LOG(format_string, ...)
#endif



namespace xl::templates {


struct TemplateSubjects {
    inline static std::string subject_names[] = {"default", "compile"};

    enum class Subjects {
        Default, Compile, LOG_LAST_SUBJECT
    };
};

using LogT = ::xl::log::Log<::xl::log::DefaultLevels, TemplateSubjects>;
inline LogT log;

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
    Template(std::string const & tmpl) : _tmpl(tmpl) {}
    Template(std::string && tmpl = "") : _tmpl(std::move(tmpl)) {}

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

    if constexpr(is_passthrough_provider_v<T>) {
        XL_TEMPLATE_LOG("fill got passthrough provider {}, recursively calling fill with underlying provider", source.get_name());
        return fill<ProviderContainer>(source.get_underlying_provider(), input_data.templates);
    }

    if constexpr(std::is_same_v<ProviderPtr, std::remove_reference_t<T>>) {
        if (!input_data.name.empty()) {
            auto named_provider = source->get_named_provider(input_data);
            return this->fill(named_provider, std::move(input_data));
        }
    }


    // used for storing the provider if necessary
    std::unique_ptr<Provider_Interface> provider_interface_unique_pointer;

    // used for consistent interface for assigning to reference later
    Provider_Interface * provider_interface_pointer;
    if constexpr(std::is_base_of_v<Provider_Interface, std::decay_t<T>>) {
//        std::cerr << fmt::format("**** already had provider interface") << std::endl;
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



    // whichever way the object was provided, get a reference to the object for convenience here
    Provider_Interface & provider = *provider_interface_pointer;

    std::string result{""};
    XL_TEMPLATE_LOG("just created variable 'result': '{}'", result);
    result.reserve(this->minimum_result_length);


    for(int i = 0; i < this->compiled_static_strings.size(); i++) {

        result.insert(result.end(), this->compiled_static_strings[i].begin(), this->compiled_static_strings[i].end());
        XL_TEMPLATE_LOG("fill: just added static section {}: '{}'", i, this->compiled_static_strings[i]);

        if (this->compiled_substitutions.size() > i) {
            ProviderData data(this->compiled_substitutions[i]);
            if (input_data.inline_template) {
                data.inline_template = input_data.inline_template;
            }
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

                XL_TEMPLATE_LOG("created ProviderData data on the stack at {}", (void*) &data);

                XL_TEMPLATE_LOG("about to call provider() named '{}' at {} and inline_template: {}",
                                provider.get_name(), (void*)&provider, (void*)data.inline_template.get());
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
                    result.insert(result.end(), data.contingent_trailing_content.begin(), data.contingent_trailing_content.end());
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
(?(DEFINE)(?<UntilDoubleBrace>(?:\s|\\(?&OpenDelimiterHead)|\\(?&CloseDelimiterHead)|[^{}>]|[{](?!\{)|[}](?!\})|>(?!\}\}))*))
(?(DEFINE)(?<UntilEndOfLine>[^\n]*\n))



(?&NotEmptyAssertion)
(?<Literal>(?&UntilDoubleBrace))

# Start Substitution
(?:(?<Substitution>
    (?<OpenDelimiterHere>\{\{)
    \s*
    (?<IgnoreEmptyBeforeMarker><)?
    \s*

    # If there's a leading !, then another template is inserted here instead of a value from a provider
    (?<TemplateInsertionMarker>!)?

    # Replacement name
    (?:(?<SubstitutionName>(?:\\\}|\\\{|[^|%>](?!{{)|>(?!}}))*?)\s*(?=(?&OpenDelimiter)|(?&CloseDelimiter)|\||\%|>|$))

    # Join string, starting with %, if specified
    (?:(?<JoinStringMarker>%)(?<LeadingJoinStringMarker>%?)(?<JoinString>(?:\\\||>(?!}})|[^|>])*))?

    # Everything after the | before the }}
    (?:[|]
        (?<InlineTemplateMarker>!)?
        (?<IgnoreWhitespaceTilEndOfLine>!(?&UntilEndOfLine))?
        (?<SubstitutionData>((?&UntilDoubleBrace)(?&Substitution)?)*)


    )? # end PIPE
    (?<IgnoreEmptyAfterMarker>>)?


    (?<CloseDelimiterHere>\}\})
) # end Substitution
| (?<UnmatchedOpen>\{\{) | (?<UnmatchedClose>\}\}) | $)


)",
        xl::OPTIMIZE | xl::EXTENDED | xl::DOTALL);


    // find all escaped characters - anything following a backslash
    static xl::RegexPcre post_process_regex("(?:\\\\(.))", xl::OPTIMIZE);


    // the portion of the template string which hasn't yet been parsed by the main regex
    std::string remaining_template = this->c_str();
    log.info(TemplateSubjects::Subjects::Compile, "compiling template: '{}'", this->_tmpl.c_str());

    bool first_line_belongs_to_last_substitution = false;

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
        log.info(TemplateSubjects::Subjects::Compile, "postprocessing: '{}'", literal_string);

        literal_string = post_process_regex.replace(literal_string, "$1");
        log.info(TemplateSubjects::Subjects::Compile, "into: '{}'", literal_string);


        bool ignore_empty_replacements_before = matches.has("IgnoreEmptyBeforeMarker");
        log.info(TemplateSubjects::Subjects::Compile, "ignoring empty replacements? {}", ignore_empty_replacements_before);
        std::string contingent_leading_content;

        if (first_line_belongs_to_last_substitution) {
            first_line_belongs_to_last_substitution = false;
            static Regex first_line_regex(R"(^([^\n]*)(.*)$)", xl::DOTALL | xl::DOLLAR_END_ONLY);
            if (auto results = first_line_regex.match(literal_string)) {
                literal_string = results[2];

                // get previous substitution
                this->compiled_substitutions.back().contingent_trailing_content = results[1];
            } else {
                assert(false);
            }
        }

        if (ignore_empty_replacements_before) {
            // trim off everything after the last newline on the static and put it in the template
            static Regex last_line_regex(R"(^(.*?)(\n?[^\n]*)$)", xl::DOTALL | xl::DOLLAR_END_ONLY);
            auto results = last_line_regex.match(literal_string);
            if (results) {
                literal_string = results[1];
                contingent_leading_content = results[2];
            }
        }

        bool ignore_empty_replacements_after = matches.has("IgnoreEmptyAfterMarker");
        if (ignore_empty_replacements_after) {
            first_line_belongs_to_last_substitution = true;
        }


        this->compiled_static_strings.push_back(literal_string);
        this->minimum_result_length += this->compiled_static_strings.back().size();



        // if no substitution found, everything was a literal and is handled as a "trailing literal" outside
        //   this loop
        if (!matches.has("Substitution")) {
            break;
        }


        ProviderData data;

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
            log.info(TemplateSubjects::Subjects::Compile, "Template::compile - creating inline template from '{}'", matches["SubstitutionData"]);
            auto inline_template_text = matches["SubstitutionData"];
            log.info(TemplateSubjects::Subjects::Compile, "inline template text: {}", inline_template_text);
            data.inline_template = std::make_shared<Template>(inline_template_text);
        }

        if (matches.has("TemplateInsertionMarker")) {
            data.template_name = matches["SubstitutionName"];
        }

        this->compiled_substitutions.emplace_back(std::move(data));
    }

    assert(remaining_template.empty());
//std::cerr << fmt::format("remaining template: '{}'", remaining_template) << std::endl;
//    std::cerr << fmt::format("pushing on remaining template: '{}'", std::regex_replace(remaining_template, post_process_regex, "$1")) << std::endl;
    //this->compiled_static_strings.push_back(post_process_regex.replace(remaining_template, "$1"));
}

} // end namespace xl
