#pragma once

#include <type_traits>

namespace xl {


/**
 * Instantiating this type is an error caused by specifying a type that does not implement clone()
 * @tparam T 
 */
template<class T, class = void>
class CloneableUniquePtr;


/**
 * Same as std::unique_ptr (any differences are bugs/missing features) except copyable via clone method on
 * contained value.  Only allows contained values with clone() method defined
 * @tparam T Contained type
 */
template<class T>
class CloneableUniquePtr<T, std::enable_if_t<
        std::is_same_v<
            std::result_of_t<decltype(&T::clone)(T)>, T*
        >>> {
private:
    T * value = nullptr;
    
public:
    CloneableUniquePtr() {}
    CloneableUniquePtr(T * value) : value(value) {}
    
    template<class U, std::enable_if_t<std::is_base_of<T, U>::value, int> = 0>
    CloneableUniquePtr(U const & other) : value(other.clone()) {}
    
    CloneableUniquePtr(CloneableUniquePtr<T> && other) : value(other.release()) {}
    
    explicit CloneableUniquePtr(CloneableUniquePtr<T> const & other) : 
            value(other.value != nullptr ? other.value->clone() : nullptr) 
    {}
    
    CloneableUniquePtr<T> & operator=(CloneableUniquePtr<T> && other) {
        this->reset();
        this->value = other.value;
        other.value = nullptr;
        return *this;
    }

    /**
     * Clones `other` value to create copy
     * @param other cloned value
     * @return this
     */
    CloneableUniquePtr<T> & operator=(CloneableUniquePtr<T> const & other) {
        this->reset();
        this->value = other.value != nullptr ? other.value->clone() : nullptr;
        return *this;
    }


    void reset() {
        delete this->value;
        this->value = nullptr;
    }

    ~CloneableUniquePtr() {
        delete value;
    }
    
    T * get() const {
        return this->value;
    }
    T * release() {
        T * return_value = this->value;
        this->value = nullptr;
        return return_value;
    }
    
    T & operator*() {
        return *this->value;
    }
    
    T * operator->() {
        return this->value;
    }

    T const * operator->() const {
        return this->value;
    }


    operator bool() {
        return this->value != nullptr;
    }
};


template<class T, class... Args>
auto make_cloneable_unique_ptr(Args&&... args) {
    return CloneableUniquePtr<T>(new T(std::forward<Args>(args)...));
};

} // end namespace xl