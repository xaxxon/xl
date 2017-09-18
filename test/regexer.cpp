
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "regexer.h"

using namespace xl;


TEST(Regexer, EmptyRegex) {
    {
        auto result = regex("", "");
        EXPECT_EQ(result.matches().size(), 1);
        EXPECT_EQ(result.size(), 1);
    }
}

TEST(Regexer, EmptyString) {
    {
        auto result = regex("a", "");
        EXPECT_TRUE(result.matches().empty());
        EXPECT_TRUE(result.empty());
    }
}

TEST(Regexer, InvalidString) {
    {
        EXPECT_THROW(regex("[", ""), std::regex_error);
    }
}


TEST(Regexer, BasicRegex) {
    {
        auto result = regex("a", "a");
        EXPECT_EQ(result.matches().size(), 1);
    }
    {
        auto result = regex("(a)b", "ab");
        EXPECT_EQ(result.matches().size(), 2);

        // second test is a shortcut for the first
        EXPECT_EQ(result.matches()[0], "ab");
        EXPECT_EQ(result[0], "ab");

        // second test is a shortcut for the first
        EXPECT_EQ(result.matches()[1], "a");
        EXPECT_EQ(result[1], "a");

        // alternate syntax
        EXPECT_EQ(regex("(a)b", "ab").matches()[1], "a");
    }
}
