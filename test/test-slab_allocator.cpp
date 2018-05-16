
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "slab_allocator.h"
#include "fmt/ostream.h"

using namespace xl;

class MyClass {
public:
    static inline size_t constructor_count = 0;
    static inline size_t destructor_count = 0;
    static void reset_counts() {
        constructor_count = 0;
        destructor_count = 0;
    }
    int i;
    char j;
    char * str;
    MyClass() {
        constructor_count++;
    }
    ~MyClass() {
        destructor_count++;
    }
};



TEST(slab_allocator, simple) {

    {
        Allocator<MyClass, 1> allocator;
        EXPECT_EQ(MyClass::constructor_count, 0);
        EXPECT_EQ(MyClass::destructor_count, 0);
        ASSERT_EQ(allocator.count_allocations(), 1);
        auto mem = allocator.allocate().release();
        EXPECT_EQ(MyClass::constructor_count, 1);
        EXPECT_EQ(MyClass::destructor_count, 0);

        allocator.free(mem);
        EXPECT_EQ(MyClass::constructor_count, 1);
        EXPECT_EQ(MyClass::destructor_count, 1);

        auto mem2 = allocator.allocate().release();
        EXPECT_EQ(MyClass::constructor_count, 2);
        EXPECT_EQ(MyClass::destructor_count, 1);

        EXPECT_EQ(mem, mem2);
        ASSERT_EQ(allocator.count_allocations(), 1);
    }
    EXPECT_EQ(MyClass::constructor_count, 2);
    EXPECT_EQ(MyClass::destructor_count, 1); // last allocated object never freed so not destructed
}


TEST(slab_allocator, multiple_allocation_chunks) {
    ASSERT_EQ(sizeof(MyClass), 16); // math further down relies on this

    Allocator<MyClass, 1> allocator;
    ASSERT_EQ(allocator.count_allocations(), 1);


    auto mem = allocator.allocate().release();
    mem = allocator.allocate().release();
    mem = allocator.allocate().release();
    mem = allocator.allocate().release();
    ASSERT_EQ(allocator.count_allocations(), 1);

    auto mem2 = allocator.allocate();
    ASSERT_EQ(allocator.count_allocations(), 2);

    mem2.reset();
}


TEST(slab_allocator, many_allocation_chunks) {
    ASSERT_EQ(sizeof(MyClass), 16); // math further down relies on this

    Allocator<MyClass, 1> allocator;
    std::vector<MyClass *> v;
    
    for(int i = 0; i < 1000000; i++) {
        v.push_back(allocator.allocate().release());
    }
    
    EXPECT_EQ(allocator.count_allocations(), 250000);
    
    EXPECT_EQ(allocator.count_available_returned_positions(), 0);
    
    for(auto allocation : v) {
        allocator.free(allocation);
    }
    EXPECT_EQ(allocator.count_available_returned_positions(), 1000000);
    v.clear();

    // do it again and make sure no additional memory is allocated
    for(int i = 0; i < 1000000; i++) {
        v.push_back(allocator.allocate().release());
    }
    EXPECT_EQ(allocator.count_allocations(), 250000);
    EXPECT_EQ(allocator.count_available_returned_positions(), 0);

    for(auto allocation : v) {
        allocator.free(allocation);
    }
    EXPECT_EQ(allocator.count_available_returned_positions(), 1000000);
}

static_assert(sizeof(decltype(Allocator<MyClass, 1>().allocate())) == 2*sizeof(MyClass*));



