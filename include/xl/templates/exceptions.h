#pragma once
#include "../exceptions.h" // to pick up FormattedException
#include "log.h"
namespace xl::templates {


struct TemplateSubjects {
    inline static std::string subject_names[] = {"default", "compile", "exception"};

    enum class Subjects {
        Default = 0, Compile, Exception, LOG_LAST_SUBJECT
    };
};


using LogT = ::xl::log::Log<::xl::log::DefaultLevels, TemplateSubjects>;
inline LogT log;


class TemplateException : public xl::FormattedException {

public:

    template<typename... Args>
    TemplateException(xl::zstring_view format_string, Args&&... args);
};


} // end namespace xl