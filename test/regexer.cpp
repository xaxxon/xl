
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
        EXPECT_EQ(result.size(), 1ul);
    }
}

TEST(Regexer, EmptyResult) {
    RegexResultPcre result;
    EXPECT_FALSE(result);
    EXPECT_FALSE(result.has("empty"));
    EXPECT_EQ(result["empty"], "");
}

TEST(Regexer, EmptyString) {
    {
        auto result = regexer("a", ""_re);
        EXPECT_EQ(result.size(), 1ul);
    }
}

TEST(Regexer, InvalidString) {
    {
        EXPECT_THROW(regexer("", "[[[["_re), xl::RegexException);
    }
}


TEST(Regexer, BasicRegex) {
    {
        auto result = regexer("a", "a"_re);
        EXPECT_EQ(result.size(), 1ul);
    }
    {
        auto result = regexer("ab", "(a)b"_re);
        EXPECT_EQ(result.size(), 2ul);

        // second test is a shortcut for the first
//        EXPECT_EQ(result[0], "ab");

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


TEST(Regexer, Next) {
    {
        RegexPcre char_regex("(.)");

        auto result = char_regex.match("ab");
        EXPECT_EQ(result[0], "a");
        EXPECT_EQ(result[1], "a");

        auto next = result.next();
        EXPECT_EQ(next[0], "b");
        EXPECT_EQ(next[1], "b");

        auto third = next.next();
        EXPECT_FALSE(third);
    }
}

TEST(Regexer, LoopedRegex) {
    {
        RegexPcre char_regex("(.)");
        std::vector<std::string> chars;
        for(auto & result : char_regex.match("abcde")) {
            chars.push_back(result[1]);
        }
        auto expected = std::vector<std::string>{"a", "b", "c", "d", "e"};
        EXPECT_EQ(chars, expected);
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

TEST(Regexer, Replace_DoNothing) {
    {
        auto result = RegexPcre("").replace("part1:part2", "");
        EXPECT_EQ(result, "part1:part2");
    }
    {
        // replace all, but match is empty, so it should not go forever
        auto result = RegexPcre("").replace("part1:part2", "", true);
        EXPECT_EQ(result, "part1:part2");
    }
}


TEST(Regexer, Replace) {
    {
        auto result = RegexPcre("(.*):(.*)").replace("part1:part2", "$2:$1");
        EXPECT_EQ(result, "part2:part1");
    }
    {
        auto result = RegexPcre("(.*):(.*)").replace("part1:part2", "");
        EXPECT_EQ(result, "");
    }
    {
        auto result = RegexPcre("^([^:]*):([^:]*)").replace("part1:part2:part3", "");
        EXPECT_EQ(result, ":part3");
    }
    {
        auto result = RegexPcre("^([^:]*):([^:]*)").replace("part1:part2:part3", "$1");
        EXPECT_EQ(result, "part1:part3");
    }
    {
        auto result = RegexPcre(":([^:]*):").replace("part1:part2:part3", ":");
        EXPECT_EQ(result, "part1:part3");
    }
    {
        EXPECT_THROW(RegexPcre("(.*):(.*)").replace("part1:part2", "$3"), RegexException);
    }
    {
        EXPECT_EQ(RegexPcre("(.*):(.*)").replace("part1part2", "$1$2$9"), "part1part2");
    }
    {
        EXPECT_EQ(RegexPcre("[abc]").replace("abcdef", "X", true), "XXXdef");
    }

}


TEST(Regexer, all) {
    {
        auto match_list = RegexStd("(.)").all("abc");
        EXPECT_EQ(match_list.size(), 3ul);
        EXPECT_EQ(match_list[0][1], "a");
        EXPECT_EQ(match_list[1][1], "b");
        EXPECT_EQ(match_list[2][1], "c");
    }
    {
        auto match_list = RegexPcre("(.)").all("abc");
        EXPECT_EQ(match_list.size(), 3ul);
        EXPECT_EQ(match_list[0][1], "a");
        EXPECT_EQ(match_list[1][1], "b");
        EXPECT_EQ(match_list[2][1], "c");
    }
}

#endif

