#pragma once

namespace xl {

// https://quuxplusone.github.io/blog/2018/08/11/the-auto-macro/
//   renamed "Auto" => "Defer"

template<class L>
class AtScopeExit {
    L & m_lambda;
public:
    AtScopeExit(L & action) : m_lambda(action) {}

    ~AtScopeExit() { m_lambda(); }
};

#define TOKEN_PASTEx(x, y) x ## y
#define TOKEN_PASTE(x, y) TOKEN_PASTEx(x, y)

#define Defer_INTERNAL1(lname, aname, ...) \
    auto lname = [&]() { __VA_ARGS__; }; \
    AtScopeExit<decltype(lname)> aname(lname);

#define Defer_INTERNAL2(ctr, ...) \
    Defer_INTERNAL1(TOKEN_PASTE(Defer_func_, ctr), \
        TOKEN_PASTE(Defer_instance_, ctr), __VA_ARGS__)

#define Defer(...) \
    Defer_INTERNAL2(__COUNTER__, __VA_ARGS__)

}