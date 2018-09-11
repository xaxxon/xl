#pragma once

#include <string>
#include <map>
#include <type_traits>
#include <iostream>

#include "../library_extensions.h"
#include "../demangle.h"

#include "exceptions.h"
#include "templates.h"
#include "substitution.h"
#include "provider_interface.h"
#include "substitution_state.h"
#include "compiled_template.h"

#include "../deferred.h"

namespace xl::templates {
//
//struct ProviderStackGuard {
//    SubstitutionState & substitution;
//    Provider_Interface const * provider;
//    ProviderStackGuard(SubstitutionState & substitution, Provider_Interface const * provider) :
//        substitution(substitution),
//        
//        provider(provider)
//    {
//        assert(this->provider != nullptr);
//        this->substitution.provider_stack.push_front(provider);
//        std::cerr << fmt::format("size check: {}", this->substitution.provider_stack.size()) << std::endl;
//    }
//    ~ProviderStackGuard() {
//        //substitution.provider_stack.pop_front(); // maybe not necessary?
//    }
//};


template <typename ProviderContainer>
struct DefaultProviders {
    
    

    template <class T, class = void>
    class Provider : public Provider_Interface {
        using NoRefT = std::remove_reference_t<T>;
        static_assert(!std::is_pointer_v<T>, "make sure the type of the dereferenced pointer has a get_provider method for it");
        static_assert(std::is_same_v<void, T*>, "This class should never be instantiated");
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
    template <class T, class = void>
    class is_provider_type : public std::false_type {
    };

    template <class T>
    class is_provider_type<T, std::enable_if_t<std::is_same_v<
        std::optional<std::string>,
            std::result_of_t<Provider < remove_refs_and_wrapper_t<T>>(SubstitutionState &)
                >
            > // is same
        > // enable_if
    > : public std::true_type {

    };

    template <class T>
    static constexpr bool is_provider_type_v = is_provider_type<T>::value;

    static_assert(is_provider_type_v<std::string>);


    template <class T, class = void>
    class is_provider_callback_type : public std::false_type {};

    template <class Callback>
    class is_provider_callback_type<Callback, std::enable_if_t<
        is_provider_type_v<
            std::result_of_t<
                Callback()
            > // result_of
        > // is_provider_type
    > // enable_if
    > : public std::true_type {
    };


    // it's a provider callback type if when called, it returns a provider
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
    
    
    // provider interface provider
    template <typename T>
    class Provider<T, std::enable_if_t<std::is_same_v<remove_refs_and_wrapper_t<T>, Provider_Interface>>> : public Provider_Interface {
        T t;
        Provider_Interface & get() const {
            return t;
        }

    public:
        Provider(T t) : t(t) {}

        std::optional<std::string> operator()(SubstitutionState & data) const override {
            return t(data);
        }
        
        

        std::string get_name() const override {
            return this->get().get_name();
        }

        bool provides_named_lookup() const override {
            return this->get().provides_named_lookup();
        }

//        ProviderPtr get_named_provider(Substitution & substitution) override {
//            return this->get().get_named_provider(substitution);
//        }

        bool is_fillable_provider() const override {
            return this->get().is_fillable_provider();
        }

        ProviderPtr get_fillable_provider() override {
            return this->get().get_fillable_provider();
        }

        bool needs_raw_template() const override {
            return false;
        }

        bool is_rewind_point() const override {
            return false;
        }

        bool is_template_passthrough() const override {
            return this->get().is_template_passthrough();
        }
    };
    
    
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
//            XL_TEMPLATE_LOG("Created string provider with: '{}'", this->string);
        }

        virtual ~Provider() {
//            XL_TEMPLATE_LOG("Destroyed string provider for string '{}'", this->string);
        }

        std::optional<std::string> operator()(SubstitutionState & substitution_state) const override {
//            XL_TEMPLATE_LOG("grabbed data for compiled_subsitution 'X' - it has name '{}' and inline template: '{}'",
//                            substitution_state.substitution->get_name(), (void*)substitution_state.substitution->final_data.inline_template.get());

            if (!substitution_state.substitution->name_entries.empty() || // there can't be any more names
                substitution_state.substitution->final_data.inline_template != nullptr) {
                XL_TEMPLATE_LOG("name_entries: {}, inline template: {}", 
                    xl::join(substitution_state.substitution->name_entries), 
                    (void*)substitution_state.substitution->final_data.inline_template.get());
                return {};
            } // else if (substitution_state.substitution.)
            return this->string;
        }

