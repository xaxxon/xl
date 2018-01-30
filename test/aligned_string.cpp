
#include "library_extensions.h"
#include "aligned_string/aligned_string.h"
#include "aligned_string/dynamic_buffer.h"
#include "aligned_string/static_buffer.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <new>


using namespace xl;

TEST(AlignedString_Static16, EmptyString) {
    {
        AlignedString<AlignedStringBuffer_Static<16>> string;
        EXPECT_EQ(sizeof(string), 16);
        EXPECT_EQ(string.capacity(), 15);
        EXPECT_EQ(string.length(), 0);
        EXPECT_EQ(string.alignment, 16);
        EXPECT_TRUE(string == "");
        EXPECT_FALSE(string == "aligned string");
        EXPECT_EQ(string.strchr('a'), nullptr);
    }

    {
        AlignedString<AlignedStringBuffer_Static<32>> string;
        EXPECT_EQ(sizeof(string), 32);
        EXPECT_EQ(string.capacity(), 31);
        EXPECT_EQ(string.length(), 0);
        EXPECT_EQ(string.alignment, 16);
        EXPECT_TRUE(string == "");
        EXPECT_FALSE(string == "aligned string");
        EXPECT_EQ(string.strchr('a'), nullptr);

    }


}

TEST(AlignedString_Static16, SimpleString) {
    AlignedString<AlignedStringBuffer_Static<16>> string("aligned string");
    EXPECT_EQ(string.length(), 14);
    EXPECT_TRUE(string == "aligned string");
    EXPECT_FALSE(string == "other string");
    EXPECT_EQ(string.strchr('a'), string.buffer());
    EXPECT_EQ(string.strchr('l'), string.buffer() + 1);
    EXPECT_EQ(string.strchr('r'), string.buffer() + 10);

}


TEST(AlignedString_Static16, OverfullString) {
    EXPECT_THROW(AlignedString<AlignedStringBuffer_Static<16>> string("aligned string too long"), AlignedStringException);
}

TEST(AlignedString_Static16, DoubleAlignmentLengthString) {
    {
        AlignedString<AlignedStringBuffer_Static<32>> string1("0123456789abcdefBBBBBB");
        AlignedString<AlignedStringBuffer_Static<32>> string2("0123456789abcdefCABBBB");
        EXPECT_TRUE(string1 < string2);
        EXPECT_FALSE(string2 < string1);
    }
    {
        AlignedString<AlignedStringBuffer_Static<32>> string1("0123456789abcdefBBBB\0B");
        AlignedString<AlignedStringBuffer_Static<32>> string2("0123456789abcdefBBBB\0C");
        EXPECT_EQ(string1.length(), 20);
        EXPECT_TRUE(string1 == string2);
        EXPECT_FALSE(string1 < string2);
        EXPECT_FALSE(string2 < string1);

    }

}



TEST(AlignedString_Static64, EmptyString) {
    AlignedString<AlignedStringBuffer_Static<64>> string;
    EXPECT_EQ(sizeof(string), 64);
    EXPECT_EQ(string.capacity(), 63);
    EXPECT_EQ(string.length(), 0);
    EXPECT_EQ(string.alignment, 64);
    EXPECT_TRUE(string == "");
    EXPECT_FALSE(string == "aligned string");
    EXPECT_EQ(string.strchr('a'), nullptr);
}

TEST(AlignedString_Static64, SimpleString) {
    AlignedString<AlignedStringBuffer_Static<64>> string("aligned string");
    EXPECT_EQ(string.length(), 14);
    EXPECT_TRUE(string == "aligned string");
    EXPECT_FALSE(string == "other string");
    EXPECT_EQ(string.strchr('a'), string.buffer());
    EXPECT_EQ(string.strchr('l'), string.buffer() + 1);
    EXPECT_EQ(string.strchr('r'), string.buffer() + 10);

}


TEST(AlignedString_Static64, OverfullString) {

    // 63-character string (fits)
    AlignedString<AlignedStringBuffer_Static<64>> string1(
                     "01234567890123456789012345678901234567890123456789012345678901-");
    EXPECT_EQ(string1.strchr('-'), &string1.buffer()[62]);
    EXPECT_EQ(strlen(string1.buffer()), 63);

    // 64-character string (too big)
    EXPECT_THROW(AlignedString<AlignedStringBuffer_Static<64>> string2(
                     "012345678901234567890123456789012345678901234567890123456789---+"), AlignedStringException);

}


