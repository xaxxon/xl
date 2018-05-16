#pragma once

#include <string>
#include <map>
#include <type_traits>
#include <iostream>

#include "../library_extensions.h"
#include "../demangle.h"

#include "exceptions.h"
#include "templates.h"
#include "provider_data.h"
#include "provider_interface.h"


namespace xl::templates {


template <typename ProviderContainer>
struct DefaultProviders {

    template <class T, class = void>
    class Provider {
        using NoRefT = std::remove_reference_t<T>;
        static_assert(!std::is_pointer_v<T>, "make sure the type of the dereferenced pointer has a get_provider method for it");
        Provider(T t);
    };
    
    template <typename T, typename = std::enable_if_t<std::is_pointer_v<std::remove_reference_t<T>>>>
    Provider(T) -> Provider<std::remove_reference_t<T>, void>;
    


    template  <class T, class = ProviderContainer, class = void>
    struct has_get_provider_in_provider_container : public std::false_type {};

    template <class T, class PC_COPY>
    struct has_get_provider_in_provider_container<T, PC_COPY, std::void_t<decltype(PC_COPY::get_provider(std::declval<T>()))>> : public std::true_type {};

    template <class T>
    static constexpr bool has_get_provider_in_provider_container_v = has_get_provider_in_provider_container<T>::value;



    // it's a provider type if it can be turned into a provider
    template <class, class = void>
    class is_provider_type : public std::false_type {
    };

    template <class T>
    class is_provider_type<T, std::enable_if_t<std::is_same_v<
        std::string,
            std::result_of_t<Provider < remove_refs_and_wrapper_t<T>>(Substitution &)
                >
            > // is same
        > // enable_if
    > : public std::true_type {};

    template <class T>
    static constexpr bool is_provider_type_v = is_provider_type<T>::value;


    template <class T, class = void>
    class is_provider_callback_type : public std::false_type {
    };

    template <class T>
    class is_provider_callback_type<T, std::enable_if_t<
        is_provider_type_v<
            std::result_of_t<
                T()
            > // result_of
        > // is_provider_type
    > // enable_if
    > : public std::true_type {
    };

    template <class T>
    static constexpr bool is_provider_callback_type_v = is_provider_callback_type<T>::value;



    template <class, class = void>
    struct has_get_provider_member : public std::false_type {
    };
    template <class T>
    struct has_get_provider_member<T, std::enable_if_t<std::is_same_v<
        ProviderPtr,
        decltype(std::declval<T>().get_provider())>>> : public std::true_type {
    };

    template <class T>
    static inline constexpr bool has_get_provider_member_v = has_get_provider_member<T>::value;


    template <class, class = void>
    struct has_get_provider_free_function : public std::false_type {
    };

    template <class T>
    struct has_get_provider_free_function<T, std::enable_if_t<std::is_same_v<
        ProviderPtr,
        decltype(get_provider(std::declval<T>()))> // end is_same
    > // end enable_if
    > : public std::true_type {
    };

    template <class T>
    static inline constexpr bool has_get_provider_free_function_v = has_get_provider_free_function<T>::value;


    template <class, class = void>
    struct can_get_provider_for_type : public std::false_type {};

    template <class T>
    struct can_get_provider_for_type<T, std::enable_if_t<
        std::disjunction_v< // logical OR
            has_get_provider_member<T>,
            has_get_provider_free_function<T>,
            has_get_provider_in_provider_container<T>
        > // end disjunction
    > // end enable_if
    > : public std::true_type {
    };


    template <class T>
    static inline constexpr bool can_get_provider_for_type_v = can_get_provider_for_type<T>::value;


    static_assert(std::is_convertible_v<remove_reference_wrapper_t<std::reference_wrapper<const char *const>>, std::string>);
    /**
     * String provider
     * @tparam T
     */
    template <class T>
    class Provider<T, std::enable_if_t<std::is_convertible_v<remove_reference_wrapper_t<T>, std::string>>> : public Provider_Interface {
        std::string string;
        using WithoutRefWrapper = xl::remove_reference_wrapper_t<T>;

    public:
        Provider(WithoutRefWrapper string) : 
            string(std::move(string)) 
        {
            XL_TEMPLATE_LOG("Created string provider with: '{}'", this->string);
            volatile int i;
            i = 0;
        }

        virtual ~Provider() {
            XL_TEMPLATE_LOG("Destroyed string provider for string '{}'", this->string);
        }

        std::string operator()(Substitution &) const override {
            return this->string;
        }

