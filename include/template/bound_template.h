#pragma once

#include "provider.h"


namespace xl {

class Template;
class Provider_Interface;
class ProviderOptions;


class BoundTemplate {
private:
    std::unique_ptr<Template> tmpl;
    std::vector <std::unique_ptr<Provider_Interface>> providers;

public:

    template<class T>
    BoundTemplate(Template && tmpl, T && source);

    BoundTemplate(BoundTemplate &&) = default;
    BoundTemplate(BoundTemplate const &);

    ~BoundTemplate();

    template<class T>
    BoundTemplate(Template & tmpl, std::vector <T> && source_vector = std::vector < T > {});


    std::string operator()();
};


};