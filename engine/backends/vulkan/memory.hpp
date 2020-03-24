#pragma once

#include "base.hpp"

namespace benzene::vulkan
{
    class Buffer {
        public:
        Buffer(): instance{nullptr}, buf{nullptr}, allocation{nullptr} {}
        Buffer(Instance* instance, size_t size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties): instance{instance} {
            vk::BufferCreateInfo buffer_info{};
            buffer_info.size = size;
            buffer_info.usage = usage;
            buffer_info.sharingMode = vk::SharingMode::eExclusive;
            
            vma::AllocationCreateInfo alloc_info{};
            alloc_info.usage = vma::MemoryUsage::eUnknown;
            alloc_info.requiredFlags = properties;

            std::tie(buf, allocation) = instance->allocator.createBuffer(buffer_info, alloc_info);
        }

        void clean(){
            instance->allocator.destroyBuffer(buf, allocation);
        }

        vk::Buffer& handle(){
            return buf;
        }

        vma::Allocation& allocation_handle(){
            return allocation;
        }

        private:
        Instance* instance;
        vk::Buffer buf;
        vma::Allocation allocation;
    };
} // namespace benzene::vulkan
