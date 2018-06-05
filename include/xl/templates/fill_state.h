#pragma once

#include <deque>


namespace xl::templates {


class Provider_Interface;
class Template;

using ProviderStack = std::deque<Provider_Interface const *>;

struct FillState {
    // providers used to get to the current position
    ProviderStack provider_stack;

    /// map of template names to templates
    std::map<std::string, Template> const * templates = nullptr;

    FillState(){}
    FillState(ProviderStack provider_stack, std::map<std::string, Template> const * templates) :
        provider_stack(std::move(provider_stack)),
        templates(templates)
    {}
};

} // end namespace xl::templates