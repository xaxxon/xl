
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