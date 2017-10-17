
#include <vector>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "library_extensions.h"

using namespace xl;
using namespace std;


TEST(LibraryExtensions, erase_if) {
    vector<int> vi{1,2,3,4,5};
    erase_if(vi, [](int i){return i % 2;});
    EXPECT_EQ(vi.size(), 2);

    erase_if(vector<int>{1,2,3,4,5}, [](int i){return i % 2;});
    EXPECT_EQ(vi.size(), 2);
}

TEST(LibraryExtensions, copy_if) {
    {
        vector<int> vi{1, 2, 3, 4, 5};
        auto result = xl::copy_if(vi, [](int i) { return i % 2; });
        EXPECT_EQ(result.size(), 3);
    }
    {
        vector<int> vi{1, 2, 3, 4, 5};
        auto callback = [](int i) { return i % 2 == 0; };
        auto result = xl::copy_if(vi, callback);
        EXPECT_EQ(result.size(), 2);
    }
}