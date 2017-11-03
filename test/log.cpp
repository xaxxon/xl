
#include <sstream>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "log.h"

using namespace xl;

TEST(log, SimpleLog) {
    {
        using LogT = xl::Log<xl::log::DefaultLevels, xl::log::DefaultSubjects>;
        int call_count = 0;
        LogT log([&call_count](auto & message){call_count++;});
        EXPECT_EQ(call_count, 0);
        log.log(xl::log::DefaultLevels::Levels::Warn, xl::log::DefaultSubjects::Subjects::Default, "test");
        EXPECT_EQ(call_count, 1);
        log.log(xl::log::DefaultLevels::Levels::Warn, xl::log::DefaultSubjects::Subjects::Default, "test");
        EXPECT_EQ(call_count, 2);
        log.info(xl::log::DefaultSubjects::Subjects::Default, "Info");
        log.warn(xl::log::DefaultSubjects::Subjects::Default, "Warning");
        log.error(xl::log::DefaultSubjects::Subjects::Default, "Error");
        EXPECT_EQ(call_count, 5);
        log.set_level_status(xl::log::DefaultLevels::Levels::Warn, false);
        log.warn(xl::log::DefaultSubjects::Subjects::Default, "Warning");
        EXPECT_EQ(call_count, 5);
        log.info(xl::log::DefaultSubjects::Subjects::Default, "Info");
        log.error(xl::log::DefaultSubjects::Subjects::Default, "Error");
        EXPECT_EQ(call_count, 7);

        EXPECT_EQ(log.get_subject_name(xl::log::DefaultSubjects::Subjects::Default), "default");
        EXPECT_EQ(log.get_level_name(xl::log::DefaultLevels::Levels::Warn), "warn");
    }
}




TEST(log, LogCallbackGuard) {

    int global_log_count = 0;
    using LogT = xl::Log<xl::log::DefaultLevels, xl::log::DefaultSubjects>;
    struct LogCallback {
        int & counter ;

        LogCallback(int & counter) : counter(counter) {}
        void operator()(LogT::LogMessage const & message) {
            counter++;
        }
    };

    LogT log;

    EXPECT_EQ(global_log_count, 0);
    log.log(xl::log::DefaultLevels::Levels::Warn, xl::log::DefaultSubjects::Subjects::Default, "test");
    EXPECT_EQ(global_log_count, 0);
    {
        LogCallbackGuard<LogCallback, LogT> g(log, global_log_count);
        log.log(xl::log::DefaultLevels::Levels::Warn, xl::log::DefaultSubjects::Subjects::Default, "test");
        EXPECT_EQ(global_log_count, 1);
        {
            LogCallbackGuard<LogCallback, LogT> g2(log, global_log_count);
            log.log(xl::log::DefaultLevels::Levels::Warn, xl::log::DefaultSubjects::Subjects::Default, "test");
            EXPECT_EQ(global_log_count, 3);

        }
        log.log(xl::log::DefaultLevels::Levels::Warn, xl::log::DefaultSubjects::Subjects::Default, "test");
        EXPECT_EQ(global_log_count, 4);

    }
    log.log(xl::log::DefaultLevels::Levels::Warn, xl::log::DefaultSubjects::Subjects::Default, "test");
    EXPECT_EQ(global_log_count, 4);
}

TEST(log, LogCallbackGuard_ReUseCallbackObject) {

    using LogT = xl::Log<xl::log::DefaultLevels, xl::log::DefaultSubjects>;
    struct LogCallback {
        int counter = 0;

        LogCallback() {}
        void operator()(LogT::LogMessage const & message) {
            this->counter++;
        }
        ~LogCallback() {
//            std::cerr << fmt::format("log callback destructor") << std::endl;
        }
    };

    LogT log;
    LogCallback log_callback;
    log.log(xl::log::DefaultLevels::Levels::Warn, xl::log::DefaultSubjects::Subjects::Default, "test");
    EXPECT_EQ(log_callback.counter, 0);

    {
        LogCallbackGuard g(log, log_callback);
        log.log(xl::log::DefaultLevels::Levels::Warn, xl::log::DefaultSubjects::Subjects::Default, "test");
        EXPECT_EQ(log_callback.counter, 1);
        {
            LogCallbackGuard g(log, log_callback);
            log.log(xl::log::DefaultLevels::Levels::Warn, xl::log::DefaultSubjects::Subjects::Default, "test");
            EXPECT_EQ(log_callback.counter, 3);

        }
        log.log(xl::log::DefaultLevels::Levels::Warn, xl::log::DefaultSubjects::Subjects::Default, "test");
        EXPECT_EQ(log_callback.counter, 4);
    }
    log.log(xl::log::DefaultLevels::Levels::Warn, xl::log::DefaultSubjects::Subjects::Default, "test");
    EXPECT_EQ(log_callback.counter, 4);
}


