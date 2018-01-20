#define XL_LOG_WITH_TEMPLATES

#ifdef XL_LOG_WITH_TEMPLATES
#include "../templates.h"
#endif

// requires a traditional include guard because of circular dependencies with xl::templates
#ifndef XL_LOG_LOG_H
#define XL_LOG_LOG_H


#include <type_traits>
#include <fstream>
#include <chrono>


#include "../library_extensions.h"

#include "../exceptions.h"
#include "../regex/regexer.h"
#include "../zstring_view.h"
#include "../date.h"

#include "log_status_file.h"

namespace xl::log {


class LogException : public xl::FormattedException {

public:

    using xl::FormattedException::FormattedException;
};



struct DefaultLevels  {
    inline static std::string level_names[] = {"info", "warn", "error"};

    enum class Levels {
        Info = 0, Warn, Error, LOG_LAST_LEVEL
    };
};



struct DefaultSubjects {
    inline static std::string subject_names[] = {"default"};

    enum class Subjects {
        Default = 0, LOG_LAST_SUBJECT
    };
};


template<typename LevelsT, typename SubjectsT, typename Clock>
class CopyLogger {
    using Subjects = typename SubjectsT::Subjects;
    using Levels = typename LevelsT::Levels;

    Log<LevelsT, SubjectsT, Clock> & log_object;
    Levels level;
    Subjects subject;
    std::string message_prefix;

public:
    CopyLogger(Log<LevelsT, SubjectsT, Clock> & log_object, Levels level, Subjects subject, xl::string_view message_prefix = {}) :
        log_object(log_object), level(level), subject(subject), message_prefix(message_prefix)
    {}

    template <typename... Ts>
    CopyLogger & operator()(xl::zstring_view format_string, Ts&&... args) {
        log_object.log(level, subject, fmt::format(format_string.c_str(), std::forward<Ts>(args)...));
        return *this;
    }

    CopyLogger & operator()(xl::zstring_view log_message) {
        log_object.log(level, subject, log_message);
        return *this;
    }
};


/**
 * Objects of this type take messages to be logged and route them to the registered callback.
 * @tparam Levels must provide an enum named Levels and static std::string const & get_name(Levels)
 * @tparam Subjects must provide an enum named Subjects and static std::string const & get_name(Subjects)
 */
template<class LevelsT = log::DefaultLevels, class SubjectsT = log::DefaultSubjects, class Clock = std::chrono::system_clock>
class Log {

    static_assert((size_t)LevelsT::Levels::LOG_LAST_LEVEL >= 0, "Levels enumeration must have LOG_LAST_LEVEL as its final entry");
    static_assert((size_t)SubjectsT::Subjects::LOG_LAST_SUBJECT >= 0, "Subjects enumeration must have LOG_LAST_SUBJECT as its final entry");
public:

    using LevelsBase = log::LogLevelsBase<LevelsT>;
    using SubjectsBase = log::LogSubjectsBase<SubjectsT>;

    using Levels = typename log::LogLevelsBase<LevelsT>::Levels;
    using Subjects = typename log::LogSubjectsBase<SubjectsT>::Subjects;

    using LevelsUnderlyingType = std::underlying_type_t<Levels>;
    using SubjectsUnderlyingType = std::underlying_type_t<Subjects>;

    static constexpr LevelsUnderlyingType level_count = static_cast<LevelsUnderlyingType>(Levels::LOG_LAST_LEVEL);
    static constexpr SubjectsUnderlyingType subject_count = static_cast<SubjectsUnderlyingType>(Subjects::LOG_LAST_SUBJECT);


    constexpr static auto get(Levels level) {
        return static_cast<LevelsUnderlyingType>(level);
    }

    constexpr static auto get(Subjects subject) {
        return static_cast<SubjectsUnderlyingType>(subject);
    }

    auto to(Levels level, Subjects subject, xl::string_view message_prefix = "") {
        return CopyLogger(*this, level, subject, message_prefix);
    }

    /**
     * Representation of an individual message to be logged.  Includes level, subject, and the string.
     * Also includes a reference to the log object so
     */
    struct LogMessage {
        Levels level;
        Subjects subject;
        std::string string;
        typename Clock::time_point time = Clock::now();

        LogMessage(Levels level, Subjects subject, std::string string) :
            level(level),
            subject(subject),
            string(std::move(string))
        {}