        std::string get_name() const override {
            return fmt::format("Provider: {} is convertible to std::string: '{}'", demangle<T>(), string);
        }

        bool provides_named_lookup() const override {
            return true;
        }

//        ProviderPtr get_named_provider(Substitution &) override {
//            return std::make_unique<Provider>(*this);
//        }

        bool is_fillable_provider() const override {
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
//            XL_TEMPLATE_LOG("Created callback provider with rvalue callback");
        }
        

        Provider(NoRefT & callback) : 
            callback(callback) 
        {
//            XL_TEMPLATE_LOG("Created callback provider with lvalue callback");
        }
        
        
        ~Provider() {
//            XL_TEMPLATE_LOG("Destroyed callback provider");
        }


        std::optional<std::string> operator()(SubstitutionState & data) const override {
            data.fill_state.provider_stack.push_front(this);
            Defer(data.fill_state.provider_stack.pop_front());
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




    template <typename T>
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
            t_holder(std::forward<T>(t_holder)) 
        {
            NoRefT & t = this->t_holder;
//            XL_TEMPLATE_LOG("Created can_get_provider Provider with lvalue {}", (void*)&t);
        }
        ~Provider() {
            NoRefT & t = this->t_holder;

//            XL_TEMPLATE_LOG("Destroyed can_get_provider Provider for value at {}", (void*)&t);
        }


        // get-provider provider
        std::optional<std::string> operator()(SubstitutionState & substitution_state) const override {
            substitution_state.fill_state.provider_stack.push_front(this);
            Defer(substitution_state.fill_state.provider_stack.pop_front());

            NoRefT const & t = this->t_holder;

            ProviderPtr provider = get_underlying_provider();
            XL_TEMPLATE_LOG(LogT::Subjects::Provider, "got underlying provider: {} => {}", this->get_name(), provider->get_name());


            assert(provider.get() != this);
            
            

//            if (data.inline_template) {
//                auto tmpl = data.inline_template.get();
//                return tmpl->fill<ProviderContainer>(provider, std::move(data));
//            } else {
                return provider->operator()(substitution_state);
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
        }

        ~Provider() {
        }

        std::optional<std::string> operator()(SubstitutionState & data) const override {

            UniquePtrT & unique_ptr = t;
            if (!unique_ptr) {
                throw TemplateException("unique_ptr provider '{}' has null pointer", this->get_name());
            }

            data.fill_state.provider_stack.push_front(this);
            Defer(data.fill_state.provider_stack.pop_front());
            XL_TEMPLATE_LOG(LogT::Subjects::Provider, "unique_ptr provider called");
            
            // if the stored type is already a Provider_Interface, then just call it directly
            if constexpr(std::is_same_v<PointeeT, Provider_Interface>) {
                return (t.get())->operator()(data);
            } 
            // otherwise create the provider for the pointee and call that 
            else {
                return make_provider(*static_cast<UniquePtrT &>(t))->operator()(data);
            }
        }

        auto get_underlying_provider() {
            UniquePtrT & unique_ptr = t;
            return Provider<make_reference_wrapper_t<PointeeT>>(*unique_ptr);
        }
        std::string get_name() const override {
            return fmt::format("Provider: unique_ptr<{}> passthrough", xl::demangle<PointeeT>());
        }

        ProviderPtr get_fillable_provider() override {
            UniquePtrT & unique_ptr = t;
            return std::make_unique<Provider<make_reference_wrapper_t<PointeeT>>>(*unique_ptr);
        }
    };
    
    

    // number provider
    template<typename T>
    class Provider<T, std::enable_if_t<std::is_arithmetic_v<remove_refs_and_wrapper_t<T>>>> : public Provider_Interface {
        T t;
    public:
        Provider(T t) : t(t) {}
        std::optional<std::string> operator()(SubstitutionState &) const override {
            std::stringstream string_stream;
            string_stream << t;
            return string_stream.str();
        }
        std::string get_name() const override {
            return fmt::format("Number provider for {}: {}", xl::demangle<T>(), t);
        }
    };
    