        std::string get_name() const override {
            return fmt::format("Provider: {} is convertible to std::string: '{}'", demangle<T>(), string);
        }

        bool provides_named_lookup() override {
            return true;
        }

        ProviderPtr get_named_provider(Substitution & data) override {
            data.name.clear();
            return std::make_unique<Provider>(*this);
        }

        bool is_fillable_provider() override {
            return true;
        }
    };


    template <class... Ts, std::enable_if_t<std::conjunction_v<is_pair<Ts>...> && !(sizeof...(Ts) <= 1), int> = 0>
    static ProviderPtr make_provider(Ts&&... pairs) {
        return ProviderPtr(
            new Provider<std::map<std::string, ProviderPtr>>(std::forward<Ts>(pairs)...));
        
        //             return ProviderPtr(new Provider<decltype(map)>(std::move(map)));
    };



    /**
     * Callback Provider
     * @tparam T Callback type
     */
    template <class T>
    class Provider<T, std::enable_if_t<is_provider_callback_type_v<std::decay_t<xl::remove_reference_wrapper_t<T>>>>> : public Provider_Interface {

        using NoRefT = std::remove_reference_t<T>;
        NoRefT callback;

        using CallbackResultT = std::remove_reference_t<std::result_of_t<T()>>;

    public:
        Provider(NoRefT && callback) : 
            callback(std::move(callback)) 
        {
            XL_TEMPLATE_LOG("Created callback provider with rvalue callback");
        }
        

        Provider(NoRefT & callback) : 
            callback(callback) 
        {
            XL_TEMPLATE_LOG("Created callback provider with lvalue callback");
        }
        
        
        ~Provider() {
            XL_TEMPLATE_LOG("Destroyed callback provider");
        }


        std::string operator()(Substitution & data) const override {
            auto callback_result = this->callback();
            auto provider = Provider<CallbackResultT>(std::move(callback_result));
            auto provider_result = provider(data);
            return provider_result;
        }

        std::string get_name() const override {
            return fmt::format("Provider: Callback {} returning Providerable type {}", demangle<NoRefT>(), demangle<CallbackResultT>());
        }

        ProviderPtr get_fillable_provider() override {
            auto result = this->callback();
            auto provider = Provider<CallbackResultT>(std::move(result));
            if (provider.is_fillable_provider()) {
                return std::make_unique<decltype(provider)>(std::move(provider)); // turn it into a ProviderPtr
            } else {
                return provider.get_fillable_provider();
            }
        }

        bool is_template_passthrough() const override {
            return true;
        }
    };




    template <class T>
    static ProviderPtr make_provider(T && t) {
        if constexpr(std::is_same_v<std::decay_t<T>, ProviderPtr>) {
            return std::move(t);
        } else if constexpr(is_pair_v<T>) {
            std::map<std::string, decltype(t.second)> map;

//            std::cerr << fmt::format("ref {} rvalue {} lvalue {}",
//            std::is_reference_v<T&&>,
//            std::is_rvalue_reference_v<T&&>,
//            std::is_lvalue_reference_v<T&&>) << std::endl;

//            if constexpr(std::is_rvalue_reference_v<T>) {
                map.insert(std::forward<T>(t));
//            } else {
//                map.emplace(t.first, t.second);
//            }
            return ProviderPtr(new Provider<decltype(map)>(std::move(map)));
        } else {
            return ProviderPtr(new Provider<T>(std::forward<T>(t)));
        }
    }

    template <class R, class... Args, std::enable_if_t<is_provider_type_v<R>, int> = 0>
    static ProviderPtr make_provider(R(* f)(Args...)) {
        return ProviderPtr(new Provider<std::function<R(Args...)>>(std::function<R(Args...)>(f)));
    };


    // IF YOU GET STRANGE AMBIGUITY ERRORS HERE, Make sure none of your providers are implicitly constructible
    //   from the other provider this is ambiguous with (like std::string)

    /**
     * get_provider Provider
     * @tparam T type which can have get_provider called with/on it
     */
    template <class T>
    class Provider<T, std::enable_if_t<can_get_provider_for_type_v<remove_refs_and_wrapper_t<T>>>> : public Provider_Interface {
        using NoRefT = remove_refs_and_wrapper_t<T>;
        static_assert(!std::is_pointer_v<NoRefT>, "do not make get_provider for pointer types");
    private:
        T t_holder;

    public:

        using XL_TEMPLATES_PASSTHROUGH_TYPE = T;

