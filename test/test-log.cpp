
#include <sstream>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "log.h"

using namespace xl;
using namespace xl::templates;
using namespace xl::log;


TEST(log, SimpleLog) {
    {
        using LogT = xl::log::Log<xl::log::DefaultLevels, xl::log::DefaultSubjects>;
        int call_count = 0;
        LogT log([&call_count](auto & message){call_count++;});
        EXPECT_EQ(call_count, 0);
        log.log(xl::log::DefaultLevels::Levels::Warn, LogT::Subjects::Default, "test");
        EXPECT_EQ(call_count, 1);
        log.log(xl::log::DefaultLevels::Levels::Warn, xl::log::DefaultSubjects::Subjects::Default, "test");
        EXPECT_EQ(call_count, 2);
        log.info(xl::log::DefaultSubjects::Subjects::Default, "Info");
        log.warn(xl::log::DefaultSubjects::Subjects::Default, "Warning");
        log.error(xl::log::DefaultSubjects::Subjects::Default, "Error");
        EXPECT_EQ(call_count, 5);
        log.set_status(xl::log::DefaultLevels::Levels::Warn, false);
        log.warn(xl::log::DefaultSubjects::Subjects::Default, "Warning");
        EXPECT_EQ(call_count, 5);
        log.info(xl::log::DefaultSubjects::Subjects::Default, "Info");
        log.error(xl::log::DefaultSubjects::Subjects::Default, "Error");
        EXPECT_EQ(call_count, 7);

        EXPECT_EQ(log.get_name(xl::log::DefaultSubjects::Subjects::Default), "default");
        EXPECT_EQ(log.get_name(xl::log::DefaultLevels::Levels::Warn), "warn");
    }
}