        template<typename ClockCopy = Clock, std::enable_if_t<std::is_same_v<ClockCopy, std::chrono::system_clock>> * = nullptr>
        std::string get_time_string() const {
            return date::format("%H:%M:%S", this->time);
        }
    };


    using CallbackT = std::function<void(LogMessage const & message)>;

    // if false, logs of this level/subject will be ignored
    std::bitset<level_count + subject_count> statuses;


    std::unique_ptr<LogStatusFile> log_status_file;

 void set_regex_filter(xl::zstring_view regex_string) {
        if (regex_string.empty()) {
            this->filter_regex = xl::Regex();
            this->filter_string = "";
        } else {
            this->filter_regex = xl::Regex(regex_string);
            this->filter_string = regex_string;
        }
        if (this->log_status_file) {
            this->log_status_file->regex_filter = regex_string;
        }
    }

private:
    // unique_ptr so the callback objects themselves don't move if the vector resizes
    std::vector<std::unique_ptr<CallbackT>> callbacks;

    void initialize_from_status_file() {
        if (!this->log_status_file) {
            return;
        }

        // disable updating status file as we update the object FROM the log file
        auto temp = std::move(this->log_status_file);

        this->set_regex_filter(temp->regex_filter);

        if (auto all_levels = std::get_if<bool>(&temp->levels)) {
            this->set_all_levels(*all_levels);
        } else {
            for(LevelsUnderlyingType i = 0; i < level_count; i++) {
                if (i < std::get<LogStatusFile::Statuses>(temp->levels).size()) {
                    typename LevelsT::Levels level = static_cast<Levels>(i);
                    this->set_status(level, std::get<LogStatusFile::Statuses>(temp->levels)[i].second);
                }
            }
        }

        if (auto all_subjects = std::get_if<bool>(&temp->subjects)) {
            this->set_all_subjects(*all_subjects);
        } else {
            for(SubjectsUnderlyingType i = 0; i < subject_count; i++) {
                if (i < std::get<LogStatusFile::Statuses>(temp->subjects).size()) {
                    typename SubjectsT::Subjects subject = static_cast<Subjects>(i);
                    this->set_status(subject, std::get<LogStatusFile::Statuses>(temp->subjects)[i].second);
                }
            }
        }


        // re-enable status file
        this->log_status_file = std::move(temp);
    }

    // if set, only show log messages matching this regex
    xl::Regex filter_regex;
    std::string filter_string;


public:


    static auto subjects() {
        return LogSubjectsBase<SubjectsT>();
    }


    static auto levels() {
        return log::LogLevelsBase<LevelsT>();
    }


    std::string get_status_string() const {
        std::stringstream status;

        for(size_t i = 0; i < get(Levels::LOG_LAST_LEVEL); i++) {
            auto level = (Levels)i;
            status << fmt::format("{}: {}", LevelsT::get_name(level), (bool)get_status(level));
        }
        for(size_t i = 0; i < get(Subjects::LOG_LAST_SUBJECT); i++) {
            auto subject = (Subjects)i;
            status << fmt::format("{}: {}", SubjectsT::get_name(subject), (bool)get_status(subject));
        }
        return status.str();
    }


    bool get_status(Levels level) const {
        return this->statuses[get(level)];
    }


    bool set_status(Levels level, bool new_status) {
        bool previous_status;
        previous_status = get_status(level);

        statuses[get(level)] = new_status;
        if (this->log_status_file) {
            this->log_status_file->write(*this);
        }
        return previous_status;
    }


    void set_all_subjects(bool new_status) {
        for(size_t i = 0; i < get(Subjects::LOG_LAST_SUBJECT); i++) {
            set_status(static_cast<Subjects>(i), new_status);
        }
    }

    bool get_status(Subjects subject) const {
        return statuses[get(Levels::LOG_LAST_LEVEL) + get(subject)];
    }

    void set_all_levels(bool new_status) {
        for(size_t i = 0; i < get(Levels::LOG_LAST_LEVEL); i++) {
            set_status((Levels)i, new_status);
        }
    }


    bool set_status(Subjects subject, bool new_status) {
        bool previous_status = get_status(subject);
        statuses[get(Levels::LOG_LAST_LEVEL) + get(subject)] = new_status;
        if (this->log_status_file) {
            this->log_status_file->write(*this);
        }
        return previous_status;
    }
    

