#pragma once

namespace xl {


class TemplateException : public std::exception {
    std::string reason;

public:

    TemplateException(std::string const & reason) : reason(reason) {}
    TemplateException(std::string && reason) : reason(std::move(reason)) {}

    const char * what() const noexcept {
        return reason.c_str();
    }

};

} // end namespace xl