        Provider(T t_holder) : 
            t_holder(t_holder) 
        {
            NoRefT & t = this->t_holder;
            XL_TEMPLATE_LOG("Created can_get_provider Provider with lvalue {}", (void*)&t);
        }
        ~Provider() {
            NoRefT & t = this->t_holder;

            XL_TEMPLATE_LOG("Destroyed can_get_provider Provider for value at {}", (void*)&t);
        }


        std::string operator()(Substitution & data) const override {
            NoRefT const & t = this->t_holder;

            ProviderPtr provider = get_underlying_provider();
            XL_TEMPLATE_LOG("got underlying provider name: {}", provider->get_name());
            XL_TEMPLATE_LOG("t: {}", (void*)&t);
            

//            if (data.inline_template) {
//                auto tmpl = data.inline_template.get();
//                return tmpl->fill<ProviderContainer>(provider, std::move(data));
//            } else {
                return provider->operator()(data);
//            }
        }

        auto get_underlying_provider() const {
            NoRefT const & t = this->t_holder;
            if constexpr(has_get_provider_free_function_v<NoRefT>)
            {
                return get_provider(t);
            } else if constexpr(has_get_provider_member_v<NoRefT>)
            {
                return t.get_provider();
            } else if constexpr(has_get_provider_in_provider_container_v<NoRefT>) {
                return ProviderContainer::get_provider(t);
            } else {
                throw xl::templates::TemplateException("this shouldn't happen");
            }
        }

        std::string get_name() const override {
            return fmt::format("Provider: can_get_provider_for_type_v<{}>", demangle<T>());
        }

        ProviderPtr get_fillable_provider() override {
            return this->get_underlying_provider();
        }

    };



    /**
     * Unique_ptr Provider
     * @tparam T
     * @tparam Deleter
     */
    template <class T>
    class Provider<T, std::enable_if_t<is_template_for_v<std::unique_ptr, remove_reference_wrapper_t<T>>>> : public Provider_Interface {
        using UniquePtrT = remove_reference_wrapper_t<T>;
        using PointeeT = unique_ptr_type_t<UniquePtrT>;
        T t;
    public:
        using XL_TEMPLATES_PASSTHROUGH_TYPE = PointeeT;

        Provider(T t) :
            t(t) {
            XL_TEMPLATE_LOG("Created std::unique_ptr Provider for value at {}", (void*)&this->t);
        }

        ~Provider() {
            XL_TEMPLATE_LOG("unique_ptr provider destructor called");
        }

        std::string operator()(Substitution & data) const override {
            return make_provider(*static_cast<UniquePtrT &>(t))->operator()(data);
        }

        auto get_underlying_provider() {
            UniquePtrT & unique_ptr = t;
            return Provider<make_reference_wrapper_t<PointeeT>>(*unique_ptr);
        }
        std::string get_name() const override {
            return fmt::format("Provider: unique_ptr<{}> passthrough", demangle<PointeeT>());
        }

        ProviderPtr get_fillable_provider() override {
            UniquePtrT & unique_ptr = t;
            return std::make_unique<Provider<make_reference_wrapper_t<PointeeT>>>(*unique_ptr);
        }

    };

//


    /**
     * Pointer Provider -- except char (const) *
     * @tparam T
     */
    template <class T>
    class Provider<T, std::enable_if_t<
            std::is_pointer_v<remove_refs_and_wrapper_t<T>> &&
            !std::is_same_v<std::decay_t<std::remove_pointer_t<remove_refs_and_wrapper_t<T>>>, char>>
        > : public Provider_Interface
    {

        using NoRefT = remove_refs_and_wrapper_t<T>;
        using NoPtrT = std::remove_pointer_t<NoRefT>;

        NoPtrT * const t;
        static_assert(is_provider_type_v<NoPtrT> ||
            can_get_provider_for_type_v<NoPtrT>
            , "make sure the ProviderContainer was specified, if needed");

    public:
        Provider(NoPtrT * const t) : t(t) {
            XL_TEMPLATE_LOG("Creating pointer provider with pointer to {}", (void*)this->t);
        }

        ~Provider() {
            XL_TEMPLATE_LOG("Destroyed pointer Provider for {}", (void*)this->t);
        }

        std::string operator()(Substitution & data) const override {
            return Provider<make_reference_wrapper_t<NoPtrT>>(*t)(data);
        }

        std::string get_name() const override {
            return "Pointer provider";
        }

        using XL_TEMPLATES_PASSTHROUGH_TYPE = T;

        auto get_underlying_provider() {
            return Provider<make_reference_wrapper_t<NoPtrT>>(*t);
        }

        ProviderPtr get_fillable_provider() override {
            return std::make_unique<Provider<make_reference_wrapper_t<NoPtrT>>>(*t);
        }
    };


