#pragma once

#include <string>
#include <vector>


#include "substitution.h"
#include "substitution_state.h"

namespace xl::templates {


class CompiledTemplate {
    friend class Template;

XL_PRIVATE_UNLESS_TESTING:
    // static portions of the template as separated by substitutions
    std::vector<std::string> static_strings;

    // substitutions in the template
    std::vector<Substitution> substitutions;


    // sum of the length of all the fixed portions of the template
    mutable size_t minimum_result_length = 0;
    
    inline static std::shared_ptr<CompiledTemplate> empty_compiled_template = 
        Template::empty_template->compile();


public:

    Template const * const source_template;



    template <typename ProviderContainer = void>
    std::string fill(SubstitutionState &) const;
    
    template<typename ProviderContainer = void>
    std::string fill(FillState const &) const;

    template<typename ProviderContainer, class T, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, FillState>>>
    std::string fill(T && source, std::map<std::string, Template> template_map) const;




        CompiledTemplate(Template const * const source_template) :
        source_template(source_template) {}


    // create a template with a pre-built substitution instead of compiling
    //   the template from a string
    CompiledTemplate(Template const * const source_template, Substitution) :
        source_template(source_template) {}


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

    void add_substitution(Substitution substitution) {
        if (this->static_strings.size() <= this->substitutions.size()) {
            this->static_strings.push_back("");
        }
        this->substitutions.push_back(std::move(substitution));
    }

};

}

#include "templates.h"

namespace xl::templates {


template<typename ProviderContainer>
std::string CompiledTemplate::fill(FillState const & fill_state) const {

    assert(!fill_state.provider_stack.empty());
    Provider_Interface const & provider = *fill_state.provider_stack.front();

    std::string result{""};
//    XL_TEMPLATE_LOG("just created variable 'result': '{}'", result);
    result.reserve(this->minimum_result_length);


    for(size_t i = 0; i < this->static_strings.size(); i++) {

        result.insert(result.end(), this->static_strings[i].begin(), this->static_strings[i].end());
        XL_TEMPLATE_LOG(LogT::Subjects::Template, "fill: just added static section {}: '{}'", i, this->static_strings[i]);

        if (this->substitutions.size() > i) {
            SubstitutionState current_substitution(*this, fill_state, &this->substitutions[i]);


            XL_TEMPLATE_LOG("grabbed data for compiled_subsitution '{}' - it has name '{}' and inline template: '{}'",
                            i, current_substitution.substitution->get_name(), (void*)current_substitution.substitution->final_data.inline_template.get());
            current_substitution.current_template = this;
//            XL_TEMPLATE_LOG("substitution instantiation data.name: '{}'", current_substitution.substitution->get_name());

            // substituting another template in with {{!template_name}}
            if (current_substitution.substitution->final_data.template_name != "") {
                if (fill_state.templates->empty()) {
                    throw TemplateException("Cannot refer to another template if no other templates specified: " + current_substitution.substitution->final_data.template_name);
                }
                auto template_iterator = fill_state.templates->find(current_substitution.substitution->final_data.template_name);
                if (template_iterator == fill_state.templates->end()) {
                    throw TemplateException("No template found named: {}", current_substitution.substitution->final_data.template_name);
                }
                auto inline_template_result = template_iterator->second.fill(fill_state);
                if (!inline_template_result.empty()) {
                    result.insert(result.end(), current_substitution.substitution->initial_data.contingent_leading_content.begin(), current_substitution.substitution->initial_data.contingent_leading_content.end());
                }
                result.insert(result.end(), begin(inline_template_result), end(inline_template_result));
                if (!inline_template_result.empty()) {
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
//                    XL_TEMPLATE_LOG("replacement for {} is: {}", this->source_template->c_str(), substitution_result);
                    XL_TEMPLATE_LOG("provider() named {} returned: '{}'", provider.get_name(), substitution_result);
                    if (!current_substitution.substitution->initial_data.contingent_leading_content.empty() &&
                        !substitution_result.empty()) {
//                        XL_TEMPLATE_LOG("adding contingent data: {}",
//                                        current_substitution.substitution->initial_data.contingent_leading_content);

                        result.insert(result.end(),
                                      current_substitution.substitution->initial_data.contingent_leading_content.begin(),
                                      current_substitution.substitution->initial_data.contingent_leading_content.end());
                    }
                    result.insert(result.end(), substitution_result.begin(), substitution_result.end());
                    if (!current_substitution.substitution->initial_data.contingent_trailing_content.empty() &&
                        !substitution_result.empty()) {
//                        XL_TEMPLATE_LOG("inserting trailing content: {}",
//                                        current_substitution.substitution->initial_data.contingent_trailing_content);
                        result.insert(result.end(),
                                      current_substitution.substitution->initial_data.contingent_trailing_content.begin(),
                                      current_substitution.substitution->initial_data.contingent_trailing_content.end());
                    }
                }
            }
        }
    }
//    std::cerr << fmt::format("fillstate fill returning: {}\n", result);
    return result;
}



}