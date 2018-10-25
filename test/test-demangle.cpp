#include <utility>
#include <vector>
#include <string>
#include <memory>


#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "demangle.h"
#include "library_extensions.h"

using namespace std;


TEST(demangle, demangle) {
    EXPECT_EQ(xl::demangle<int>(), "int");
    EXPECT_EQ(xl::demangle<int const>(), "int const");
    EXPECT_EQ(xl::demangle<const int>(), "int const");
}


