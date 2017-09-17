

#pragma once

#include <type_traits>
#include <memory>
#include <string>
#include <string_view>
#include <new>
#include <cstdint>
#include <assert.h>
#include <emmintrin.h>
#include <immintrin.h>
#include <iostream>
#include <exception>

#include <stdio.h>
#include <stdlib.h>
#include <fmt/ostream.h>
#include <exception>

namespace xl {

class AlignedStringException : public std::exception {
    std::string reason;

public:
    AlignedStringException(std::string const & reason) : reason(reason) {}
    char const * what() const noexcept {return reason.c_str();}
};

/**
 * High performance string class where the string data is aligned on 16 or 64-byte boundaries
 * @tparam alignment
 */
template<class AlignedStringBuffer_t>
class AlignedString : public AlignedStringBuffer_t {

private:



public:

    /**
     * Creates an empty string
     */
    AlignedString() {};


    template<class... Stringishes>
    AlignedString(Stringishes && ... stringishes) {
        auto total_size = (stringishes.length() + ...);
        auto allocation_size = total_size + 1;

        this->allocate(allocation_size);
        (*this += ... += stringishes);
    }

    /**
     * Creates an aligned string from a string view
     * @param string_view source string view
     */
    AlignedString(std::string_view const
                  & string_view) {
        this->concat(string_view.data(), string_view.length());
//        memcpy((void *)this->_buffer, string_view.data(), string_view.length());
    }

    /**
     * Creates an AlignedString from a nul-terminated c-style string
     * @param source
     */
    AlignedString(char const * source) :
        AlignedString(std::string_view(source, strlen(source))) {}

    AlignedString(char * source) :
        AlignedString((char const *) source) {}


    AlignedString(AlignedString
                  && other) = default;


    /**
     * Copy constructor makes a new copy of the string
     * @param other source AlignedString
     */
    AlignedString(AlignedString const
                  & other) :
        AlignedString(std::string_view{other.c_str(), other.length()}) {}

    AlignedString & operator=(AlignedString && other) = default;

    AlignedString & operator=(AlignedString const
                              & other) = default;


    /**
     * Returns a c-style nul-terminated string O(1)
     * @return
     */
    char const * c_str() const {
        // a moved-away-from string should still be valid, but since it will almost never
        //   be used again, re-allocate lazily
        if (!this->buffer()) {
            this->allocate(1);
        }
        return this->buffer();
    }


    /**
     * Returns a reference to the character at the index, no bounds checking
     * @param index position of character to return reference to
     * @return writeable reference to character at position
     */
    char & operator[](size_t index) {
        return this->buffer()[index];
    }


    /**
     * Returns std::string_view corresponding to the string in this object (O(1))
     * @return std::string_view into this string
     */
    operator std::string_view() const {
        return std::string_view{(char *) this->buffer(), this->length()};
    }


    template<auto alignment = AlignedStringBuffer_t::alignment, std::enable_if_t<alignment == 16, int> = 0>
    bool operator==(AlignedString<AlignedStringBuffer_t> const & other) const {
        if (this->length() != other.length()) {
            return false;
        }

        size_t offset = 0;
        while(offset < this->length()) {

            auto str1 = _mm_load_si128((__m128i *) (this->buffer() + offset));
            auto str2 = _mm_load_si128((__m128i *) (other.buffer() + offset));

            auto result = _mm_cmpeq_epi8(str1, str2);
            auto all_ones = _mm_test_all_ones(result);
            if (!all_ones) {
//            std::cerr << fmt::format("NOT EQUAL") << std::endl;
                return false;
            }
            offset += 16;
        }
        return true;
    }



    template<bool expect_match_v = true, auto alignment = AlignedStringBuffer_t::alignment, std::enable_if_t<alignment == 16, int> = 0>
    char const * strchr(char c) const {

        auto needles = _mm_set_epi8(c, c, c, c, c, c, c, c, c, c, c, c, c, c, c, c);

//        std::cerr << "strchr on string: " << this->buffer() << " and length: " << (int)this->length() << " looking for " << c <<  std::endl;
        size_t offset = 0;
        auto buffer = this->buffer();
        while (offset < this->length()) {
            auto data = _mm_load_si128((__m128i *) (buffer + offset));

            auto results = _mm_cmpeq_epi8(needles, data);

            auto mask_results = _mm_movemask_epi8(results);

            if (mask_results) {

                auto position = __builtin_ctz(mask_results);

                if (position < 16 && offset + position < this->length()) {
//                    std::cerr << "Position is: " << position << std::endl;
//                    std::cerr << "found match at " << offset + position << " for " << c << std::endl;
                    return buffer + offset + position;
                }
            }
            offset += 16;
        }
        return nullptr;
    };


