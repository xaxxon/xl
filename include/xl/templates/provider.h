#pragma once

#include <string>
#include <map>
#include <type_traits>
#include <iostream>

#include "../library_extensions.h"
#include "../magic_ptr.h"

#include "exceptions.h"
#include "template.h"
#include "provider_data.h"
namespace xl::templates {


template<class T, class=void>
class Provider {
    Provider(T&&);
    Provider(T&);
};



template<class... Keys, class... Values>
Provider(std::pair<Keys, Values>&&...) -> Provider<std::map<std::string, std::unique_ptr<Provider_Interface>>>;



template<class, class = void>
class is_provider_type : public std::false_type {};

template<class T>
class is_provider_type<T, std::enable_if_t<std::is_same_v<
    std::string,
    std::result_of_t<Provider<std::remove_reference_t<T>>(ProviderData const &)>
> // is same
> // enable_if

> : public std::true_type {};

template<class T>
constexpr bool is_provider_type_v = is_provider_type<T>::value;


template<class T, class = void>
class is_provider_callback_type : public std::false_type {};

template<class T>
class is_provider_callback_type<T, std::enable_if_t<
    is_provider_type_v<
        std::result_of_t<
            T()
                > // result_of
            > // is_provider_type
    > // enable_if
> : public std::true_type {};

template<class T>
constexpr bool is_provider_callback_type_v = is_provider_callback_type<T>::value;


template<class, class = void>
struct can_get_provider_for_type : public std::false_type {};

template<class, class = void>
struct has_get_provider_member : public std::false_type {};
template<class T>
struct has_get_provider_member<T, std::enable_if_t<std::is_same_v<
    std::unique_ptr<Provider_Interface>,
    decltype(std::declval<T>().get_provider())>>> : public std::true_type {};

template<class T>
constexpr bool has_get_provider_member_v = has_get_provider_member<T>::value;


template<class, class = void>
struct has_get_provider_free_function : public std::false_type {};

template<class T>
struct has_get_provider_free_function<T, std::enable_if_t<std::is_same_v<
    std::unique_ptr<Provider_Interface>,
    decltype(get_provider(std::declval<T>()))> // end is_same
> // end enable_if
> : public std::true_type {};

template<class T>
constexpr bool has_get_provider_free_function_v = has_get_provider_free_function<T>::value;

template<class T>
struct can_get_provider_for_type<T, std::enable_if_t<
    std::disjunction_v< // logical OR
        has_get_provider_member<T>,
        has_get_provider_free_function<T>
    > // end disjunction
> // end enable_if
        > : public std::true_type {};


template<class T>
constexpr bool can_get_provider_for_type_v = can_get_provider_for_type<T>::value;

template<class T>
class Provider<T, std::enable_if_t<std::is_convertible_v<T, std::string>>> : public Provider_Interface {
    std::string string;

public:
    Provider(T string) : string(std::move(string))
    {}


    std::string operator()(ProviderData const & data) override {
        return this->string;
    }
};



template<class... Keys, class... Values, std::enable_if_t<!(sizeof...(Keys) <= 1), int> = 0>
std::unique_ptr<Provider_Interface> make_provider(std::pair<Keys, Values>&&... pairs) {
    return std::unique_ptr<Provider_Interface>(new Provider(std::forward<std::pair<Keys, Values>&&>(pairs)...));
}


// Is T callable and returns a type which can be made into a Provider?
template<class T>
class Provider<T, std::enable_if_t<is_provider_callback_type_v<T>>> : public Provider_Interface {

    T callback;

    using CallbackResultT = std::remove_reference_t<std::result_of_t<T()>>;

public:
    Provider(T && callback) : callback(std::move(callback))
    {}
    Provider(T & callback) : callback(callback)
    {}


