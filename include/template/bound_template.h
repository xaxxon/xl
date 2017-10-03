#pragma once

#include <vector>
#include <cloneable_unique_pointer.h>

#include "provider.h"
#include "template.h"

namespace xl {

class Template;
class Provider_Interface;
class ProviderOptions;


class BoundTemplate {
private:
    Template tmpl;
    std::vector<Provider> providers;

public:

    // just like a normal vector except this is assumed to not
    //   be specialized in MakeProvider and as such is used to
    //   represent a list of Provider objects
    template<class T>
    class vector : public std::vector<T> {
        using std::vector<T>::vector;
    };

    BoundTemplate(BoundTemplate &&) = default;
    BoundTemplate(BoundTemplate const &) = default;

    ~BoundTemplate();

    template<class T = Provider>
    BoundTemplate(Template tmpl, BoundTemplate::vector<T> const & source_vector);

    template<class T>
    BoundTemplate(Template tmpl, T && source = T{});

    std::string operator()();
};

};

