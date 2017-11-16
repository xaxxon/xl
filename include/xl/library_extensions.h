#pragma once

/**
 * @file
 * Helper functions/classes/type traits
 */

#include <utility>
#include <type_traits>
#include <vector>
#include <map>
#include <sstream>
#include <optional>

#include "magic_ptr.h"
#include "fmt/format.h"
namespace xl {

#ifdef XL_TESTING
#define XL_PRIVATE_UNLESS_TESTING public
#else
#define XL_PRIVATE_UNLESS_TESTING private
#endif


template<typename T, typename = void>
struct has_insertion_operator : std::false_type {};

template<typename T>
struct has_insertion_operator<T,
    std::void_t<decltype(
    operator<<(
        std::declval<std::ostream>(),
        std::declval<T>()))>
> : std::true_type {};

/**
 * LibC++ with clang5 doesn't work with this code - reports true when it should be false for newly created
 * user-defined type:  class A{}; has_insertion_operator_v<A> <== true but should be false
 * https://bugs.llvm.org/show_bug.cgi?id=35063
 * @tparam T Type to check for operator<<
 */
template<class T>
constexpr bool has_insertion_operator_v = has_insertion_operator<T>::value;


template<typename T, typename V, typename = void>
struct has_find_for : public std::false_type {};

/// @cond HIDDEN_SYMBOLS
template<typename T, typename V>
struct has_find_for<T, V, std::void_t<decltype(std::declval<T>().find(std::declval<V>()))>> : public std::true_type {};
/// @endcond


/// @cond HIDDEN_SYMBOLS
template<class T>
struct _unique_ptr_type;

template<class T, class... Args>
struct _unique_ptr_type<std::unique_ptr<T, Args...>> {
    using type = T;
};
/// @endcond

/**
 * Type trait for the type a unique_ptr points to
 */
template<class T>
using unique_ptr_type = _unique_ptr_type<std::decay_t<T>>;

/**
 * The type a unique_ptr points to
 */
template<class T>
using unique_ptr_type_t = typename unique_ptr_type<T>::type;

/**
 * Whether type T has a member function `find` which can take a type V
 * @tparam T Container type
 * @tparam V Type to find
 */
template<typename T, typename V>
constexpr bool has_find_for_v = has_find_for<T, V>::value;



/**
 * Type trait for whether the specified type is an instantiation of the given template
 * @tparam tmpl template to check against
 * @tparam T type to check against
 */
template<template<class...> class tmpl, typename T>
struct _is_template_for : public std::false_type {};

template<template<class...> class tmpl, class... Args>
struct _is_template_for<tmpl, tmpl<Args...>> : public std::true_type {};

template<template<class...> class tmpl, typename... Ts>
using is_template_for = std::conjunction<_is_template_for<tmpl, std::decay_t<Ts>>...>;

template<template<class...> class tmpl, typename... Ts>
constexpr bool is_template_for_v = is_template_for<tmpl, Ts...>::value;



template<typename T, typename = void>
struct is_map : public is_template_for<std::map, T> {};

template <typename T>
constexpr bool is_map_v = is_map<T>::value;


template<typename T, typename = void>
struct is_pair : public is_template_for<std::pair, T> {};


template <typename T>
constexpr bool is_pair_v = is_pair<T>::value;



template<typename... Ts>
struct map_for_pair;

template<typename T1, typename T2, typename... Args>
struct map_for_pair<std::pair<T1, T2>, Args...> {
    using type = std::map<std::string, T2>;
};

template<typename... Ts>
using map_for_pair_t = typename map_for_pair<Ts...>::type;


/**
 * returns whether the container contains the specified value
 * @tparam C Container type to look in
 * @tparam V Value type to look for
 * @param container container to be searched
 * @param value value to search the container for
 * @return whether the container contains the specified value
 */
template<typename C, typename V, std::enable_if_t<has_find_for_v<C, V>> * = nullptr>
bool contains(C && container, V && value) {
    return container.find(value) != container.end();
};

template<typename C, typename V, std::enable_if_t<!has_find_for_v<C, V>> * = nullptr>
bool contains(C && container, V && value) {
    return std::find(begin(container), end(container), value) != std::end(container);
};



template<class ValueT, class... Rest, template<class, class...> class ContainerT>
auto find(ContainerT<ValueT, Rest...> const & container, ValueT const & value) {
    return std::find(begin(container), end(container), value);
};

template<class Container, class Callback>
auto copy_if(Container const & container, Callback && callback) {
    Container result;
    for(auto & element : container) {
        if (callback(element)) {
            result.insert(std::end(result), element);
        }
    }
    return result;
};


/**
 * Takes a container and produces tuples with each value of the container and an associated counter
 */
template<class, class...>
struct EachI;


template<size_t... Is, class... Containers>
struct EachI<std::index_sequence<Is...>, Containers...> {
public:
    std::tuple<magic_ptr<Containers>...> containers;
public:

