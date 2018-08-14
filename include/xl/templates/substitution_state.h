#pragma once

#include "not_null.h"

#include "substitution.h"
#include "fill_state.h"


namespace xl::templates {

// full state created inside of Template::fill needed to be sent to the provider
struct SubstitutionState {

    FillState fill_state;

    Substitution const * substitution;
    
    not_null<CompiledTemplate const *> current_template;

    bool searching_provider_stack = false;

    SubstitutionState(CompiledTemplate const & tmpl, FillState const & fill_state, Substitution const * substitution) :
        fill_state(fill_state),
        substitution(substitution),
        current_template(&tmpl)
    {}

    
    /**
     * Gets the template associated with the Substitution object when combined with
     * the TemplateMap in this SubstitutionState object
     * @return template associated with the substitution in this SubstitutionState
     */
    inline Template const * get_template() {

        // initialize with inline_template then look up by name if no inline template
        Template const * result = this->substitution->final_data->inline_template.get();
        if (result != nullptr) {
            return result;
        }

        // if no inline template provided and no named template is requested, return empty template
        if (this->substitution->parameters.empty()) {
            return Template::empty_template.get();
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

        return &template_iterator->second;
   
        
    }

//    SubstitutionState(SubstititionState const &) = delete;
};

} // end namespace xl::templates