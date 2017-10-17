
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <fmt/ostream.h>

#include "magic_ptr.h"

using namespace xl;

TEST(magic_ptr, test_empty) {
    {
        magic_ptr<int> mp;
        EXPECT_FALSE(mp);
        EXPECT_TRUE(mp.empty());
    }
}
TEST(magic_ptr, test_not_empty) {

    {
        magic_ptr<int> mpi(5);
        EXPECT_TRUE(mpi);
        EXPECT_FALSE(mpi.empty());
    }
}

TEST(magic_ptr, lvalue) {
    {
        int i;
        magic_ptr mpi(i);
        EXPECT_EQ(i, *mpi);
    }
}

TEST(magic_ptr, rvalue) {
    {
        magic_ptr mpi(5);
        EXPECT_EQ(*mpi, 5);
    }
}

TEST(magic_ptr, unique_ptr) {
    {
        auto upi = std::make_unique<int>(4);
        magic_ptr mpi(std::move(upi));
        EXPECT_EQ(*mpi, 4);
    }
}

TEST(magic_ptr, manual_pointers) {

}