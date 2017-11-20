#pragma once

#include <map>
#include <string>
#include <optional>

namespace xl::templates {

class Template;


using TemplateMap = std::map<std::string, Template>;


/**
 * All the data needed to complete a specific replacement.  Initially created during Template compilation
 *   and then used each time the template is filled
 */
struct ProviderData {

    /// name to look up in the provider for a replacement value
    std::string name = "";

    /// string to put between multiple replcaements of the same template via a container provider
    std::string join_string = "\n";

    /// map of template names to templates
    TemplateMap const * templates = nullptr;

    /// any other data associated with the substitution
    std::string parameters = "";

    /// content preceeding the substitution which may or may not be shown
    std::string contingent_leading_content = "";

    /// content trailing the substitution which may or may not be shown
    std::string contingent_trailing_content = "";

    /// if true, then if the replcaement is the empty string, the replacement will act like it never existed
    bool ignore_empty_replacements = false;

    /// if true, the join string will be placed in front of the first element of multiple replacements
    bool leading_join_string = false;

    bool comment = false;

    /// an inline template, if specified
    std::shared_ptr<Template> inline_template;

    // name to look up in template map to get template to insert in place - useful to insert long literal strings
    std::string template_name = "";

    /// stores the current template being processed - useful for debugging information if something goes wrong in provider
    Template const * current_template = nullptr;


    ProviderData(TemplateMap const * templates) : templates(templates)
    {}


    ProviderData() {
//        std::cerr << fmt::format("Created ProviderData at {}", (void*)this) << std::endl;
    }


    ~ProviderData(){
//        std::cerr << fmt::format("providerdata destructor for {}", (void*)this) << std::endl;
    }


    ProviderData(ProviderData const &) = default;
};




} // end namespace xl