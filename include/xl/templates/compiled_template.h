#pragma once

#include <string>
#include <vector>

#include "../deferred.h"

#include "substitution.h"
#include "substitution_state.h"

namespace xl::templates {


class CompiledTemplate {
    friend class Template;

XL_PRIVATE_UNLESS_TESTING:
    // static portions of the template as separated by substitutions
    std::vector<std::string> static_strings;

    // substitutions in the template - a pointer so the Substitution object maintains address identity
    std::vector<std::unique_ptr<Substitution>> substitutions;


    // sum of the length of all the fixed portions of the template
    mutable size_t minimum_result_length = 0;
    
    // attempt to find a match in providers already on the provider stack
    xl::expected<std::string, ErrorList> rewind_results(SubstitutionState &) const;
    

public:

    Template const * const source_template;

    template <typename ProviderContainer = void>
    xl::expected<std::string, ErrorList> fill(SubstitutionState &) const;
    
    template<typename ProviderContainer = void>
    xl::expected<std::string, ErrorList> fill(FillState const &) const;

    template<typename ProviderContainer, class T, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, FillState>>>
    xl::expected<std::string, ErrorList> fill(T && source, std::map<std::string, Template> template_map) const;

    inline static std::shared_ptr<CompiledTemplate> empty_compiled_template = 
        *Template::empty_template->compile();

    
    CompiledTemplate(Template const * const source_template) :
        source_template(source_template) 
    {}


    // create a template with a pre-built substitution instead of compiling
    //   the template from a string
    CompiledTemplate(Template const * const source_template, Substitution) :
        source_template(source_template) 
    {}


    // helper function for getting a human-readable string representation
    std::string details_string() {
        std::stringstream details_string;

        details_string << fmt::format("static strings: {}, substitutions: {}", this->static_strings.size(),
                                      this->substitutions.size());

        for (decltype(static_strings.size()) i = 0; i < static_strings.size(); i++) {

            details_string << this->static_strings[i] << "\n";

            if (this->substitutions.size() > i) {
                details_string << this->substitutions[i] << "\n";
            }
        }

        return details_string.str();
    }

    // add a substitution directly - useful for splitting Substitution's
    void add_substitution(std::unique_ptr<Substitution> substitution) {
        if (this->static_strings.size() <= this->substitutions.size()) {
            this->static_strings.push_back("");
        }
        this->substitutions.push_back(std::move(substitution));
    }

};

}

#include "templates.h"

