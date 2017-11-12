#pragma once

#include <exception>
#include <string>

#include <fmt/ostream.h>

#include "zstring_view.h"

namespace xl {


class FormattedException : public std::exception {

private:
    std::string what_string;


public:

    template<typename... Args>
    FormattedException(xl::zstring_view format_string, Args&&... args) :
        what_string(fmt::format(format_string.c_str(), std::forward<Args>(args)...))
    {}


    char const * what() const noexcept override  {
        return this->what_string.c_str();
    }
};


} // end namespace xl