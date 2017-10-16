
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
        EXPECT_EQ(result.size(), 1);
    }
}

TEST(Regexer, EmptyString) {
    {

        auto result = regexer("a", ""_re);
        EXPECT_EQ(result.size(), 1);
    }
}

TEST(Regexer, InvalidString) {
    {
        EXPECT_THROW(regexer("", "["_re), xl::RegexException);
    }
}


TEST(Regexer, BasicRegex) {
    {
        auto result = regexer("a", "a"_re);
        EXPECT_EQ(result.size(), 1);
    }
    {
        auto result = regexer("ab", "(a)b"_re);
        EXPECT_EQ(result.size(), 2);

        // second test is a shortcut for the first
        EXPECT_EQ(result[0], "ab");

        // second test is a shortcut for the first
        EXPECT_EQ(result[1], "a");

        // alternate syntax
        EXPECT_EQ(regexer("ab", "(a)b"_re)[1], "a");
    }
}

TEST(Regexer, NoMatch) {
    {
        EXPECT_FALSE(regexer("b", "a"_re));
        EXPECT_TRUE(regexer("b", "b"_re));
    }
}

#if defined XL_USE_PCRE

TEST(Regexer, RecursivePattern) {
    {
        Regex r("\\w{3}\\d{3}(?R)?");
        EXPECT_TRUE(r.match("abc123abc13abc123"));
    }

    {
        // match arbitrarily deep nested parenthesis
        Regex r("^(\\(((?>[^()]+)|(?1))*\\))$");
        EXPECT_TRUE(r.match("(Test((test)te(s)t))"));
        EXPECT_FALSE(r.match("Test((test)te(s)t)"));
        EXPECT_FALSE(r.match("(Test((test)te(st))"));
    }
}



#endif

