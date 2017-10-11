#pragma once

#include <new>

#include "aligned_string.h"

namespace xl {


/**
 * Allocates an aligned block of at least the requested size
 * @tparam alignment required alignment of allocated block
 * @param minimum_bytes in: minimum size out: actual size
 * @return newly allocated buffer
 */
template<size_t alignment>
inline std::unique_ptr<char[]> allocate_aligned_blocks(size_t & minimum_bytes) {

    // get the size >= minimum size that is a multiple of the alignment 0 => 0, 1 => alignment, alignment => alignment, alignment + 1 => 2*alignment
    size_t actual_bytes = (minimum_bytes + (alignment - 1)) & ~(alignment-1);

    auto buffer = new(static_cast<std::align_val_t>(alignment)) char[actual_bytes]();

    assert(buffer[actual_bytes-1] == '\0');
    assert(((intptr_t)buffer & alignment-1) == 0);

    minimum_bytes = actual_bytes;
    return std::unique_ptr<char[]>(buffer);
}

/**
 * Dynamically-allocated buffer for AlignedString.  Buffer will be aligned to the specified alignment and grows in
 * multiples of the alignment
 * @tparam alignment_v alignment of the dynamically-allocated buffer  Must be 16 or 64
 */
template<size_t alignment_v>
class AlignedStringBuffer_Dynamic {
public:
    static constexpr size_t alignment = alignment_v;
    static_assert(alignment == 16 || alignment == 64, "AlignedStringBuffer_Dynamic alignment must be 16 or 64");
protected:

private:
    std::unique_ptr<char[]> _buffer;

    uint32_t _capacity = 0;
    uint32_t _length = 0;


public:
    AlignedStringBuffer_Dynamic() = default;

    void allocate(size_t initial_capacity) {
        if (initial_capacity <= this->capacity()) {
            throw AlignedStringException("New size isn't larger than old size");
        }

        auto new_buffer = allocate_aligned_blocks<alignment_v>(initial_capacity);
        if (!this->empty()) {
            strcpy(new_buffer.get(), this->buffer());
        }
        this->_buffer = std::move(new_buffer);
        _capacity = initial_capacity - 1;
    }


    char * buffer() {
        if (this->_buffer.get() == nullptr) {
            return reinterpret_cast<char *>(&this->_buffer);
        }
        return this->_buffer.get();
    }
    auto const buffer() const { return const_cast<AlignedStringBuffer_Dynamic<alignment> *>(this)->buffer(); }


    char const * c_str() {
        return this->buffer();
    }


    /**
     * Maximum string length which can be held in the current buffer
     * @return Maximum string length which can be held in the current buffer
     */
    auto capacity() const { return this->_capacity; }

    auto length() const { return this->_length; }

    void concat(char const * source, uint32_t length)  {
        if (this->length() + length > this->capacity()) {
            this->allocate(this->length() + length);
        }
        strncat(this->buffer(), source, length);
        this->_length += length;
    }

    bool empty() const {
        return this->_length == 0;
    }
};

// make sure the size is always at most 16 bytes and doesn't vary based on size
static_assert(sizeof(AlignedStringBuffer_Dynamic<16>) <= 16);
static_assert(sizeof(AlignedStringBuffer_Dynamic<64>) <= 16);

// also AlignedString must not contribute anything to the size
static_assert(sizeof(AlignedString<AlignedStringBuffer_Dynamic<16>>) == sizeof(AlignedStringBuffer_Dynamic<16>));
static_assert(sizeof(AlignedString<AlignedStringBuffer_Dynamic<64>>) == sizeof(AlignedStringBuffer_Dynamic<64>));

}