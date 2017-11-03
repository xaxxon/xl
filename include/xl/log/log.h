#pragma once

#include <type_traits>
#include <fstream>
#include <chrono>
#include <fmt/ostream.h>
#include <experimental/filesystem>
#include "../library_extensions.h"

namespace fs = std::experimental::filesystem;

using namespace std::literals;

#include "../regex/regexer.h"

#include "../zstring_view.h"


namespace xl {

namespace log {


class DefaultLevels {
    inline static std::string level_names[] = {"info", "warn", "error"};

public:
    enum class Levels {
        Info, Warn, Error, LOG_LAST_LEVEL
    };

    static std::string const & get_level_name(Levels level) {
        return level_names[static_cast<std::underlying_type_t<Levels>>(level)];
    }
};


class DefaultSubjects {
    inline static std::string subject_names[] = {"default"};

public:
    enum class Subjects {
        Default, LOG_LAST_SUBJECT
    };

    static std::string const & get_subject_name(Subjects subject) {
        return subject_names[static_cast<std::underlying_type_t<Subjects>>(subject)];
    }
};

} // end namespace log
// back in namespace xl

template<class LevelsT, class SubjectsT>
class Log;

class LogStatusFile {

private:
    using file_clock_type = fs::file_time_type::clock;

    // store these locally because the user may not have access to a Log object
    //   so in that case, just populate from the file
    std::string filename;
    std::chrono::time_point<file_clock_type> last_file_change_check_time;
    char * timestamp; // what type to make this?
    std::experimental::filesystem::path status_file;
    file_clock_type::time_point last_seen_write_time_for_status_file;

public:

    std::vector<std::pair<std::string, bool>> level_names;
    std::vector<std::pair<std::string, bool>> subject_names;

    template<typename LevelsT, typename SubjectsT>
    LogStatusFile(Log<LevelsT, SubjectsT> const & log, std::string filename, bool force_reset) :
        LogStatusFile(filename)
    {
        if (!std::experimental::filesystem::exists(this->status_file) || force_reset) {
//            std::cerr << fmt::format("Creating new file") << std::endl;
            this->initialize_from_log(log);
            write();
        } else {
//            std::cerr << fmt::format("not creating new file") << std::endl;
        }
    }


    template<typename LevelsT, typename SubjectsT>
    void initialize_from_log(Log<LevelsT, SubjectsT> const & log) {
        this->level_names.clear();
        for(size_t i = 0; i < (size_t)LevelsT::Levels::LOG_LAST_LEVEL; i++) {
            typename LevelsT::Levels level = static_cast<typename LevelsT::Levels>(i);
            this->level_names.emplace_back(std::pair(LevelsT::get_level_name(level), log.get_level_status(level)));
        }

        this->subject_names.clear();
        for(size_t i = 0; i < (size_t)SubjectsT::Subjects::LOG_LAST_SUBJECT; i++) {
            typename SubjectsT::Subjects subject = static_cast<typename SubjectsT::Subjects>(i);
            this->subject_names.emplace_back(std::pair(SubjectsT::get_subject_name(subject), log.get_subject_status(subject)));
        }
    }


    LogStatusFile(std::string filename) : filename(filename), status_file(filename) {
        read();
    }


    void read() {
        std::ifstream file(filename);
        if (!file) {
            std::cerr << fmt::format("file not found") << std::endl;
            return;
        }
        std::string line;
        xl::RegexPcre line_regex("^([01])\\s+(.+)\\n*$");


        std::getline(file, line);
        auto level_count = std::stoi(line);
        this->level_names.clear();
        for (int i = 0; i < level_count; i++) {
            std::getline(file, line);
            if (auto matches = line_regex.match(line)) {
//                std::cerr << fmt::format("read: {}: {}", matches[2], matches[1]) << std::endl;
                this->level_names.push_back(std::pair(matches[2], std::stoi(matches[1])));
            } else {
                // TODO: Proper error handling
                // should probably just delete the file if it's invalid
                assert(false);
            }
        }

        std::getline(file, line);
        auto subject_count = std::stoi(line);
        this->subject_names.clear();
        for (int i = 0; i < subject_count; i++) {
            std::getline(file, line);
            if (auto matches = line_regex.match(line)) {
                this->subject_names.push_back(std::pair(matches[2], std::stoi(matches[1])));
            } else {
                // TODO: Proper error handling
                // should probably just delete the file if it's invalid
                assert(false);
            }
        }

        this->last_seen_write_time_for_status_file = fs::last_write_time(this->status_file);
        auto foo = file_clock_type::to_time_t(this->last_seen_write_time_for_status_file);
//        std::cerr << fmt::format("in read(), setting last seen write time to {}", std::ctime(&foo)) << std::endl;

    }


