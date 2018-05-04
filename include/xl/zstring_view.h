#pragma once

#include <string>
#include <string_view>

namespace xl {
/**
 * basic_zstring_view is a std::basic_string_view that is guaranteed to be null terminated.
 * adds c_str() as a synonym for std::string_view::data to make it clear that it is null terminated like it is
 *   in std::string::c_str()
 */
template<
    class CharT,
    class Traits = std::char_traits<CharT>>
class basic_zstring_view : public std::basic_string_view<CharT, Traits> {

public:
    using std::basic_string_view<CharT, Traits>::basic_string_view;

    basic_zstring_view(std::basic_string<CharT> const & source) :
        std::basic_string_view<CharT, Traits>(source.c_str(), source.size())
    {}

    // There is intentionally no constructor that takes a std::string_view because it's not guaranteed to be
    //   NUL terminated and there's nowhere to store a memory allocation to store a copy of the string with a NUL

    CharT const * c_str() const {return this->data();}

    operator std::basic_string<CharT>() const {return std::basic_string<CharT>(this->data(), this->length());}
};

using zstring_view = basic_zstring_view<char>;
using wzstring_view = basic_zstring_view<wchar_t>;
using u16zstring_view = basic_zstring_view<char16_t>;
using u32zstring_view = basic_zstring_view<char32_t>;


template<
    class CharT,
    class Traits = std::char_traits<CharT>>
class basic_string_view : public ::std::basic_string_view<CharT, Traits> {

public:
    using std::basic_string_view<CharT, Traits>::basic_string_view;

    operator std::basic_string<CharT>() const {return std::basic_string<CharT>(this->data(), this->length());}

    basic_string_view(std::basic_string<CharT> const & std_string) :
        std::basic_string_view<CharT, Traits>(std_string.data(), std_string.length())
    {}
};


using string_view = basic_string_view<char>;
using wstring_view = basic_string_view<wchar_t>;
using u16string_view = basic_string_view<char16_t>;
using u32string_view = basic_string_view<char32_t>;


}