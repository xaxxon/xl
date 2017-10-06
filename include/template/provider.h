#pragma once

#include <string>
#include <map>
#include <type_traits>
#include <library_extensions.h>

#include "exceptions.h"
#include "template.h"

namespace xl {




template<class, class = void>
struct MakeProvider;



template<class T, class = void>
class ContainerProvider;


template<class T>
class ContainerProvider<T, std::enable_if_t<xl::is_range_for_loop_able_v<T>>> : public Provider_Interface {

private:
    T t;
public:

    ContainerProvider(T const & t) : t(t) {}

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
            auto p = MakeProvider<std::decay_t<decltype(element)>>()(element);

            if (!first) {
                result << join_string;
//                std::cerr << fmt::format("inserting join string '{}' on subsequent pass", join_string) << std::endl;

            } else {
//                std::cerr << fmt::format("skipping join string '{}' on first pass", join_string) << std::endl;

            }
            first = false;
            result << tmpl.fill(*p, *data.templates);

        }
        return result.str();
    }
};



class MapProvider : public Provider_Interface {
public:

    std::map<std::string, std::unique_ptr<Provider_Interface>> map;

    template<class... Keys, class... Values>
    MapProvider(std::pair<Keys, Values>&&... pairs) {
        (this->map.emplace(pairs.first, MakeProvider<Values>()(pairs.second)),...);
//        std::cerr << fmt::format("done adding pairs to map") << std::endl;
//        std::cerr << fmt::format("map size: {}", this->map.size()) << std::endl;
//        for(auto const & [a,b] : this->map) {
//            std::cerr << fmt::format("{}: {}", a, (void*)b.get()) << std::endl;
//        }
    }

    std::string operator()(ProviderData const & data) override {
        if (auto i = map.find(data.name); i != map.end()) {
            return map[data.name]->operator()(data);
        }
        throw TemplateException(fmt::format("substitution name '{}' not found in map provider", data.name));
    }
};


class StringProvider : public Provider_Interface {
private:
    std::string string;

public:
    StringProvider(std::string const & string) : string(string) {}
    StringProvider(std::string && string) : string(std::move(string)) {}

    std::string operator()(ProviderData const & data) override {
        return this->string;
    }
};

class StringCallbackProvider : public Provider_Interface {
private:
    std::function<std::string()> callback;

public:
    StringCallbackProvider(std::function<std::string()> callback) : callback(callback) {}

    std::string operator()(ProviderData const & data) override {
        return this->callback();
    }
};




template<class T>
struct MakeProvider<T, std::enable_if_t<std::is_convertible_v<T, std::string>>> {

    std::unique_ptr<Provider_Interface> operator()(std::decay_t<T> t) {
        return std::make_unique<StringProvider>(std::forward<T>(t));
    }
};



/**
 * If the type has a method called get_provider that returns a unique_ptr<Provider>
 * @tparam T Type to check for a get_provider method
 */
template<class T>
struct MakeProvider<T, std::enable_if_t<std::is_same_v<std::unique_ptr<xl::Provider_Interface>, decltype(std::declval<T>().get_provider())> > > {
private:

public:
    static constexpr bool is_leaf = true;
    std::unique_ptr<Provider_Interface> operator()(std::remove_reference_t<T> const & t) {
        return t.get_provider();
    }
    std::unique_ptr<Provider_Interface> operator()(std::remove_reference_t<T> & t) {
        return t.get_provider();
    }

    std::unique_ptr<Provider_Interface> operator()(std::remove_reference_t<T> && t) {
        return t.get_provider();
    }
};

template<class T>
struct MakeProvider<T, std::enable_if_t<std::is_convertible_v<T, std::function<std::string()>>>> {
    std::unique_ptr<Provider_Interface> operator()(std::decay_t<T> t) {
        return std::make_unique<StringCallbackProvider>(t);
    }
};


template<class T>
struct MakeProvider<T, std::enable_if_t<xl::is_range_for_loop_able_v<T> && !std::is_convertible_v<T, std::string>>> {
    std::unique_ptr<Provider_Interface> operator()(std::decay_t<T> t) {
        return std::make_unique<ContainerProvider<T>>(std::move(t));
    }
};


//
//
//template<class T>
//struct MakeProvider<T, std::enable_if_t<std::is_base_of_v<Provider_Interface, T>>> {
//
//    static constexpr bool is_leaf = true;
//
//    std::unique_ptr<Provider_Interface> operator()(T & p) {
//        std::make_unique<
//    }
//    std::unique_ptr<Provider_Interface> operator()(T && p) {
//        this->stored_p = std::move(p);
//        return this->stored_p;
//    }
//};



} // end namespace xl