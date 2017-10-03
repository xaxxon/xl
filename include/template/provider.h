#pragma once

#include <string>
#include <map>
#include <type_traits>

#include "exceptions.h"


namespace xl {

class Provider;


struct ProviderOptions {
    std::string const parameters;

    ProviderOptions(std::string const & parameters = "") :
        parameters(parameters) {}
};



class Stringable {
private:
    using StringableTypes = std::variant<
        std::string,
        std::function<std::string()>>;

    StringableTypes stringable;
public:

    template<class T>
    Stringable(T && t) : stringable(std::forward<T>(t))
    {}


    std::string operator()(ProviderOptions & options) {
        return "BOGUS";
    }
};




class Provider;


class Provider_Interface {
public:
    virtual std::string operator()(std::string const & name, ProviderOptions && provider_options);
    virtual bool provides(std::string const & name);

};


class Provider : public Provider_Interface {
public:

    // or any function that returns a FinalType
    using MapT = std::map<std::string, Stringable>;


private:
    MapT _providers;

public:

    void set(std::string const & key, std::string const & value) {
        _providers.emplace(key, value);
    }

    void set(std::string const & key, std::function<std::string()> callback) {
        _providers.emplace(key, callback);
    }


    template<class... Keys, class... Values>
    Provider(std::pair<Keys, Values>&&... pairs) {
        (this->set(std::forward<Keys>(pairs.first), std::forward<Values>(pairs.second)),...);
    }

    bool provides(std::string const & name) override {
        return _providers.find(name) != _providers.end();
    }

    std::string operator()(std::string const & name, ProviderOptions && provider_options) override {
        if (auto i = _providers.find(name); i == std::end(_providers)) {
            throw TemplateException("Unknown name made it to actual lookup time - should have called provides");
        } else {
            return i->second(provider_options);
        }
    }

    Provider(Provider &&) = default;

    Provider(Provider const &) = default;

    Provider & operator=(Provider &&) = default;

    Provider & operator=(Provider const &) = default;
};




template<class, class = void>
struct MakeProvider;



/**
 * If the type has a method called get_provider that returns a unique_ptr<Provider>
 * @tparam T Type to check for a get_provider method
 */
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


    Provider & operator()(Container<T, Rest...> &&) {

    }
};

} // end namespace xl