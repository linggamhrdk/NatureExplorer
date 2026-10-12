#pragma once

#include "Aroxorn/Device.hpp"
#include <vulkan/vulkan.h>
#include <mutex>
#include <vector>
#include <memory>
#include <cstdint>

namespace Aroxorn {

    struct Allocation {
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkDeviceSize offset = 0;
        VkDeviceSize size = 0;
        void* mappedData = nullptr;
        bool isLinear = false;
        // Pointer back to the sub-allocator that created this to ensure safe frees
        void* allocator = nullptr;
        // Internal handle to track the specific block or free-list node
        void* internalData = nullptr;
    };

    // Represents a large chunk of Vulkan memory allocated from the device
    class MemoryBlock {
    public:
        MemoryBlock(Device& device, VkDeviceSize size, uint32_t memoryTypeIndex, bool isHostVisible);
        ~MemoryBlock();

        MemoryBlock(const MemoryBlock&) = delete;
        MemoryBlock& operator=(const MemoryBlock&) = delete;

        VkDeviceMemory GetMemory() const { return m_Memory; }
        VkDeviceSize GetSize() const { return m_Size; }
        uint32_t GetMemoryTypeIndex() const { return m_MemoryTypeIndex; }
        void* GetMappedData() const { return m_MappedData; }

    private:
        Device& m_Device;
        VkDeviceMemory m_Memory = VK_NULL_HANDLE;
        VkDeviceSize m_Size = 0;
        uint32_t m_MemoryTypeIndex = 0;
        void* m_MappedData = nullptr;
    };

    struct FreeNode {
        VkDeviceSize offset;
        VkDeviceSize size;
        FreeNode* next;
    };

    class FreeListAllocator {
    public:
        FreeListAllocator(Device& device, VkDeviceSize blockSize, uint32_t memoryTypeIndex, bool isHostVisible);
        ~FreeListAllocator();

        FreeListAllocator(const FreeListAllocator&) = delete;
        FreeListAllocator& operator=(const FreeListAllocator&) = delete;

        Allocation Allocate(VkDeviceSize size, VkDeviceSize alignment, bool threadSafe);
        void Free(const Allocation& allocation, bool threadSafe);

        uint32_t GetMemoryTypeIndex() const { return m_Block->GetMemoryTypeIndex(); }

    private:
        Device& m_Device;
        std::unique_ptr<MemoryBlock> m_Block;

        FreeNode* m_Head;
        std::mutex m_Mutex;

        void InsertNode(FreeNode* prevNode, FreeNode* newNode);
        void RemoveNode(FreeNode* prevNode, FreeNode* delNode);
        void Coalesce(FreeNode* prevNode, FreeNode* freeNode);
    };

    class LinearAllocator {
    public:
        LinearAllocator(Device& device, VkDeviceSize blockSize, uint32_t memoryTypeIndex, bool isHostVisible);
        ~LinearAllocator() = default;

        LinearAllocator(const LinearAllocator&) = delete;
        LinearAllocator& operator=(const LinearAllocator&) = delete;

        Allocation Allocate(VkDeviceSize size, VkDeviceSize alignment, bool threadSafe);
        void Reset(bool threadSafe);

        uint32_t GetMemoryTypeIndex() const { return m_Block->GetMemoryTypeIndex(); }

    private:
        Device& m_Device;
        std::unique_ptr<MemoryBlock> m_Block;

        VkDeviceSize m_Offset = 0;
        std::mutex m_Mutex;
    };

    class MemoryAllocator {
    public:
        MemoryAllocator(Device& device);
        ~MemoryAllocator();

        MemoryAllocator(const MemoryAllocator&) = delete;
        MemoryAllocator& operator=(const MemoryAllocator&) = delete;

        Allocation Allocate(VkMemoryRequirements requirements, VkMemoryPropertyFlags properties, bool useLinear, bool threadSafe = true);
        void Free(const Allocation& allocation, bool threadSafe = true);
        void ResetLinear(bool threadSafe = true);

        uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

    private:
        Device& m_Device;

        // Block size defaults
        static constexpr VkDeviceSize LINEAR_BLOCK_SIZE = 1024 * 1024 * 64; // 64 MB for per-frame data
        static constexpr VkDeviceSize FREE_LIST_BLOCK_SIZE = 1024 * 1024 * 256; // 256 MB for persistent resources

        // We map memory type index to an allocator instance.
        // For linear, we generally just need one host-visible block per frame in flight.
        // To keep it simple for now, we'll store one linear allocator per memory type used.
        std::vector<std::unique_ptr<LinearAllocator>> m_LinearAllocators;
        std::vector<std::unique_ptr<FreeListAllocator>> m_FreeListAllocators;

        LinearAllocator* GetOrCreateLinearAllocator(uint32_t memoryTypeIndex, bool isHostVisible);
        FreeListAllocator* GetOrCreateFreeListAllocator(uint32_t memoryTypeIndex, bool isHostVisible);

        std::mutex m_AllocatorMutex;
    };

}
