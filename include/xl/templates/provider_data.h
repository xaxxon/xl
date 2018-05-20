#pragma once

#include <map>
#include <string>
#include <deque>
#include <optional>

namespace xl::templates {



class Template;
class Provider_Interface;

using TemplateMap = std::map<std::string, Template>;
using ProviderStack = std::deque<Provider_Interface const *>;


struct JustForABreakPoint {
    JustForABreakPoint(){}
    JustForABreakPoint(JustForABreakPoint const &) {
        std::cerr << fmt::format("blah") << std::endl;
    }
    JustForABreakPoint(JustForABreakPoint&&) = default;
};
//
//
//struct ProviderStack {
//    std::deque<Provider_Interface const *> providers;
//    
//    ProviderStack(ProviderStack const & other) {
//        
//    }
//};

/**
 * All the data needed to complete a specific replacement.  Initially created during Template compilation
 *   and then used each time the template is filled
 */
struct Substitution {
    
    /// name to look up in the provider for a replacement value
    std::vector<std::string> name_entries;
    
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
    
    Template const & tmpl;

//    Substitution(TemplateMap const * templates) : templates(templates)
//    {}


    Substitution(Template const & tmpl) :
        tmpl(tmpl)
    {}
    
    Substitution(Substitution const &) = default;
    Substitution(Substitution &&) = default;

    ~Substitution(){
//        std::cerr << fmt::format("providerdata destructor for {}", (void*)this) << std::endl;
    }
};


struct FillState {
    // providers used to get to the current position
    ProviderStack provider_stack;

    /// map of template names to templates
    TemplateMap const * templates = nullptr;
    
    FillState(){}
    FillState(ProviderStack provider_stack, TemplateMap const * templates) :
        provider_stack(std::move(provider_stack)),
        templates(templates)
    {}

};

struct SubstitutionState {
    
    FillState fill_state;
    
    Substitution const * substitution;
    

    

    std::vector<std::string> name_entries;
    
    
    Template const * current_template = nullptr;
    
    bool searching_provider_stack = false;

    SubstitutionState(FillState const & fill_state, Substitution const * substitution) :
        fill_state(fill_state),
        substitution(substitution)
    {
        name_entries = substitution->name_entries;
    }
    
    
    
    
//    SubstitutionState(SubstititionState const &) = delete;
};




} // end namespace xl