
#pragma once

#include <experimental/filesystem>
#include <chrono>
#include <fmt/ostream.h>
#include <fstream>
#include <variant>

#include "../exceptions.h"

// Make sure json include before regexer include so PCRE is used
#include "../json.h"
#include "../regexer.h"

#include "log_enum_bases.h"


namespace xl::log {

/**
 * Whether the contents of an existing log status file should be used to initialize
 * the log status or if it should be overwritten with new values
 */
enum class StatusFile {RESET_FILE_CONTENTS = 0, USE_FILE_CONTENTS};


namespace fs = std::experimental::filesystem;
using namespace std::chrono_literals;


class LogStatusFileException : public xl::FormattedException {

public:

    using xl::FormattedException::FormattedException;
};


template<class LevelsT, class SubjectsT, class Clock>
class Log;

class LogStatusFile {

private:
    using file_clock_type = fs::file_time_type::clock;

    // store these locally because the user may not have access to a Log object
    //   so in that case, just populate from the file
    std::string filename;
    decltype(std::chrono::system_clock::now()) last_file_change_check_time;
    std::experimental::filesystem::path status_file;
    file_clock_type::time_point last_seen_write_time_for_status_file;

public:

    std::string regex_filter;
    using StatusPair = std::pair<std::string, bool>;
    using Statuses = std::vector<StatusPair>;
    std::variant<bool, Statuses> levels = true;
    std::variant<bool, Statuses> subjects = true;

    Statuses & level_vector() {
        if (auto level_vector = std::get_if<Statuses>(&this->levels)) {
            return *level_vector;
        } else {
            throw LogStatusFileException("levels are not currently available as a vector");
        }
    }

    Statuses & subject_vector() {
        if (auto subject_vector = std::get_if<Statuses>(&this->subjects)) {
            return *subject_vector;
        } else {
            throw LogStatusFileException("subjects are not currently available as a vector");
        }
    }



    LogStatusFile(std::string filename, StatusFile status_file_flag) : filename(filename), status_file(filename) {
        if (std::experimental::filesystem::exists(this->status_file) && status_file_flag == StatusFile::USE_FILE_CONTENTS) {
            this->read();
        } else {
            // write out default config
            this->write();
        }
    }

    template<typename LevelsT, typename SubjectsT, typename Clock>
    LogStatusFile(Log<LevelsT, SubjectsT, Clock> const & log, std::string filename, StatusFile status_file_flag) :
        LogStatusFile(filename, status_file_flag)
    {
        // If the file doesn't exist or user specified not to use it then write out the current state of the log
        //   object this object is attached to
        if (!std::experimental::filesystem::exists(this->status_file) || status_file_flag == StatusFile::RESET_FILE_CONTENTS) {
            this->initialize_from_log(log);
            this->write();
        }
    }

    ~LogStatusFile() {}


    template<typename LevelsT, typename SubjectsT, typename Clock>
    void initialize_from_log(Log<LevelsT, SubjectsT, Clock> const & log) {
        this->levels = Statuses{};
        for(size_t i = 0; i < LogLevelsBase<LevelsT>::get(LevelsT::Levels::LOG_LAST_LEVEL); i++) {
            typename LevelsT::Levels level = static_cast<typename LevelsT::Levels>(i);
            std::get<std::vector<std::pair<std::string, bool>>>(this->levels).emplace_back(std::pair(log.get_name(level), log.get_status(level)));
        }

        this->subjects = Statuses{};
        for(size_t i = 0; i < LogSubjectsBase<SubjectsT>::get(SubjectsT::Subjects::LOG_LAST_SUBJECT); i++) {
            typename SubjectsT::Subjects subject = static_cast<typename SubjectsT::Subjects>(i);
            auto pair = std::pair(log.get_name(subject), log.get_status(subject));
            std::get<std::vector<std::pair<std::string, bool>>>(this->subjects).emplace_back(std::pair(log.get_name(subject), log.get_status(subject)));
        }
        this->write();
    }





