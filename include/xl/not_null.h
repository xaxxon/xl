#pragma once


#include <algorithm>    
#include <iosfwd>       
#include <memory>       
#include <system_error> 
#include <type_traits>  


namespace xl
{


template <class T>
class not_null
{

public:
    template <typename U, typename = typename std::enable_if<std::is_convertible_v<T, U>>
    constexpr not_null(U&& u) : ptr_(std::forward<U>(u))
    {
        Expects(ptr_ != nullptr);
    }

    template <typename U, typename = std::enable_if_t<std::is_convertible<U, T>::value>>
    constexpr not_null(const not_null<U>& other) : not_null(other.get())
    {}

    not_null(not_null&& other) = default;
    not_null(const not_null& other) = default;
    not_null& operator=(const not_null& other) = default;

    
    constexpr T const & get() const
    {
        return ptr_;
    }

    constexpr operator T const & () const { return get(); }
    constexpr T const & operator->() const { return get(); }
    constexpr decltype(auto) operator*() const { return *get(); } 

    // prevents compilation when someone attempts to assign a null pointer constant
    not_null(std::nullptr_t) = delete;
    not_null& operator=(std::nullptr_t) = delete;

    // unwanted operators...pointers only point to single objects!
    not_null& operator++() = delete;
    not_null& operator--() = delete;
    not_null operator++(int) = delete;
    not_null operator--(int) = delete;
    not_null& operator+=(std::ptrdiff_t) = delete;
    not_null& operator-=(std::ptrdiff_t) = delete;

    template<typename U, typename = decltype(std::declval<T>()[std::declval<U>()])>
    decltype(auto) operator[](U&& u) { return ptr_[std::forward<U>(u)]; }

    template<typename U, typename = decltype(std::declval<T>()(std::declval<U>()))>
    decltype(auto) operator()(U&& u) { return ptr_(std::forward<U>(u)); }


private:
    T ptr_;
};

template <class T>
std::ostream& operator<<(std::ostream& os, const not_null<T>& val)
{
    os << val.get();
    return os;
}

template <class T, class U>
auto operator==(const not_null<T>& lhs, const not_null<U>& rhs) -> decltype(lhs.get() == rhs.get())
{
    return lhs.get() == rhs.get();
}

template <class T, class U>
auto operator!=(const not_null<T>& lhs, const not_null<U>& rhs) -> decltype(lhs.get() != rhs.get())
{
    return lhs.get() != rhs.get();
}

template <class T, class U>
auto operator<(const not_null<T>& lhs, const not_null<U>& rhs) -> decltype(lhs.get() < rhs.get())
{
    return lhs.get() < rhs.get();
}

template <class T, class U>
auto operator<=(const not_null<T>& lhs, const not_null<U>& rhs) -> decltype(lhs.get() <= rhs.get())
{
    return lhs.get() <= rhs.get();
}

template <class T, class U>
auto operator>(const not_null<T>& lhs, const not_null<U>& rhs) -> decltype(lhs.get() > rhs.get())
{
    return lhs.get() > rhs.get();
}

template <class T, class U>
auto operator>=(const not_null<T>& lhs, const not_null<U>& rhs) -> decltype(lhs.get() >= rhs.get())
{
    return lhs.get() >= rhs.get();
}

// more unwanted operators
template <class T, class U>
std::ptrdiff_t operator-(const not_null<T>&, const not_null<U>&) = delete;
template <class T>
not_null<T> operator-(const not_null<T>&, std::ptrdiff_t) = delete;
template <class T>
not_null<T> operator+(const not_null<T>&, std::ptrdiff_t) = delete;
template <class T>
not_null<T> operator+(std::ptrdiff_t, const not_null<T>&) = delete;

} // namespace gsl

namespace std
{
template <class T>
struct hash<gsl::not_null<T>>
{
    std::size_t operator()(const gsl::not_null<T>& value) const { return hash<T>{}(value); }
};

} // namespace std

