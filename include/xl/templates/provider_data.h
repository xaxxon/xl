#pragma once

#include <map>
#include <string>
#include <optional>

#include "exceptions.h"
namespace xl::templates {

class Template;


using TemplateMap = std::map<std::string, Template>;

struct ProviderData {

    /// name to look up in the provider for a replacement value
    std::string name;

    /// map of template names to templates
    TemplateMap const * templates;

    /// any other data associated with the substitution
    std::string parameters;

    bool ignore_empty_replacements = false;

    /// an inline template, if specified
    std::optional<Template> inline_template;
    std::string template_name;

    ProviderData(std::string const & name,
                 TemplateMap const * templates = nullptr,
                 std::string parameters = "",
                 std::optional<Template> inline_template = std::optional<Template>{},
                 std::string template_name = "") :
        name(name),
        templates(templates),
        parameters(parameters),
        inline_template(inline_template),
        template_name(template_name)
    {}

    ProviderData(ProviderData const &) = default;

    ProviderData && set_ignore_empty_replacements(bool ignore) && {
        this->ignore_empty_replacements = ignore;
        return std::move(*this);
    }

    ProviderData & set_ignore_empty_replacements(bool ignore) & {
        this->ignore_empty_replacements = ignore;
        return *this;
    }

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