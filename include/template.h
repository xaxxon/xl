
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

class TemplateException : public std::exception {
    std::string reason;

public:

    TemplateException(std::string const & reason) : reason(std::move(reason))
    {}

    const char* what() const noexcept {
        return reason.c_str();
    }
};


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



class Provider;

class ProviderOptions {
    Provider & provider;
    std::string parameters;

    ProviderOptions(Provider & provider, std::string const & parameters) :
        provider(provider),
        parameters(parameters)
    {}
};


class Stringable {
private:
    using StringableTypes = std::variant<
        std::string,
        std::function<std::string()>,
        std::vector<std::string>,
        Template
    >;



    StringableTypes stringable;
public:

    template<class T, class... Rest, template<class, class...> class Container>
    Stringable(Container<T, Rest...> && container) {
        this->stringable = xl::transform(container, [](T const & t) { return std::string(t); });
    }

    Stringable(Template t) : stringable(t) {}
    Stringable(std::string & string) : stringable(string) {}
    Stringable(std::string && string) : stringable(std::move(string)) {}
    Stringable(std::function<std::string()> callback) : stringable(callback) {}
    Stringable(char const * string) : stringable(string) {}

    std::string operator()(Provider & p) {
        if (auto string = std::get_if<std::string>(&stringable)) {
            return *string;
        } else if (auto callback = std::get_if<std::function<std::string()>>(&stringable)) {
            return (*callback)();
        } else if (auto vector = std::get_if<std::vector<std::string>>(&stringable)) {
            std::stringstream result;
            bool first = true;
            for (auto const & element : *vector) {
                std::cerr << fmt::format("looking at: {}", element) << std::endl;
                if (!first) {
                    result << ", ";
                }
                first = false;
                result << element;
            }
            return result.str();
        } else if (auto tmpl = std::get_if<Template>(&stringable)) {
            return tmpl->fill(p);
        } else {
            throw TemplateException("Unhandled stringable variant type");
        }
    }
};



class Provider {
public:

    // or any function that returns a FinalType
    using MapT = std::map<std::string, Stringable>;



private:
    MapT _providers;

public:
    Provider(MapT && providers = MapT{}) :
        _providers(std::move(providers))
    {}

    bool provides(std::string const & name) {
        return _providers.find(name) != _providers.end();
    }

    virtual std::string operator()(std::string const & name, std::string const & vector_concatentation_string) {
        if (auto i = _providers.find(name); i == std::end(_providers)) {
            throw TemplateException("Unknown name made it to actual lookup time - should have called provides");
        } else {
            return i->second(*this);
        }
    }

    Provider(Provider &&) = default;
    Provider(Provider const &) = default;

    Provider & operator=(Provider&&) = default;
    Provider & operator=(Provider const &) = default;
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

template<class T, class... Rest, template<class, class...> class Container>
struct MakeProvider<Container<T, Rest...>> {
    Provider p; // dummy
    Provider & operator()(Container<T, Rest...> && ) {
        return this->p;
    }
};


/**
 * Fills out `this` template and returns the result
 *
 * @tparam T Source type - must either be a Provider or something convertible to a Provider
 * @param source object to be used to populate `this` template
 * @return populated template result
 */
template<class T>
std::string Template::fill(T && source) {

    MakeProvider<T> make_provider{};
    auto & provider = make_provider(std::forward<T>(source));

    // must match:
    //   a string with no replacement
    //   a replacement with no other string
    //   a string followed by a replacement
    //   BUT NOT neither. (leading positive lookahead assertion makes sure string isn't empty)
    static std::regex r(
        // forward lookahead that the string has at least one character (not empty)
        "^(?=.)"

        // grab everything up until a first lone curly brace
        "((?:[^{}]|[{]{2}|[}]{2})*)"

        // grab a brace (potentially the wrong one which will be checked for later) and then a submatch of
        //   everything inside the braces excluding leading and trailing whitespace
        //   followed by a closing curly brace or potentially end-of-line in case of a no-closing-brace error
        // negative forward assertion to make sure the closing brace isn't escaped as }}
        "(?:([{}]?)\\s*((?:[^}{]|[}]{2}|[{]{2})*?)(?::((?:[^{}]|[}]{2}|[{]{2})*))?\\s*([}]|$)(?!\\}))?",

        std::regex::optimize
    );

    // submatch positions for the above regex
    enum RegexMatchIndex{
        WHOLE_STRiNG_INDEX = 0,
        LITERAL_STRING_INDEX,
        OPEN_BRACE_INDEX,
        REPLACEMENT_NAME_INDEX,
        REPLACEMENT_OPTIONS_INDEX,
        CLOSE_BRACE_INDEX,
    };

    std::stringstream result;

    std::cmatch matches;
    char const * remaining_template = this->_tmpl.c_str();
//    std::cerr << fmt::format("filling template: '{}'", this->_tmpl) << std::endl;

    while(std::regex_search(remaining_template, matches, r)) {

//        std::cerr << fmt::format("matching against: '{}'", remaining_template) << std::endl;
//        for(int i = 0; i < matches.size(); i++) {
//            std::cerr << fmt::format("match[{}]: {}", i, matches[i].str()) << std::endl;
//        }

        // if no substitution found, everything was a literal and is handled as a "trailing literal" outside
        //   this loop
        if (matches[OPEN_BRACE_INDEX].str() == "" && matches[CLOSE_BRACE_INDEX].str() == "") {
            break;
        }

        // check for open but no close or incorrect brace type
        if (matches[OPEN_BRACE_INDEX].str() != "{" || matches[CLOSE_BRACE_INDEX].str() != "}") {
            throw TemplateException("Found mismatched braces");
        }

        remaining_template = matches.suffix().first;
        result << matches[1];


        if (!provider.provides(matches[REPLACEMENT_NAME_INDEX])) {
            throw TemplateException(fmt::format("Provider doesn't provide value for name: '{}'", matches[REPLACEMENT_NAME_INDEX]));
        }

        if (matches[REPLACEMENT_OPTIONS_INDEX].str() != "") {
//            std::cerr << fmt::format("GOT REPLACEMENT OPTIONS: {}", matches[REPLACEMENT_OPTIONS_INDEX].str()) << std::endl;

        }
        std::string provider_data = provider(matches[REPLACEMENT_NAME_INDEX].str(), matches[REPLACEMENT_OPTIONS_INDEX].str());
        result << provider_data;

    }
//    std::cerr << fmt::format("putting on remaining template after loop exits: {}", remaining_template) << std::endl;
    result << remaining_template;

//    std::cerr << fmt::format("fill result: '{}'", result.str()) << std::endl;

    return result.str();
}


} // end namespace xl