    /**
     * Container Provider
     * @tparam T Container type
     */
    template <class T>
    class Provider<T, std::enable_if_t<is_range_for_loop_able_v<remove_reference_wrapper_t<T>> &&
                                       !std::is_convertible_v<remove_reference_wrapper_t<T>, std::string> && // std::string is iteratable
                                       !is_map_v<remove_reference_wrapper_t<T>>>> // maps are handled differently
        : public Provider_Interface {

        using NoRefT = std::remove_reference_t<T>;
        using ContainerT = remove_refs_and_wrapper_t<T>;
        using ValueT = typename ContainerT::value_type;

        using ConstMatchedValueT = match_const_of_t<ValueT, ContainerT>;

    private:

        // can be the container type or std::reference_wrapper of the container type
        NoRefT t_holder;

    public:

        // T can be either a container itself or a ref-wrapper around one, so taking it 
        //   "by value" is fine
        Provider(T t_holder) : 
            t_holder(std::move(t_holder)) 
        {
            ContainerT & t = this->t_holder;
            XL_TEMPLATE_LOG("Created container Provider for lvalue at {}", (void*)&t);
        }



        ~Provider() {
            ContainerT & t = t_holder;
            XL_TEMPLATE_LOG("Destroyed container provider for container at {}", (void*)&t);
        }


        std::string operator()(Substitution & data) const override {

            std::cerr << fmt::format("types {} {}", xl::demangle<T>(), xl::demangle<ContainerT const & >()) << std::endl;
            ContainerT const & t = this->t_holder;


            XL_TEMPLATE_LOG("container provider looking at substution data for: {}, {}", data.name, (bool)data.inline_template);
            std::stringstream result;



            XL_TEMPLATE_LOG("inline template exists? {}", (bool)data.inline_template);
            Template const & tmpl = [&] {
                if (data.inline_template) {
                    return *data.inline_template;
                } else {
                    if (data.templates == nullptr) {
                        throw TemplateException("ContainerProvider received nullptr template map so it can't possibly find a template by name");
                    }
                    auto template_iterator = data.templates->find(data.parameters);
                    if (template_iterator == data.templates->end()) {
                        if (data.templates->empty()) {
                            throw TemplateException(
                                "ContainerProvider received empty template map so it can't possibly find a template for its members" +
                                data.name);
                        }
                        throw TemplateException(
                            fmt::format("ContainerProvider couldn't find template named: '{}' from template {}", data.parameters, data.current_template->c_str()));
                    }
                    return template_iterator->second;
                }
            }();

            // whether the current replacement should have the join string before it
            //   off initially unless leading join string is specified
            bool needs_join_string = data.leading_join_string;

            // Iterate through the container
            XL_TEMPLATE_LOG("provider iterator iterating through container of size {}", t.size());
            for (auto & element : t) {

                auto p = Provider<make_reference_wrapper_t<
                    match_const_of_t<
                        remove_refs_and_wrapper_t<ValueT>,
                        remove_reference_wrapper_t<decltype(element)> // use element not container because non-const std::set has const element
                    >>>(std::ref(element));

                if (needs_join_string) {
                    result << data.join_string;
                    XL_TEMPLATE_LOG("inserting join string '{}' on subsequent pass", data.join_string);
                } else {
                    XL_TEMPLATE_LOG("skipping join string '{}' on first pass", data.join_string);
                }


                needs_join_string = true;

                // each element of the container gets its own copy of data, as each should be treated identically
                //   not based on whatever is done by a previous element
                auto fill_result = tmpl.fill<ProviderContainer>(p, Substitution(data));

                XL_TEMPLATE_LOG("replacement is {}\n - Ignore_empty_replacements is {}", fill_result, data.ignore_empty_replacements);
                if (fill_result == "" && data.ignore_empty_replacements) {
                    needs_join_string = false;
                }
                result << fill_result;

            }
            return result.str();
        }

        std::string get_name() const override {
            return fmt::format("Provider: container of {}", demangle<T>());
        }

        bool is_fillable_provider() override {
            return true;
        }

        bool is_template_passthrough() const override {
            return true;
        }
    };


    /**
     * Map Provider
     */
    template <typename T>
    class Provider<T, std::enable_if_t<is_template_for_v<std::map, remove_refs_and_wrapper_t<T>>>> : public Provider_Interface {
    public:

