#ifndef __MEMORY_BLOCK_HPP__
#define __MEMORY_BLOCK_HPP__

#include <utility>

/**
 * std::byte* is a type-safe char*
 * ::operator new() is the way to allocate raw memory without calling any constructor
**/

struct MemoryBlock {
    std::byte* data;
    size_t size;
    size_t used;
    MemoryBlock* next;

    explicit MemoryBlock(size_t block_size) : size(block_size), used(0), next(nullptr)
    {
        data = static_cast<std::byte*>(::operator new(block_size));
    }
    ~MemoryBlock()
    {
        ::operator delete(data);
    }
    std::byte* current() const
    {
        return data + used;
    }
    size_t available() const 
    {
        return size - used;
    }
};

#endif /*__MEMORY_BLOCK_HPP__ */