    template<class... ConstructorContainers>
    EachI(ConstructorContainers&&... containers) :
        containers(std::forward<ConstructorContainers>(containers)...)
    {}


    struct iterator {
        int counter = 0;
        using ResultTuple = std::tuple<typename Containers::value_type&..., int>;
        std::tuple<typename Containers::iterator...> iterators;

        iterator(typename Containers::iterator... i) :
            iterators(i...)
        {}

        iterator(){}

        void operator++() {
            counter++;
            (std::get<Is>(iterators)++,...);
        }

        ResultTuple operator*(){
            return ResultTuple{*std::get<Is>(iterators)..., counter};
        }
        bool operator!=(iterator const & other) {
            return std::get<0>(this->iterators) != std::get<0>(other.iterators);
        }
    };


    auto begin() {
        return iterator(std::get<Is>(this->containers)->begin()...);
    }

    auto end() {
        return iterator(std::get<Is>(this->containers)->end()...);
    }
};

template<class... Containers>
auto each_i(Containers&&... containers) {
    return EachI<std::index_sequence_for<Containers...>, std::remove_reference_t<Containers>...>(std::forward<Containers>(containers)...);
}


template<class...>
using int_t = int;

template<class T, class = void>
struct is_range_for_loop_able : public std::false_type {};

template<class T>
struct is_range_for_loop_able<T, std::void_t<
    decltype(begin(std::declval<T>())),
    decltype(end(std::declval<T>()))>
> : public std::true_type {};


template<class T>
constexpr bool is_range_for_loop_able_v = is_range_for_loop_able<T>::value;

static_assert(!is_range_for_loop_able_v<int>);


/**
 * Returns a container only containing elements of the original array which return false when sent to the callback
 * @tparam ValueT Value type of each element to be processed
 * @tparam Rest
 * @tparam ContainerT
 * @tparam Callable
 * @param container
 * @param callable callback used to determine whether put an element in the output container or not
 * @return container a copy of the containercontaining all the elements of the source container unless the callback returns true for a given element
 */
template<class ValueT, class... Rest, template<class, class...> class ContainerT, class Callable>
ContainerT<ValueT, Rest...> remove_copy_if(ContainerT<ValueT, Rest...> const & container, Callable callable) {
    return std::remove_copy_if(begin(container), end(container), callable);
};


/**
 * Takes a container and returns a container of the type returned by running each element through the specified callback
 * @tparam ValueT Source value type of container
 * @tparam Rest Any additional template parameters on the source container type
 * @tparam ContainerT Container template
 * @tparam Callable Callback type
 * @param container source container
 * @param callable each element of the input container is sent to this callback to populate the returned container
 * @return vector containing result of sending each element of the source container to the callback
 */
template<class ValueT, class... Rest, template<class, class...> class ContainerT, class Callable>
ContainerT<std::result_of_t<Callable(ValueT)>> transform(ContainerT<ValueT, Rest...> const & container, Callable callable) {

    ContainerT<std::result_of_t<Callable(ValueT)>> result;

    std::transform(begin(container), end(container), std::inserter(std::end(result), result), callable);

    return result;
}


/**
 * Given an input container of type Container<T> and a Callable returing std::optional<U>, returns
 * a container Container<U> containing only the elements which weren't an empty std::optional.
 * @return container of non-empty results from callable
 */
template<class ValueT, class... Rest, template<class, class...> class ContainerT, class Callable,
    std::enable_if_t<is_template_for_v<std::optional, std::result_of_t<Callable(ValueT)>>, int> = 0>
auto transform_if(ContainerT<ValueT, Rest...> const & container, Callable callable) {
    using result_value_type = std::remove_reference_t<decltype(*callable(std::declval<ValueT>()))>;
    ContainerT<result_value_type> result;
    for (auto const & element : container) {
        auto filter_result = callable(element);
        if (filter_result) {
            result.insert(std::end(result), *filter_result);
        }
    }

    return result;
}

template<typename T>
auto get_pointer(T && t) {
    using NoRefT = std::remove_reference_t<T>;
    if constexpr(std::is_pointer_v<NoRefT>) {
        return t;
    } else if constexpr(
        is_template_for_v<std::unique_ptr, NoRefT> ||
        is_template_for_v<std::shared_ptr, NoRefT>) {

        return t.get();

    } else {
        return &t;
    }
}


/**
 * Similar to std::remove_if but the rejected elements are removed from the container
 * @tparam T
 * @tparam Callable
 * @param container
 * @param callable
 * @return same container passed in but without the rejected elements
 */
template<class T, class Callable, int_t<decltype(std::remove_if(begin(std::declval<T>()), end(std::declval<T>()), std::declval<Callable>()))> = 0>
auto & erase_if(T & container, Callable callable) {
    auto end_of_keep_elements = std::remove_if(begin(container), end(container), callable);
    container.erase(end_of_keep_elements, container.end());
    return container;
}

template<class T, class Callable, int_t<decltype(std::remove_if(begin(std::declval<T>()), end(std::declval<T>()), std::declval<Callable>()))> = 0>
auto && erase_if(T && container, Callable callable) {
    return std::move(erase_if(container, std::move(callable)));
}



/**
 * If the type is copyable, returns a copy of it
 */
template<class T, std::enable_if_t<std::is_copy_constructible_v<T>, int> = 0>
T copy(T const & t) {
    return t;
};

/**
 * If it's a container of things that aren't copyable, return a new container of reference (wrappers) to the elements
 * of the original container
 * @return new container with reference_wrapper's to the objects in the original container
 */
template<class ValueT, class... Rest, template<class, class...> class ContainerT>
auto copy(ContainerT<ValueT, Rest...> const & container) {
    if constexpr(std::is_copy_constructible_v<ValueT>) {
        return ContainerT<ValueT, Rest...>();
    } else {
        ContainerT<std::reference_wrapper<ValueT>> results;
        for (ValueT & e : const_cast<ContainerT<ValueT, Rest...>&>(container)) {
            results.push_back(std::ref(e));
        }
        return results;
    }
}


template<class, class = void>
class EAC;

template<class T, class Return, class Class>
class EAC<Return(Class::*)(T)> {
private:
    using NoRefT = std::remove_reference_t<T>;
    using PtrT = std::add_pointer_t<NoRefT>;
    Return(Class::*pointer)(T);

public:

    EAC(Return(Class::*pointer)(T)) : pointer(pointer)
    {}

    auto operator()(NoRefT * t) {
        if constexpr(std::is_pointer_v<NoRefT>) {
            return (Class().*pointer)(t);
        } else {
            return (Class().*pointer)(*t);
        }
    }
    auto operator()(NoRefT & t) {
        return this->operator()(&t);
    }
    auto operator()(NoRefT && t) {
        return this->operator()(&t);
    }


};

template<class T>
class EAC<T> {
private:
    T t;

public:
    EAC(T t) : t(t) {

    }

    template<class V>
    auto operator()(V &&) {
        return t;
    }
};

template<class T, class Return, class Class>
auto eac(Return(Class::*pointer)(T)) {
    return EAC<Return(Class::*)(T)>(pointer);
};

template<class T>
auto eac(T t) {
    return EAC<T>(t);
};



template<typename T>
struct FilteredInsertIterator {
    using value_type = typename T::value_type;
    using filter_type = typename std::function<bool(value_type const &)>;
    using iterator_type = typename T::iterator;

private:

    filter_type filter;
    std::insert_iterator<T> insert_iterator;

public:
    FilteredInsertIterator(filter_type filter, T & c, typename T::iterator i) :
            filter(std::move(filter)),
            insert_iterator(std::inserter(c, i))
    {}