    void read() {
        this->levels   = true; // default back to showing everything
        this->subjects = true; // default back to showing everything
        this->regex_filter.clear();

        std::ifstream file(filename);
        if (!file) {
            return;
        }
        std::string log_status_file_contents((std::istreambuf_iterator<char>(file)),
                                                    std::istreambuf_iterator<char>());
//        std::cerr << fmt::format("Just loaded log status file contents: {}", log_status_file_contents) << std::endl;
        xl::json::Json log_status(log_status_file_contents);

        if (auto regex = log_status["regex"].get_string()) {
            this->regex_filter = *regex;
        }
        if (auto all_level_status = log_status["all_level_status"].get_boolean()) {
            this->levels = *all_level_status;
        }
        if (auto all_subject_status = log_status["all_subject_status"].get_boolean()) {
            this->subjects = *all_subject_status;
        }

        if (log_status["levels"].get_array()) {
            this->levels = Statuses{};
            for (auto level : log_status["levels"].as_array()) {
                auto name = level.as_object()["name"].get_string();
                auto status = level.as_object()["status"].get_boolean();
                if (!name || !status) {
                    throw LogStatusFileException("Invalid log level configuration");
                }
                std::get<Statuses>(this->levels).emplace_back(*name, *status);
            }
        }

        if (log_status["subjects"].get_array()) {
            this->subjects = Statuses{};
            for (auto subject : log_status["subjects"].as_array()) {
                auto name = subject.as_object()["name"].get_string();
                auto status = subject.as_object()["status"].get_boolean();
                if (!name || !status) {
                    throw LogStatusFileException("Invalid log subject configuration: {}", subject.get_source());
                }
                std::get<Statuses>(this->subjects).emplace_back(*name, *status);
            }
        }

        this->last_seen_write_time_for_status_file = fs::last_write_time(this->status_file);
    }


    template<typename LevelsT, typename SubjectsT, typename Clock>
    void write(Log<LevelsT, SubjectsT, Clock> const & log) {
        this->initialize_from_log(log);
        this->write();
    };


    void write() {
        std::ofstream file(this->filename);
        if (!file) {
            return;
        }

        file << "{\n";
        if (!this->regex_filter.empty()) {
            file << "\"regex\": \"" << this->regex_filter << "\",\n";
        }
        if (auto all_level_status = std::get_if<bool>(&this->levels)) {
            file << "\"all_level_status\": " << (*all_level_status ? "true" : "false") << ",\n";
        }
        if (auto all_subject_status = std::get_if<bool>(&this->subjects)) {
            file << "\"all_subject_status\": " << (*all_subject_status ? "true" : "false") << ",\n";
        }
        if (auto levels = std::get_if<Statuses>(&this->levels)) {
            file << "    \"levels\": [\n";
            for (auto const & [name, status] : *levels) {
                file << "{\"name\": \"" << name << "\", \"status\": " << (status ? "true" : "false") << "},\n";
            }
            file << "    ],\n";
        }
        if (auto subjects = std::get_if<Statuses>(&this->subjects)) {
            file << "    \"subjects\": [\n";
            for (auto const &[name, status] : *subjects) {
                file << "{\"name\": \"" << name << "\", \"status\": " << (status ? "true" : "false") << "},\n";
            }
            file << "    ],\n";
        }
        file << "}\n";

    }


    bool check() {
        // check to see if the timestamp on the status file has been updated
        if (std::chrono::system_clock::now() - this->last_file_change_check_time < 1000ms) {
            return false;
        }
        this->last_file_change_check_time = std::chrono::system_clock::now();

        if (std::experimental::filesystem::exists(this->status_file)) {
            auto last_write_time = fs::last_write_time(this->status_file);
            if (last_write_time > this->last_seen_write_time_for_status_file) {
                // need to read the new values
                this->read();
                return true;
            } else {
                // nothing to do here - status file hasn't changed
                return false;
            }
        }
        return false;
    }
};



} // end namespace xl::log
