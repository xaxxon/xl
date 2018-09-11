#pragma once

#include <cassert>

#include "substitution.h"
#include "templates.h"
#include "exceptions.h"

namespace xl::templates {




// careful where this is implemented because xl::log uses xl::templates and vice versa
template<typename... Args>
TemplateException::TemplateException(xl::zstring_view format_string, Args&&... args) :
xl::FormattedException(format_string.c_str(), std::forward<Args>(args)...)
{
xl::templates::log.error(LogT::Subjects::Exception, this->what());
}

inline void Substitution::split() {

    // if there are more than one value in name_entries, then split off the last one
    if (this->name_entries.size() > 1) {
        auto new_template = std::make_unique<CompiledTemplate>(this->tmpl);
        
        auto new_substitution = std::make_unique<Substitution>();
        new_substitution->raw_text = fmt::format("Split off of {}", this->raw_text);
        
        // this follows the new substitution object until the end of the split so only
        //   the last one has this data
        new_substitution->final_data = std::move(this->final_data);
        new_substitution->shared_data = this->shared_data;
        
        new_substitution->name_entries = std::move(this->name_entries);

        this->name_entries.push_front(new_substitution->name_entries.front());
        new_substitution->name_entries.pop_front();

        std::cerr << fmt::format("split substitution name_entries now: {}\n", xl::join(this->name_entries));
        std::cerr << fmt::format("setting new_substitution->name_entries = {}\n", xl::join(new_substitution->name_entries));

        new_substitution->parent_substitution = this;
        
        
        new_substitution->split();
        
        
        
        
        new_template->add_substitution(std::move(new_substitution));
        
        this->final_data.inline_template = std::move(new_template);
        std::cerr << fmt::format("FOO: {}\n", xl::join(this->final_data.inline_template->substitutions[0]->name_entries));

    }
    
    
    assert(this->name_entries.size() <= 1);
}




inline std::shared_ptr<CompiledTemplate> & Template::compile() const {

    if (this->compiled_template) {
        return this->compiled_template;
    }

    XL_TEMPLATE_LOG(TemplateSubjects::Subjects::Compile, "Compiling: {}", this->_tmpl);

    this->compiled_template = std::make_shared<CompiledTemplate>(this);

    auto & substitutions = const_cast< std::vector<std::unique_ptr<Substitution>> &>(this->compiled_template->substitutions);

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
        (?<GroupingSubstitution>@)(?R)|
        (?<Comment>\#(?&PretendMatch)*)|
        (?<IgnoreEmptyBeforeMarker><<?)?

        \s*


        # If there's a leading !, then another template is inserted here instead of a value from a provider
        (?<TemplateInsertionMarker>!)?

        # Replacement name
        (?:\.?(?<ProviderStackRewind>\.*)(?<SubstitutionName>(?:\\\}|\\\{|[^|%>](?!{{)|>(?!}}))*?)\s*(?=(?&OpenDelimiter)|(?&CloseDelimiter)|\||\%|>|$))

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
        
        // value regex was run against that is not changed during the body of this while loop
        std::string const last_matched_template_string = remaining_template;

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
            throw TemplateException("Unmatched Close (missing opening '}}') in {}", last_matched_template_string);
        }



        remaining_template = matches.suffix();

        std::string literal_string = std::string(matches["Literal"]);
        log.info(TemplateSubjects::Subjects::Compile, "postprocessing: '" + literal_string + "'");

        literal_string = post_process_regex.replace(literal_string, "$1");
        log.info(TemplateSubjects::Subjects::Compile, std::string("into: ") + literal_string);


        bool ignore_empty_replacements_before = matches.has("IgnoreEmptyBeforeMarker");
//        log.info(TemplateSubjects::Subjects::Compile, std::string("ignoring empty replacements? ") + (ignore_empty_replacements_before ? "true" : "false"));
        std::string contingent_leading_content = "";


        // if the current literal string has contingent data for the previous substitution, grab it off now
        if (first_line_belongs_to_last_substitution) {
            XL_TEMPLATE_LOG("trailing contingent ({}) on: '{}'\n", first_line_belongs_to_last_substitution, literal_string);
            static Regex first_line_regex(R"(^([^\n]*)(.*)$)", xl::DOTALL | xl::DOLLAR_END_ONLY);
//            static Regex first_line_and_empty_lines_regex(R"(^([^\n]*\n*)(.*)$)", xl::DOTALL | xl::DOLLAR_END_ONLY);

//            auto & regex = first_line_belongs_to_last_substitution == 1 ?
//                           first_line_regex : first_line_and_empty_lines_regex;
            if (first_line_belongs_to_last_substitution == 1) {
                if (auto results = first_line_regex.match(literal_string)) {
    
                    XL_TEMPLATE_LOG(TemplateSubjects::Subjects::Compile,
                                    "contingent trailing content '{}' and literal string '{}'", results[1], results[2]);
    
                    
                        substitutions.back()->initial_data.contingent_trailing_content = results[1];
                        literal_string = results[2];
                        // if there's no substitution, then the entire literal string goes to the previous substitution
                } else {
                    assert(false);
                }
            } else if (first_line_belongs_to_last_substitution == 2) {
                substitutions.back()->initial_data.contingent_trailing_content = std::move(literal_string);
                literal_string.clear();
            
            } else {
                assert(false);
            }
            
        }

        if (matches.has("IgnoreEmptyBeforeMarker")) {
            // trim off everything after the last newline on the static and put it in the template
            static Regex last_line_regex(R"(^(.*?)(\n?[^\n]*)$)", xl::DOTALL | xl::DOLLAR_END_ONLY);
            static Regex last_line_and_blank_lines_regex(R"(^(.+?\n?)??(\n*[^\n]*)$)", xl::DOTALL | xl::DOLLAR_END_ONLY);
//            auto & regex = matches.length("IgnoreEmptyBeforeMarker") == 1 ?
//                           last_line_regex : last_line_and_blank_lines_regex;

            auto ignore_empty_before_marker_count = matches.length("IgnoreEmptyBeforeMarker");
//            std::cerr << fmt::format("Running ignore empty before marker on '{}'", literal_string) << std::endl;
            if (ignore_empty_before_marker_count == 1) {
                auto results = last_line_regex.match(literal_string);
                if (results) {
                    XL_TEMPLATE_LOG(TemplateSubjects::Subjects::Compile,
                                    "literal_string '{}' contingent_leading_content '{}'", results[1], results[2]);
                    literal_string = results[1];
                    contingent_leading_content = results[2];
                }
            } else if (ignore_empty_before_marker_count == 2) {
                contingent_leading_content = std::move(literal_string);
                literal_string.clear();
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


        auto data = std::make_unique<Substitution>(*this);
        data->raw_text = matches[0];

        // if the substition is a comment, nothing else matters
        if (matches.has("Comment")) {
            XL_TEMPLATE_LOG(TemplateSubjects::Subjects::Compile, "substitution is a comment");
            data->comment = true;
        } else if (matches.has("GroupingSubstitution")) {
            
            std::cerr << fmt::format("found grouping substitution (not implemented)\n");
        } else {
            
            data->initial_data.rewind_provider_count = matches["ProviderStackRewind"].length();

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


                        data->name_entries.emplace_front(substitution_name.data(), position, new_position - position);

                        XL_TEMPLATE_LOG(TemplateSubjects::Subjects::Compile, "substitution sub-name: {}",
                                        data->name_entries.front());

                        position = new_position + 1;
                    }
                    data->name_entries.emplace_front(substitution_name.data(), position,
                                                    substitution_name.length() - position);
                    std::reverse(data->name_entries.begin(), data->name_entries.end());
                }
                XL_TEMPLATE_LOG(TemplateSubjects::Subjects::Compile, "parsed name into {}",
                                xl::join(data->name_entries));
            } else {
                data->final_data.template_name = matches["SubstitutionName"];
            }

            if (matches.has("JoinStringMarker")) {
                data->shared_data->join_string = matches["JoinString"];
                XL_TEMPLATE_LOG(TemplateSubjects::Subjects::Compile, "Join string for '{}' set to: '{}'\n",
                                this->c_str(), data->shared_data->join_string);
            } else {
                XL_TEMPLATE_LOG(TemplateSubjects::Subjects::Compile,
                                "join string not found in '{}', using default: '{}'\n", this->c_str(),
                                data->shared_data->join_string);
            }

            if (matches.has("LeadingJoinStringMarker")) {
                data->shared_data->leading_join_string = true;
            }

            data->initial_data.contingent_leading_content = contingent_leading_content;

            data->shared_data->ignore_empty_replacements = ignore_empty_replacements_before;

            if (matches.has("InlineTemplateMarker")) {
                log.info(TemplateSubjects::Subjects::Compile,
                         std::string("Template::compile - creating inline template from '") +
                         std::string(matches["SubstitutionData"]) + "'");
                auto inline_template_text = matches["SubstitutionData"];
                log.info(TemplateSubjects::Subjects::Compile,
                         "inline template text: " + std::string(inline_template_text));
//                data->final_data.inline_template = std::make_shared<Template>(inline_template_text);
                data->final_data.inline_template = Template(inline_template_text).compile();
            } else {
                data->parameters = matches["SubstitutionData"];
            }
        }

