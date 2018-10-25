#pragma once

#if !defined XL_FORCE_NO_DEMANGLE_NAMES
// if it can be determined safely that cxxabi.h is available, include it for name demangling

#if defined XL_USE_CXX_ABI
#include <cxxabi.h>
#include <mutex>
#undef XL_DEMANGLE_NAMES
#define XL_DEMANGLE_NAMES
#endif

// Useful for when not RTTI available
#if defined XL_USE_BOOST
#include <boost/type_index.hpp>
#undef XL_DEMANGLE_NAMES
#define XL_DEMANGLE_NAMES
#endif

#if !defined XL_DEMANGLE_NAMES
static_assert(false, "name mangling not disabled but no mechanism specified");
#endif

#endif // !defined XL_FORCE_NO_DEMANGLE_NAMES


// The windows function to demangle names appears to be: 
//   https://msdn.microsoft.com/en-us/library/windows/desktop/ms681400(v=vs.85).aspx

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
    size_t length;
    auto demangled_name_needs_to_be_freed = abi::__cxa_demangle(mangled_name.c_str(), nullptr, &length, &status);
    if (demangled_name_needs_to_be_freed == nullptr) {
        return mangled_name;
    }
    
    result = demangled_name_needs_to_be_freed;


    if (status == 0) {
        result = demangled_name_needs_to_be_freed;
    } else {
        // https://gcc.gnu.org/onlinedocs/libstdc++/libstdc++-html-USERS-4.3/a01696.html
        //-1: A memory allocation failiure occurred.
        //-2: mangled_name is not a valid name under the C++ ABI mangling rules.
        //-3: One of the arguments is invalid.
        result = mangled_name;
    }
    
    // ok if demangle failed because it will be null
    free(demangled_name_needs_to_be_freed);
    
    return result;
    

#else
    return mangled_name;
#endif
}


template<class T>
std::string & demangle() {
    std::atomic<bool> cache_set = false;
    static std::string cached_name;

    if (cache_set) {
        return cached_name;
    }

#if defined XL_USE_BOOST

    cached_name = boost::typeindex::type_id_with_cvr<T>().pretty_name();
    cache_set = true;

#elif defined XL_FORCE_NO_DEMANGLE_NAMES
    cached_name = "NO NAME MANGLING AVAILABLE";

#else

    static std::mutex mutex;

    std::lock_guard<std::mutex> lock_guard(mutex);
    if (!cache_set) {
        auto demangled_name = demangle_typeid_name(typeid(T).name());
        std::string constness = std::is_const<T>::value ? "const " : "";
        std::string volatility = std::is_volatile<T>::value ? "volatile " : "";
        cached_name = constness + volatility + demangled_name;
        cache_set = true;
    }

#endif
    return cached_name;

}

} // end namespace xl