namespace xl::templates {

inline xl::expected<std::string, ErrorList> CompiledTemplate::rewind_results(SubstitutionState & substitution_state) const {

    // first level has already been rewound before entering this function
    unsigned int rewind_count = 1;
    
    ErrorList error_list;

    if (!substitution_state.fill_state.searching_provider_stack && !substitution_state.fill_state.provider_stack.empty()) {

 
        // move substitution back if it's a split() substitution
        while (substitution_state.substitution->parent_substitution != nullptr) {
            substitution_state.substitution = substitution_state.substitution->parent_substitution;
        }


        for (auto * provider : substitution_state.fill_state.provider_stack) {

            XL_TEMPLATE_LOG("rewinding through provider stack, current provider:{}",
                                     provider->get_name());
            XL_TEMPLATE_LOG(fmt::format("{}", substitution_state.fill_state.provider_stack));

            // only rewind on "core" providers
            

            

            // if rewind count is set, rewind at least that many levels
            if (rewind_count < substitution_state.substitution->initial_data.rewind_provider_count) {
                if (provider->is_rewind_point()) {
                    rewind_count++;
                }
//                std::cerr << fmt::format("is rewind point: {} new rewind count: {}/{}\n",
//                                         provider->is_rewind_point(),
//                                         rewind_count,
//                                         substitution_state.substitution->initial_data.rewind_provider_count
//                ) << std::endl;
                continue;
            }
            
            
            if (!substitution_state.substitution->final_data.template_name.empty()) {
                // not even sure what this would actually do
                return xl::make_unexpected(ErrorList(fmt::format("Rewinding through a named template '{}' is not supported",
                                                                 substitution_state.substitution->final_data.template_name)));
            }

            // start over from scratch
            auto copy = SubstitutionState(*substitution_state.current_template,
                                          substitution_state.fill_state,
                                          substitution_state.substitution);
            copy.fill_state.searching_provider_stack = true;
            copy.fill_state.provider_stack.clear();

            substitution_state.substitution->initial_data.rewound = true;
            Defer(substitution_state.substitution->initial_data.rewound = false);

            auto result = provider->operator()(copy);
            if (result) {
                return result;
            } else {
                error_list.append(result.error());
            }
//            std::cerr << fmt::format("tried rewound provider, got failure: {}\n", result.error());
            continue;
        
        }
    }

//    std::string template_text = "<unknown template name>";
//    if (substitution_state.current_template != nullptr) {
//        template_text = substitution_state.current_template->source_template->c_str();
//    }
//     error_list.append(fmt::format("Couldn't find substitute for '{}' in template '{}'", 
//        substitution_state.substitution->raw_text,
//        substitution_state.current_template->source_template->c_str()
//        ));
    
    return xl::make_unexpected(error_list);
}


template<typename ProviderContainer>
xl::expected<std::string, ErrorList> CompiledTemplate::fill(FillState const & fill_state) const {

    assert(!fill_state.provider_stack.empty());
    Provider_Interface const & provider = *fill_state.provider_stack.front();

    std::string result{""};
//    XL_TEMPLATE_LOG("just created variable 'result': '{}'", result);
    result.reserve(this->minimum_result_length);

    ErrorList error_list(fmt::format("Filling {}", this->source_template->c_str()));
    for(size_t i = 0; i < this->static_strings.size(); i++) {

        result.insert(result.end(), this->static_strings[i].begin(), this->static_strings[i].end());
        XL_TEMPLATE_LOG(LogT::Subjects::Template, "fill: just added static section {}: '{}'", i, this->static_strings[i]);

        if (this->substitutions.size() > i) {
            SubstitutionState current_substitution(*this, fill_state, this->substitutions[i].get());


            XL_TEMPLATE_LOG("grabbed data for compiled_subsitution '{}' - it has name '{}' and inline template: '{}'",
                            i, current_substitution.substitution->get_name().value_or("<NO NAME AVAILABLE>"), (void*)current_substitution.substitution->final_data.inline_template.get());
            current_substitution.current_template = this;
            current_substitution.fill_state.searching_provider_stack = fill_state.searching_provider_stack;

            // substituting another template in with {{!template_name}}
            if (!current_substitution.substitution->final_data.template_name.empty()) {
                if (fill_state.templates->empty()) {
                    return xl::make_unexpected(error_list.append(fmt::format("Cannot refer to another template if no other templates specified: " + current_substitution.substitution->final_data.template_name)));
                }
                auto template_iterator = fill_state.templates->find(current_substitution.substitution->final_data.template_name);
                if (template_iterator == fill_state.templates->end()) {
                    return xl::make_unexpected(error_list.append(fmt::format("No template found named: {}", current_substitution.substitution->final_data.template_name)));
                }
                auto inline_template_result = template_iterator->second.fill(fill_state);

                if (!inline_template_result) {
                    return xl::make_unexpected(ErrorList(fmt::format("No-rewind path returning failure for named template: '{}'\n", 
                        current_substitution.substitution->final_data.template_name)).
                        append(inline_template_result.error()));
                }
//                if (!inline_template_result) {
//                    XL_TEMPLATE_LOG("About to start rewind because: {}", inline_template_result.error().get_pretty_string());
//                    
//                    std::cerr << fmt::format("provider stack before rewind: {}\n", current_substitution.fill_state.provider_stack);
//                    
//                    // don't rewind on the same provider that was just tried
//                    bool rewound_through_rewind_point = false;
//                    while(!current_substitution.fill_state.provider_stack.empty() &&
//                        !rewound_through_rewind_point) {
//                        if (current_substitution.fill_state.provider_stack.front()->is_rewind_point()) {
//                            rewound_through_rewind_point = true;
//                        }
//                        current_substitution.fill_state.provider_stack.pop_front();
//                    }
//
//                    inline_template_result = this->rewind_results(current_substitution);
//                    if (!inline_template_result) {
//                        return xl::make_unexpected(inline_template_result.error());
//                    }
//                }
                if (!inline_template_result->empty()) {
                    result.insert(result.end(), current_substitution.substitution->initial_data.contingent_leading_content.begin(), current_substitution.substitution->initial_data.contingent_leading_content.end());
                }
                result.insert(result.end(), begin(*inline_template_result), end(*inline_template_result));
                if (!inline_template_result->empty()) {
                    result.insert(result.end(), current_substitution.substitution->initial_data.contingent_trailing_content.begin(), current_substitution.substitution->initial_data.contingent_trailing_content.end());
                }
            }
                // filling a template
            else {

                if (current_substitution.substitution->comment) {
                    XL_TEMPLATE_LOG("Handled comment");
                } else {

//                    XL_TEMPLATE_LOG("created Substitution data for '{}' on the stack at {}",
//                                    current_substitution.substitution->get_name(), (void *) &current_substitution);

//                    XL_TEMPLATE_LOG("about to call provider() named '{}' at {} and inline_template: {}",
//                                    provider.get_name(), (void *) &provider,
//                                    (void *) current_substitution.substitution->final_data.inline_template.get());
                    auto substitution_result = provider(current_substitution);
                    
                    
                    
                    if (!substitution_result) {

                        XL_TEMPLATE_LOG("About to start rewind because: {}", substitution_result.error());

                        std::cerr << fmt::format("provider stack before rewind: {}\n", current_substitution.fill_state.provider_stack);

                        // don't rewind on the same provider that was just tried
                        bool rewound_through_rewind_point = false;
                        while(!current_substitution.fill_state.provider_stack.empty() &&
                              !rewound_through_rewind_point) {
                            if (current_substitution.fill_state.provider_stack.front()->is_rewind_point()) {
                                rewound_through_rewind_point = true;
                            }
                            current_substitution.fill_state.provider_stack.pop_front();
                        }                        


                        auto rewind_result = this->rewind_results(current_substitution);
                        if (!rewind_result) {
                            
                            substitution_result.error().append(rewind_result.error());
                            return xl::make_unexpected(error_list.append(substitution_result.error()));
                        } else {
                            substitution_result = rewind_result;
                        }
                    }
//                    XL_TEMPLATE_LOG("replacement for {} is: {}", this->source_template->c_str(), substitution_result);
                    XL_TEMPLATE_LOG("provider() named {} returned: '{}'", provider.get_name(), *substitution_result);
                    
                    
                    if (!current_substitution.substitution->initial_data.contingent_leading_content.empty() &&
                        !substitution_result->empty()) {
                        XL_TEMPLATE_LOG("adding contingent leading content: {}",
                                        current_substitution.substitution->initial_data.contingent_leading_content);

                        result.insert(result.end(),
                                      current_substitution.substitution->initial_data.contingent_leading_content.begin(),
                                      current_substitution.substitution->initial_data.contingent_leading_content.end());
                    }
                    result.insert(result.end(), substitution_result->begin(), substitution_result->end());
                    if (!current_substitution.substitution->initial_data.contingent_trailing_content.empty() &&
                        !substitution_result->empty()) {
                        XL_TEMPLATE_LOG("inserting contingent trailing content: {}",
                                        current_substitution.substitution->initial_data.contingent_trailing_content);
                        result.insert(result.end(),
                                      current_substitution.substitution->initial_data.contingent_trailing_content.begin(),
                                      current_substitution.substitution->initial_data.contingent_trailing_content.end());
                    }
                }
            }
        }
    }
    return result;
}



}
