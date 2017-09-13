#pragma once

namespace xl {

/**
 * An inline storage buffer for AlignedString which uses a fixed-length array for storage
 * @tparam size maximum amount of size the entire object can use, including any overhead
 */
template<size_t size>
class alignas(size % 64 == 0 ? 64 : 16) AlignedStringBuffer_Static {
public:

    // the alignment of the buffer, either 16 or 64
    static constexpr size_t alignment = size % 64 == 0 ? 64 : 16;


private:

    // larger than this requires more bytes for _length and probably doesn't make sense anyhow
    static_assert(size <= 256, "Aligned string static buffer max size is 255 bytes");
    static_assert(size % 16 == 0, "Aligned string static buffer size must be a multiple of 16");

    using length_t = uint8_t;
    char _buffer[size - sizeof(length_t)] = {};
    length_t _length = 0;

    // maximum string length which can be stored
    static constexpr size_t max_string_length = size - sizeof(decltype(_length)) - 1 /* for NUL terminator */;


public:



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
        assert(((size_t)this) % 16 == 0);
    }

    AlignedStringBuffer_Static(AlignedStringBuffer_Static && other) : _length(other.length) {
        memcpy(&this->_buffer, &other._buffer, sizeof(_buffer));
    }

    /**
     * Returns the raw string buffer
     * @return the raw string buffer
     */
    auto buffer() { return &this->_buffer[0]; }

    /**
    * Returns the raw string buffer
    * @return the raw string buffer
    */
    auto const buffer() const { return this->_buffer; }

    /**
     * Returns the current maximum length of a string of the buffer
     * @return the current maximum length of a string of the buffer
     */
    auto capacity() const { return sizeof(_buffer) - 1; }

    /**
     * The length of the string currently held in the buffer.  Always 0 <= length()<= capacity()
     * @return the length of the string currently stored
     */
    auto length() const { return this->_length; }

    /**
     * Adds the specified string to the current string
     * @param source raw source buffer
     * @param length number of bytes to concat into this buffer
     */
    void concat(char const * source, uint32_t length)  {
        if (this->length() + length > this->max_string_length) {
            throw AlignedStringException("Resulting string too long for buffer");
        }
        strncat(this->buffer(), source, length);
        this->_length = length;
    }


    bool empty() const {
        return this->_length == 0;
    }
};

}