    Log() :
        statuses(0xffffffffffffffff)
    {
        // in case there are more statuses than a bitfield constructor will initialize
        //   then set them here
        if constexpr(level_count + subject_count > sizeof(long long) * 8) {
            this->statuses.set(); // sets all bits to 1
        }
    }


    Log(std::string filename) : Log() {
        this->enable_status_file(filename);
    }


    Log(CallbackT log_callback) : Log()
    {
        this->add_callback(std::move(log_callback));
    }


    void clear_callbacks() {
        this->callbacks.clear();
    }


    CallbackT & add_callback(CallbackT callback) {
        this->callbacks.push_back(std::make_unique<CallbackT>(callback));
        return *this->callbacks.back();
    }


    CallbackT & add_callback(std::ostream & ostream, std::string prefix = "") {
        return this->add_callback([&ostream, prefix](LogMessage const & message) {
            ostream << "[" << message.get_time_string() << "] " << prefix << message.string << std::endl;
        });
    }


    /**
     * If the callback was passed in as a reference wrapper, this can find any corresponding entries and remove them
     * @param t pass in the object to find (not as a reference wrapper)
     */
    void remove_callback(CallbackT & callback) {

        auto i = this->callbacks.begin();
        while(i != this->callbacks.end()) {
            CallbackT & c = **i;
            if (&c == &callback) {
                i = this->callbacks.erase(i);
            } else {
                i++;
            }
        }
    }


    /**
     * Causes a status file with the given name to be maintained with the current
     *   settings of this log object
     * @param filename filename to use as status file
     * @param skip_reset don't read from the file if it already exists
     */
    void enable_status_file(std::string filename, bool skip_reset = false) {
        this->log_status_file = std::make_unique<LogStatusFile>(*this, filename, skip_reset);
        initialize_from_status_file();
    }


    void disable_status_file() {
        this->log_status_file.release();
    }


    bool is_status_file_enabled() const {
        return (bool)this->log_status_file;
    }


    void log(Levels level, Subjects subject, xl::zstring_view const & string) {
        if (this->log_status_file) {
            if (this->log_status_file->check()) {
                this->initialize_from_status_file();
            }
        }

        if (!is_live(level, subject)) {
            return;
        }

        // if there's a filter, discard the message if it doesn't match
        if (this->filter_regex && !this->filter_regex.match(string)) {
            return;
        }

        for (auto & callback : this->callbacks) {
            if (this->get_status(level) &&
                this->get_status(subject)) {
                (*callback)(LogMessage(level, subject, string));
            }
        }
    }

    template<class T = Levels, std::enable_if_t<(int)T::Info >= 0, int> = 0>
    void info(Subjects subject, xl::zstring_view message) {
        log(Levels::Info, subject, message);
    }

    template<class L = Levels, class S = Subjects,
             std::enable_if_t<(int)L::Levels::Info >= 0 && (int)S::Default >= 0, int> = 0>
    void info(xl::zstring_view message) {
        log(Levels::Info, Subjects::Default, message);
    }


    template<class T = Levels, std::enable_if_t<(int)T::Warn >= 0, int> = 0>
    void warn(Subjects subject, xl::zstring_view message) {
        log(Levels::Warn, subject, message);
    }

    template<class T = Levels, std::enable_if_t<(int)T::Error >= 0, int> = 0>
    void error(Subjects subject, xl::zstring_view message) {
        log(Levels::Error, subject, message);
    }

    static std::string const & get_name(Subjects subject) {
        return SubjectsBase::get_name(subject);
    }

    static std::string const & get_name(Levels level) {
        return LevelsBase::get_name(level);
    }

    auto get_statuses() const {
        return this->statuses;
    }

    void set_statuses(decltype(Log::statuses) statuses) {
        this->statuses = std::move(statuses);
        if (this->log_status_file) {
            this->log_status_file->write();
        }
    }

