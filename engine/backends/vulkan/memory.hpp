#pragma once

#include <vulkan/vulkan.hpp>
#include "libs/vk_mem_alloc.hpp"

namespace benzene::vulkan
{
    class buffer {
        public:
        buffer(): allocator{nullptr}, buf{nullptr}, allocation{nullptr} {}
        buffer(vma::Allocator allocator, size_t size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties): allocator{allocator} {
            vk::BufferCreateInfo buffer_info{};
            buffer_info.size = size;
            buffer_info.usage = usage;
            buffer_info.sharingMode = vk::SharingMode::eExclusive;
            
            //auto requirements = dev.getBufferMemoryRequirements(this->buf);
            
            vma::AllocationCreateInfo alloc_info{};
            alloc_info.usage = vma::MemoryUsage::eUnknown;
            alloc_info.requiredFlags = properties;
            //alloc_info.memoryTypeBits = requirements.memoryTypeBits;


            std::tie(buf, allocation) = allocator.createBuffer(buffer_info, alloc_info);
        }

        void clean(){
            allocator.destroyBuffer(buf, allocation);
        }

        vk::Buffer& handle(){
            return buf;
        }

        vma::Allocation& allocation_handle(){
            return allocation;
        }

        /*vk::DeviceMemory& memory_handle(){
            return mem;
        }*/

        private:
        vma::Allocator allocator;
        vk::Buffer buf;
        vma::Allocation allocation;
    };
} // namespace benzene::vulkan