    FilteredInsertIterator &
    operator=(value_type const & value ) {
        if (this->filter(value)) {
            insert_iterator = value;
        }
        return *this;
    }

    FilteredInsertIterator &
    operator=(value_type && value ) {
        if (this->filter(value)) {
            insert_iterator = std::move(value);
        }
        return *this;
    }

    FilteredInsertIterator & operator*() {
        return *this;
    }

    FilteredInsertIterator & operator++() {
        return *this;
    }

    FilteredInsertIterator & operator++( int ) {
        return *this;
    }

};


template<typename T>
FilteredInsertIterator<T> filtered_inserter(
    typename FilteredInsertIterator<T>::filter_type filter,
    T & container,
    typename T::iterator iterator) {
    return FilteredInsertIterator<T>(std::move(filter), container, iterator);
}






/// same as forward_as_tuple but for pair
template<class T1, class T2>
auto forward_as_pair(T1 && t1, T2 && t2) {
    return std::pair<T1, T2>(std::forward<T1>(t1), std::forward<T2>(t2));
}





/**
 * type trait removing std::reference_wrapper from around a type, if present
 */
template<typename T, typename = void>
struct remove_reference_wrapper {
    using type = T;
};


/**
 * The type with std::reference_wrapper removed, if present
 */
template<typename T>
using remove_reference_wrapper_t = typename remove_reference_wrapper<T>::type;


/// @cond HIDDEN_SYMBOLS
template<typename T>
struct _remove_reference_wrapper;

template<typename T>
struct _remove_reference_wrapper<std::reference_wrapper<T>> {
    using type = T;
};

// this is required so that if it's a NOT a reference wrapper that the type isn't std::decay'd
template<typename T>
struct remove_reference_wrapper<T, std::enable_if_t<is_template_for_v<std::reference_wrapper, T>>> :
    public _remove_reference_wrapper<std::decay_t<T>>
{};
/// @endcond




template<class T>
using remove_refs_and_wrapper = remove_reference_wrapper<std::remove_reference_t<T>>;

template<class T>
using remove_refs_and_wrapper_t = typename remove_refs_and_wrapper<T>::type;




template<class T, class U>
struct match_const_of {
    using type = std::conditional_t<std::is_const_v<std::remove_reference_t<U>>, const T, T>;
};

template<class T, class U>
using match_const_of_t = typename match_const_of<T, U>::type;


template<class T>
struct make_reference_wrapper {
    using type = std::reference_wrapper<T>;
};

template<class T>
struct make_reference_wrapper<std::reference_wrapper<T>> {
    using type = std::reference_wrapper<T>;
};

template<class T>
using make_reference_wrapper_t = typename make_reference_wrapper<T>::type;


/**
 * Joins together the elements of the container with the specified join string between them
 * @param container
 * @param join_string
 * @return string of all the elements combined
 */
template<class T>
std::string join(T const & container, std::string_view join_string = ", ") {
    std::stringstream result;

    bool first_iteration = true;
    for (auto const & element : container ) {
        if (!first_iteration) {
            result << join_string;
        }
        first_iteration = false;

        result << element;
    }

    return result.str();
}

/// @cond HIDDEN_SYMBOLS
template<typename T>
struct _is_std_array : public std::false_type {};

template<typename T, std::size_t size>
struct _is_std_array<std::array<T, size>> : public std::true_type {};
/// @endcond

/**
 * Test
 */
template<typename T>
using is_std_array = _is_std_array<std::decay_t<T>>;

template<typename T>
constexpr bool is_std_array_v = is_std_array<T>::value;


} // end namespace xl

namespace std {


inline namespace __1 {
template<class T1, class T2>
pair(T1 t1, T2 t) -> pair<T1, T2>;
} // end namespace __1
} // end namespace std

