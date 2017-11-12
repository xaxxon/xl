#pragma once

#include <exception>
#include <string>

#include <fmt/ostream.h>

#include "zstring_view.h"

namespace xl {


/**
 * Exception class which makes it easy to use libfmt-style formatted strings to create exception message
 */
class FormattedException : public std::exception {

private:
    std::string what_string = "";


public:

    /**
     * If only a single string is provided, no formatting will be done, so { and } don't need to be escaped
     * @param format_string
     */
    FormattedException(xl::zstring_view format_string) :
        what_string(format_string)
    {}


    /**
     * If more than one parameter provided, the first will be used as a formatting string and the other
     * parameters will be used as substitution arguments for that formatting string
     * @param format_string format string for libfmt
     * @param args substitutions for the format string
     */
    template<typename... Args>
    FormattedException(xl::zstring_view format_string, Args&&... args) :
        what_string(fmt::format(format_string.c_str(), std::forward<Args>(args)...))
    {}


    char const * what() const noexcept override  {
        return this->what_string.c_str();
    }
};


} // end namespace xl