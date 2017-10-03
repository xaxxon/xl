
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
#include "bound_template.h"

namespace xl {







class Template {
private:
    std::string _tmpl;

public:
    Template(std::string const & tmpl = "") : _tmpl(tmpl) {}
    char const * c_str() const {return this->_tmpl.c_str();}


    template<class T>
    BoundTemplate bind(T && t);


    template<class T>
    std::string fill(T && t) {
        return this->bind(std::forward<T>(t))();
    }
};


} // end namespace xl