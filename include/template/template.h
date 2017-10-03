
#pragma once

#include <string_view>
#include <map>
#include <string>
#include <variant>
#include <sstream>
#include <regex>
#include <type_traits>
#include <functional>

#include <fmt/ostream.h>

#include "library_extensions.h"

#include "provider.h"

namespace xl {


class Template {
private:
    std::string _tmpl;

public:
    Template(std::string const & tmpl = "") : _tmpl(tmpl) {}
    char const * c_str() const {return this->_tmpl.c_str();}

    template<class T>
    std::string fill(T && t);
};


} // end namespace xl

#include "bound_template_impl.h"

namespace xl {
template<class T>
std::string Template::fill(T && t) {
    return BoundTemplate(*this, std::forward<T>(t))();
}
} // end namespace xl