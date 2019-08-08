#pragma once

#ifdef XL_USE_LIB_FMT
#include <fmt/format.h>
#endif

namespace xl {

/**
 * An inline storage buffer for AlignedString which uses a fixed-length array for storage
 *
 * Allows storage of a string of length size-1 and then a NUL terminator
 *
 * Uses the last byte to store a representation of the length in such a way that the value is
 *   0 when the string is full and therefor doubles as the NUL terminator
 * @tparam size maximum amount of size the entire object can use, including any overhead
 */
template<size_t size>
class alignas(size % 64 == 0 ? 64 : 16) AlignedStringBuffer_Static {

    // The size is stored in the last byte, so the size can't exceed 256
    static_assert(size <= 256, "Aligned string static buffer max size is 256 bytes");
    static_assert(size % 16 == 0, "Aligned string static buffer size must be a multiple of 16");

public:

    // the alignment of the buffer, either 16 or 64
    static constexpr size_t alignment = size % 64 == 0 ? 64 : 16;

private:
    using length_t = uint8_t;
    char _buffer[size] = {};

    void set_length(length_t length) {
        // if the string is full (size - 1 characters), then the last byte should be 0 to match NUL terminator
        this->_buffer[size - 1] = size - (length + 1);
    }

public:

    length_t length() const {
        // if the length byte is 0, then the string is full which means there are size - 1 characters and a NUL
        auto result = size - this->_buffer[size - 1] - 1;
        return result;
    }


    /**
     * Doesn't do anything because the size of the buffer is fixed at compile time, but will throw an
     * exception if the requested size is larger than the static size
     * @param new_size
     */
    void allocate(size_t new_size) const {
        if (new_size > sizeof(_buffer)) {
            throw AlignedStringException("Requested allocation size larger than static size");
        }
    }


    AlignedStringBuffer_Static() {
        this->set_length(0);
    }


    AlignedStringBuffer_Static(AlignedStringBuffer_Static && other) {
        // this copies the size along with the content
        memcpy(&this->_buffer, &other._buffer, sizeof(_buffer));
    }



    /**
     * Returns the raw string buffer
     * @return the raw string buffer
     */
    char * buffer() { return &this->_buffer[0]; }
    char const * buffer() const { return const_cast<AlignedStringBuffer_Static<size> *>(this)->buffer(); }


    char const * c_str() {
        return this->buffer();
    }

    /**
     * Returns the current maximum length of a string of the buffer
     * @return the current maximum length of a string of the buffer
     */
    auto capacity() const { return sizeof(_buffer) - 1; }


    /**
     * Adds the specified string to the current string
     * @param source raw source buffer
     * @param length number of bytes to concat into this buffer
     */
    void concat(char const * source, uint32_t length)  {

        // the size doesn't include the NUL terminator
        if (this->length() + length >= size) {
            throw AlignedStringException("Resulting string too long for buffer");
        }
        strncat(this->buffer(), source, length);
        this->set_length(length);
    }


    bool empty() const {
        return this->_buffer[0] == '\0';
    }
};

// make sure the size is always exactly the templated size in bytes
static_assert(sizeof(AlignedStringBuffer_Static<16>) == 16);
static_assert(sizeof(AlignedStringBuffer_Static<64>) == 64);

// also AlignedString must not contribute anything to the size
static_assert(sizeof(AlignedString<AlignedStringBuffer_Static<16>>) == sizeof(AlignedStringBuffer_Static<16>));
static_assert(sizeof(AlignedString<AlignedStringBuffer_Static<64>>) == sizeof(AlignedStringBuffer_Static<64>));


} // end namespace xl