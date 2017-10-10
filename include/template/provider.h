#pragma once

#include <string>
#include <map>
#include <type_traits>
#include <library_extensions.h>

#include "exceptions.h"
#include "template.h"

namespace xl {
//
//class Stringable {
//private:
//    std::variant<std::string, std::function<std::string()>> source;
//public:
//    Stringable(std::string source) : source(source) {}
//    Stringable(std::function<std::string()> source) : source(source) {}
//    operator std::string() {
//        if (auto string = std::get_if<std::string>(&source)) {
//            return *string;
//        } else if (auto callback = std::get_if<std::function<std::string()>>(&source)) {
//            return (*callback)();
//        } else {
//            assert(false);
//        }
//    }
//
//};



template<class T, class=void>
class Provider {
public:
    Provider(T && t); // intentionally not defined
    Provider(T & t); // intentionally not defined
};




template<class, class = void>
class is_provider_type : public std::false_type {};

template<class T>
class is_provider_type<T, std::enable_if_t<std::is_same_v<
    std::string,
    std::result_of_t<Provider<T>(ProviderData const &)>
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


template<class T>
class Provider<T, std::enable_if_t<std::is_convertible_v<T, std::string>>> : public Provider_Interface {
    std::string string;

public:
    Provider(T && string) : string(std::move(string))
    {}
    Provider(T & string) : string(string)
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
class Provider<T, std::enable_if_t<is_provider_callback_type_v<std::remove_reference_t<T>>>> : public Provider_Interface {

    T callback;

    using CallbackResultT = std::result_of_t<T()>;

public:
    Provider(std::remove_reference_t<T> && callback) : callback(std::move(callback))
    {}
    Provider(std::remove_reference_t<T> & callback) : callback(callback)
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
class Provider<T, std::enable_if_t<
    std::is_same_v<
        std::unique_ptr<Provider_Interface>,
        decltype(std::declval<T>().get_provider())>>> : public Provider_Interface {

private:
    T t;

public:
    Provider(T && t) : t(std::move(t)) {}
    Provider(T & t) : t(t) {}
    Provider(T const & t) : t(t) {}


    std::string operator()(ProviderData const & data) override {
        return t.get_provider()->operator()(data);
    }
};

template<class T>
class Provider<T, std::enable_if_t<xl::is_range_for_loop_able_v<T> && !std::is_convertible_v<T, std::string>>> : public Provider_Interface {

private:
    T t;
public:

    Provider(T const & t) : t(t) {}

    std::string operator()(ProviderData const & data) override {
        std::stringstream result;

        static std::regex parameters_regex("^([^|]*)[|]?(.*)$");
        enum ParameterRegexIndex { EVERYTHING = 0,   TEMPLATE_NAME_INDEX, JOIN_STRING_INDEX };

        std::smatch matches;
        if (!std::regex_match(data.parameters, matches, parameters_regex)) {
            throw TemplateException(fmt::format("Unknown template parameters for ContainerProvider: '{}'", data.parameters));
        }
//        for(int i = 0; i < matches.size(); i++) {
//            std::cerr << fmt::format("parameter regex[{}]: '{}'", i, matches[i].str()) << std::endl;
//        }



        bool first = true;
        std::string join_string = matches[JOIN_STRING_INDEX].str();
//        std::cerr << fmt::format("looking up template named: '{}'", matches[TEMPLATE_NAME_INDEX].str()) << std::endl;
//        std::cerr << fmt::format("join string: '{}'", join_string) << std::endl;
        auto template_iterator = data.templates->find(matches[TEMPLATE_NAME_INDEX].str());
        if ( template_iterator == data.templates->end()) {
            if (data.templates->empty()) {
                throw TemplateException("ContainerProvider received empty template map so it can't possibly find a template for its members");
            }
            throw TemplateException(fmt::format("ContainerProvider couldn't find template named: '{}'", data.parameters));
        }
        Template const & tmpl = template_iterator->second;


        for (auto const & element : t) {
            auto p = Provider<std::decay_t<decltype(element)>>(element);

            if (!first) {
                result << join_string;
//                std::cerr << fmt::format("inserting join string '{}' on subsequent pass", join_string) << std::endl;

            } else {
//                std::cerr << fmt::format("skipping join string '{}' on first pass", join_string) << std::endl;

            }
            first = false;
            result << tmpl.fill(p, *data.templates);

        }
        return result.str();
    }
};





template<>
class Provider<std::map<std::string, std::unique_ptr<Provider_Interface>>> : public Provider_Interface {
public:

    std::map<std::string, std::unique_ptr<Provider_Interface>> map;

    template<class... Keys, class... Values>
    Provider(std::pair<Keys, Values> && ... pairs) {
        (this->map.emplace(pairs.first, make_provider(pairs.second)),...);
//        std::cerr << fmt::format("done adding pairs to map") << std::endl;
//        std::cerr << fmt::format("map size: {}", this->map.size()) << std::endl;
//        for(auto const & [a,b] : this->map) {
//            std::cerr << fmt::format("{}: {}", a, (void*)b.get()) << std::endl;
//        }
    }

    std::string operator()(ProviderData const & data) override {
        if (auto provider_iterator = this->map.find(data.name); provider_iterator != this->map.end()) {
            return provider_iterator->second->operator()(data);
        } else {
            std::cerr << fmt::format("in map:") << std::endl;
            for(auto const & [k,v] : this->map) {
                std::cerr << fmt::format("key: {}", k) << std::endl;
            }
            throw TemplateException("unknown name: " + data.name);
        }
    }
};




template<class... Keys, class... Values>
Provider(std::pair<Keys, Values>&&...) -> Provider<std::map<std::string, std::unique_ptr<Provider_Interface>>>;

static_assert(is_provider_type_v<std::string>);
static_assert(is_provider_callback_type_v<std::__1::function<std::__1::basic_string<char> ()> &>);


} // end namespace xl