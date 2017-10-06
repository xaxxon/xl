#pragma once

#include <map>
#include <string>


#include "exceptions.h"

namespace xl {

class Template;


using TemplateMap = std::map<std::string, Template>;

struct ProviderData {
    std::string name;
    TemplateMap const & templates;
    std::string const parameters;

    ProviderData(std::string const & name, TemplateMap const & templates = TemplateMap{}, std::string parameters = "") :
        name(name),
        templates(templates),
        parameters(parameters) {}
};


/**
 * A provider either directly provides content to fill a template or it contains a set of providers to fill a
 * template multiple times and concatenate the results
 */
class Provider_Interface {
public:
    virtual std::string operator()(ProviderData const & data) = 0;
};

class EmptyProvider : public Provider_Interface {
    std::string operator()(ProviderData const & data) override {
        throw TemplateException("EmptyProvider cannot provide content");
    }
};


} // end namespace xl