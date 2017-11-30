#pragma once


#include <string>
#include <type_traits>

namespace xl::log {



template<class T>
class EnumIterator {
    using underlying_type = std::underlying_type_t<T>;
    underlying_type value = 0;

public:

    EnumIterator() : value(0)
    {}


    EnumIterator(T t) : value(static_cast<underlying_type>(t))
    {}


    bool operator!=(EnumIterator<T> const & other) {
        return this->value != other.value;
    }

    auto operator*() const {
        //return static_cast<underlying_type>(value);
        return static_cast<T>(value);
    }

    auto operator++() {
        value++;
        return *this;
    }
};


template<class T>
struct LogLevelsBase {

    using Levels = typename T::Levels;


    EnumIterator<typename T::Levels> begin() {
        return EnumIterator<typename T::Levels>();
    }
    EnumIterator<typename T::Levels> end(){
        return EnumIterator<typename T::Levels>(T::Levels::LOG_LAST_LEVEL);
    }

    static std::string const & get_name(typename T::Levels level) {
        return T::level_names[static_cast<std::underlying_type_t<typename T::Levels>>(level)];
    }

    using UnderlyingType = std::underlying_type_t<typename T::Levels>;

    constexpr static auto get(typename T::Levels level) {
        return static_cast<UnderlyingType>(level);
    }

    static_assert(get(Levels::LOG_LAST_LEVEL) == sizeof(T::level_names) / sizeof(std::string));

};


template<class T>
struct LogSubjectsBase {


    using Subjects = typename T::Subjects;



    EnumIterator<typename T::Subjects> begin() const {
        return EnumIterator<typename T::Subjects>();
    }
    typename T::Subjects end() const {
        return T::Subjects::LOG_LAST_SUBJECT;
    }

    static std::string const & get_name(typename T::Subjects subject) {
        return T::subject_names[static_cast<std::underlying_type_t<typename T::Subjects>>(subject)];
    }

    using UnderlyingType = std::underlying_type_t<typename T::Subjects>;

    constexpr static auto get(typename T::Subjects subject) {
        return static_cast<UnderlyingType>(subject);
    }
    static_assert(get(Subjects::LOG_LAST_SUBJECT) == sizeof(T::subject_names) / sizeof(std::string));

};



} // end namespace xl::log