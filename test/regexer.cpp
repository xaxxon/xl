
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "regex/regexer.h"

using namespace xl;
#include "library_extensions.h"

TEST(RegexPcre, SimpleMatch) {
    xl::Regex regex("(a)(b)");
    regex.match("ab");
}


TEST(Regexer, EmptyRegex) {
    {
        auto result = regexer("", ""_re);
        EXPECT_EQ(result.matches().size(), 1);
        EXPECT_EQ(result.size(), 1);
    }
}

TEST(Regexer, EmptyString) {
    {

        auto result = regexer("a", ""_re);
        EXPECT_TRUE(result.matches().empty());
        EXPECT_TRUE(result.empty());
    }
}

TEST(Regexer, InvalidString) {
    {
        EXPECT_THROW(regexer("", "["_re), std::regex_error);
    }
}


TEST(Regexer, BasicRegex) {
    {
        auto result = regexer("a", "a"_re);
        EXPECT_EQ(result.matches().size(), 1);
    }
    {
        auto result = regexer("ab", "(a)b"_re);
        EXPECT_EQ(result.matches().size(), 2);

        // second test is a shortcut for the first
        EXPECT_EQ(result.matches()[0], "ab");
        EXPECT_EQ(result[0], "ab");

        // second test is a shortcut for the first
        EXPECT_EQ(result.matches()[1], "a");
        EXPECT_EQ(result[1], "a");

        // alternate syntax
        EXPECT_EQ(regexer("ab", "(a)b"_re).matches()[1], "a");
    }
}

TEST(Regexer, NoMatch) {
    {
        EXPECT_FALSE(regexer("b", "a"_re));
        EXPECT_TRUE(regexer("b", "b"_re));
    }
}