TEST(log, LogCallbackGuard) {

    int global_log_count = 0;
    using LogT = xl::log::Log<xl::log::DefaultLevels, xl::log::DefaultSubjects>;
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
        ::xl::log::LogCallbackGuard<LogCallback, LogT> g(log, global_log_count);
        log.log(xl::log::DefaultLevels::Levels::Warn, xl::log::DefaultSubjects::Subjects::Default, "test");
        EXPECT_EQ(global_log_count, 1);
        {
            ::xl::log::LogCallbackGuard<LogCallback, LogT> g2(log, global_log_count);
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

    using LogT = xl::log::Log<xl::log::DefaultLevels, xl::log::DefaultSubjects>;
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
        ::xl::log::LogCallbackGuard g(log, log_callback);
        log.log(xl::log::DefaultLevels::Levels::Warn, xl::log::DefaultSubjects::Subjects::Default, "test");
        EXPECT_EQ(log_callback.counter, 1);
        {
            ::xl::log::LogCallbackGuard g(log, log_callback);
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
        using LogT = xl::log::Log<xl::log::DefaultLevels, xl::log::DefaultSubjects>;
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




struct CustomSubjects {

    static inline std::string subject_names[] = {"CustomSubject1", "CustomSubject2", "CustomSubject3"};
    enum class Subjects {CustomSubject1 = 0, CustomSubject2, CustomSubject3, LOG_LAST_SUBJECT};
};



struct CustomLevels {

    inline static std::string level_names[] = {"info", "warn"};
    enum class Levels {Info, Warn, LOG_LAST_LEVEL};
};



TEST(log, CustomLog) {
    using LogT = xl::log::Log<CustomLevels, CustomSubjects>;
    int call_count = 0;

    LogT log([&call_count](auto & message){
        call_count++;
        if (message.subject == CustomSubjects::Subjects::CustomSubject1) {
            EXPECT_EQ(LogT::get_name(message.subject), "CustomSubject1");
        } else if (message.subject == CustomSubjects::Subjects::CustomSubject2) {
            EXPECT_EQ(LogT::get_name(message.subject), "CustomSubject2");
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


TEST(log, SubjectAndLevelIteration) {
    using LogT = xl::log::Log<CustomLevels, CustomSubjects>;
    int call_count = 0;
    LogT log;

    for(auto i : log.subjects()) {
        EXPECT_TRUE(log.get_status(i));
    }
    for(auto i : log.levels()) {
        EXPECT_TRUE(log.get_status(i));
    }

    log.set_status(CustomLevels::Levels::Info, false);
    log.set_status(CustomLevels::Levels::Warn, false);

    for(auto i : log.levels()) {
        EXPECT_FALSE(log.get_status(i));
    }

    log.set_status(CustomSubjects::Subjects::CustomSubject1, false);
    log.set_status(CustomSubjects::Subjects::CustomSubject2, false);
    log.set_status(CustomSubjects::Subjects::CustomSubject3, false);


    for(auto i : log.subjects()) {
        EXPECT_FALSE(log.get_status(i));
    }
}


TEST(log, SubjectStatusSaveAndRestore) {
    using LogT = xl::log::Log<CustomLevels, CustomSubjects>;
    LogT log;

    log.set_status(CustomLevels::Levels::Warn, false);
    log.set_status(CustomSubjects::Subjects::CustomSubject2, false);

    EXPECT_TRUE(log.get_status(CustomSubjects::Subjects::CustomSubject1));
    EXPECT_FALSE(log.get_status(CustomSubjects::Subjects::CustomSubject2));

    auto subject_backup = log.get_statuses();

    log.set_all_subjects(true);
    for(auto i : log.subjects()) {
        EXPECT_TRUE(log.get_status(i));
    }

    log.set_statuses(std::move(subject_backup));
    EXPECT_FALSE(log.get_status(CustomSubjects::Subjects::CustomSubject2));
}


TEST(log, OstreamCallbackHelper) {
    using LogT = xl::log::Log<xl::log::DefaultLevels, xl::log::DefaultSubjects>;
    LogT log;
    std::stringstream output;
    log.add_callback(output, "PREFIX: ");
    std::stringstream output2;
    log.add_callback(output2, "");

    log.info("test");
    EXPECT_TRUE(Regex("\\[[^]]+\\] PREFIX: test\n").match(output.str()));
    EXPECT_TRUE(Regex("\\[[^]]+\\] test\n").match(output2.str()));
}

//int log_count = 0;
//log.add_callback([&log_count](LogT::LogMessage const & message) {
//log_count++;
//});

TEST(log, LogStatusFileFromLog) {
    using LogT = xl::log::Log<xl::log::DefaultLevels, xl::log::DefaultSubjects>;
    LogT log;

    auto status_file_filename = "test_log_status_file";
    log.enable_status_file(status_file_filename, true); // force status reset
    EXPECT_TRUE(log.log_status_file);
    EXPECT_EQ(std::get<LogStatusFile::Statuses>(log.log_status_file->subjects).size(), 1);
    EXPECT_EQ(std::get<LogStatusFile::Statuses>(log.log_status_file->levels).size(), 3);

    log.set_status(xl::log::DefaultLevels::Levels::Warn, false);

    // create a second log status file sharing the same status file
    ::xl::log::LogStatusFile other(status_file_filename);

    // make sure it has the same number of subjects and levels
    EXPECT_EQ(std::get<LogStatusFile::Statuses>(log.log_status_file->subjects).size(), 1);

    EXPECT_EQ(std::get<LogStatusFile::Statuses>(log.log_status_file->levels).size(), 3);

    // make sure the level and subject statuses are as expected (all true)
    for (auto const &[name, status] : std::get<LogStatusFile::Statuses>(log.log_status_file->levels)) {
        if (name == "warn") {
            EXPECT_FALSE(status);
        } else {
            EXPECT_TRUE(status);
        }
    }

    for (auto const &[name, status] : std::get<LogStatusFile::Statuses>(log.log_status_file->subjects)) {
        EXPECT_TRUE(status);
    }
}


TEST(log, LogStatusFileFromFileAllStatusesTrueByDefault) {
    using TestLogT = xl::log::Log<xl::log::DefaultLevels, xl::log::DefaultSubjects>;
    TestLogT log;

    auto status_file_filename = "LogStatusFileFromFileAllStatusesTrueByDefault";
    ::xl::log::LogStatusFile other(status_file_filename, true);

    log.enable_status_file(status_file_filename, false); // do not force status reset - load what was just written

    // there shouldn't be any levels/subjects listed, just the boolean version of the variant
    EXPECT_TRUE(std::get<bool>(log.log_status_file->subjects));
    EXPECT_TRUE(std::get<bool>(log.log_status_file->levels));

    for (auto level : TestLogT::levels()) {
        EXPECT_TRUE(log.get_status(level));
    }
    for (auto subject : TestLogT::subjects()) {
        EXPECT_TRUE(log.get_status(subject));
    }
}


TEST(log, LogStatusFileFromFileAllStatusesSetToFalse) {
    using TestLogT = xl::log::Log<xl::log::DefaultLevels, xl::log::DefaultSubjects>;
    TestLogT log;

    auto status_file_filename = "LogStatusFileFromFileAllStatusesSetToFalse";
    ::xl::log::LogStatusFile other(status_file_filename, true);
    other.subjects = false;
    other.levels = false;
    other.write();

    log.enable_status_file(status_file_filename, false); // do not force status reset - load what was just written

    // there shouldn't be any levels/subjects listed, just the boolean version of the variant
    EXPECT_FALSE(std::get<bool>(log.log_status_file->subjects));
    EXPECT_FALSE(std::get<bool>(log.log_status_file->levels));

    for (auto level : TestLogT::levels()) {
        EXPECT_FALSE(log.get_status(level));
    }
    for (auto subject : TestLogT::subjects()) {
        EXPECT_FALSE(log.get_status(subject));
    }
}





TEST(log, templates) {
    using LogT = xl::log::Log<xl::log::DefaultLevels, xl::log::DefaultSubjects>;
    LogT log;
    int log_count = 0;
    std::string last_message;
    log.add_callback([&](LogT::LogMessage const & message) {
        log_count++;
        last_message = message.string;
    });

    log.info(Template("{{cheap}}").fill(make_provider(
        std::pair("cheap", "CHEAP"),
        std::pair("expensive", []{EXPECT_TRUE(false); return "";})
    )));
    EXPECT_EQ(log_count, 1);
    EXPECT_EQ(last_message, "CHEAP");

    log.set_status(LogT::Levels::Info, false);

    log.info(LogT::Subjects::Default, Template("{{cheap}} {{expensive}}"), make_provider(
        std::pair("cheap", "CHEAP"),
        std::pair("expensive", []{EXPECT_TRUE(false); return "";})
    ));

    // shouldn't have changed
    EXPECT_EQ(log_count, 1);
}



TEST(log, copy_logger) {
    using LogT = xl::log::Log<xl::log::DefaultLevels, xl::log::DefaultSubjects>;
    LogT log;
    auto logger = log.to(LogT::Levels::Warn, LogT::Subjects::Default);
    int log_count = 0;
    log.add_callback([&](LogT::LogMessage const & message) {
        log_count++;
        EXPECT_EQ(message.level, LogT::Levels::Warn);
        EXPECT_EQ(message.subject, LogT::Subjects::Default);
    });

    logger("Message 1");
    logger("Message 2");

    EXPECT_EQ(log_count, 2);

}


TEST(log, xl_string_views) {
    using LogT = xl::log::Log<xl::log::DefaultLevels, xl::log::DefaultSubjects>;
    LogT log;
    auto logger = log.to(LogT::Levels::Warn, LogT::Subjects::Default);

    logger("{}", xl::string_view("test xl stringview"));
}
