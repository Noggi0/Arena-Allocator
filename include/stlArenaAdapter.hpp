#ifndef __STL_ARENA_ADAPTER_HPP__
#define __STL_ARENA_ADAPTER_HPP__

#include "include/arenaAllocator.hpp"

template<typename T>
class StlArenaAdapter
{
    public:
        using value_type = T;
        using pointer = T*;
        using const_pointer = const T*;
        using reference = T&;
        using const_reference = const T&;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;

        template <typename U>
        struct rebind {
            using other = StlArenaAdapter<U>;
        };

        explicit StlArenaAdapter(ArenaAllocator<>& arena) noexcept
            : arena(arena) {}

        template <typename U>
        StlArenaAdapter(const StlArenaAdapter<U>& other) noexcept
            : arena(other.arena) {}
            
        T* allocate(size_type n)
        {
            void* memory = arena.allocate(n * sizeof(T), alignof(T));
            return static_cast<T*>(memory);
        }
        void deallocate(T* p, size_type n) noexcept
        {
            // It's a no-op in our case
            // Arena doesn't allow individual deallocation
        }

        bool operator==(const StlArenaAdapter& other) const noexcept
        {
            return &arena == &other.arena;
        }
        bool operator!=(const StlArenaAdapter& other) const noexcept
        {
            return &arena != &other.arena;
        }
        template<typename>
        friend class StlArenaAdapter;
    private:
        ArenaAllocator<>& arena;
};

#endif /* __STL_ARENA_ADAPTER_HPP__ */