#pragma once

namespace xl::templates {


struct TemplateSubjects {
    inline static std::string subject_names[] = {"default", "compile", "exception"};

    enum class Subjects {
        Default, Compile, Exception, LOG_LAST_SUBJECT
    };
};


using LogT = ::xl::log::Log<::xl::log::DefaultLevels, TemplateSubjects>;
inline LogT log;


class TemplateException : public xl::FormattedException {

public:

    template<typename... Args>
    TemplateException(xl::zstring_view format_string, Args&&... args) :
        xl::FormattedException(format_string.c_str(), std::forward<Args>(args)...)
    {
        xl::templates::log.error(LogT::Subjects::Exception, this->what());
    }
};


} // end namespace xl