    std::string operator()(ProviderData const & data) override {
        return Provider<CallbackResultT>(this->callback())(data);
    }
};



template<class T>
std::unique_ptr<Provider_Interface> make_provider(T && t) {
    if constexpr(std::is_same_v<std::remove_reference_t<T>, std::unique_ptr<Provider_Interface>>) {
        return std::move(t);
    } else {
        return std::unique_ptr<Provider_Interface>(new Provider(std::forward<T>(t)));
    }
}

template<class R, class... Args, std::enable_if_t<is_provider_type_v<R>, int> = 0>
std::unique_ptr<Provider_Interface> make_provider(R(*f)(Args...)) {
    return std::unique_ptr<Provider_Interface>(new Provider(std::function<R(Args...)>(f)));
};



// Does T have a get_provider method on it?
template<class T>
class Provider<T, std::enable_if_t<can_get_provider_for_type_v<T>>> : public Provider_Interface {

private:
    xl::magic_ptr<T> t;

public:

    using XL_TEMPLATES_PASSTHROUGH_TYPE = T;

    Provider(T && t) : t(std::move(t)) {}
    Provider(T & t) : t(t) {}



    std::string operator()(ProviderData const & data) override {
        std::unique_ptr<Provider_Interface> provider = get_underlying_provider();



        if (data.inline_template) {
            return data.inline_template->fill(provider);
        } else {
            return provider->operator()(data);
        }
    }

    auto get_underlying_provider(){
        if constexpr(has_get_provider_free_function_v<T>){
            return get_provider(*t);
        } else if constexpr(has_get_provider_member_v<T>) {
            return t->get_provider();
        } else {
            throw xl::templates::TemplateException("this shouldn't happen");
        }
    }
};


template<class T, class Deleter>
class Provider<std::unique_ptr<T, Deleter>> : public Provider_Interface {
    T & t;
public:
    using XL_TEMPLATES_PASSTHROUGH_TYPE = T;

    Provider(std::unique_ptr<T, Deleter> & t) :
        t(*t)
    {}

    std::string operator()(ProviderData const & data) {
        return make_provider(t)->operator()(data);
    }

    auto get_underlying_provider(){
        return Provider<T>(t);
    }
};

static_assert(is_passthrough_provider_v<Provider<std::unique_ptr<int>>>);

template<class T, class Deleter>
class Provider<std::unique_ptr<T, Deleter> const> : public Provider_Interface {
    T const & t;
public:

    using XL_TEMPLATES_PASSTHROUGH_TYPE = T;

    Provider(std::unique_ptr<T, Deleter> const & t) :
        t(*t)
    {}

    std::string operator()(ProviderData const & data) {
        return make_provider(t)->operator()(data);
    }
    auto get_underlying_provider(){
        return Provider<T const>(t);
    }

};

static_assert(is_passthrough_provider_v<Provider<std::unique_ptr<int> const>>);



template<class T>
class Provider<T, std::enable_if_t<xl::is_range_for_loop_able_v<T> && !std::is_convertible_v<T, std::string>>> : public Provider_Interface {

private:
    magic_ptr<std::remove_reference_t<T>> t;
public:

    Provider(T & t) : t(t) {}
    Provider(T && t) : t(std::move(t)) {}