    bool is_live(Levels level, Subjects subject) {
        return !this->callbacks.empty() && this->get_status(level) && this->get_status(subject);
    }


#ifdef XL_USE_LIB_FMT
    template<class... Ts>
    void log(Levels level, Subjects subject, xl::zstring_view const & format_string, Ts && ... args) {
        if (this->is_live(level, subject)) { // don't build the string if it wont be used
            log(level, subject, fmt::format(format_string.c_str(), std::forward<Ts>(args)...));
        }
    }
    template<class L = Levels, class S = Subjects, class... Ts,
        std::enable_if_t<(int)L::Info >= 0 && (int)S::Default >= 0, int> = 0>
    void info(xl::zstring_view format_string, Ts&&... args) {
        log(Levels::Info, Subjects::Default, format_string, std::forward<Ts>(args)...);
    }

    template<class... Ts, class T = Levels, std::enable_if_t<(int)T::Info >= 0, int> = 0>
    void info(Subjects subject, xl::zstring_view const & format_string, Ts && ... args) {
        log(Levels::Info, subject, format_string, std::forward<Ts>(args)...);
    }
    template<class... Ts, class T = Levels, std::enable_if_t<(int)T::Warn >= 0, int> = 0>
    void warn(Subjects subject, xl::zstring_view const & format_string, Ts && ... args) {
        log(Levels::Warn, subject, format_string, std::forward<Ts>(args)...);
    }
    template<class... Ts, class T = Levels, std::enable_if_t<(int)T::Error >= 0, int> = 0>
    void error(Subjects subject, xl::zstring_view const & format_string, Ts && ... args) {
        log(Levels::Error, subject, format_string, std::forward<Ts>(args)...);
    }
#endif


    /**
     * For strings which are useful to log but expensive enough to be desirable to not build the string
     * when the log level/subject is disabled, using a xl::Template will allow for specifying callbacks
     * or user-defined provider objects which are only used if the string will be sent to a callback
     */
    template<typename... Ts>
    void log(Levels level, Subjects subject, xl::templates::Template const & tmpl, Ts&&... args) {
        if (this->is_live(level, subject)) { // don't build the string if it wont be used
            this->log(level, subject, tmpl.fill(std::forward<Ts>(args)...));
        }
    };
    template<class... Ts, class T = Levels, std::enable_if_t<(int)T::Info >= 0, int> = 0>
    void info(Subjects subject, xl::templates::Template const & tmpl, Ts && ... args) {
        this->log(Levels::Info, subject, tmpl, std::forward<Ts>(args)...);
    }
    template<class... Ts, class T = Levels, std::enable_if_t<(int)T::Warn >= 0, int> = 0>
    void warn(Subjects subject, xl::templates::Template const & tmpl, Ts && ... args) {
        this->log(Levels::Warn, subject, tmpl, std::forward<Ts>(args)...);
    }
    template<class... Ts, class T = Levels, std::enable_if_t<(int)T::Error >= 0, int> = 0>
    void error(Subjects subject, xl::templates::Template const & tmpl, Ts && ... args) {
        this->log(Levels::Error, subject, tmpl, std::forward<Ts>(args)...);
    }


};



/**
 * Associates a log callback being active with a xl::Log object with its own lifetime.
 * @tparam CallbackT the type of the log callback
 * @tparam LogT the type of the log object
 */
template<class CallbackT, class LogT>
class LogCallbackGuard {

    std::shared_ptr<CallbackT> callback;
    LogT & logger;
    typename LogT::CallbackT * registered_callback = nullptr;

public:

    /**
     * Uses a user-provided object as the callback.  Does not destroy it when this object is destroyed
     * @param logger
     * @param callback
     */
    LogCallbackGuard(LogT & logger, CallbackT & existing_callback_object) :
        logger(logger)
    {
        this->callback = std::shared_ptr<CallbackT>(&existing_callback_object, [](CallbackT *){});
        this->registered_callback = &this->logger.add_callback(std::ref(*callback));
    }

    /**
     * Creates an object of the specified type.  Destroys it when this object is destroyed
     * @tparam Args argument types of parameters for CallbackT's constructor
     * @param logger
     * @param args perfectly forwarded parameters to the constructor of CallbackT
     */
    template<class... Args>
    LogCallbackGuard(LogT & logger, Args&&... args) :
        callback(std::make_shared<CallbackT>(std::forward<Args>(args)...)),
        logger(logger)
    {
        this->registered_callback = &this->logger.add_callback(std::ref(*callback));
    }

    ~LogCallbackGuard(){
        this->logger.remove_callback(*this->registered_callback);
    }
};

} // end namespace xl::log

#endif