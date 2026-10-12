#include "Aroxorn/MemoryAllocator.hpp"
#include <stdexcept>

namespace Aroxorn {

    MemoryBlock::MemoryBlock(Device& device, VkDeviceSize size, uint32_t memoryTypeIndex, bool isHostVisible)
        : m_Device(device), m_Size(size), m_MemoryTypeIndex(memoryTypeIndex) {

        VkMemoryAllocateFlagsInfo allocFlagsInfo{};
        allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
        allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT; // We need this for RT and buffers

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = size;
        allocInfo.memoryTypeIndex = memoryTypeIndex;
        allocInfo.pNext = &allocFlagsInfo;

        if (vkAllocateMemory(m_Device.GetVkDevice(), &allocInfo, nullptr, &m_Memory) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate large memory block!");
        }

        if (isHostVisible) {
            if (vkMapMemory(m_Device.GetVkDevice(), m_Memory, 0, size, 0, &m_MappedData) != VK_SUCCESS) {
                throw std::runtime_error("Failed to map host visible memory block!");
            }
        }
    }

    MemoryBlock::~MemoryBlock() {
        if (m_MappedData != nullptr) {
            vkUnmapMemory(m_Device.GetVkDevice(), m_Memory);
        }
        vkFreeMemory(m_Device.GetVkDevice(), m_Memory, nullptr);
    }

    MemoryAllocator::MemoryAllocator(Device& device) : m_Device(device) {
    }

    MemoryAllocator::~MemoryAllocator() {
    }

    uint32_t MemoryAllocator::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(m_Device.GetVkPhysicalDevice(), &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("Failed to find suitable memory type!");
    }

    // --- Free-List Allocator Implementation ---

    FreeListAllocator::FreeListAllocator(Device& device, VkDeviceSize blockSize, uint32_t memoryTypeIndex, bool isHostVisible)
        : m_Device(device) {
        m_Block = std::make_unique<MemoryBlock>(device, blockSize, memoryTypeIndex, isHostVisible);

        m_Head = new FreeNode;
        m_Head->offset = 0;
        m_Head->size = blockSize;
        m_Head->next = nullptr;
    }

    FreeListAllocator::~FreeListAllocator() {
        FreeNode* curr = m_Head;
        while (curr != nullptr) {
            FreeNode* next = curr->next;
            delete curr;
            curr = next;
        }
    }

    Allocation FreeListAllocator::Allocate(VkDeviceSize size, VkDeviceSize alignment, bool threadSafe) {
        if (threadSafe) m_Mutex.lock();

        FreeNode* prev = nullptr;
        FreeNode* curr = m_Head;
        FreeNode* bestFitPrev = nullptr;
        FreeNode* bestFit = nullptr;
        VkDeviceSize bestFitOffset = 0;
        VkDeviceSize smallestDiff = ~0ULL;

        while (curr != nullptr) {
            VkDeviceSize alignedOffset = (curr->offset + alignment - 1) & ~(alignment - 1);
            VkDeviceSize padding = alignedOffset - curr->offset;

            if (curr->size >= size + padding) {
                VkDeviceSize diff = curr->size - (size + padding);
                if (diff < smallestDiff) {
                    smallestDiff = diff;
                    bestFitPrev = prev;
                    bestFit = curr;
                    bestFitOffset = alignedOffset;
                }
            }
            prev = curr;
            curr = curr->next;
        }

        if (bestFit == nullptr) {
            if (threadSafe) m_Mutex.unlock();
            throw std::runtime_error("FreeList Allocator out of memory or severely fragmented!");
        }

        VkDeviceSize padding = bestFitOffset - bestFit->offset;

        Allocation alloc{};
        alloc.memory = m_Block->GetMemory();
        alloc.offset = bestFitOffset;
        alloc.size = size;
        alloc.isLinear = false;
        alloc.allocator = this;

        // We stash a pointer to the original node metadata for freeing later
        // In a real system, you'd store this externally to avoid leaking memory to `new`,
        // but for now, we dynamically track it.
        FreeNode* allocatedMeta = new FreeNode;
        allocatedMeta->offset = bestFit->offset; // Original offset including padding
        allocatedMeta->size = size + padding;
        allocatedMeta->next = nullptr;
        alloc.internalData = allocatedMeta;

        if (m_Block->GetMappedData() != nullptr) {
            alloc.mappedData = static_cast<char*>(m_Block->GetMappedData()) + bestFitOffset;
        }

        if (bestFit->size == size + padding) {
            RemoveNode(bestFitPrev, bestFit);
            delete bestFit;
        } else {
            bestFit->offset += (size + padding);
            bestFit->size -= (size + padding);
        }

        if (threadSafe) m_Mutex.unlock();

        return alloc;
    }

    void FreeListAllocator::Free(const Allocation& allocation, bool threadSafe) {
        if (threadSafe) m_Mutex.lock();

        FreeNode* meta = static_cast<FreeNode*>(allocation.internalData);

        FreeNode* newNode = new FreeNode;
        newNode->offset = meta->offset;
        newNode->size = meta->size;
        newNode->next = nullptr;

        delete meta;

        FreeNode* prev = nullptr;
        FreeNode* curr = m_Head;

        while (curr != nullptr && curr->offset < newNode->offset) {
            prev = curr;
            curr = curr->next;
        }

        InsertNode(prev, newNode);
        Coalesce(prev, newNode);

        if (threadSafe) m_Mutex.unlock();
    }

