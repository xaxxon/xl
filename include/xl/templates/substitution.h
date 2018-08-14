#pragma once

#include <map>
#include <string>
#include <deque>
#include <optional>

namespace xl::templates {

class Template;
class CompiledTemplate;
class Provider_Interface;

using TemplateMap = std::map<std::string, Template>;



/**
 * All the data needed to complete a specific replacement.  Initially created during Template compilation
 *   and then used each time the template is filled
 */
struct Substitution {
    
    struct InitialSplit {
        /// content preceeding the substitution which may or may not be shown
        std::string contingent_leading_content = ""; // only on first name/split

        /// content trailing the substitution which may or may not be shown
        std::string contingent_trailing_content = ""; // only on first name/split
    };
    
    struct FinalSplit {
        /// an inline template, if specified
        std::shared_ptr<Template> inline_template; // only on last/final split

        // name to look up in template map to get template to insert in place - useful to insert long literal strings
        std::string template_name = ""; // only on last/final split
    };
    
    struct Shared {
        /// string to put between multiple replacements of the same template via a container provider
        std::string join_string = "\n"; // same across all splits

        /// if true, then if the replacement is the empty string, the replacement will act like it never existed
        bool ignore_empty_replacements = false; // same across all splits

        /// if true, the join string will be placed in front of the first element of multiple replacements
        bool leading_join_string = false; // same across all splits
    };
    
    std::shared_ptr<Shared> shared_data = std::make_shared<Shared>();
    std::unique_ptr<InitialSplit> initial_data = std::make_unique<InitialSplit>();
    std::unique_ptr<FinalSplit> final_data = std::make_unique<FinalSplit>();
    
    /// name to look up in the provider for a replacement value
    std::deque<std::string> name_entries; // must only have one entry when object fully constructed
    

    /// any other data associated with the substitution
    std::string parameters = ""; // ??

    /// whether this substitution is a comment
    bool comment = false; // cannot be split
    

    std::string const & get_name(bool required = false) const {
        
        if (this->name_entries.size() == 0) {
            if (!this->final_data->template_name.empty()) {
                return this->final_data->template_name;
            } else {
                static std::string bogus = "BOGUS";

                if (required) {
                    assert(this->name_entries.size() == 1);
                    return bogus;
                } else {
                    return bogus;
                }
            }
        } else {
            return this->name_entries.front();
        }
    }
    
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
    Substitution split_substitution();
};



} // end namespace xl