#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "not_null.h"
using namespace xl;



TEST(not_null, simple) {
    int i;
    not_null<int *> nnpi = &i;
}

template<typename T>
struct CopyPointerType {
    static inline int count = 0;
    static void reset_count() {
        count = 0;
    }
    
    CopyPointerType(T t) {
        count++;
        ptr = t;
    }
    
    CopyPointerType(CopyPointerType const &){
        count++;
    }
    
    bool operator==(T) {
        return false;
    }
    
    operator T() {
        return T{};
    }
    
    T   operator->(){
        return &ptr;    
    }
    
    T ptr;
};

TEST(not_null, copy_type) {
    int i;
    
    EXPECT_EQ(CopyPointerType<int*>::count, 0);
    not_null<CopyPointerType<int*>> copy_only = &i;
    EXPECT_EQ(CopyPointerType<int*>::count, 1);
}


