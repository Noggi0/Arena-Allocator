#ifndef __ALIGNMENT_HPP__
#define __ALIGNMENT_HPP__

#include <cstdint>

namespace alignment
{
    inline void* align_up(void* ptr, size_t alignment)
    {
        const uintptr_t address = reinterpret_cast<uintptr_t>(ptr);
        const uintptr_t aligned = (address + alignment - 1) & ~(alignment - 1);
        return reinterpret_cast<void*>(aligned);
    };

    inline size_t adjustment(void* ptr, size_t alignment)
    {
        const uintptr_t address = reinterpret_cast<uintptr_t>(ptr);
        const uintptr_t aligned = (address + alignment - 1) & ~(alignment - 1);
        return aligned - address;
    };

    inline bool isAligned(void* ptr, size_t alignment)
    {
        return reinterpret_cast<uintptr_t>(ptr) & (alignment - 1);
    };
}

#endif /* __ALIGNMENT_HPP__ */