TEST(log, MultipleCallbacks) {
    {
        using LogT = xl::Log<xl::log::DefaultLevels, xl::log::DefaultSubjects>;
        struct LogCallback {
            int counter = 0;
            void operator()(LogT::LogMessage const & message) {
                counter++;
            }
        };

        LogT log;
        LogCallback callback1;
        LogCallback callback2;
        auto & cb1_callback = log.add_callback(std::ref(callback1));
        log.log(xl::log::DefaultLevels::Levels::Warn, xl::log::DefaultSubjects::Subjects::Default, "test");
        EXPECT_EQ(callback1.counter, 1);
        auto & cb2_callback = log.add_callback(std::ref(callback2));
        log.log(xl::log::DefaultLevels::Levels::Warn, xl::log::DefaultSubjects::Subjects::Default, "test");
        EXPECT_EQ(callback1.counter, 2);
        EXPECT_EQ(callback2.counter, 1);

        log.remove_callback(cb2_callback);
        log.log(xl::log::DefaultLevels::Levels::Warn, xl::log::DefaultSubjects::Subjects::Default, "test");
        EXPECT_EQ(callback1.counter, 3);
        EXPECT_EQ(callback2.counter, 1);

        log.remove_callback(cb1_callback);
        log.log(xl::log::DefaultLevels::Levels::Warn, xl::log::DefaultSubjects::Subjects::Default, "test");
        EXPECT_EQ(callback1.counter, 3);
        EXPECT_EQ(callback2.counter, 1);



    }
}




class CustomSubjects {

    static inline std::string subject_names[] = {"CustomSubject1", "CustomSubject2", "CustomSubject3"};

public:
    enum class Subjects {CustomSubject1, CustomSubject2, CustomSubject3, LOG_LAST_SUBJECT};

    static std::string const & get_subject_name(Subjects subject) {
        return CustomSubjects::subject_names[static_cast<std::underlying_type_t<Subjects>>(subject)];
    }
};

class CustomLevels {

    inline static std::string level_names[] = {"info", "warn"};

public:
    enum class Levels {Info, Warn, LOG_LAST_LEVEL};

    static std::string const & get_level_name(Levels level) {
        return CustomLevels::level_names[static_cast<std::underlying_type_t<Levels>>(level)];
    }
};


TEST(log, CustomLog) {
    using LogT = xl::Log<CustomLevels, CustomSubjects>;
    int call_count = 0;

    LogT log([&call_count](auto & message){
        call_count++;
        if (message.subject == CustomSubjects::Subjects::CustomSubject1) {
            EXPECT_EQ(message.log.get_subject_name(message.subject), "CustomSubject1");
        } else if (message.subject == CustomSubjects::Subjects::CustomSubject2) {
            EXPECT_EQ(message.log.get_subject_name(message.subject), "CustomSubject2");
        } else {
            EXPECT_EQ(1,2);
        }
    });
    EXPECT_EQ(call_count, 0);
    log.log(CustomLevels::Levels::Warn, CustomSubjects::Subjects::CustomSubject1, "test");
    EXPECT_EQ(call_count, 1);
    log.log(CustomLevels::Levels::Warn, CustomSubjects::Subjects::CustomSubject2, "test");
    EXPECT_EQ(call_count, 2);

    // CustomLevels doesn't have Error, so the error method is removed
//    log.error(CustomSubjects::Subjects::CustomSubject2, "this should fail");

    std::stringstream my_stringstream;
    std::string s = "str";
    const char *c = "str";
    my_stringstream << c << s;
}

TEST(log, OstreamCallbackHelper) {
    using LogT = xl::Log<xl::log::DefaultLevels, xl::log::DefaultSubjects>;
    LogT log;
    std::stringstream output;
    log.add_callback(output, "PREFIX: ");
    std::stringstream output2;
    log.add_callback(output2, "");

    log.info("test");
    EXPECT_EQ(output.str(), "PREFIX: test\n");
    EXPECT_EQ(output2.str(), "test\n");

}


TEST(log, LogStatusFile) {
    using LogT = xl::Log<xl::log::DefaultLevels, xl::log::DefaultSubjects>;
    LogT log;
    int log_count = 0;
    log.add_callback([&log_count](LogT::LogMessage const & message) {
        log_count++;
    });

    auto status_file_filename = "test_log_status_file";
    log.enable_status_file(status_file_filename);
    EXPECT_TRUE(log.log_status_file);
    EXPECT_EQ(log.log_status_file->subject_names.size(), 1);
    EXPECT_EQ(log.log_status_file->level_names.size(), 3);

    LogStatusFile other(status_file_filename);
    EXPECT_EQ(other.subject_names.size(), 1);
    EXPECT_EQ(other.level_names.size(), 3);
    for(auto const & [name, status] : other.level_names) {
        EXPECT_TRUE(status);
    }
    for(auto const & [name, status] : other.subject_names) {
        EXPECT_TRUE(status);
    }

    log.set_level_status((xl::log::DefaultLevels::Levels)0, false);

    other.read();
    EXPECT_EQ(other.subject_names.size(), 1);
    EXPECT_EQ(other.level_names.size(), 3);


    EXPECT_FALSE(other.level_names[0].second);


    other.level_names[0].second = true;
    other.level_names[1].second = false;
    other.write();

    auto & callback = log.add_callback([](LogT::LogMessage const & message) {
        EXPECT_TRUE(false); // this shouldn't be called
    });

    log.warn(LogT::Subjects::Subjects::Default, "This should be filtered");


    // enable it in file, but not time for actual logger to check so it should still be filtered
    other.level_names[1].second = true;
    other.level_names[2].second = false;

    // sleep before write so the timestamp changes
    sleep(1);
    log.warn(LogT::Subjects::Subjects::Default, "This should be filtered because not time to re-check status file");
    other.write();

    log.warn(LogT::Subjects::Subjects::Default, "This should be filtered because not time to re-check status file");
    log.remove_callback(callback);

    // sleep so next log picks up the changes
    sleep(1);

    int before_count = log_count;
    log.warn(LogT::Subjects::Subjects::Default, "This should not be filtered");
    EXPECT_EQ(log_count, before_count+1);


}