TEST(AlignedString_Dynamic16, EmptyString) {
    {
        AlignedString<AlignedStringBuffer_Dynamic<16>> string;
        EXPECT_EQ(string.capacity(), 0);
        EXPECT_EQ(string.length(), 0);
        EXPECT_EQ(strlen(string.buffer()), 0);
        EXPECT_EQ(string.alignment, 16);
        EXPECT_TRUE(string == "");
        EXPECT_FALSE(string == "aligned string");
    }
}

TEST(AlignedString_Dynamic16, SimpleString) {
    AlignedString<AlignedStringBuffer_Dynamic<16>> string("aligned string");
    EXPECT_EQ(string.length(), 14);
    EXPECT_EQ(strlen(string.buffer()), 14);
    EXPECT_TRUE(string == "aligned string");
    EXPECT_FALSE(string == "other string");
}


TEST(AlignedString_Dynamic16, DoubleAlignmentLengthString) {
    {
        AlignedString<AlignedStringBuffer_Dynamic<16>> string1("0123456789abcdefBBBBBC");
        AlignedString<AlignedStringBuffer_Dynamic<16>> string2("0123456789abcdefCABBBB");
        EXPECT_FALSE(string1 == string2);
        EXPECT_TRUE(string1 < string2);
        EXPECT_FALSE(string2 < string1);
    }
    {
        AlignedString<AlignedStringBuffer_Dynamic<16>> string1("0123456789abcdefBBBB\0B");
        AlignedString<AlignedStringBuffer_Dynamic<16>> string2("0123456789abcdefBBBB\0C");
        EXPECT_EQ(string1.length(), 20);
        EXPECT_TRUE(string1 == string2);
        EXPECT_FALSE(string1 < string2);
        EXPECT_FALSE(string2 < string1);

        // make sure it finds it on the second group of 16, not the first
        EXPECT_EQ(string1.strchr('B'), string1.buffer() + 16);
    }


}



TEST(AlignedString_Dynamic64, EmptyString) {
    AlignedString<AlignedStringBuffer_Dynamic<64>> string;
    EXPECT_EQ(string.capacity(), 0);
    EXPECT_EQ(string.length(), 0);
    EXPECT_EQ(strlen(string.buffer()), 0);
    EXPECT_EQ(string.alignment, 64);
    EXPECT_TRUE(string == "");
    EXPECT_FALSE(string == "aligned string");


}

TEST(AlignedString_Dynamic64, SimpleString) {
    AlignedString<AlignedStringBuffer_Dynamic<64>> string("aligned string");
    EXPECT_EQ(string.length(), 14);
    EXPECT_EQ(strlen(string.buffer()), 14);

    EXPECT_TRUE(string == "aligned string");
    EXPECT_FALSE(string == "other string");
}


TEST(AlignedString_Dynamic64, BlockOverflow) {

    {
        AlignedString<AlignedStringBuffer_Dynamic<64>> string1(
            "012345678901234567890123456789012345678901234567890123456789012");
        EXPECT_EQ(string1.capacity(), 63);
        EXPECT_EQ(strlen(string1.buffer()), 63);


        EXPECT_EQ(string1, "012345678901234567890123456789012345678901234567890123456789012");

        //                     * <== difference
        EXPECT_TRUE(string1 <"022345678901234567890123456789012345678901234567890123456789012");
        EXPECT_FALSE(string1 == "022345678901234567890123456789012345678901234567890123456789012");

        //                                                                    ===> *    * <== differences
        EXPECT_TRUE(string1 < "012345678901234567890123456789012345678901234567890133456689012");
        EXPECT_FALSE(string1 == "012345678901234567890123456789012345678901234567890133456689012");
    }

    {
        AlignedString<AlignedStringBuffer_Dynamic<64>> string1(
            "0123456789012345678901234567890123456789012345678901234567890123");
        EXPECT_EQ(string1.capacity(), 127);
        EXPECT_FALSE(string1 < "0123456789012345678901234567890123456789012345678901234567890123");
        EXPECT_TRUE(string1 == "0123456789012345678901234567890123456789012345678901234567890123");
    }


    // find char in second 16-byte chunk in first 64-byte block
    {
        auto long_std_string = std::string(17, 'a');
        long_std_string += 'b';

        AlignedString<AlignedStringBuffer_Dynamic<64>> s(long_std_string);
        EXPECT_EQ(s.strchr('b'), s.buffer() + 17);
        EXPECT_EQ(s.strchr<false>('b'), s.buffer() + 17);


    }

    // find char in subsequent 64-byte block
    {
        auto long_std_string = std::string(65, 'a');
        long_std_string += 'b';

        AlignedString<AlignedStringBuffer_Dynamic<64>> s(long_std_string);
        EXPECT_EQ(s.strchr('b'), s.buffer() + 65);
    }

}

