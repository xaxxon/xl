#pragma once

#include <algorithm>
#include <iosfwd>
#include <memory>
#include <system_error>
#include <type_traits>


namespace xl
{


template <typename T>
class destructive_move
{

public:

    using pointer = T;

    template <typename... Args>
    destructive_move(Args && ... args) :
        ptr_(std::forward<Args>(args)...)
    {}

    template <typename U, typename = std::enable_if<std::is_convertible_v<T, U>>>
    constexpr destructive_move(U && u) : ptr_(std::forward<U>(u))
    {}

    template <typename U, typename = std::enable_if_t<std::is_convertible_v<U, T>>>
    constexpr destructive_move(destructive_move<U> const & other) : destructive_move(other.get())
    {}

    destructive_move(destructive_move && other) :
        ptr_(std::move(other.ptr_))
    {
        if (__builtin_expect(destroyed_ == true, 0)) {
            assert(false);
            exit(0);
        }
        other.destroyed_ = true;
    };
    destructive_move(destructive_move const & other) = default;
    destructive_move& operator=(destructive_move const & other) = default;



    constexpr T const & get() const
    {
        if (__builtin_expect(destroyed_ == true, 0)) {
            assert(false);
            exit(0);
        }
        return ptr_;
    }

    constexpr T & get()
    {
        if (__builtin_expect(destroyed_ == true, 0)) {
            assert(false);
            exit(0);
        }
        return ptr_;
    }


    // constexpr operator T const & () const & { return get(); }
    constexpr operator T & () & { return get(); }
    constexpr operator T && () && { this->destroyed_ = true; return std::move(this->ptr_); }
    constexpr T const & operator->() const { return get(); }
    constexpr decltype(auto) operator*() const { return *get(); }

    // prevents compilation when someone attempts to assign a null pointer constant
    destructive_move(std::nullptr_t) = delete;
    destructive_move& operator=(std::nullptr_t) = delete;

    // unwanted operators...pointers only point to single objects!
    destructive_move& operator++() = delete;
    destructive_move& operator--() = delete;
    destructive_move operator++(int) = delete;
    destructive_move operator--(int) = delete;
    destructive_move& operator+=(std::ptrdiff_t) = delete;
    destructive_move& operator-=(std::ptrdiff_t) = delete;

    template<typename U, typename = decltype(std::declval<T>()[std::declval<U>()])>
    decltype(auto) operator[](U&& u) { return ptr_[std::forward<U>(u)]; }

    template<typename U, typename = decltype(std::declval<T>()(std::declval<U>()))>
    decltype(auto) operator()(U&& u) { return ptr_(std::forward<U>(u)); }


    T ptr_;
    bool destroyed_ = false;
};

template <typename T>
std::ostream& operator<<(std::ostream& os, const destructive_move<T>& val)
{
    os << val.get();
    return os;
}

template <typename T, typename U>
auto operator==(const destructive_move<T>& lhs, const destructive_move<U>& rhs) -> decltype(lhs.get() == rhs.get())
{
    return lhs.get() == rhs.get();
}

template <typename T, typename U>
auto operator!=(const destructive_move<T>& lhs, const destructive_move<U>& rhs) -> decltype(lhs.get() != rhs.get())
{
    return lhs.get() != rhs.get();
}

template <typename T, typename U>
auto operator<(const destructive_move<T>& lhs, const destructive_move<U>& rhs) -> decltype(lhs.get() < rhs.get())
{
    return lhs.get() < rhs.get();
}

template <typename T, typename U>
auto operator<=(const destructive_move<T>& lhs, const destructive_move<U>& rhs) -> decltype(lhs.get() <= rhs.get())
{
    return lhs.get() <= rhs.get();
}

template <typename T, typename U>
auto operator>(const destructive_move<T>& lhs, const destructive_move<U>& rhs) -> decltype(lhs.get() > rhs.get())
{
    return lhs.get() > rhs.get();
}

template <typename T, typename U>
auto operator>=(const destructive_move<T>& lhs, const destructive_move<U>& rhs) -> decltype(lhs.get() >= rhs.get())
{
    return lhs.get() >= rhs.get();
}

// more unwanted operators
template <typename T, typename U>
std::ptrdiff_t operator-(const destructive_move<T>&, const destructive_move<U>&) = delete;
template <typename T>
destructive_move<T> operator-(const destructive_move<T>&, std::ptrdiff_t) = delete;
template <typename T>
destructive_move<T> operator+(const destructive_move<T>&, std::ptrdiff_t) = delete;
template <typename T>
destructive_move<T> operator+(std::ptrdiff_t, const destructive_move<T>&) = delete;

}

namespace std
{

template <typename T>
struct hash<xl::destructive_move<T>>
{
    std::size_t operator()(xl::destructive_move<T> const & value) const {
        return hash<T>{}(value);
    }
};

//template<typename T>
//auto move(xl::destructive_move<T> & t) noexcept -> T&&
//{
//    if (__builtin_expect(t.destroyed_ == true, 0)) {
//        assert(false);
//        exit(0);
//    }
//
//    auto & result = t.get();
//    t.destroyed_ = true;
//    return std::move(result);
//}


template<typename T>
auto move(xl::destructive_move<T> && t) noexcept -> T&&
{
    return std::move(t.get());
}



} // namespace std

