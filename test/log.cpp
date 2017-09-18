
#include <sstream>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "log.h"



TEST(log, SimpleLog) {
    {
        using LogT = xl::Log<xl::log::DefaultLevels, xl::log::DefaultSubjects>;
        int call_count = 0;
        LogT log([&call_count](auto & logger, auto & message){call_count++;});
        EXPECT_EQ(call_count, 0);
        log.log(xl::log::DefaultLevels::Levels::Warn, xl::log::DefaultSubjects::Subjects::Default, "test");
        EXPECT_EQ(call_count, 1);
        log.log(xl::log::DefaultLevels::Levels::Warn, xl::log::DefaultSubjects::Subjects::Default, "test");
        EXPECT_EQ(call_count, 2);
    }
}