    std::string operator()(ProviderData const & data) override {
//        std::cerr << fmt::format("container provider looking at substution data for: {}, {}", data.name, (bool)data.inline_template) << std::endl;
        std::stringstream result;

        // find an optional join string if specified
        static std::regex parameters_regex("^([^|]*)[|]?(.*)$");
        enum ParameterRegexIndex { EVERYTHING = 0, TEMPLATE_NAME_INDEX, JOIN_STRING_INDEX };

        std::smatch matches;
        if (!std::regex_match(data.parameters, matches, parameters_regex)) {
            throw TemplateException(fmt::format("Unknown template parameters for container Provider: '{}'", data.parameters));
        }
//        for(int i = 0; i < matches.size(); i++) {
//            std::cerr << fmt::format("parameter regex[{}]: '{}'", i, matches[i].str()) << std::endl;
//        }

//        std::cerr << fmt::format("inline template exists? {}", (bool)data.inline_template) << std::endl;
        Template const & tmpl = [&]{
            if (data.inline_template) {
                return *data.inline_template;
            } else {
//        std::cerr << fmt::format("looking up template named: '{}'", matches[TEMPLATE_NAME_INDEX].str()) << std::endl;
//        std::cerr << fmt::format("join string: '{}'", join_string) << std::endl;
                auto template_iterator = data.templates->find(matches[TEMPLATE_NAME_INDEX].str());
                if (template_iterator == data.templates->end()) {
                    if (data.templates->empty()) {
                        throw TemplateException(
                            "ContainerProvider received empty template map so it can't possibly find a template for its members" + data.name);
                    }
                    throw TemplateException(
                        fmt::format("ContainerProvider couldn't find template named: '{}'", data.parameters));
                }
                return template_iterator->second;
            }
        }();

        std::string join_string = matches[JOIN_STRING_INDEX].str() != "" ? matches[JOIN_STRING_INDEX].str() : "\n";
        bool needs_join_string = false;

        // Iterate through the container
        for (auto & element : *t) {
            auto p = Provider<std::remove_reference_t<decltype(element)>>(element);

            if (needs_join_string) {
                result << join_string;
//                std::cerr << fmt::format("inserting join string '{}' on subsequent pass", join_string) << std::endl;

            } else {
//                std::cerr << fmt::format("skipping join string '{}' on first pass", join_string) << std::endl;

            }



            needs_join_string = true;
            auto fill_result = tmpl.fill(p, *data.templates);

//            std::cerr << fmt::format("replacement is {}, ignore is {}", fill_result, data.ignore_empty_replacements) << std::endl;
            if (fill_result == "" && data.ignore_empty_replacements) {
                needs_join_string = false;
            }
            result << fill_result;

        }
        return result.str();
    }
};




template<class Value>
class Provider<std::map<std::string, Value>, std::enable_if_t<xl::is_range_for_loop_able_v<std::map<std::string, Value>> && !std::is_convertible_v<std::map<std::string, Value>, std::string>>> : public Provider_Interface {
public:

    std::map<std::string, Value> map;

    template<class... Keys, class... Values>
    Provider(std::pair<Keys, Values> && ... pairs) {
        (this->map.emplace(std::pair<std::string, std::unique_ptr<Provider_Interface>>(std::move(pairs.first), make_provider(pairs.second))),...);
//        std::cerr << fmt::format("done adding pairs to map") << std::endl;
//        std::cerr << fmt::format("map size: {}", this->map.size()) << std::endl;
//        for(auto const & [a,b] : this->map) {
//            std::cerr << fmt::format("{}: {}", a, (void*)b.get()) << std::endl;
//        }
    }

    Provider(std::map<std::string, Value> map) : map(std::move(map))
    {}

    std::string operator()(ProviderData const & data) override {
        if (auto provider_iterator = this->map.find(data.name); provider_iterator != this->map.end()) {
            if constexpr(std::is_base_of_v<Provider_Interface, Value>) {
                return provider_iterator->second()(data);
            } else if constexpr(std::is_same_v<std::unique_ptr<Provider_Interface>, Value>) {
                return provider_iterator->second->operator()(data);
            } else {
                return Provider<Value>(provider_iterator->second)(data);
            }
        } else {
//            std::cerr << fmt::format("in map:") << std::endl;
//            for(auto const & [k,v] : this->map) {
//                std::cerr << fmt::format("key: {}", k) << std::endl;
//            }
            throw TemplateException("unknown name`: '" + data.name + "'");
        }
    }
};



static_assert(!can_get_provider_for_type_v<std::string>);
static_assert(is_provider_type_v<std::string>);
static_assert(is_provider_callback_type_v<std::__1::function<std::__1::basic_string<char> ()> &>);


} // end namespace xl