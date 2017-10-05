#pragma once

#include <string>
#include <map>
#include <type_traits>
#include <experimental/coroutine>
#include <library_extensions.h>

#include "exceptions.h"
#include "template.h"

namespace xl {


class ProviderOptions;


template<class, class = void>
struct MakeProvider;



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
        if (auto string = std::get_if<std::string>(&this->stringable)) {
            return *string;
        } else if (auto callback = std::get_if<std::function<std::string()>>(&this->stringable)) {
            return (*callback)();
        } else {
            throw TemplateException("Unhandled variant type in Stringable");
        }
    }
};



/**
 * Three categories:
 * list providing sublists
 * list providing leaf providers
 * leaf provider
 */

/**
 * A provider either directly provides content to fill a template or it contains a set of providers to fill a
 * template multiple times and concatenate the results
 */
class Provider_Internal {
public:
    virtual std::string operator()(Template const & tmpl, ProviderOptions const & provider_options) = 0;
};

class Provider_Leaf {
public:
    virtual std::string operator()(std::string const & name, ProviderOptions const & provider_options) = 0;
};




class EmptyProvider : public Provider_Leaf {
    std::string operator()(std::string const & name, ProviderOptions const & provider_options) {
        throw TemplateException("EmptyProvider tried to provide value, but cannot");
    }

};


template<class T, class = void>
class Provider;


template<class T>
class Provider<T, std::enable_if_t<xl::is_range_for_loop_able_v<T>>> : public Provider_Interface {

private:
    T t;
public:

    using ChildProviderT = MakeProvider<T>()
    constexpr bool has_leafs = std::is_base_of<

    Provider(T && t) : t(std::forward<T>(t)) {}

    std::string operator()(Template const & tmpl, ProviderOptions const & provider_options) override {
        std::stringstream result;
        for (auto const & element : t) {
            Provider p = MakeProvider<std::decay_t<decltype(element)>>()(element);
            result << p(tmpl, provider_options);
        }
        return result.str();
    }
};





/**
 * If the type has a method called get_provider that returns a unique_ptr<Provider>
 * @tparam T Type to check for a get_provider method
 */
template<class T>
struct MakeProvider<T, std::enable_if_t<std::is_same_v<std::unique_ptr<xl::Provider_Interface>, decltype(std::declval<T>().get_provider())> > > {
private:
    std::unique_ptr<Provider_Interface> provider;

public:
    static constexpr bool is_leaf = true;
    Provider_Interface & operator()(std::remove_reference_t<T> const & t) {
        this->provider = t.get_provider();
        return *this->provider;
    }
    Provider_Interface & operator()(std::remove_reference_t<T> & t) {
        this->provider = t.get_provider();
        return *this->provider;
    }

    Provider_Interface & operator()(std::remove_reference_t<T> && t) {
        return this->operator()(t);
    }
};


template<class T>
struct MakeProvider<Provider<T>> : public Provider_Leaf {
    Provider<T> stored_p;

    static constexpr bool is_leaf = true;

    Provider<T> & operator()(Provider<T> & p) {
        stored_p = p;
        return stored_p;
    }
    Provider<T> & operator()(Provider<T> && p) {
        this->stored_p = std::move(p);
        return this->stored_p;
    }
};



} // end namespace xl