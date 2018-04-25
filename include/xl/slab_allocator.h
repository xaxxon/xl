#pragma once

#include <memory>


namespace xl {

template<typename T, size_t size>
struct Allocation {
    // this is wasteful if this isn't true
    static_assert(sizeof(T) >= sizeof(T *));

    using memory_type = std::aligned_union_t<1, T *, T>;
    constexpr static size_t memory_type_size = sizeof(memory_type);

    memory_type buffer[size] = {};
    Allocation * next = nullptr;

    T * operator[](size_t n) {
        return reinterpret_cast<T *>(&buffer) + n;
    }
};

template<typename T, size_t cache_lines = 64>
struct Allocator;


template<typename Allocator>
struct SlabAllocatorDeleter {
    Allocator * allocator;

    SlabAllocatorDeleter(Allocator * allocator) :
        allocator(allocator) {}

    void operator()(typename Allocator::element_type * p) {
        allocator->free(p);
    }
};

template<typename T, size_t cache_lines>
struct Allocator {

    using element_type = T;

    using Self = Allocator<T, cache_lines>;

    constexpr static size_t allocation_size = 64 * cache_lines / sizeof(T);
    using allocation_type = Allocation<T, allocation_size>;

    allocation_type * allocation = new allocation_type();
    allocation_type * last_allocation = allocation;

    size_t last_allocation_unused_places = allocation_size;
    T * next_free = nullptr;


    ~Allocator() {
        // iterative cleanup to avoid very large recursive destructor chains
        allocation_type * current_allocation = this->allocation;
        while (current_allocation != nullptr) {
            auto next = current_allocation->next;
            delete current_allocation;
            current_allocation = next;
        }
    }


    /**
     * Default deleter increases the returned type result to 2xPointer, use 
     * custom deleter that intrinsically knows what allocator to free from to reduce that
     * pointer size
     * @tparam Ts 
     * @tparam Deleter 
     * @param ts 
     * @return 
     */
    template<typename Deleter = SlabAllocatorDeleter<Self>, typename... Ts>
    std::unique_ptr<T, Deleter> allocate(Ts && ... ts) {
        T * memory = nullptr;

        // if there aren't any returned elements, return a 'fresh' one
        if (next_free == nullptr) {

            // is an (additional) allocation required?
            if (last_allocation_unused_places == 0) {
                assert(!last_allocation->next);
                last_allocation->next = new allocation_type();
                last_allocation = last_allocation->next;
                last_allocation_unused_places = allocation_size;
            }
            assert(last_allocation_unused_places > 0);

            memory = (*last_allocation)[allocation_size - last_allocation_unused_places];
            last_allocation_unused_places--;
        } else {
            memory = next_free;
            next_free = *reinterpret_cast<T **>(next_free);
        }
        assert(memory != nullptr);
        ::new(memory) T(std::forward<Ts>(ts)...);
        return std::unique_ptr<T, Deleter>(memory, Deleter(this));
    }

    void free(T * element) {
        element->T::~T();
        *reinterpret_cast<T **>(element) = next_free;
        next_free = element;
    }

    // debug function runs in linear time
    size_t count_allocations() const {
        size_t count = 0;
        auto current_allocation = this->allocation;

        while (current_allocation) {
            count++;
            current_allocation = current_allocation->next;
        }

        return count;
    }

    size_t count_available_returned_positions() const {
        size_t count = 0;
        T * current_position = this->next_free;
        while (current_position != nullptr) {
            count++;
            current_position = *reinterpret_cast<T **>(current_position);
        }
        return count;
    }
};

} // end namespace xl
