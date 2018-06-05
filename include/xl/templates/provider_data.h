#pragma once

#include <map>
#include <string>
#include <deque>
#include <optional>

#include "not_null.h"
#include "templates.h"
#include "fill_state.h"

namespace xl::templates {



class Template;
class Provider_Interface;

using TemplateMap = std::map<std::string, Template>;



/**
 * All the data needed to complete a specific replacement.  Initially created during Template compilation
 *   and then used each time the template is filled
 */
struct Substitution {
    
    /// name to look up in the provider for a replacement value
    std::deque<std::string> name_entries;
    
    /// string to put between multiple replacements of the same template via a container provider
    std::string join_string = "\n";

    /// any other data associated with the substitution
    std::string parameters = "";

    /// content preceeding the substitution which may or may not be shown
    std::string contingent_leading_content = "";

    /// content trailing the substitution which may or may not be shown
    std::string contingent_trailing_content = "";

    /// if true, then if the replacement is the empty string, the replacement will act like it never existed
    bool ignore_empty_replacements = false;

    /// if true, the join string will be placed in front of the first element of multiple replacements
    bool leading_join_string = false;

    /// whether this substitution is a comment
    bool comment = false;

    /// an inline template, if specified
    std::shared_ptr<Template> inline_template;

    // name to look up in template map to get template to insert in place - useful to insert long literal strings
    std::string template_name = "";
    
    Template const * tmpl = nullptr;

//    Substitution(TemplateMap const * templates) : templates(templates)
//    {}

    Substitution() {}

    Substitution(Template const & tmpl) :
        tmpl(&tmpl)
    {}
    
    Substitution(Substitution const &) = default;
    Substitution(Substitution &&) = default;

    ~Substitution(){
//        std::cerr << fmt::format("providerdata destructor for {}", (void*)this) << std::endl;
    }
    
//    /**
//     * semi-private function which takes a substitution with multiple name entries and 
//     * creates a parent substitution with the top-level name and a "pass-through" inline
//     * template calling the remaining substitution
//     * returns new substitution which should be used at the current position in the template
//     * and `this` is passed deeper into the generated "virtual" templates
//     */
//    Substitution split_substitution() {
//        // if there's nothing to split, this substitution is already the substitution
//        // which should be used 
//        if (name_entries.size() == 1) {
//            return std::move(*this);
//        }
//        
//        // create a new substitution
//        auto first_name_entry = name_entries.front();
//        this->name_entries.pop_front();
//        
//        
//        // if input looks like "x {{foo.bar.baz|x}} y"
//        // it should turn into:
//        // x {{foo|{{bar.baz|x}}}} y
//        // x {{foo|{{bar|{{baz|x}}}}}} y
//
//        Substitution new_current_substitution;
//        
//        // this template becomes the inline template of new_current_substitution
//        // and it has one substitution, which is the trimmed `this` Substitution
//        Template new_current_substitution_inline_template(*this);
//        
//        new_current_substitution_inline_template.compiled_substitutions.emplace_back(this->split_substitution());
//        
//        
//        return new_current_substitution;
//        
//    }
};


struct SubstitutionState {
    
    FillState fill_state;
    
    
    Substitution const * substitution;
    
    std::deque<std::string> name_entries;
    
    not_null<Template const *> current_template;
    
    bool searching_provider_stack = false;

    SubstitutionState(Template const & tmpl, FillState const & fill_state, Substitution const * substitution) :
        fill_state(fill_state),
        substitution(substitution),
        current_template(&tmpl)
    {
        name_entries = substitution->name_entries;
    }
    
//    SubstitutionState(SubstititionState const &) = delete;
};



} // end namespace xl