#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <type_traits>
#include <memory>


using namespace std;

template<typename T, size_t size>
struct Allocation {
    // this is wasteful if this isn't true
    static_assert(sizeof(T) >= sizeof(T*));

    using memory_type = std::aligned_union_t<1, T *, T>;
    constexpr static size_t memory_type_size = sizeof(memory_type);

    memory_type buffer[size] = {};
    std::unique_ptr<Allocation> next;

    T * operator[](size_t n) {
        return reinterpret_cast<T*>(&buffer) + n;
    }

    Allocation() {
        cerr << "allocated buffer from " << (void*)&buffer[0] << " to " << &buffer[size-1] << endl;
    }
};

template<typename T, size_t cache_lines = 64>
struct Allocator {

    constexpr static size_t allocation_size = 64 * cache_lines / sizeof(T);
    using allocation_type = Allocation<T, allocation_size>;

    std::unique_ptr<allocation_type> allocation = std::make_unique<allocation_type>();
    allocation_type * last_allocation = allocation.get();

    size_t last_allocation_unused_places = allocation_size;
    T * next_free = nullptr;

    template<typename... Ts>
    T * allocate(Ts&&... ts) {
        cerr << "Allocating with " << last_allocation_unused_places << " free positions in last allocation"  << endl;
        if (next_free == nullptr) {
            if (last_allocation_unused_places == 0) {
                cerr << "allocating more space" << endl;
                assert(!last_allocation->next);
                last_allocation->next = std::make_unique<allocation_type>();
                last_allocation_unused_places = allocation_size;
            }
            cerr << "handing out never-used memory at: " << (void*)(*last_allocation)[allocation_size - last_allocation_unused_places]  << endl;
            assert(last_allocation_unused_places > 0);

            auto result = (*last_allocation)[allocation_size - last_allocation_unused_places];
            last_allocation_unused_places--;
            return result;
        } else {
            cerr << "reusing element" << endl;
            T * result = next_free;
            next_free = &*next_free;
            return result;
        }
    }

    void free(T * element) {

        cerr << "Freeing: " << (void*)element << " with next_free as: " << (void*) next_free << endl;
        cerr << "TODO: Call destructor" << endl;

        // treat the memory at where element is pointing as a T*
        cerr << "setting element" << endl;
        *reinterpret_cast<T**>(element) = next_free;
        cerr << "setting next_free" << endl;
        next_free = element;
        cerr << "after free, next_free is " << (void*)next_free << endl;
    }
};

class MyClass {
public:
    int i;
    char j;
    char * str;
};

// TODO: Should return special unique_ptr that properly returns memory

//
//int main() {
//
//    cerr << "sizeof MyClass: " << sizeof(MyClass) << endl;
//
//    Allocator<MyClass, 1> allocator;
//    auto mem = allocator.allocate();
//    auto mem2 = allocator.allocate();
//    auto mem3 = allocator.allocate();
//    auto mem4 = allocator.allocate();
//    auto mem5 = allocator.allocate();
//    allocator.free(mem);
//    allocator.free(mem2);
//    allocator.free(mem3);
//    mem = allocator.allocate();
//    mem = allocator.allocate();
//    mem = allocator.allocate();
//    mem = allocator.allocate();
//}
