
#include <sstream>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "log.h"



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
    }
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
        int call_count = 0;
        LogCallback callback1;
        LogCallback callback2;
        log.add_callback(std::ref(callback1));
        log.log(xl::log::DefaultLevels::Levels::Warn, xl::log::DefaultSubjects::Subjects::Default, "test");
        EXPECT_EQ(callback1.counter, 1);
        int call_count2 = 0;
        log.add_callback(std::ref(callback2));
        log.log(xl::log::DefaultLevels::Levels::Warn, xl::log::DefaultSubjects::Subjects::Default, "test");
        EXPECT_EQ(callback1.counter, 2);
        EXPECT_EQ(callback2.counter, 1);

        log.remove_callback(callback2);
        log.log(xl::log::DefaultLevels::Levels::Warn, xl::log::DefaultSubjects::Subjects::Default, "test");
        EXPECT_EQ(callback1.counter, 3);
        EXPECT_EQ(callback2.counter, 1);

        log.remove_callback(callback1);
        log.log(xl::log::DefaultLevels::Levels::Warn, xl::log::DefaultSubjects::Subjects::Default, "test");
        EXPECT_EQ(callback1.counter, 3);
        EXPECT_EQ(callback2.counter, 1);



    }
}




class CustomSubjects {

    static inline std::string subject_names[] = {"CustomSubject1", "CustomSubject2", "CustomSubject3"};

public:
    enum class Subjects {CustomSubject1, CustomSubject2, CustomSubject3};

    static std::string const & get_subject_name(Subjects subject) {
        return CustomSubjects::subject_names[static_cast<std::underlying_type_t<Subjects>>(subject)];
    }
};

class CustomLevels {

    inline static std::string level_names[] = {"info", "warn"};

public:
    enum class Levels {Info, Warn};

    static std::string const & get_subject_name(Levels level) {
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