#pragma once

#include "../not_null.h"

#include "substitution.h"
#include "fill_state.h"

#include "../log.h"

namespace xl::templates {

// full state created inside of Template::fill needed to be sent to the provider
struct SubstitutionState {

    FillState fill_state;

    Substitution const * substitution;
    
    not_null<CompiledTemplate const *> current_template;


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
    
    CompiledTemplate const * get_template();
    

//    SubstitutionState(SubstititionState const &) = delete;
};

} // end namespace xl::templates
