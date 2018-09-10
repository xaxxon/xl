#pragma once

#include <map>
#include <string>
#include <deque>
#include <optional>

#include "exceptions.h"

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
        std::string contingent_leading_content = "";

        /// content trailing the substitution which may or may not be shown
        std::string contingent_trailing_content = ""; 
        
        // how many providers to back up by to start searching for this substitution name
        unsigned short rewind_provider_count = 0;
        
        mutable bool rewound = false;
    };
    
    struct FinalSplit {
        /// an inline template, if specified
        std::shared_ptr<CompiledTemplate> inline_template;

        // name to look up in template map to get template to insert in place - useful to insert long literal strings
        std::string template_name = ""; 
    };
    
    struct Shared {
        /// string to put between multiple replacements of the same template via a container provider
        std::string join_string = "\n"; 

        /// if true, then if the replacement is the empty string, the replacement will act like it never existed
        bool ignore_empty_replacements = false; 

        /// if true, the join string will be placed in front of the first element of multiple replacements
        bool leading_join_string = false; 
    };
    
    std::string raw_text;
    
    std::shared_ptr<Shared> shared_data = std::make_shared<Shared>();
    InitialSplit initial_data;
    FinalSplit final_data;
    
    /// name to look up in the provider for a replacement value
    std::deque<std::string> name_entries; // must only have one entry when object fully constructed
    

    /// any other data associated with the substitution
    std::string parameters = ""; // ??

    /// whether this substitution is a comment
    bool comment = false; // cannot be split


    // is this substitution a spot to start a search during a 'rewind' or not?
    // basically every substitution except ones created by a name1.name2 name construct
    Substitution * parent_substitution = nullptr;

    std::string const & get_name(bool required = false) const;
    
    Template const * tmpl = nullptr;

//    Substitution(TemplateMap const * templates) : templates(templates)
//    {}

    Substitution() {
        std::cerr << fmt::format("creating empty substitution\n");
    }

    Substitution(Template const & tmpl) :
        tmpl(&tmpl)
    {}
    
    Substitution(Substitution const &) = default;
    Substitution(Substitution &&) = default;

    ~Substitution(){
//        std::cerr << fmt::format("providerdata destructor for {}", (void*)this) << std::endl;
    }

    
    void split();
    
};

inline std::ostream & operator<<(std::ostream & ostream, Substitution const & substitution) {
    ostream << substitution.raw_text;
    return ostream;
}



} // end namespace xl