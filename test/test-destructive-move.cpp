#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "destructive_move.h"

#ifdef XL_USE_LIB_FMT
#include "fmt/ostream.h"
#endif

using namespace xl;



struct SimpleClass {
    std::unique_ptr<int> upi = std::make_unique<int>(5);

    SimpleClass() = default;
    SimpleClass(SimpleClass&&) = default;
    SimpleClass(SimpleClass const &) = delete;
};

static_assert(std::is_same_v<decltype(std::move(std::declval<SimpleClass>())), SimpleClass&&>);

TEST(destructive_move, simple) {
    destructive_move<SimpleClass> dmsc;
    SimpleClass sc2(std::move(dmsc));
    SimpleClass sc3(std::move(dmsc));
    EXPECT_TRUE(dmsc.destroyed_);
}