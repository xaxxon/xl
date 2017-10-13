#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "zstring_view.h"

using namespace xl;
using namespace std;


TEST(ZStringView, ConstexprZStringView) {
    {
        constexpr zstring_view zstring_view("asdf");
        constexpr auto length = zstring_view.length();
        EXPECT_EQ(zstring_view.length(), 4);
        EXPECT_EQ(zstring_view.c_str()[4], '\0');
    }
    {
        constexpr zstring_view zstring_view("asdf", 4);
        constexpr auto length = zstring_view.length();
        EXPECT_EQ(zstring_view.length(), 4);
        EXPECT_EQ(zstring_view.c_str()[4], '\0');
    }
}

TEST(ZStringView, ConstructorTests) {
    {
        std::string s("stuff");
        zstring_view zs(s);
    }
    {
        // explicitly disallowed because it doesn't make sense to take an rvalue
//        zstring_view zs(std::string{});
    }
    {
        // not allowed because string_view doesn't guarantee NUL termination
//        zstring_view zs(std::string_view{});
    }
    {
        zstring_view zs("asdf");
    }

}
