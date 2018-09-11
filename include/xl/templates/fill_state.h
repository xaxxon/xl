#pragma once

#include <deque>


namespace xl::templates {


class Provider_Interface;
class Template;

using ProviderStack = std::deque<Provider_Interface const *>;
inline std::ostream & operator<<(std::ostream & os, ProviderStack const & provider_stack) {
    os << "Provider stack:\n";
    for (auto const & provider : provider_stack) {
        os << "\t" << provider->get_name() << "\n";
    }
    return os;
}

// state data needed to begin a call to Template::fill
struct FillState {
    // providers used to get to the current position
    ProviderStack provider_stack;

    /// map of template names to templates
    std::map<std::string, Template> const * templates = nullptr;

    bool searching_provider_stack = false;


    FillState(){}
    FillState(ProviderStack provider_stack, std::map<std::string, Template> const * templates) :
        provider_stack(std::move(provider_stack)),
        templates(templates)
    {}
    
    
};

} // end namespace xl::templates