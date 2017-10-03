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
    return std::find(std::begin(container), std::end(container), value) != std::end(container);
};


template<class ValueT, class... Rest, template<class, class...> class ContainerT, class Callable>
bool contains(ContainerT<ValueT, Rest...> const & container, Callable callback) {
    return std::find_if(std::begin(container), std::end(container), callback) != std::end(container);
};


template<class ValueT, class... Rest, template<class, class...> class ContainerT>
auto find(ContainerT<ValueT, Rest...> const & container, ValueT const & value) {
    return std::find(std::begin(container), std::end(container), value);
};





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
    return std::remove_copy_if(std::begin(container), std::end(container), callable);
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

    std::transform(std::begin(container), std::end(container), std::back_inserter(result), callable);

    return result;
}


template<class T1, class T2>
class pair : public std::pair<T1, T2> {
public:
    pair(T1 && t1, T2 && t2) :
        std::pair<T1, T2>(std::forward<T1>(t1), std::forward<T2>(t2))
    {}


};
template<class T1, class T2>
pair(T1 t1, T2 && t) -> pair<T1, T2>;

} // end namespace xl

namespace std {
    inline namespace __1 {
        template<class T1, class T2>
        pair(T1 t1, T2 t) -> pair<T1, T2>;
    } // end namespace __1
} // end namespace std
