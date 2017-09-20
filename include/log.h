#pragma once

#include <type_traits>

#include <fmt/ostream.h>

#include "zstring_view.h"


namespace xl {

namespace log {
class DefaultLevels {
    inline static std::string level_names[] = {"info", "warn", "error"};

public:
    enum class Levels {
        Info, Warn, Error
    };

    static std::string const & get_level_name(Levels level) {
        return level_names[static_cast<std::underlying_type_t<Levels>>(level)];
    }
};

class DefaultSubjects {
    inline static std::string subject_names[] = {"default"};

public:
    enum class Subjects {
        Default
    };

    static std::string const & get_subject_name(Subjects subject) {
        return subject_names[static_cast<std::underlying_type_t<Subjects>>(subject)];
    }
};
}

template<class Levels = log::DefaultLevels, class Subjects = log::DefaultSubjects>
class Log {
public:

    struct LogMessage {
        typename Levels::Levels level;
        typename Subjects::Subjects subject;
        zstring_view string;

        LogMessage(typename Levels::Levels level, typename Subjects::Subjects subject, zstring_view string) :
            level(level),
            subject(subject),
            string(string)
        {}
    };


    using LogCallback = std::function<void(Log const &, LogMessage const & message)>;


private:
    LogCallback log_callback;

public:


    Log(LogCallback log_callback = LogCallback()) :
        log_callback(log_callback)
    {}

    void set_log_callback(LogCallback log_callback) {
        this->log_calback = log_callback;
    }

    void log(typename Levels::Levels level, typename Subjects::Subjects subject, xl::zstring_view const & string) {
        if (this->log_callback) {
            this->log_callback(*this, LogMessage(level, subject, string));
        }
    }

#ifdef XL_USE_LIB_FMT
    template<class... Ts>
    void log(typename Levels::Levels level, typename Subjects::Subjects subject, xl::zstring_view const & format_string, Ts && ... args) {
        log(level, subject, fmt::format(format_string.c_str(), std::forward<Ts>(args)...));
    }
    template<class... Ts>
    void info(typename Subjects::Subjects subject, xl::zstring_view const & format_string, Ts && ... args) {
        log(Levels::Levels::Info, subject, fmt::format(format_string.c_str(), std::forward<Ts>(args)...));
    }
    template<class... Ts>
    void warn(typename Subjects::Subjects subject, xl::zstring_view const & format_string, Ts && ... args) {
        log(Levels::Levels::Warn, subject, fmt::format(format_string.c_str(), std::forward<Ts>(args)...));
    }
    template<class... Ts>
    void error(typename Subjects::Subjects subject, xl::zstring_view const & format_string, Ts && ... args) {
        log(Levels::Levels::Error, subject, fmt::format(format_string.c_str(), std::forward<Ts>(args)...));
    }
#endif
};




}