        // if there are multiple names in the substitution, split it off into multiple
        //   template/substitution pairs
        data->split();

        substitutions.emplace_back(std::move(data));
    }

    assert(this->compiled_template->static_strings.size() == this->compiled_template->substitutions.size() ||
           this->compiled_template->static_strings.size() == this->compiled_template->substitutions.size() + 1);


    XL_TEMPLATE_LOG("compiled template:\n{}\n", this->compiled_template->details_string());

    assert(remaining_template.empty());

    return this->compiled_template;
    
}


inline CompiledTemplate const * SubstitutionState::get_template() {

    // initialize with inline_template then look up by name if no inline template
    CompiledTemplate const * result = this->substitution->final_data.inline_template.get();
    if (result != nullptr) {
        return result;
    }

    // if no inline template provided and no named template is requested, return empty template
    if (this->substitution->parameters.empty()) {
        assert(CompiledTemplate::empty_compiled_template.get() != nullptr);
        return CompiledTemplate::empty_compiled_template.get();
    }


    if (this->fill_state.templates == nullptr) {
        throw TemplateException("ContainerProvider received nullptr template map so it can't possibly find a template by name");
    }

    auto template_iterator = this->fill_state.templates->find(this->substitution->parameters);
    if (template_iterator == this->fill_state.templates->end()) {
        if (this->fill_state.templates->empty()) {
            throw TemplateException(
                "ContainerProvider received empty template map so it can't possibly find a template for its members" +
                this->substitution->get_name());
        }
        throw TemplateException(
            fmt::format("ContainerProvider couldn't find template named: '{}' from template {}", this->substitution->parameters, this->current_template->source_template->c_str()));
    }

    XL_TEMPLATE_LOG("Returning named template: {}", template_iterator->second.c_str());
    
    return template_iterator->second.compile().get();


}



inline std::string const & Substitution::get_name(bool required) const {

    if (this->name_entries.size() > 1) {
        std::cerr << fmt::format("too many name entries: {}\n", xl::join(this->name_entries));
        assert(false);
    }

    if (this->name_entries.size() == 0) {
        if (!this->final_data.template_name.empty()) {
            return this->final_data.template_name;
        } else {
            static std::string bogus = "BOGUS";

            if (required) {
                throw TemplateException("Name required but not available in substitution: {}", this->raw_text);
            } else {
                return bogus;
            }
        }
    } else {
        return this->name_entries.front();
    }
}


} // end xl::templates