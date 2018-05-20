#pragma once

#include <string>
#include "provider_data.h"
#include "exceptions.h"


namespace xl::templates {

class Provider_Interaface;

using ProviderPtr = std::unique_ptr<Provider_Interface>;


/**
 * A provider either directly provides content to fill a template or it contains a set of providers to fill a
 * template multiple times and concatenate the results
 */
class Provider_Interface {
public:
    
    virtual ~Provider_Interface() = default;
    
    virtual std::string operator()(SubstitutionState & data) const = 0;

    /**
     * A string useful for a human to figure out what Provider this is
     * @return Returns a human-readable, useful name for the type of the provider
     */
    virtual std::string get_name() const = 0;

    /**
     * Whether this provider can
     * @return
     */
    virtual bool provides_named_lookup() {return false;}


    virtual ProviderPtr get_named_provider(SubstitutionState &) {
        throw TemplateException("Provider does not support get_named_provider call");
    };

    virtual bool is_fillable_provider() {
        return false;
    }

    virtual ProviderPtr get_fillable_provider() {
        throw TemplateException("Called get_fillable_provider on Provider which doesn't override it");
    }


    // is this provider something that is fillable and does not consume a template (basically just container provider)
    virtual bool is_template_passthrough() const {
        return false;
    }

};



template<class, class = void>
struct is_passthrough_provider : public std::false_type {};

/**
 * @cond HIDDEN_SYMBOLS
 * @tparam T
 */
template<class T>
struct is_passthrough_provider<T, std::enable_if_t<std::is_same_v<void, std::void_t<typename std::decay_t<T>::XL_TEMPLATES_PASSTHROUGH_TYPE>>>> :
    public std::true_type {};
/// @endcond

template<class T>
constexpr bool is_passthrough_provider_v = is_passthrough_provider<T>::value;


} // end namespace xl::templates
