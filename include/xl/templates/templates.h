
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
#include "substitution.h"


// All logging is compile-time disabled unless XL_TEMPLATE_LOG_ENABLE is specified
#if defined XL_TEMPLATE_LOG_ENABLE
#define XL_TEMPLATE_LOG(format_string, ...) \
    xl::templates::log.info(format_string, ##__VA_ARGS__);
#else
#define XL_TEMPLATE_LOG(format_string, ...)
#endif

#include "fill_state.h"

 
namespace xl::templates {



struct Substitution;
struct SubstitutionState;
class Provider_Interface;



class Template {
XL_PRIVATE_UNLESS_TESTING:
    
    // full, unprocessed template string
    std::string _tmpl;
    
    mutable std::shared_ptr<CompiledTemplate> compiled_template;

public:
    
    explicit Template(std::string tmpl = "{{}}") : _tmpl(std::move(tmpl)) {}
    Template(Template const &) = default;

    inline char const * c_str() const { return this->_tmpl.c_str(); }

    
    template <typename ProviderContainer = void>
    std::string fill(SubstitutionState &) const;

    
    template <typename ProviderContainer = void>
    std::string fill(FillState const &) const;


    template<typename ProviderContainer = void, class T = char const *, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, FillState>>>
    std::string fill(T && source = "", std::map<std::string, Template> template_map = {}) const;

    
    
    // compiles the template for faster processing
    // benchmarks showed ~200x improvement when re-using templates 1000 times
    inline void compile() const;

    inline bool is_compiled() const;
    
    static inline std::unique_ptr<Template> empty_template = std::make_unique<Template>();
};

class CompiledTemplate {
    friend class Template;
XL_PRIVATE_UNLESS_TESTING:
    // static portions of the template as separated by substitutions
    std::vector<std::string> static_strings;

    // substitutions in the template
    std::vector<Substitution> substitutions;


    // sum of the length of all the fixed portions of the template
    mutable size_t minimum_result_length = 0;


public:
    
    Template const * const source_template;
    

    template <typename ProviderContainer = void>
    std::string fill(FillState const &) const;

    
    CompiledTemplate(Template const * const source_template) :
        source_template(source_template)
    {}
    

    // create a template with a pre-built substitution instead of compiling
    //   the template from a string
    CompiledTemplate(Template const * const source_template, Substitution) :
        source_template(source_template)
    {}

};




} // end namespace xl

// need to call directly to this .h file because circular dependencies make things tricky
#include "../log/log.h"

#include "provider.h"
#include "directory_loader.h"

namespace xl::templates {

class provider_data;


template <typename ProviderContainer>
std::string Template::fill(FillState const & fill_state) const {

    if (!this->is_compiled()) {
        this->compile();
    }
    
    return this->compiled_template->fill(fill_state);
}


template<typename ProviderContainer, class T, typename>
std::string Template::fill(T && source, std::map<std::string, Template> template_map) const {
    
    if (!this->is_compiled()) {
        this->compile();
    }

    XL_TEMPLATE_LOG("Filling template: '{}'", this->c_str());

    // used for storing the provider if necessary
    std::unique_ptr<Provider_Interface> provider_interface_unique_pointer;
    // used for consistent interface for assigning to reference later
    Provider_Interface * provider_interface_pointer;
    if constexpr(std::is_base_of_v<Provider_Interface, std::decay_t<T>>) {
        assert(false); // this shouldn't happen anymore?
        provider_interface_pointer = &source;
        assert(provider_interface_pointer != nullptr);

    } else if constexpr(std::is_same_v<ProviderPtr, std::decay_t<T>>) {
        provider_interface_pointer = source.get();
    } else {

        // need to store the unique_ptr to maintain object lifetime then assign to normal pointer
        //   so there is a common way to get the object below for assignment to reference type
        provider_interface_unique_pointer = DefaultProviders<ProviderContainer>::template make_provider(std::forward<T>(source));
        XL_TEMPLATE_LOG("**** got unique ptr at {}", (void *) provider_interface_unique_pointer.get());
        provider_interface_pointer = provider_interface_unique_pointer.get();

        /**
         * IF THIS IS FIRING, PERHAPS YOU ARE USING AN RVALUE PROVIDER MULTIPLE TIMES
         */
        assert(provider_interface_pointer != nullptr);


        XL_TEMPLATE_LOG("**** set provider interface pointer to {}", (void *) provider_interface_pointer);
    }

    XL_TEMPLATE_LOG("outside: provider interface pointer to {}", (void *) provider_interface_pointer);

    
    ProviderPtr fillable_provider = {};
    if (!provider_interface_pointer->is_fillable_provider()) {
        fillable_provider = provider_interface_pointer->get_fillable_provider();
        provider_interface_pointer = fillable_provider.get();
    }

    ProviderStack provider_stack{provider_interface_pointer};
    std::cerr << fmt::format("ps size: {}", provider_stack.size()) << std::endl;
    FillState fill_state(std::move(provider_stack), &template_map);
    std::cerr << fmt::format("fill state provider stack size: {}", fill_state.provider_stack.size()) << std::endl;
    return fill<ProviderContainer>(fill_state);
}


template<typename ProviderContainer>
std::string Template::fill(SubstitutionState & substitution_state) const {
    
    // if the top provider exists and wants the entire template as-is, provide it directly
    if (!substitution_state.fill_state.provider_stack.empty() && substitution_state.fill_state.provider_stack.front()->needs_raw_template()) {
        return substitution_state.fill_state.provider_stack.front()->operator()(substitution_state);
    } else {
        return this->fill(substitution_state.fill_state);
    }
}


template<typename ProviderContainer>
std::string CompiledTemplate::fill(FillState const & fill_state) const {

    assert(!fill_state.provider_stack.empty());
    Provider_Interface const & provider = *fill_state.provider_stack.front();

    std::string result{""};
    XL_TEMPLATE_LOG("just created variable 'result': '{}'", result);
    result.reserve(this->minimum_result_length);
    

    for(size_t i = 0; i < this->static_strings.size(); i++) {

        result.insert(result.end(), this->static_strings[i].begin(), this->static_strings[i].end());
        XL_TEMPLATE_LOG("fill: just added static section {}: '{}'", i, this->static_strings[i]);

        if (this->substitutions.size() > i) {
            SubstitutionState current_substitution(*this, fill_state, &this->substitutions[i]);
            
           
            XL_TEMPLATE_LOG("grabbed data for compiled_subsitution {} - it has name {} and inline template: {}",
                            i, current_substitution.substitution->get_name(), (void*)current_substitution.substitution->final_data->inline_template.get());
            current_substitution.current_template = this;
            XL_TEMPLATE_LOG("substitution instantiation data.name: '{}'", current_substitution.substitution->get_name());

            // substituting another template in with {{!template_name}}
            if (current_substitution.substitution->final_data->template_name != "") {
                if (fill_state.templates->empty()) {
                    throw TemplateException("Cannot refer to another template if no other templates specified: " + current_substitution.substitution->final_data->template_name);
                }
                auto template_iterator = fill_state.templates->find(current_substitution.substitution->final_data->template_name);
                if (template_iterator == fill_state.templates->end()) {
                    throw TemplateException("No template found named: {}", current_substitution.substitution->final_data->template_name);
                }
                auto inline_template_result = template_iterator->second.fill(fill_state);
                if (!inline_template_result.empty()) {
                    result.insert(result.end(), current_substitution.substitution->initial_data->contingent_leading_content.begin(), current_substitution.substitution->initial_data->contingent_leading_content.end());
                }
                result.insert(result.end(), begin(inline_template_result), end(inline_template_result));
                if (!inline_template_result.empty()) {
                    result.insert(result.end(), current_substitution.substitution->initial_data->contingent_trailing_content.begin(), current_substitution.substitution->initial_data->contingent_trailing_content.end());
                }
            }
            // filling a template
            else {

                if (current_substitution.substitution->comment) {
                    XL_TEMPLATE_LOG("Handled comment");
                } else {

                    XL_TEMPLATE_LOG("created Substitution data for '{}' on the stack at {}",
                                    current_substitution.substitution->get_name(), (void *) &current_substitution);

                    XL_TEMPLATE_LOG("about to call provider() named '{}' at {} and inline_template: {}",
                                    provider.get_name(), (void *) &provider,
                                    (void *) current_substitution.substitution->final_data->inline_template.get());
                    auto substitution_result = provider(current_substitution);
                    XL_TEMPLATE_LOG("replacement for {} is: {}", this->source_template->c_str(), substitution_result);
                    XL_TEMPLATE_LOG("provider() named {} returned: '{}'", provider.get_name(), substitution_result);
                    if (!current_substitution.substitution->initial_data->contingent_leading_content.empty() &&
                        !substitution_result.empty()) {
                        XL_TEMPLATE_LOG("adding contingent data: {}",
                                        current_substitution.substitution->initial_data->contingent_leading_content);

                        result.insert(result.end(),
                                      current_substitution.substitution->initial_data->contingent_leading_content.begin(),
                                      current_substitution.substitution->initial_data->contingent_leading_content.end());
                    }
                    result.insert(result.end(), substitution_result.begin(), substitution_result.end());
                    if (!current_substitution.substitution->initial_data->contingent_trailing_content.empty() &&
                        !substitution_result.empty()) {
                        XL_TEMPLATE_LOG("inserting trailing content: {}",
                                        current_substitution.substitution->initial_data->contingent_trailing_content);
                        result.insert(result.end(),
                                      current_substitution.substitution->initial_data->contingent_trailing_content.begin(),
                                      current_substitution.substitution->initial_data->contingent_trailing_content.end());
                    }
                }
            }
        }
    }
    return result;
}


bool Template::is_compiled() const {
//    return !this->static_strings.empty() || !this->substitutions.empty();
    return this->compiled_template.get() != nullptr;
}


void Template::compile() const {

    XL_TEMPLATE_LOG("Compiling: {}", this->_tmpl);

    this->compiled_template = std::make_shared<CompiledTemplate>(this);

    auto & substitutions = const_cast< std::vector<Substitution> &>(this->compiled_template->substitutions);

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


(?(DEFINE)(?<PretendMatch>([^{}]+|{(?!{)|}(?!})|{{(?&PretendMatch)}}|)))



(?&NotEmptyAssertion)
(?<Literal>(?&UntilDoubleBrace))

# Start Substitution
(?:(?<Substitution>
    (?<OpenDelimiterHere>\{\{)
    \s*
    (?:
        (?<Comment>\#(?&PretendMatch)*)|
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
//                XL_TEMPLATE_LOG(TemplateSubjects::Subjects::Compile, "{}: '{}'", i, s);
//            }
//        }
        XL_TEMPLATE_LOG(TemplateSubjects::Subjects::Compile, "literal: '{}', substutition: '{}'",
                                 matches["Literal"],
                                 matches["Substitution"]);
//        for (size_t i = 0; i < matches.size(); i++) {
//            XL_TEMPLATE_LOG("match[{}]: '{}'", i, matches[i]);
//        }


        // check for open but no close or incorrect brace type
        if (matches.length("UnmatchedOpen")) {
            throw TemplateException("Unmatched Open");
        }
        if (matches.length("UnmatchedClose")) {
            throw TemplateException("Unmatched Close (missing opening }})");
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

                XL_TEMPLATE_LOG(TemplateSubjects::Subjects::Compile, "got '{}' and '{}'", results[1], results[2]);

                // get previous substitution
                substitutions.back().initial_data->contingent_trailing_content = results[1];

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
                XL_TEMPLATE_LOG(TemplateSubjects::Subjects::Compile, "got '{}' and '{}'", results[1], results[2]);
                literal_string = results[1];
                contingent_leading_content = results[2];
            }
        }
        
        this->compiled_template->static_strings.push_back(literal_string);

        first_line_belongs_to_last_substitution = matches.length("IgnoreEmptyAfterMarker");
        XL_TEMPLATE_LOG(TemplateSubjects::Subjects::Compile, "ignore empty after marker: {}", matches["IgnoreEmptyAfterMarker"]);
        XL_TEMPLATE_LOG(TemplateSubjects::Subjects::Compile, "setting first line belongs to last substitution to {} on {}", first_line_belongs_to_last_substitution, matches["Substitution"]);


        // if no substitution found, everything was a literal and is handled as a "trailing literal" outside
        //   this loop
        if (!matches.has("Substitution")) {
            break;
        }


        Substitution data(*this);

        // if the substition is a comment, nothing else matters
        if (matches.has("Comment")) {
            data.comment = true;
            continue;
        } 

        if (!matches.has("TemplateInsertionMarker")) {
            auto substitution_name = matches["SubstitutionName"];
            XL_TEMPLATE_LOG(TemplateSubjects::Subjects::Compile, "substitution name: {}", substitution_name);
            if (!substitution_name.empty()) {

                // split it on . and add each to name_entries
                size_t position = 0;
                size_t new_position = position;
                while ((new_position = substitution_name.find('.', position)) != std::string_view::npos) {


                    // create a new template for the partial name and a new inline template for it
                    
//                    {{a.b|!{{foo}} }} 
//                      becomes:
//                    {{a|!{{b|!{{foo}} }} }}
                    
                    // the problem is that it changes the outer-most template which is what's being compiled.   
                    // it's hard to just switch it out at this point.
                    
                    
                    data.name_entries.emplace_back(substitution_name.data(), position, new_position - position);

                    XL_TEMPLATE_LOG(TemplateSubjects::Subjects::Compile, "substitution sub-name: {}", data.name_entries.back());

                    position = new_position + 1;
                }
                data.name_entries.emplace_back(substitution_name.data(), position,
                                               substitution_name.length() - position);
                std::reverse(data.name_entries.begin(), data.name_entries.end());
            }
            XL_TEMPLATE_LOG(TemplateSubjects::Subjects::Compile, "parsed name into {}", xl::join(data.name_entries));
        } else {
            data.final_data->template_name = matches["SubstitutionName"];
        }

        if (matches.has("JoinStringMarker")) {
            data.shared_data->join_string = matches["JoinString"];
            std::cerr << fmt::format("Join string for '{}' set to: '{}'\n", this->c_str(), data.shared_data->join_string);
        } else {
            std::cerr << fmt::format("join string not found in '{}', using default: '{}'\n", this->c_str(), data.shared_data->join_string);
        }

        if (matches.has("LeadingJoinStringMarker")) {
            data.shared_data->leading_join_string = true;
        }
        
        data.initial_data->contingent_leading_content = contingent_leading_content;

        data.shared_data->ignore_empty_replacements = ignore_empty_replacements_before;

        if (matches.has("InlineTemplateMarker")) {
            log.info(TemplateSubjects::Subjects::Compile, std::string("Template::compile - creating inline template from '") + std::string(matches["SubstitutionData"]) + "'");
            auto inline_template_text = matches["SubstitutionData"];
            log.info(TemplateSubjects::Subjects::Compile, "inline template text: " + std::string(inline_template_text));
            data.final_data->inline_template = std::make_shared<Template>(inline_template_text);
        } else {
            data.parameters = matches["SubstitutionData"];
        }

        

        substitutions.emplace_back(std::move(data));
    }
//    std::cerr << fmt::format("remaining template '{}', sizes: {}", remaining_template, remaining_template.size()) << std::endl;
    assert(remaining_template.empty());
    
}


// Requires Template definition

// If current substitution is for a.b.c, this creates a new Substitution for
// `b.c` leaving `this` with just `a`
inline Substitution Substitution::split_substitution() {
    // if there's nothing to split, this substitution is already the substitution
    // which should be used 
    if (name_entries.size() == 1) {
        return std::move(*this);
    }

    // create a new substitution
    auto first_name_entry = name_entries.front();
    this->name_entries.pop_front();


    // if input looks like "x {{foo.bar.baz|x}} y"
    // it should turn into:
    // x {{foo|{{bar.baz|x}}}} y
    // x {{foo|{{bar|{{baz|x}}}}}} y

    Substitution new_current_substitution;
    
    new_current_substitution.shared_data = this->shared_data;
    new_current_substitution.final_data = std::move(this->final_data);
    
    
    

    // this template becomes the inline template of new_current_substitution
    // and it has one substitution, which is the trimmed `this` Substitution
//    Template new_current_substitution_inline_template(*this);

//    new_current_substitution_inline_template.substitutions.emplace_back(this->split_substitution());


    return new_current_substitution;

}

} // end namespace xl
