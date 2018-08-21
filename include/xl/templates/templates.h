
#pragma once

#include <string_view>
#include <map>
#include <string>
#include <variant>
#include <sstream>
#include <type_traits>
#include <functional>

#include <fmt/ostream.h>

#include "../regex/regexer.h"
#include "../library_extensions.h"


// All logging is compile-time disabled unless XL_TEMPLATE_LOG_ENABLE is specified
#if defined XL_TEMPLATE_LOG_ENABLE
#define XL_TEMPLATE_LOG(format_string, ...) \
    ::xl::templates::log.info(format_string, ##__VA_ARGS__);
#else
#define XL_TEMPLATE_LOG(format_string, ...)
#endif



namespace xl::templates {



struct Substitution;
struct SubstitutionState;
class Provider_Interface;
class CompiledTemplate;
struct FillState;

class Template {
XL_PRIVATE_UNLESS_TESTING:
    
    // full, unprocessed template string
    std::string _tmpl;
    
    mutable std::shared_ptr<CompiledTemplate> compiled_template;

public:
    
    explicit Template(std::string tmpl = "{{}}") : _tmpl(std::move(tmpl)) {}
    Template(Template const &) = default;

    inline char const * c_str() const { return this->_tmpl.c_str(); }

    
    template <typename ProviderContainer = void>
    std::string fill(SubstitutionState &) const;

    
    template <typename ProviderContainer = void>
    std::string fill(FillState const &) const;


    template<typename ProviderContainer = void, class T = char const *, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, FillState>>>
    std::string fill(T && source = "", std::map<std::string, Template> template_map = {}) const;

    
    
    // compiles the template for faster processing
    // benchmarks showed ~200x improvement when re-using templates 1000 times
    inline std::shared_ptr<CompiledTemplate> & compile() const;

    inline bool is_compiled() const;
    
    static inline std::unique_ptr<Template> empty_template = std::make_unique<Template>();
};




} // end namespace xl

// need to call directly to this .h file because circular dependencies make things tricky
#include "../log/log.h"

#include "provider.h"
#include "directory_loader.h"

namespace xl::templates {

class provider_data;


template <typename ProviderContainer>
std::string Template::fill(FillState const & fill_state) const {

    if (!this->is_compiled()) {
        this->compile();
    }
    
    return this->compiled_template->fill(fill_state);
}


template<typename ProviderContainer, class T, typename>
std::string Template::fill(T && source, std::map<std::string, Template> template_map) const {

    return this->compile()->fill<ProviderContainer>(std::forward<T>(source), std::move(template_map));
}


template<typename ProviderContainer, class T, typename>
std::string CompiledTemplate::fill(T && source, std::map<std::string, Template> template_map) const {

    XL_TEMPLATE_LOG("Filling template: '{}'", this->source_template->c_str());

    // used for storing the provider if necessary
    std::unique_ptr<Provider_Interface> provider_interface_unique_pointer;
    // used for consistent interface for assigning to reference later
    Provider_Interface * provider_interface_pointer;
    if constexpr(std::is_base_of_v<Provider_Interface, std::decay_t<T>>) {
        assert(false); // this shouldn't happen anymore?
        provider_interface_pointer = &source;
        assert(provider_interface_pointer != nullptr);

    } else if constexpr(std::is_same_v<ProviderPtr, std::decay_t<T>>) {
        provider_interface_pointer = source.get();
    } else {

        // need to store the unique_ptr to maintain object lifetime then assign to normal pointer
        //   so there is a common way to get the object below for assignment to reference type
        provider_interface_unique_pointer = DefaultProviders<ProviderContainer>::template make_provider(std::forward<T>(source));
//        XL_TEMPLATE_LOG("**** got unique ptr at {}", (void *) provider_interface_unique_pointer.get());
        provider_interface_pointer = provider_interface_unique_pointer.get();

        /**
         * IF THIS IS FIRING, PERHAPS YOU ARE USING AN RVALUE PROVIDER MULTIPLE TIMES
         */
        assert(provider_interface_pointer != nullptr);


//        XL_TEMPLATE_LOG("**** set provider interface pointer to {}", (void *) provider_interface_pointer);
    }

//    XL_TEMPLATE_LOG("outside: provider interface pointer to {}", (void *) provider_interface_pointer);

    
    ProviderPtr fillable_provider = {};
    if (!provider_interface_pointer->is_fillable_provider()) {
        fillable_provider = provider_interface_pointer->get_fillable_provider();
        provider_interface_pointer = fillable_provider.get();
    }

    ProviderStack provider_stack{provider_interface_pointer};
//    std::cerr << fmt::format("ps size: {}", provider_stack.size()) << std::endl;
    FillState fill_state(std::move(provider_stack), &template_map);
//    std::cerr << fmt::format("fill state provider stack size: {}", fill_state.provider_stack.size()) << std::endl;
    return fill<ProviderContainer>(fill_state);
}


template<typename ProviderContainer>
std::string Template::fill(SubstitutionState & substitution_state) const {
    return this->compile()->fill(substitution_state);
}
    

template<typename ProviderContainer>
std::string CompiledTemplate::fill(SubstitutionState & substitution_state) const {

        // if the top provider exists and wants the entire template as-is, provide it directly
    if (!substitution_state.fill_state.provider_stack.empty() && substitution_state.fill_state.provider_stack.front()->needs_raw_template()) {
        substitution_state.current_template = this;
        auto result = substitution_state.fill_state.provider_stack.front()->operator()(substitution_state);
//        std::cerr << fmt::format("substitution state fill if true returning: {}\n", result);
        return result;
    } else {
        auto result = this->fill(substitution_state.fill_state);
//        std::cerr << fmt::format("substitution state fill if false returning: {}\n", result);
        return result;
    }
}



bool Template::is_compiled() const {
//    return !this->static_strings.empty() || !this->substitutions.empty();
    return this->compiled_template.get() != nullptr;
}





} // end namespace xl


#include "substitution_impl.h"