
#pragma once

#include <string_view>
#include <map>
#include <string>
#include <variant>
#include <sstream>
#include <regex>
#include <type_traits>
#include <functional>

#include <fmt/ostream.h>

#include "library_extensions.h"

namespace xl {

class Provider_Interface {
public:

    virtual std::string_view get_named(std::string_view name);
};



class Provider;


class Template {
private:
    std::string _tmpl;

public:
    Template(std::string const & tmpl = "") : _tmpl(tmpl) {}


    template<class T>
    std::string fill(T && source);
};


template<class, class = void>
struct MakeProvider;


class Provider {
public:
    using MapT = std::map<std::string, std::variant<std::function<std::string()>, std::string, Template>>;

private:
    MapT _providers;

public:
    Provider(MapT && providers = MapT()) :
        _providers(std::move(providers))
    {}

    virtual std::string operator()(std::string const & name) {
        if (auto i = _providers.find(name); i == std::end(_providers)) {
            return std::string();
        } else {
            if (auto callback = std::get_if<std::function<std::string()>>(&i->second)) {
                // callback
                auto callback_result = (*callback)();
//                std::cerr << fmt::format("callback result: {}", callback_result) << std::endl;
                return callback_result;
            } else if (auto string = std::get_if<std::string>(&i->second)) {
                // plain string
                return *string;
            } else if (auto tmpl = std::get_if<Template>(&i->second)) {
                // template
//                std::cerr << fmt::format("handling provider providing a nested template") << std::endl;
                return tmpl->fill(*this);
            } else {
                // unhandled variant type
                assert(false);
            }
            return std::string();
        }
    }
};

template<class T>
struct MakeProvider<T, std::enable_if_t<std::is_same_v<std::unique_ptr<xl::Provider>, decltype(std::declval<T>().get_provider())> > > {
private:
    std::unique_ptr<Provider> provider;

public:
    Provider & operator()(std::remove_reference_t<T> & t) {
        this->provider = t.get_provider();
        return *this->provider;
    }
    Provider & operator()(std::remove_reference_t<T> && t) {
        return this->operator()(t);
    }

};

template<>
struct MakeProvider<Provider &> {
    Provider stored_p;
    Provider & operator()(Provider & p) {
        stored_p = p;
        return stored_p;
    }
};

template<>
struct MakeProvider<Provider> {
    Provider stored_p;
    Provider & operator()(Provider && p) {
        this->stored_p = std::move(p);
        return this->stored_p;
    }
};


template<class T>
std::string Template::fill(T && source) {

    MakeProvider<T> make_provider{};
    auto & provider = make_provider(std::forward<T>(source));

    // must match:
    //   a string with no replacement
    //   a replacement with no other string
    //   a string followed by a replacement
    //   BUT NOT neither. -- CURRENT REGEX MATCHES NEITHER AND IT MUST BE DISQUALIFIED IN WHILE LOOP BODY
    static std::regex r("^((?:[^{]|[{]{2}|[}]{2})*)[{]\\s*((?:[^}{]|[}]{2}|[{]{2})+?)\\s*[}](?!\\})");

    std::stringstream result;

    std::cmatch matches;
    char const * remaining_template = this->_tmpl.c_str();
//    std::cerr << fmt::format("filling template: '{}'", this->_tmpl) << std::endl;

    while(std::regex_search(remaining_template, matches, r)) {
        if (matches[1].first == matches[2].second) {
//            std::cerr << fmt::format("breaking from loop because both submatches == \"\"") << std::endl;
            break;
        }
        remaining_template = matches.suffix().first;
//        std::cerr << fmt::format("adding matches 1 '{}' and 2 '{}'", matches[1].str(), matches[2].str()) << std::endl;
        result << matches[1];
        std::string provider_data = provider(matches[2].str());
//        std::cerr << fmt::format("provider data: '{}'", provider_data) << std::endl;
        result << provider_data;
//        std::cerr << fmt::format("remaining template: '{}'", remaining_template) << std::endl;
//        std::cerr << fmt::format("partial result: '{}'", result.str()) << std::endl;

    }
//    std::cerr << fmt::format("putting on remaining template after loop exits: {}", remaining_template) << std::endl;
    result << remaining_template;
//    std::cerr << fmt::format("fill result: '{}'", result.str()) << std::endl;
    return result.str();
}


} // end namespace xl