    template<bool expect_match_v = true, auto alignment = AlignedStringBuffer_t::alignment, std::enable_if_t<alignment == 64, int> = 0>
    char const * strchr(char c) const {
        if constexpr(expect_match_v) {
            return this->strchr<true, 16>(c);
        }

        auto needles = _mm_set_epi8(c, c, c, c, c, c, c, c, c, c, c, c, c, c, c, c);

        int offset = 0;
        auto buffer = this->buffer();

        while (offset < this->length()) {

            __m128i data[4];

            data[0] = _mm_load_si128((__m128i *) (buffer + offset));
            data[1] = _mm_load_si128((__m128i *) (buffer + 16 + offset));
            data[2] = _mm_load_si128((__m128i *) (buffer + 32 + offset));
            data[3] = _mm_load_si128((__m128i *) (buffer + 48 + offset));

            __m128i results[4];
            results[0] = _mm_cmpeq_epi8(needles, data[0]);
            results[1] = _mm_cmpeq_epi8(needles, data[1]);
            results[2] = _mm_cmpeq_epi8(needles, data[2]);
            results[3] = _mm_cmpeq_epi8(needles, data[3]);

            auto combined = _mm_or_si128(_mm_or_si128(results[0], results[1]),
                                         _mm_or_si128(results[2], results[3]));

            // if no match was found anywhere
            if (_mm_test_all_zeros(combined, combined)) {
                return nullptr;
            }

            for (int i = 0; i < 4; i++) {
                if (!_mm_test_all_zeros(results[i], results[i])) {
                    for (int j = 0; j < 16; j++) {
                        if (offset + j >= this->length()) {
                            return nullptr;
                        }
                        if (buffer[offset + j] == c) {
                            return &buffer[offset + j];
                        }
                    }
                    assert(false); // found difference in chunk, but not the specific byte
                }

                offset += 16;
            }
        }
        return nullptr;
    }



    AlignedString<AlignedStringBuffer_t> operator+(AlignedString<AlignedStringBuffer_t> const & other) const {
        return AlignedString<AlignedStringBuffer_t>(*this, other);
    }

    AlignedString<AlignedStringBuffer_t> & operator+=(std::string_view const & source) {
//    memcpy(this->buffer() + this->length(), source.data(), source.size());
//    this->length() += source.size();
//    return *this;
        this->concat(source.data(), source.size());
        return *this;
    }


    template<auto alignment = AlignedStringBuffer_t::alignment, std::enable_if_t<alignment == 64, int> = 0>
    bool operator==(AlignedString<AlignedStringBuffer_t> const & other) const {


        if (this->length() != other.length()) {
            return false;
        }

        size_t offset = 0;
        while(offset < this->length()) {

            auto str1_0 = _mm_load_si128((__m128i *) (this->buffer() + offset));
            auto str1_1 = _mm_load_si128((__m128i *) (this->buffer() + 16 + offset));
            auto str1_2 = _mm_load_si128((__m128i *) (this->buffer() + 32 + offset));
            auto str1_3 = _mm_load_si128((__m128i *) (this->buffer() + 48 + offset));

            auto str2_0 = _mm_load_si128((__m128i *) (other.buffer() + offset));
            auto str2_1 = _mm_load_si128((__m128i *) (other.buffer() + 16 + offset));
            auto str2_2 = _mm_load_si128((__m128i *) (other.buffer() + 32 + offset));
            auto str2_3 = _mm_load_si128((__m128i *) (other.buffer() + 48 + offset));

            auto result0 = _mm_cmpeq_epi8(str1_0, str2_0);
            auto result1 = _mm_cmpeq_epi8(str1_1, str2_1);
            auto result2 = _mm_cmpeq_epi8(str1_2, str2_2);
            auto result3 = _mm_cmpeq_epi8(str1_3, str2_3);

            // it's ok to include length in here since they must have the same length to be equal
            auto mask = _mm_movemask_epi8(_mm_and_si128(_mm_and_si128(result0, result1), _mm_and_si128(result2, result3)));

            if (mask != 0xffff) {
                return false;
            }
            offset += 64;
        }
        return true;
    }