    void FreeListAllocator::InsertNode(FreeNode* prevNode, FreeNode* newNode) {
        if (prevNode == nullptr) {
            newNode->next = m_Head;
            m_Head = newNode;
        } else {
            newNode->next = prevNode->next;
            prevNode->next = newNode;
        }
    }

    void FreeListAllocator::RemoveNode(FreeNode* prevNode, FreeNode* delNode) {
        if (prevNode == nullptr) {
            m_Head = delNode->next;
        } else {
            prevNode->next = delNode->next;
        }
    }

    void FreeListAllocator::Coalesce(FreeNode* prevNode, FreeNode* freeNode) {
        if (freeNode->next != nullptr && freeNode->offset + freeNode->size == freeNode->next->offset) {
            FreeNode* nextNode = freeNode->next;
            freeNode->size += nextNode->size;
            RemoveNode(freeNode, nextNode);
            delete nextNode;
        }

        if (prevNode != nullptr && prevNode->offset + prevNode->size == freeNode->offset) {
            prevNode->size += freeNode->size;
            RemoveNode(prevNode, freeNode);
            delete freeNode;
        }
    }

    // --- Linear Allocator Implementation ---

    LinearAllocator::LinearAllocator(Device& device, VkDeviceSize blockSize, uint32_t memoryTypeIndex, bool isHostVisible)
        : m_Device(device) {
        m_Block = std::make_unique<MemoryBlock>(device, blockSize, memoryTypeIndex, isHostVisible);
    }

    Allocation LinearAllocator::Allocate(VkDeviceSize size, VkDeviceSize alignment, bool threadSafe) {
        if (threadSafe) {
            m_Mutex.lock();
        }

        // Calculate aligned offset
        VkDeviceSize alignedOffset = (m_Offset + alignment - 1) & ~(alignment - 1);

        if (alignedOffset + size > m_Block->GetSize()) {
            if (threadSafe) m_Mutex.unlock();
            throw std::runtime_error("Linear Allocator out of memory! Requested size exceeds block capacity.");
        }

        Allocation alloc{};
        alloc.memory = m_Block->GetMemory();
        alloc.offset = alignedOffset;
        alloc.size = size;
        alloc.isLinear = true;
        alloc.allocator = this;

        if (m_Block->GetMappedData() != nullptr) {
            alloc.mappedData = static_cast<char*>(m_Block->GetMappedData()) + alignedOffset;
        }

        m_Offset = alignedOffset + size;

        if (threadSafe) {
            m_Mutex.unlock();
        }

        return alloc;
    }

    void LinearAllocator::Reset(bool threadSafe) {
        if (threadSafe) std::lock_guard<std::mutex> lock(m_Mutex);
        m_Offset = 0;
    }

    // --- Memory Allocator Implementation ---

    LinearAllocator* MemoryAllocator::GetOrCreateLinearAllocator(uint32_t memoryTypeIndex, bool isHostVisible) {
        std::lock_guard<std::mutex> lock(m_AllocatorMutex);

        for (auto& allocator : m_LinearAllocators) {
            if (allocator->GetMemoryTypeIndex() == memoryTypeIndex) {
                return allocator.get();
            }
        }

        auto newAllocator = std::make_unique<LinearAllocator>(m_Device, LINEAR_BLOCK_SIZE, memoryTypeIndex, isHostVisible);
        m_LinearAllocators.push_back(std::move(newAllocator));
        return m_LinearAllocators.back().get();
    }

    FreeListAllocator* MemoryAllocator::GetOrCreateFreeListAllocator(uint32_t memoryTypeIndex, bool isHostVisible) {
        std::lock_guard<std::mutex> lock(m_AllocatorMutex);

        for (auto& allocator : m_FreeListAllocators) {
            if (allocator->GetMemoryTypeIndex() == memoryTypeIndex) {
                return allocator.get();
            }
        }

        auto newAllocator = std::make_unique<FreeListAllocator>(m_Device, FREE_LIST_BLOCK_SIZE, memoryTypeIndex, isHostVisible);
        m_FreeListAllocators.push_back(std::move(newAllocator));
        return m_FreeListAllocators.back().get();
    }

    Allocation MemoryAllocator::Allocate(VkMemoryRequirements requirements, VkMemoryPropertyFlags properties, bool useLinear, bool threadSafe) {
        uint32_t memoryTypeIndex = FindMemoryType(requirements.memoryTypeBits, properties);
        bool isHostVisible = (properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0;

        if (useLinear) {
            LinearAllocator* linearAlloc = GetOrCreateLinearAllocator(memoryTypeIndex, isHostVisible);
            return linearAlloc->Allocate(requirements.size, requirements.alignment, threadSafe);
        } else {
            FreeListAllocator* freeListAlloc = GetOrCreateFreeListAllocator(memoryTypeIndex, isHostVisible);
            return freeListAlloc->Allocate(requirements.size, requirements.alignment, threadSafe);
        }
    }

    void MemoryAllocator::Free(const Allocation& allocation, bool threadSafe) {
        if (allocation.isLinear) {
            return;
        }

        if (allocation.allocator) {
            static_cast<FreeListAllocator*>(allocation.allocator)->Free(allocation, threadSafe);
        }
    }

    void MemoryAllocator::ResetLinear(bool threadSafe) {
        for (auto& allocator : m_LinearAllocators) {
            allocator->Reset(threadSafe);
        }
    }

}