        using NoRefMapT = std::remove_reference_t<T>;
        using MapT = remove_refs_and_wrapper_t<T>;
        using MapKeyT = typename MapT::key_type;
        using MapValueT = typename MapT::mapped_type;

        NoRefMapT map_holder;

        template <class... Keys, class... Values>
        Provider(std::pair<Keys, Values> && ... pairs) {
            MapT & map = this->map_holder;
            (map.emplace(std::move(pairs.first), make_provider(std::move(pairs.second))),...);

            XL_TEMPLATE_LOG("done adding pairs to map at: {}", (void*)this);
            XL_TEMPLATE_LOG("map size: {}", map.size());
            for(auto const & [a,b] : map) {
                XL_TEMPLATE_LOG("{}: {}", a, (void*)b.get());
            }
        }


        Provider(T map_holder) :
            map_holder(std::move(map_holder))
        {
            MapT & map = this->map_holder;
            XL_TEMPLATE_LOG("added entire map into map provider");

            XL_TEMPLATE_LOG("done moving map into map");
            XL_TEMPLATE_LOG("map size: {}", map.size());
            for(auto const & [a,b] : map) {
                XL_TEMPLATE_LOG("{}: {}", a, (void*)&b);
            }
        }


        ~Provider() {
            XL_TEMPLATE_LOG("std::map provider destructor called for provider at {}", (void*)this);
        }


        std::string operator()(Substitution & data) const override {
            MapT const & map = this->map_holder;
            auto provider_iterator = map.find(data.name);

            auto name = std::move(data.name); // keep a copy that isn't cleared
            data.name.clear();


            XL_TEMPLATE_LOG("Looked up map name {} in operator()", name);
            std::string result;
            if (provider_iterator != map.end()) {

                if constexpr(
                    std::is_base_of_v<Provider_Interface, MapValueT> ||
                    std::is_same_v<ProviderPtr, MapValueT>) {

                    XL_TEMPLATE_LOG("value is a provider interface or ProviderPtr");
                    if (data.inline_template) {
                        result = data.inline_template->fill(provider_iterator->second, std::move(data));
                    } else {
                        result = provider_iterator->second->operator()(data);
                    }
                } else {
                    XL_TEMPLATE_LOG("value needs to be converted to provider");

                    auto provider = Provider<make_reference_wrapper_t<MapValueT const>>(std::ref(provider_iterator->second));
                    if (data.inline_template) {
                        auto inline_template = data.inline_template;
//                        data.inline_template.reset();
//                        std::cerr << fmt::format("created provider from map value: {}", provider.get_name()) << std::endl;
                        result = inline_template->fill(provider, std::move(data));
                    } else {
                        result = provider(data);
                    }

                }

            } else {
                XL_TEMPLATE_LOG("in map:");
                for(auto const & [k,v] : map) {
                    (void)v;
                    XL_TEMPLATE_LOG("key: {}", k);
                }
                std::string template_text = "<unknown template name>";
                if (data.current_template != nullptr) {
                    template_text = data.current_template->c_str();
                }
                throw TemplateException("provider {} does not provide name: '{}' - in template: '{}'", this->get_name(), name, template_text);
            }
            return result;
        }


        std::string get_name() const override {
            MapT const & map = this->map_holder;

            std::string result = fmt::format("Provider: map ({}) with keys:", (void*)this);
            for(auto const & [key, value] : map) {
                (void)value;
                result += " " + key;
            }
            return result;
        }


        bool provides_named_lookup() override {
            return true;
        }


        ProviderPtr get_named_provider(Substitution & data) override {

            if (data.name.empty()) {
                throw TemplateException("Map Provider::get_named_provider called but no name specified");
            }

            MapT & map = this->map_holder;

            auto i = map.find(data.name);
            if (i == map.end()) {
                throw TemplateException("Map provider doesn't contain value for name: " + data.name);
            } else {
                data.name.clear();
                return make_provider(i->second);
            }
        }


        bool is_fillable_provider() override {
            return true;
        }
    };


    static_assert(!can_get_provider_for_type_v<std::string>);
    static_assert(is_provider_type_v<std::string>);
    static_assert(is_provider_callback_type_v<std::__1::function<std::__1::basic_string<char>()> &>);

};


template <typename ProviderContainer = void, typename... Ts>
ProviderPtr make_provider(Ts&&... ts) {
    return DefaultProviders<ProviderContainer>::make_provider(std::forward<Ts>(ts)...);
}


} // end namespace xl