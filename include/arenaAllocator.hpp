#ifndef __ARENA_ALLOCATOR_HPP__
#define __ARENA_ALLOCATOR_HPP__

#include "memoryBlock.hpp"
#include "alignment.hpp"
#include <stdexcept>

template <size_t DefaultBlockSize = 8192>
class ArenaAllocator {
    public:
        ArenaAllocator() noexcept
            : head(nullptr), current(nullptr), totalAllocated(0), totalUsed(0), blockCount(0)
        {

        };
        explicit ArenaAllocator(size_t initialBlockSize) noexcept
            : head(nullptr), current(nullptr), totalAllocated(0), totalUsed(0), blockCount(0)
        {

        };
        ArenaAllocator(const ArenaAllocator&) = delete;
        ArenaAllocator& operator=(const ArenaAllocator&) = delete;

        ArenaAllocator(ArenaAllocator&& moved) noexcept
            : head(moved.head), current(moved.current), totalAllocated(moved.totalAllocated), totalUsed(moved.totalUsed), blockCount(moved.blockCount)
        {
            moved.head = nullptr;
            moved.current = nullptr;
            moved.totalAllocated = 0;
            moved.totalUsed = 0;
            moved.blockCount = 0;
        };
        ArenaAllocator& operator=(ArenaAllocator&& moved) noexcept
        {
            if (this != &moved)
            {
                release();
                head = moved.head;
                current = moved.current;
                totalAllocated = moved.totalAllocated;
                totalUsed = moved.totalUsed;
                blockCount = moved.blockCount;
                moved.head = nullptr;
                moved.current = nullptr;
                moved.totalAllocated = 0;
                moved.totalUsed = 0;
                moved.blockCount = 0;
            }
            return *this;
        };
        
        /**
         * @param alignment defaulted value : alignof(std::max_align_t)
         * which means that by default, the alignment is enough for every standard type.
         *
         * @return an aligned buffer of a given size, or nullptr if size == 0
         **/
        void* allocate(size_t size, size_t alignment = alignof(std::max_align_t))
        {
            if (size == 0)
                return nullptr;

            if ((alignment & (alignment - 1)) != 0)
                throw std::runtime_error("Alignment must be a power of 2.");

            if (this->current == nullptr)
                add_block(std::max(alignment, DefaultBlockSize));

            void* ptr_currentBlock = this->current->current();
            void* ptr_aligned = alignment::align_up(ptr_currentBlock, alignment);
            size_t adjusted = size + static_cast<std::byte*>(ptr_aligned) - static_cast<std::byte*>(ptr_currentBlock);

            if (this->current->used + adjusted > this->current->available())
            {
                size_t newBlockSize = std::max(size, DefaultBlockSize);
                add_block(newBlockSize);

                ptr_currentBlock = this->current->current();
                ptr_aligned = alignment::align_up(ptr_currentBlock, alignment);
                adjusted = size + static_cast<std::byte*>(ptr_aligned) - static_cast<std::byte*>(ptr_currentBlock);
            }

            this->current->used += adjusted;
            this->totalUsed += adjusted;
            return ptr_aligned;
        };

        /**
         * Headache style function hehe
         * First, we allocate memory with sufficient size and properly aligned.
         * Then, we make use of this syntax : new(placement-args) (type)
         * It allows us to create the object in the place (or memory emplacement) of our choice.
         * Finally, we create our object of type T, and we forward the arguments to its constructor.
         *
         * You can see it like an `emplace_back()` function, but type-safe.
         */
        template<typename T, typename... Args>
        T* create(Args&&... args)
        {
            void* allocatedMemory = allocate(sizeof(T), alignof(T));
            return new(allocatedMemory) T(std::forward<Args>(args)...);
        };

        /**
        * Allocates and returns a large-enough memory block (or buffer) to contain **count** elements.
        *
        * !! It doesn't initialize elements, it only allocates memory
        */
        template<typename T>
        T* allocate_array(size_t count)
        {
            void* allocatedMemory = allocate(sizeof(T) * count, alignof(T));

            return static_cast<T*>(allocatedMemory);
        };

        /**
         * Reset every memory block, but keep them allocated.
         * Use it when you want to reuse the arena.
         */
        void reset() noexcept
        {
            if (this->head == nullptr)
                return;
            MemoryBlock* block = this->head;

            while (block->next != nullptr)
            {
                block->used = 0;
                block = block->next;
            }
            this->current = this->head;
            this->totalUsed = 0;
        };

        /**
         * Deallocate every memory block.
         */
        void release() noexcept
        {
            while (this->head != nullptr)
            {
                MemoryBlock* nextBlock = this->head->next;
                delete this->head;
                this->head = nextBlock;
            }
            this->current = nullptr;
            this->totalAllocated = 0;
            this->totalUsed = 0;
            this->blockCount = 0;
        };

        size_t total_allocated() const noexcept
        {
            return this->totalAllocated;
        }

        size_t total_used() const noexcept
        {
            return this->totalUsed;
        }

        size_t block_count() const noexcept
        {
            return this->blockCount;
        }
        
        ~ArenaAllocator()
        {
            release();
        };
    private:
        MemoryBlock* head;
        MemoryBlock* current;
        size_t totalAllocated;
        size_t totalUsed;
        size_t blockCount;

        void add_block(size_t block_size)
        {
            auto newBlock = new MemoryBlock(block_size);

            this->blockCount += 1;
            this->totalAllocated += block_size;

            if (this->head == nullptr)
                this->head = newBlock;
            else
                this->current->next = newBlock;

            this->current = newBlock;};
};

#endif /* __ARENA_ALLOCATOR_HPP__ */