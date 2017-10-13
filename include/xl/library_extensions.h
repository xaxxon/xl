#pragma once

#include <utility>
#include <type_traits>

namespace xl {


/**
 * returns whether the container contains the specified value
 * @tparam ValueT
 * @tparam Rest
 * @tparam ContainerT
 * @param container
 * @param value value to search the container for
 * @return whether the container contains the specified value
 */
template<class ValueT, class... Rest, template<class, class...> class ContainerT>
bool contains(ContainerT<ValueT, Rest...> const & container, ValueT const & value) {
    return std::find(begin(container), end(container), value) != std::end(container);
};


template<class ValueT, class... Rest, template<class, class...> class ContainerT, class Callable>
bool contains(ContainerT<ValueT, Rest...> const & container, Callable callback) {
    return std::find_if(begin(container), end(container), callback) != std::end(container);
};


template<class ValueT, class... Rest, template<class, class...> class ContainerT>
auto find(ContainerT<ValueT, Rest...> const & container, ValueT const & value) {
    return std::find(begin(container), end(container), value);
};


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

    std::transform(begin(container), end(container), std::back_inserter(result), callable);

    return result;
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



template<class T1, class T2>
class pair : public std::pair<T1, T2> {
public:
    pair(T1 && t1, T2 && t2) :
        std::pair<T1, T2>(std::forward<T1>(t1), std::forward<T2>(t2))
    {}
};

template<class T1, class T2>
pair(T1 t1, T2 t) -> pair<T1, T2>;

} // end namespace xl

namespace std {
    inline namespace __1 {
        template<class T1, class T2>
        pair(T1 t1, T2 t) -> pair<T1, T2>;
    } // end namespace __1
} // end namespace std
