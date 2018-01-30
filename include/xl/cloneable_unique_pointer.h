#pragma once

#include <type_traits>
#include <memory>


namespace xl {

/**
 * Instantiating this type is an error caused by specifying a type that does not implement clone()
 * @tparam T 
 */
template<typename T, class = void>
class CloneableUniquePtr {
    static_assert(std::is_same_v<T*, void>, "CloneableUniquePtr must point to type implementing clone()");
};


/**
 * Same as std::unique_ptr (any differences are bugs/missing features) except copyable via clone method on
 * contained value.  Only allows contained values with clone() method defined
 * @tparam T Contained type
 */
template <class T>
class CloneableUniquePtr<T, std::enable_if_t<std::is_same_v<std::result_of_t<decltype (&T::clone)(T)>, T *>>> : public std::unique_ptr<T>
{
public:
    using std::unique_ptr<T>::unique_ptr;

    template <class U, std::enable_if_t<std::is_base_of_v<T, U>, int> = 0>
    CloneableUniquePtr(U const & other) : std::unique_ptr<T>(other.clone())
    {
    }

    CloneableUniquePtr(CloneableUniquePtr<T> const & other) : std::unique_ptr<T>(other ? other->clone() : nullptr) {}

    using std::unique_ptr<T>::operator=;

    CloneableUniquePtr<T> & operator=(CloneableUniquePtr<T> const & other)
    {
        this->reset(other ? other->clone() : nullptr);
        return *this;
    }
};


template<class T, class... Args>
auto make_cloneable_unique_ptr(Args&&... args) {
    return CloneableUniquePtr<T>(new T(std::forward<Args>(args)...));
};



} // end namespace xl

