
#pragma once

#include <string_view>
#include <map>
#include <string>
#include <variant>
#include <sstream>
#include <regex>

#include "library_extensions.h"

namespace xl {

class Provider_Interface {
public:

    virtual std::string_view get_named(std::string_view name);
};


class Provider {
public:
    using MapT = std::map<std::string, std::variant<std::function<std::string()>, std::string>>;

private:
    MapT _providers;

public:
    Provider(MapT && providers = MapT()) :
        _providers(std::move(providers))
    {}

    virtual std::string_view operator()(std::string const & name) {
        if (auto i = _providers.find(name); i == std::end(_providers)) {
            return std::string_view();
        } else {
            if (auto callback = std::get_if<std::function<std::string()>>(&i->second)) {
                return (*callback)();
            } else if (auto string = std::get_if<std::string>(&i->second)) {
                return *string;
            }
            return std::string_view();
        }
    }
};


class Template {
private:
    std::string _tmpl;

public:
    Template(std::string const & tmpl = "") : _tmpl(tmpl) {}

    std::string fill(Provider && provider) {
        return fill(provider);
    }

    /**
     * Returns the value of the template filled in with values from `provider`
     * @param provider source of replacement values
     * @return filled out template
     */
    std::string fill(Provider & provider) {
        static std::regex r("^((?:[^{]|[{]{2}|[}]{2})+)(?:[{]((?:[^}{]|[}]{2}||[{]{2})+)[}](?!\\}))?");

        std::stringstream result;

        std::cmatch matches;
        char const * remaining_template = this->_tmpl.c_str();

        while(std::regex_search(remaining_template, matches, r)) {
            remaining_template = matches.suffix().first;

            result << matches[1];
            result << provider(matches[2].str());
        }
        result << remaining_template;
        return result.str();
    }
};


} // end namespace xl