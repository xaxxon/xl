#pragma once

#if !defined XL_FORCE_NO_DEMANGLE_NAMES
// if it can be determined safely that cxxabi.h is available, include it for name demangling
#if defined __has_include
#if __has_include(<cxxabi.h>)
#  define XL_DEMANGLE_NAMES
#  include <cxxabi.h>
#  include <mutex>
#endif
#endif
#endif

namespace xl {

/**
 * Returns a demangled version of the typeid(T).name() passed in if it knows how,
 *   otherwise returns the mangled name exactly as passed in
 */
inline std::string demangle_typeid_name(const std::string & mangled_name) {

#ifdef XL_DEMANGLE_NAMES
    //    printf("Starting name demangling\n");
    std::string result;
    int status;
    auto demangled_name_needs_to_be_freed = abi::__cxa_demangle(mangled_name.c_str(), nullptr, 0, &status);
    result = demangled_name_needs_to_be_freed;

    if (demangled_name_needs_to_be_freed == nullptr) {
        return mangled_name;
    }

    if (status == 0) {
        result = demangled_name_needs_to_be_freed;
    } else {
        // https://gcc.gnu.org/onlinedocs/libstdc++/libstdc++-html-USERS-4.3/a01696.html
        //-1: A memory allocation failiure occurred.
        //-2: mangled_name is not a valid name under the C++ ABI mangling rules.
        //-3: One of the arguments is invalid.
        result = mangled_name;
    }
    if (demangled_name_needs_to_be_freed) {
        free(demangled_name_needs_to_be_freed);
    }
    return result;

#else
    return mangled_name;
#endif
}


template<class T>
std::string & demangle() {
#if defined XL_FORCE_NO_DEMANGLE_NAMES
    static std::string result("NO NAME MANGLING AVAILABLE");
    return result;
#else
    static std::string cached_name;
    std::atomic<bool> cache_set = false;

    if (cache_set) {
        return cached_name;
    } else {
        static std::mutex mutex;

        std::lock_guard<std::mutex> lock_guard(mutex);
        if (!cache_set) {
            auto demangled_name = demangle_typeid_name(typeid(T).name());
            std::string constness = std::is_const<T>::value ? "const " : "";
            std::string volatility = std::is_volatile<T>::value ? "volatile " : "";
            cached_name = constness + volatility + demangled_name;
            cache_set = true;
        }
    }

    return cached_name;
#endif
}

} // end namespace xl