    template<typename LevelsT, typename SubjectsT>
    void write(Log<LevelsT, SubjectsT> const & log) {
        this->initialize_from_log(log);
        this->write();
    };


    void write() {
        std::ofstream file(this->filename);
        // write number of levels
        file << this->level_names.size() << std::endl;
        for(auto const & [name, status] : this->level_names) {
            // write 0/1 then name
            file << status << " " << name << std::endl;
        }


        // write number of subjects
        file << this->subject_names.size() << std::endl;
        for(auto const & [name, status] : subject_names) {
            // write 0/1 then name
            file << status << " " << name << std::endl;
        }

    }


    bool check() {
        // check to see if the timestamp on the status file has been updated
        if (std::chrono::system_clock::now() - this->last_file_change_check_time < 1000ms) {
//            std::cerr << fmt::format("not time to check file yet") << std::endl;
            return false;
        }
        this->last_file_change_check_time = std::chrono::system_clock::now();

        if (std::experimental::filesystem::exists(this->status_file)) {
            auto last_write_time = fs::last_write_time(this->status_file);
//            auto foo1 = file_clock_type::to_time_t(last_write_time);
//            auto foo2 = file_clock_type::to_time_t(this->last_seen_write_time_for_status_file);
//            std::cerr << fmt::format("comparing write times: file: {} vs last read: {}", std::ctime(&foo1), std::ctime(&foo2)) << std::endl;
            if (last_write_time > this->last_seen_write_time_for_status_file) {
                // need to read the new values
//                std::cerr << fmt::format("need to re-read file") << std::endl;
                this->read();
                return true;
            } else {
                // nothing to do here - status file hasn't changed
//                std::cerr << fmt::format("file hasn't changed") << std::endl;
                return false;
            }
        }
        return false;
    }
};


/**
 * Objects of this type take messages to be logged and route them to the registered callback.
 * @tparam Levels must provide an enum named Levels and static std::string const & get_level_name(Levels)
 * @tparam Subjects must provide an enum named Subjects and static std::string const & get_subject_name(Subjects)
 */
template<class LevelsT = log::DefaultLevels, class SubjectsT = log::DefaultSubjects>
class Log {
    static_assert((size_t)LevelsT::Levels::LOG_LAST_LEVEL >= 0, "Levels enumeration must have LOG_LAST_LEVEL as its final entry");
    static_assert((size_t)SubjectsT::Subjects::LOG_LAST_SUBJECT >= 0, "Subjects enumeration must have LOG_LAST_SUBJECT as its final entry");
public:

    using Levels = LevelsT;
    using Subjects = SubjectsT;

    struct LogMessage {
        Log & log;
        typename Levels::Levels level;
        typename Subjects::Subjects subject;
        std::string string;

        LogMessage(Log & log, typename Levels::Levels level, typename Subjects::Subjects subject, std::string string) :
            log(log),
            level(level),
            subject(subject),
            string(std::move(string))
        {}
    };


    using CallbackT = std::function<void(LogMessage const & message)>;

    // if false, logs of this level will be ignored
    std::vector<char> level_status;

    // if false, logs for this subject will be ignored
    std::vector<char> subject_status;

    std::unique_ptr<LogStatusFile> log_status_file;

private:
    // unique_ptr so the callback objects themselves don't move if the vector resizes
    std::vector<std::unique_ptr<CallbackT>> callbacks;

