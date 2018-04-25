#pragma once


#include <algorithm>    
#include <iosfwd>       
#include <memory>       
#include <system_error> 
#include <type_traits>  


namespace xl
{


template <typename T>
class not_null
{

public:
    
    using pointer = T;
    
    template <typename U, typename = std::enable_if<std::is_convertible_v<T, U>>>
    constexpr not_null(U && u) : ptr_(std::forward<U>(u))
    {
        if (__builtin_expect(ptr_ == nullptr, 0)) {
            assert(false);
            exit(0);
        }
    }

    template <typename U, typename = std::enable_if_t<std::is_convertible_v<U, T>>>
    constexpr not_null(not_null<U> const & other) : not_null(other.get())
    {}

    not_null(not_null && other) = default;
    not_null(not_null const & other) = default;
    not_null& operator=(not_null const & other) = default;

    
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

template <typename T>
std::ostream& operator<<(std::ostream& os, const not_null<T>& val)
{
    os << val.get();
    return os;
}

template <typename T, typename U>
auto operator==(const not_null<T>& lhs, const not_null<U>& rhs) -> decltype(lhs.get() == rhs.get())
{
    return lhs.get() == rhs.get();
}

template <typename T, typename U>
auto operator!=(const not_null<T>& lhs, const not_null<U>& rhs) -> decltype(lhs.get() != rhs.get())
{
    return lhs.get() != rhs.get();
}

template <typename T, typename U>
auto operator<(const not_null<T>& lhs, const not_null<U>& rhs) -> decltype(lhs.get() < rhs.get())
{
    return lhs.get() < rhs.get();
}

template <typename T, typename U>
auto operator<=(const not_null<T>& lhs, const not_null<U>& rhs) -> decltype(lhs.get() <= rhs.get())
{
    return lhs.get() <= rhs.get();
}

template <typename T, typename U>
auto operator>(const not_null<T>& lhs, const not_null<U>& rhs) -> decltype(lhs.get() > rhs.get())
{
    return lhs.get() > rhs.get();
}

template <typename T, typename U>
auto operator>=(const not_null<T>& lhs, const not_null<U>& rhs) -> decltype(lhs.get() >= rhs.get())
{
    return lhs.get() >= rhs.get();
}

// more unwanted operators
template <typename T, typename U>
std::ptrdiff_t operator-(const not_null<T>&, const not_null<U>&) = delete;
template <typename T>
not_null<T> operator-(const not_null<T>&, std::ptrdiff_t) = delete;
template <typename T>
not_null<T> operator+(const not_null<T>&, std::ptrdiff_t) = delete;
template <typename T>
not_null<T> operator+(std::ptrdiff_t, const not_null<T>&) = delete;

} 

namespace std
{

template <typename T>
struct hash<xl::not_null<T>>
{
    std::size_t operator()(xl::not_null<T> const & value) const { 
        return hash<T>{}(value); 
    }
};

} // namespace std

