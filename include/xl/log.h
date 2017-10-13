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

} // end namespace log
// back in namespace xl

/**
 * Objects of this type take messages to be logged and route them to the registered callback.
 * @tparam Levels must provide an enum named Levels and static std::string const & get_level_name(Levels)
 * @tparam Subjects must provide an enum named Subjects and static std::string const & get_subject_name(Subjects)
 */
template<class Levels = log::DefaultLevels, class Subjects = log::DefaultSubjects>
class Log {
public:

    struct LogMessage {
        Log & log;
        typename Levels::Levels level;
        typename Subjects::Subjects subject;
        zstring_view string;

        LogMessage(Log & log, typename Levels::Levels level, typename Subjects::Subjects subject, zstring_view string) :
            log(log),
            level(level),
            subject(subject),
            string(string)
        {}
    };


    using LogCallback = std::function<void(LogMessage const & message)>;

    // if false, logs of this level will be ignored
    std::vector<char> level_status;

    // if false, logs for this subject will be ignored
    std::vector<char> subject_status;

private:
    std::vector<LogCallback> callbacks;

public:

    bool set_level_status(typename Levels::Levels level, bool new_status) {
        bool previous_status;
        if (level_status.size() <= (int)level) {
            previous_status = true;
            level_status.resize((int)level + 1, 1);
        } else {
            previous_status = level_status[(int)level];
        }
        level_status[(int)level] = new_status;
        return previous_status;
    }
    
    bool get_level_status(typename Levels::Levels level) {
        if (level_status.size() <= (int)level) {
            return true;
        } else {
            return level_status[(int)level];
        }
    }

    bool set_subject_status(typename Subjects::Subjects subject, bool new_status) {
        bool previous_status;
        if (subject_status.size() <= (int)subject) {
            previous_status = true;
            subject_status.resize((int)subject + 1, 1);
        } else {
            previous_status = subject_status[(int)subject];
        }
        subject_status[(int)subject] = new_status;
        return previous_status;
    }
    
    bool get_subject_status(typename Subjects::Subjects subject) {
        if (subject_status.size() <= (int)subject) {
            return true;
        } else {
            return subject_status[(int)subject];
        }
    }
    Log() = default;

    Log(LogCallback log_callback) :
        callbacks{log_callback}
    {}

    void set_log_callback(LogCallback log_callback) {
        this->log_callback = log_callback;
    }

    void clear_callbacks() {
        this->callbacks.clear();
    }

    void add_callback(LogCallback callback) {
        this->callbacks.push_back(callback);
    }

    /**
     * If the callback was passed in as a reference wrapper, this can find any corresponding entries and remove them
     * @param t pass in the object to find (not as a reference wrapper)
     */
    template<class T>
    void remove_callback(T const & t) {

        auto i = this->callbacks.begin();
        while(i != this->callbacks.end()) {
            auto & callback = *i;
            if (std::reference_wrapper<T> const * ref = callback.template target<std::reference_wrapper<T>>()) {
                if (&ref->get() == &t) {
                    i = this->callbacks.erase(i);
                    continue;
                }
            }
            i++;
        }
    }


    void log(typename Levels::Levels level, typename Subjects::Subjects subject, xl::zstring_view const & string) {
        for (auto & callback : this->callbacks) {
            if (this->get_level_status(level) &&
                this->get_subject_status(subject)) {
                callback(LogMessage(*this, level, subject, string));
            }
        }
    }

    template<class T = Levels, std::enable_if_t<(int)T::Levels::Info >= 0, int> = 0>
    void info(typename Subjects::Subjects subject, xl::zstring_view message) {
        log(Levels::Levels::Info, subject, message);
    }

    template<class T = Levels, std::enable_if_t<(int)T::Levels::Warn >= 0, int> = 0>
    void warn(typename Subjects::Subjects subject, xl::zstring_view message) {
        log(Levels::Levels::Warn, subject, message);
    }

    template<class T = Levels, std::enable_if_t<(int)T::Levels::Error >= 0, int> = 0>
    void error(typename Subjects::Subjects subject, xl::zstring_view message) {
        log(Levels::Levels::Error, subject, message);
    }

    std::string const & get_subject_name(typename Subjects::Subjects subject) const {
        return Subjects::get_subject_name(subject);
    }

    std::string const & get_level_name(typename Levels::Levels level) const {
        return Subjects::get_level_name(level);
    }


#ifdef XL_USE_LIB_FMT
    template<class... Ts>
    void log(typename Levels::Levels level, typename Subjects::Subjects subject, xl::zstring_view const & format_string, Ts && ... args) {
        log(level, subject, fmt::format(format_string.c_str(), std::forward<Ts>(args)...));
    }
    template<class... Ts, class T = Levels, std::enable_if_t<(int)T::Levels::Info >= 0, int> = 0>
    void info(typename Subjects::Subjects subject, xl::zstring_view const & format_string, Ts && ... args) {
        log(Levels::Levels::Info, subject, fmt::format(format_string.c_str(), std::forward<Ts>(args)...));
    }
    template<class... Ts, class T = Levels, std::enable_if_t<(int)T::Levels::Warn >= 0, int> = 0>
    void warn(typename Subjects::Subjects subject, xl::zstring_view const & format_string, Ts && ... args) {
        log(Levels::Levels::Warn, subject, fmt::format(format_string.c_str(), std::forward<Ts>(args)...));
    }
    template<class... Ts, class T = Levels, std::enable_if_t<(int)T::Levels::Error >= 0, int> = 0>
    void error(typename Subjects::Subjects subject, xl::zstring_view const & format_string, Ts && ... args) {
        log(Levels::Levels::Error, subject, fmt::format(format_string.c_str(), std::forward<Ts>(args)...));
    }
#endif
};

template<class CallbackT, class LogT>
class LogCallbackGuard {

    CallbackT callback;
    LogT & logger;

public:
    template<class... Args>
    LogCallbackGuard(LogT & logger, Args&&... args) :
        callback(std::forward<Args>(args)...),
        logger(logger)
    {
        this->logger.add_callback(std::ref(callback));
    }

    ~LogCallbackGuard(){
        this->logger.remove_callback(callback);
    }
};

} // end namespace xl