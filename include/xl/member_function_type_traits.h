
#pragma once

#include <type_traits>
#include <cinttypes>

namespace xl {


template<typename f>
struct pointer_to_member_function : std::false_type {};


template<typename R, typename C, typename...Args, bool is_noexcept>
struct pointer_to_member_function<R(C::*)(Args...) noexcept(is_noexcept)> : std::true_type {
    using return_type = R;
    using class_type = C;
    static constexpr uint16_t arity = sizeof...(Args);
    static constexpr bool const_v = false;
    static constexpr bool volatile_v = false;
    static constexpr bool lvalue_qualified = false;
    static constexpr bool rvalue_qualified = false;
	static constexpr bool noexcept_v = is_noexcept;
    
    using function_type = R(Args...);
    
    template<template<typename...> class type_list_template>
    using type_list = type_list_template<Args...>;
};

template<typename R, typename C, typename...Args, bool is_noexcept>
struct pointer_to_member_function<R(C::*)(Args...) const noexcept(is_noexcept)> : std::true_type {
    using return_type = R;
    using class_type = C;
    static constexpr uint16_t arity = sizeof...(Args);
    static constexpr bool const_v = true;
    static constexpr bool volatile_v = false;
    static constexpr bool lvalue_qualified = false;
    static constexpr bool rvalue_qualified = false;
	static constexpr bool noexcept_v = is_noexcept;
    using function_type = R(Args...);
	template<template<typename...> class type_list_template>
    using type_list = type_list_template<Args...>;
};

template<typename R, typename C, typename...Args, bool is_noexcept>
struct pointer_to_member_function<R(C::*)(Args...) volatile noexcept(is_noexcept)> : std::true_type {
    using return_type = R;
    using class_type = C;
    static constexpr uint16_t arity = sizeof...(Args);
    static constexpr bool const_v = false;
    static constexpr bool volatile_v = true;
    static constexpr bool lvalue_qualified = false;
    static constexpr bool rvalue_qualified = false;
	static constexpr bool noexcept_v = is_noexcept;
    using function_type = R(Args...);
	template<template<typename...> class type_list_template>
    using type_list = type_list_template<Args...>;
};

template<typename R, typename C, typename...Args, bool is_noexcept>
struct pointer_to_member_function<R(C::*)(Args...) const volatile noexcept(is_noexcept)> : std::true_type {
    using return_type = R;
    using class_type = C;
    static constexpr uint16_t arity = sizeof...(Args);
    static constexpr bool const_v = true;
    static constexpr bool volatile_v = true;
    static constexpr bool lvalue_qualified = false;
    static constexpr bool rvalue_qualified = false;
	static constexpr bool noexcept_v = is_noexcept;
    using function_type = R(Args...);
	template<template<typename...> class type_list_template>
    using type_list = type_list_template<Args...>;
};

// lvalue

template<typename R, typename C, typename...Args, bool is_noexcept>
struct pointer_to_member_function<R(C::*)(Args...) & noexcept(is_noexcept)> : std::true_type {
    using return_type = R;
    using class_type = C;
    static constexpr uint16_t arity = sizeof...(Args);
    static constexpr bool const_v = false;
    static constexpr bool volatile_v = false;
    static constexpr bool lvalue_qualified = true;
    static constexpr bool rvalue_qualified = false;
	static constexpr bool noexcept_v = is_noexcept;
    using function_type = R(Args...);
	template<template<typename...> class type_list_template>
    using type_list = type_list_template<Args...>;
};

template<typename R, typename C, typename...Args, bool is_noexcept>
struct pointer_to_member_function<R(C::*)(Args...) const & noexcept(is_noexcept)> : std::true_type {
    using return_type = R;
    using class_type = C;
    static constexpr uint16_t arity = sizeof...(Args);
    static constexpr bool const_v = true;
    static constexpr bool volatile_v = false;
    static constexpr bool lvalue_qualified = true;
    static constexpr bool rvalue_qualified = false;
	static constexpr bool noexcept_v = is_noexcept;
    using function_type = R(Args...);
	template<template<typename...> class type_list_template>
    using type_list = type_list_template<Args...>;
};

template<typename R, typename C, typename...Args, bool is_noexcept>
struct pointer_to_member_function<R(C::*)(Args...) volatile & noexcept(is_noexcept)> : std::true_type {
    using return_type = R;
    using class_type = C;
    static constexpr uint16_t arity = sizeof...(Args);
    static constexpr bool const_v = false;
    static constexpr bool volatile_v = true;
    static constexpr bool lvalue_qualified = true;
    static constexpr bool rvalue_qualified = false;
	static constexpr bool noexcept_v = is_noexcept;
    using function_type = R(Args...);
	template<template<typename...> class type_list_template>
    using type_list = type_list_template<Args...>;
};

template<typename R, typename C, typename...Args, bool is_noexcept>
struct pointer_to_member_function<R(C::*)(Args...) const volatile & noexcept(is_noexcept)> : std::true_type {
    using return_type = R;
    using class_type = C;
    static constexpr uint16_t arity = sizeof...(Args);
    static constexpr bool const_v = true;
    static constexpr bool volatile_v = true;
    static constexpr bool lvalue_qualified = true;
    static constexpr bool rvalue_qualified = false;
	static constexpr bool noexcept_v = is_noexcept;
    using function_type = R(Args...);
	template<template<typename...> class type_list_template>
    using type_list = type_list_template<Args...>;
};

//

template<typename R, typename C, typename...Args, bool is_noexcept>
struct pointer_to_member_function<R(C::*)(Args...) && noexcept(is_noexcept)> : std::true_type {
    using return_type = R;
    using class_type = C;
    static constexpr uint16_t arity = sizeof...(Args);
    static constexpr bool const_v = false;
    static constexpr bool volatile_v = false;
    static constexpr bool lvalue_qualified = false;
    static constexpr bool rvalue_qualified = true;
    static constexpr bool noexcept_v = is_noexcept;
    using function_type = R(Args...);
	template<template<typename...> class type_list_template>
    using type_list = type_list_template<Args...>;
};

template<typename R, typename C, typename...Args, bool is_noexcept>
struct pointer_to_member_function<R(C::*)(Args...) const && noexcept(is_noexcept)> : std::true_type {
    using return_type = R;
    using class_type = C;
    static constexpr uint16_t arity = sizeof...(Args);
    static constexpr bool const_v = true;
    static constexpr bool volatile_v = false;
    static constexpr bool lvalue_qualified = false;
    static constexpr bool rvalue_qualified = true;
    static constexpr bool noexcept_v = is_noexcept;
    using function_type = R(Args...);
	template<template<typename...> class type_list_template>
    using type_list = type_list_template<Args...>;
};

template<typename R, typename C, typename...Args, bool is_noexcept>
struct pointer_to_member_function<R(C::*)(Args...) volatile && noexcept(is_noexcept)> : std::true_type {
    using return_type = R;
    using class_type = C;
    static constexpr uint16_t arity = sizeof...(Args);
    static constexpr bool const_v = false;
    static constexpr bool volatile_v = true;
    static constexpr bool lvalue_qualified = false;
    static constexpr bool rvalue_qualified = true;
    static constexpr bool noexcept_v = is_noexcept;
    using function_type = R(Args...);
	template<template<typename...> class type_list_template>
    using type_list = type_list_template<Args...>;
};

template<typename R, typename C, typename...Args, bool is_noexcept>
struct pointer_to_member_function<R(C::*)(Args...) const volatile && noexcept(is_noexcept)> : std::true_type {
    using return_type = R;
    using class_type = C;
    static constexpr uint16_t arity = sizeof...(Args);
    static constexpr bool const_v = true;
    static constexpr bool volatile_v = true;
    static constexpr bool lvalue_qualified = false;
    static constexpr bool rvalue_qualified = true;
    static constexpr bool noexcept_v = is_noexcept;
    using function_type = R(Args...);
	template<template<typename...> class type_list_template>
    using type_list = type_list_template<Args...>;
};


template<typename F>
constexpr bool is_pointer_to_member_function_v = pointer_to_member_function<F>::value;


}