    template<auto alignment = AlignedStringBuffer_t::alignment, std::enable_if_t<alignment == 16, int> = 0>
    bool operator<(AlignedString<AlignedStringBuffer_t> const & other) const {

//    std::cerr << fmt::format("in alignedstring<16>::operator< for {} < {}", *this, other) << std::endl;
        // if no differences are found, the shorter string is less than
        bool return_result = this->length() < other.length();
        size_t offset = 0;
        while(offset < this->length()) {

            auto str1 = _mm_load_si128((__m128i *) (this->buffer() + offset));
            auto str2 = _mm_load_si128((__m128i *) (other.buffer() + offset));

            auto result = _mm_cmpeq_epi8(str1, str2);
            uint16_t mask = _mm_movemask_epi8(result);
//        std::cerr << fmt::format("mask: {}", mask) << std::endl;
            if (mask == 0xffff) {
                offset += 16;
//            std::cerr << fmt::format("no difference found in bytes {} following {} - mask result: {}", alignment, offset, mask) << std::endl;
                continue;
            }
//        std::cerr << fmt::format("Difference WAS found in bytes {} following {} - mask result: {}", alignment, offset, mask) << std::endl;

            mask = ~mask;

            // If they were the same, we wouldn't be here, so find where difference is
            auto position_of_first_set_bit = __builtin_ctz(mask);

            // if the difference is past the end of `this` but still in other, then this is shorter and therefor
            //   less than
            if (offset >= this->length() && offset < other.length()) {
//            std::cerr << fmt::format("returning true 1 - off the end of LHS but in RHS (same until end but LHS shorter") << std::endl;
                return_result = true;
                break;
            }
                // else if offset is in `this` but past the end of other, other is shorter and therefor less than
            else if (offset < this->length() && offset > other.length()) {
//            std::cerr << fmt::format("returning false 1 - off the end of RHS but in LHS (same until end but RHS shorter)") << std::endl;
                return_result = false;
                break;
            }

//        std::cerr << fmt::format("difference at byte {} = {}", position_of_first_set_bit, mask) << std::endl;

            char c1 = this->buffer()[offset + position_of_first_set_bit];
            char c2 = other.buffer()[offset + position_of_first_set_bit];
            if (c1 < c2) {
//            std::cerr << fmt::format("returning true 2 - {} < {}", a, b) << std::endl;
                return_result = true;
                break;
            } else if (c1 > c2) {
//            std::cerr << fmt::format("returning false 2 - {} < {}", a, b) << std::endl;
                return_result = false;
                break;
            }

        }

//    std::cerr << fmt::format("aligned string<16>::operator< returning {} for {} < {}",
//                             return_result, *this, other) << std::endl;

//    std::cerr << fmt::format("actually returning: {}", return_result) << std::endl;
        return return_result;
    }



    template<size_t alignment = AlignedStringBuffer_t::alignment, std::enable_if_t<alignment == 64, int> = 0>
    bool operator<(AlignedString<AlignedStringBuffer_t> const & other) const {

        size_t offset = 0;
//        std::cerr << "comparing: " << this->buffer() << std::endl;
//        std::cerr << "and:       " << other.buffer() << std::endl;
//        std::cerr << "Length: " << this->length() << " : " << other.length() << std::endl;

        auto buffer1 = this->buffer();
        auto buffer2 = other.buffer();

        auto min_length = std::min(this->length(), other.length());

        while(offset < this->length()) {

            __m128i str1[4];
            str1[0] = _mm_load_si128((__m128i *) (this->buffer() + offset));
            str1[1] = _mm_load_si128((__m128i *) (this->buffer() + 16 + offset));
            str1[2] = _mm_load_si128((__m128i *) (this->buffer() + 32 + offset));
            str1[3] = _mm_load_si128((__m128i *) (this->buffer() + 48 + offset));

            __m128i str2[4];
            str2[0] = _mm_load_si128((__m128i *) (other.buffer() + offset));
            str2[1] = _mm_load_si128((__m128i *) (other.buffer() + 16 + offset));
            str2[2] = _mm_load_si128((__m128i *) (other.buffer() + 32 + offset));
            str2[3] = _mm_load_si128((__m128i *) (other.buffer() + 48 + offset));


            // go through each of the 4 16-byte chunks
            for (int i = 0; i < 4; i++) {

                auto result = _mm_xor_si128(str1[i], str2[i]);
                if (!_mm_test_all_zeros(result, result)) {

                    // go through each byte of the chunk
                    for (int j = 0; j < 16 && offset + j < min_length; j++) {
                        char c1 = *(buffer1 + offset + j);
                        char c2 = *(buffer2 + offset + j);
//                        std::cerr << "comparing: " << c1 << " : " << c2 << " at: " << j << std::endl;
                        if (c1 == c2) {
                            continue;
                        } else {
                            if (c1 < c2) {
                                return true;
                            } else {
                                return false;
                            }
                        }
                    } // each byte in 16-byte block
                    assert(false); // a difference was detected, but not subsequently found

                } else {
//                    std::cerr << "No difference, block: " << i << std::endl;
                }

                offset += 16;
            } // each 16-byte block
        }
        return false;
    }
};








template<class AlignedStringBuffer_t>
std::ostream & operator<<(std::ostream & os, AlignedString <AlignedStringBuffer_t> const & aligned_string) {
    os << aligned_string.c_str();
    return os;
}




} // end namespace xl