    void initialize_from_status_file() {
        if (!this->log_status_file) {
            return;
        }

        // disable updating status file as we update the object FROM the log file
        auto temp = std::move(this->log_status_file);

//        std::cerr << fmt::format("init from file: file level names size: {}", temp->level_names.size()) << std::endl;
        for(size_t i = 0; i < (size_t)LevelsT::Levels::LOG_LAST_LEVEL; i++) {
            typename LevelsT::Levels level = static_cast<typename LevelsT::Levels>(i);
//            std::cerr << fmt::format("initialize from status file, level: {}", (int)level) << std::endl;

            this->set_level_status(level, temp->level_names[i].second);
        }

        for(size_t i = 0; i < (size_t)SubjectsT::Subjects::LOG_LAST_SUBJECT; i++) {
            typename SubjectsT::Subjects subject = static_cast<typename SubjectsT::Subjects>(i);
            this->set_subject_status(subject, temp->subject_names[i].second);
        }

        // re-enable status file
        this->log_status_file = std::move(temp);
    }

public:

    bool set_level_status(typename Levels::Levels level, bool new_status) {
//        std::cerr << fmt::format("setting {} to {}", (int)level, new_status) << std::endl;
        bool previous_status;
        if (level_status.size() <= (int)level) {
            previous_status = true;
            level_status.resize((int)level + 1, 1);
        } else {
            previous_status = level_status[(int)level];
        }
        level_status[(int)level] = new_status;
        if (this->log_status_file) {
            this->log_status_file->write(*this);
        }
        return previous_status;
    }
    
    bool get_level_status(typename Levels::Levels level) const {
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
        if (this->log_status_file) {
            this->log_status_file->write(*this);
        }
        return previous_status;
    }
    
    bool get_subject_status(typename Subjects::Subjects subject) const {
        if (subject_status.size() <= (int)subject) {
            return true;
        } else {
            return subject_status[(int)subject];
        }
    }
    Log() = default;
    Log(std::string filename) {
        this->enable_status_file(filename);
    }
    Log(CallbackT log_callback)
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

    CallbackT & add_callback(std::ostream & ostream, std::string prefix) {
        return this->add_callback([&](LogMessage const & message) {
            ostream << prefix << message.string << std::endl;
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
     */
    void enable_status_file(std::string filename, bool force_reset = false) {
        this->log_status_file = std::make_unique<LogStatusFile>(*this, filename, force_reset);
        initialize_from_status_file();
    }


    void log(typename Levels::Levels level, typename Subjects::Subjects subject, xl::zstring_view const & string) {
        if (this->log_status_file) {
            if (this->log_status_file->check()) {
                this->initialize_from_status_file();
            }
        }
        for (auto & callback : this->callbacks) {
            if (this->get_level_status(level) &&
                this->get_subject_status(subject)) {
                (*callback)(LogMessage(*this, level, subject, string));
            }
        }
    }

    template<class T = Levels, std::enable_if_t<(int)T::Levels::Info >= 0, int> = 0>
    void info(typename Subjects::Subjects subject, xl::zstring_view message) {
        log(Levels::Levels::Info, subject, message);
    }

    template<class L = Levels, class S = Subjects,
             std::enable_if_t<(int)L::Levels::Info >= 0 && (int)S::Subjects::Default >= 0, int> = 0>
    void info(xl::zstring_view message) {
        log(Levels::Levels::Info, Subjects::Subjects::Default, message);
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
        return Levels::get_level_name(level);
    }


#ifdef XL_USE_LIB_FMT
    template<class... Ts>
    void log(typename Levels::Levels level, typename Subjects::Subjects subject, xl::zstring_view const & format_string, Ts && ... args) {
        log(level, subject, fmt::format(format_string.c_str(), std::forward<Ts>(args)...));
    }
    template<class L = Levels, class S = Subjects, class... Ts,
        std::enable_if_t<(int)L::Levels::Info >= 0 && (int)S::Subjects::Default >= 0, int> = 0>
    void info(xl::zstring_view format_string, Ts&&... ts) {
        log(Levels::Levels::Info, Subjects::Subjects::Default, fmt::format(format_string.c_str(), std::forward<Ts>(ts)...));
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

} // end namespace xl