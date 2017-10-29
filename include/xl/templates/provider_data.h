#pragma once

#include <map>
#include <string>
#include <optional>

#include "exceptions.h"
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

    /// content which may or may not be shown
    std::string contingent_leading_content = "";

    /// if true, then if the replcaement is the empty string, the replacement will act like it never existed
    bool ignore_empty_replacements = false;

    /// an inline template, if specified
    std::optional<Template> inline_template;

    // name to look up in template map to get template to insert in place - useful to insert long literal strings
    std::string template_name = "";

    /// stores the current template being processed - useful for debugging information if something goes wrong in provider
    Template const * current_template = nullptr;


    ProviderData() {
//        std::cerr << fmt::format("Created ProviderData at {}", (void*)this) << std::endl;
    }


    ~ProviderData(){
//        std::cerr << fmt::format("providerdata destructor for {}", (void*)this) << std::endl;
    }


    ProviderData(ProviderData const &) = default;
};


/**
 * A provider either directly provides content to fill a template or it contains a set of providers to fill a
 * template multiple times and concatenate the results
 */
class Provider_Interface {
public:
    virtual std::string operator()(ProviderData const & data) = 0;

    /**
     * A string useful for a human to figure out what Provider this is
     * @return Returns a human-readable, useful name for the type of the provider
     */
    virtual std::string get_name() const = 0;
};

class EmptyProvider : public Provider_Interface {
    std::string operator()(ProviderData const & data) override {
        throw TemplateException("EmptyProvider cannot provide content");
    }
};

using ProviderPtr = std::unique_ptr<Provider_Interface>;



template<class, class = void>
struct is_passthrough_provider : public std::false_type {};

template<class T>
struct is_passthrough_provider<T, std::enable_if_t<std::is_same_v<void, std::void_t<typename std::decay_t<T>::XL_TEMPLATES_PASSTHROUGH_TYPE>>>> :
    public std::true_type {};

template<class T>
constexpr bool is_passthrough_provider_v = is_passthrough_provider<T>::value;


} // end namespace xl