    static_assert(is_provider_type_v<int>);

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
        }

        ~Provider() {
        }

        std::optional<std::string> operator()(SubstitutionState & data) const override {
            
            if (t == nullptr) {
                throw TemplateException("Pointer provider '{}' has null pointer", this->get_name());
            }
            
            data.fill_state.provider_stack.push_front(this);
            Defer(data.fill_state.provider_stack.pop_front());
            XL_TEMPLATE_LOG(LogT::Subjects::Provider, "Pointer provider {}", this->get_name());

            return Provider<make_reference_wrapper_t<NoPtrT>>(*t)(data);
        }

        std::string get_name() const override {
            return fmt::format("Pointer provider: {}", xl::demangle<T>());
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
        mutable Provider_Interface * current_value = nullptr; // during iteration, current value stored here

    public:

        // T can be either a container itself or a ref-wrapper around one, so taking it 
        //   "by value" is fine
        Provider(T t_holder) : 
            t_holder(std::move(t_holder)) 
        {
            ContainerT & t = this->t_holder;
//            XL_TEMPLATE_LOG(LogT::Subjects::Provider, "Created container Provider {} for lvalue at {}", xl::demangle<T>(),
//                (void*)&t);
        }



        ~Provider() {
            ContainerT & t = t_holder;
//            XL_TEMPLATE_LOG(LogT::Subjects::Provider, "Destroyed container provider for container at {}", (void*)&t);
        }


        // container provider   
        std::optional<std::string> operator()(SubstitutionState & data) const override {
            
            
            if (data.substitution->initial_data.rewound && this->current_value != nullptr) {
                return this->current_value->operator()(data);
            }
            
            // shouldn't have a current value and be back here without being in a rewind
            assert(this->current_value == nullptr);
            
            Defer(this->current_value = nullptr;);

            
//            std::cerr << fmt::format("types {} {}", xl::demangle<T>(), xl::demangle<ContainerT const & >()) << std::endl;
            ContainerT const & t = this->t_holder;


//            XL_TEMPLATE_LOG(LogT::Subjects::Provider, "container provider looking at substitution data for: {}, {}", data.substitution->get_name(), (bool)data.substitution->final_data.inline_template);
            std::stringstream result;

            assert(data.current_template != nullptr);
            
            // get the template to fill with each element of the container
            CompiledTemplate const * const tmpl = data.get_template();
            assert(tmpl != nullptr);

            // whether the current replacement should have the join string before it
            //   off initially unless leading join string is specified
            bool needs_join_string = data.substitution->shared_data->leading_join_string;

            // Iterate through the container
            XL_TEMPLATE_LOG("provider iterator iterating through container of size {}", t.size());
            
            int i = 0;
            for (auto & element : t) {
                i++;                
                auto p = Provider<make_reference_wrapper_t<
                    match_const_of_t<
                        remove_refs_and_wrapper_t<ValueT>,
                        remove_reference_wrapper_t<decltype(element)> // use element not container because non-const std::set has const element
                    >>>(std::ref(element));

                this->current_value = &p;


                if (needs_join_string) {
                    result << data.substitution->shared_data->join_string;
//                    XL_TEMPLATE_LOG(LogT::Subjects::Provider, "inserting join string '{}' on subsequent pass", data.substitution->shared_data->join_string);
                } else {
//                    XL_TEMPLATE_LOG(LogT::Subjects::Provider, "skipping join string '{}' on first pass", data.substitution->shared_data->join_string);
                }


                needs_join_string = true;


                // each element of the container gets its own copy of data, as each should be treated identically
                //   not based on whatever is done by a previous element
                SubstitutionState new_substitution(data);
                new_substitution.fill_state.provider_stack.push_front(&p);
                Defer(new_substitution.fill_state.provider_stack.pop_front());


                auto fill_result = tmpl->fill<ProviderContainer>(new_substitution);
                
                if (!fill_result) {
                    return {};
                }

//                XL_TEMPLATE_LOG(LogT::Subjects::Provider, "replacement for {} is {}\n - Ignore_empty_replacements is {}", data.current_template->source_template->c_str(), fill_result, data.substitution->shared_data->ignore_empty_replacements);
                if (*fill_result == "" && data.substitution->shared_data->ignore_empty_replacements) {
                    needs_join_string = false;
                }
                result << *fill_result;
                XL_TEMPLATE_LOG(LogT::Subjects::Provider, "In container provider, after {} elements, result currently {}", i, result.str());


            }
            XL_TEMPLATE_LOG(LogT::Subjects::Provider, "Container provider result: '{}'", result.str());

            return result.str();
        }

        bool is_rewind_point() const override {
            return true;
        }


        std::string get_name() const override {
            return fmt::format("Provider: container of {}", demangle<T>());
        }

        bool is_fillable_provider() const override {
            return true;
        }

        bool is_template_passthrough() const override {
            return true;
        }
        
        bool needs_raw_template() const override {
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

//            XL_TEMPLATE_LOG(LogT::Subjects::Provider, "done adding pairs to map at: {}", (void*)this);
//            XL_TEMPLATE_LOG(LogT::Subjects::Provider, "map size: {}", map.size());
//            for(auto const & [a,b] : map) {
//                XL_TEMPLATE_LOG(LogT::Subjects::Provider, "{}: {}", a, (void*)b.get());
//            }
        }


        Provider(T map_holder) :
            map_holder(std::move(map_holder))
        {
            MapT & map = this->map_holder;
//            XL_TEMPLATE_LOG(LogT::Subjects::Provider, "added entire map into map provider");
//
//            XL_TEMPLATE_LOG(LogT::Subjects::Provider, "done moving map into map");
//            XL_TEMPLATE_LOG(LogT::Subjects::Provider, "map size: {}", map.size());
//            for(auto const & [a,b] : map) {
//                XL_TEMPLATE_LOG(LogT::Subjects::Provider, "{}: {}", a, (void*)&b);
//            }
        }


        ~Provider() {
//            XL_TEMPLATE_LOG("std::map provider destructor called for provider at {}", (void*)this);
        }


        // Map Provider
        std::optional<std::string> operator()(SubstitutionState & data) const override {
//            data.fill_state.provider_stack.push_front(this);
//            std::cerr << fmt::format("Entering provider {} with: {}", 
//                this->get_name(), 
//                data.fill_state.provider_stack.size()) << std::endl;

            

            MapT const & map = this->map_holder;

            auto name_optional = data.substitution->get_name(); // keep a copy that isn't cleared
            if (!name_optional) {
                return {};
            }
            
            std::string name = std::move(*name_optional); 
            XL_TEMPLATE_LOG(LogT::Subjects::Provider, "{} Map Provider looking up name {}", this->get_name(), name);


            auto provider_iterator = map.find(name);
            
//            std::cerr << fmt::format("found name? {}\n", provider_iterator != map.end());

//            data.fill_state.provider_stack.push_front(this);

            std::optional<std::string> result = {};
            
            // if a provider was found AND
            // there is no rewind possible OR a rewind is already in flight
            if (provider_iterator != map.end() && 
                (data.substitution->initial_data.rewind_provider_count == 0 || 
                 data.substitution->initial_data.rewound)) {

                if constexpr(
                    std::is_base_of_v<Provider_Interface, MapValueT> ||
                    std::is_same_v<ProviderPtr, MapValueT>) {

                    data.fill_state.provider_stack.push_front(provider_iterator->second.get());
                    Defer(data.fill_state.provider_stack.pop_front());


//                    XL_TEMPLATE_LOG(LogT::Subjects::Provider, "value is a provider interface or ProviderPtr");
                    auto next_template = data.get_template();
                    assert(next_template != nullptr);
                    result = next_template->fill(data);

//                    std::cerr << fmt::format("1map provider for {} result: {}\n", name, result);

//                    if (data.substitution->final_data.inline_template) {
//                        result = data.substitution->final_data.inline_template->fill(data);
//                    } else {
//                        result = provider_iterator->second->operator()(data);
//                    }
                } else {
//                    XL_TEMPLATE_LOG(LogT::Subjects::Provider, "value needs to be converted to provider");

                    auto provider = Provider<make_reference_wrapper_t<MapValueT const>>(
                        std::ref(provider_iterator->second));
                    data.fill_state.provider_stack.push_front(&provider);
                    Defer(assert(!data.fill_state.provider_stack.empty());
                              data.fill_state.provider_stack.pop_front());


                    auto next_template = data.get_template();

//                    std::cerr << fmt::format("template to fill's substitution name entries: {}\n", xl::join(next_template->substitutions[0]->name_entries));
                    result = next_template->fill(data);

//                    XL_TEMPLATE_LOG(LogT::Subjects::Provider, "2map provider for {} result: {}\n", name, result);

//                    if (data.substitution->final_data.inline_template) {
//                        auto inline_template = data.substitution->final_data.inline_template;
////                        data.inline_template.reset();
////                        std::cerr << fmt::format("created provider from map value: {}", provider.get_name()) << std::endl;
//                        result = inline_template->fill(data);
//                    } else {
//                        result = provider(data);
//                    }

                }

//            // Try rewind
//            else {
//                
//                unsigned int rewind_count = 0;
//
//                if (!data.searching_provider_stack && !data.fill_state.provider_stack.empty()) {
//
//                    XL_TEMPLATE_LOG(LogT::Subjects::Provider, "couldn't find name {} in primary provider: {}", data.substitution->get_name(), this->get_name());
//
////                    std::cerr << fmt::format("right before looking for matching name in {} upstream providers:", data.fill_state.provider_stack.size()) << std::endl;
////                    for (auto * provider : data.fill_state.provider_stack) {
////                        std::cerr << fmt::format("upstream: {} {}", (void*)provider, provider->get_name()) << std::endl;
////                    }
//
//                    // move substitution back if it's a split() substitution
//                    while (data.substitution->parent_substitution != nullptr) {
//                        data.substitution = data.substitution->parent_substitution; 
//                    }
//                    
//
//                    for (auto * provider : data.fill_state.provider_stack) {
//                        
//                        std::cerr << fmt::format("going through provider stack, current provider:{}\n",
//                            provider->get_name());
//                        std::cerr << data.fill_state.provider_stack;
//
//                        if (provider == this) {
//                            continue;
//                        }
//                        
//                        // only rewind on "core" providers
//                        if (provider->is_rewind_point()) {
//                            rewind_count++;
//                        }
//                        
//                        std::cerr << fmt::format("is rewind point: {} new rewind count: {}/{}\n",
//                            provider->is_rewind_point(),
//                            rewind_count,
//                            data.substitution->initial_data.rewind_provider_count
//                        ) << std::endl;
//                        
//                        // if rewind count is set, rewind at least that many levels
//                        if (rewind_count < data.substitution->initial_data.rewind_provider_count) {
//                            continue;
//                        }
//                        
//                        // start over from scratch
//                        auto copy = SubstitutionState(*data.current_template,
//                                                      data.fill_state, 
//                                                      data.substitution);
//                        copy.searching_provider_stack = true;
//                        copy.fill_state.provider_stack.clear();
//                        
//                        try {
//                            Defer(data.substitution->initial_data.rewound = false;);
//                            data.substitution->initial_data.rewound = true;
//                            return provider->operator()(copy);
//                        } catch (TemplateException const & e) {
//                            std::cerr << fmt::format("tried rewound provider, got exception: {}\n", e.what());
//                            continue;
//                        }
//                    }
//                }

//                std::string template_text = "<unknown template name>";
//                if (data.current_template != nullptr) {
//                    template_text = data.current_template->source_template->c_str();
//                }
            }
            if (!result) {
                std::string exception_string = fmt::format("provider {} does not provide name: '{}'", this->get_name(),
                                                           name);
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


        bool provides_named_lookup() const override {
            return true;
        }


//        ProviderPtr get_named_provider(Substitution & substitution) override {
//            auto name_optional = substitution.get_name();
//            
//            
//            if (.empty()) {
//                throw TemplateException("Map Provider::get_named_provider called but no name specified");
//            }
//
//            MapT & map = this->map_holder;
//
//            auto i = map.find(substitution.get_name());
//            if (i == map.end()) {
//                throw TemplateException("Map provider doesn't contain value for name: " + substitution.get_name());
//            } else {
//                return make_provider(i->second);
//            }
//        }


        bool is_fillable_provider() const override {
            return true;
        }
        
        bool is_rewind_point() const override {
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