#pragma once

#include "base.hpp"

namespace benzene::vulkan
{
    class Buffer {
        public:
        Buffer(): internal_data{nullptr}, instance{nullptr}, buf{nullptr}, allocation{nullptr} {}
        Buffer(Instance* instance, size_t length, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties): length{length}, internal_data{nullptr}, instance{instance} {
            if(length == 0)
                length = 1;
            vk::BufferCreateInfo buffer_info{};
            buffer_info.size = length;
            buffer_info.usage = usage;
            buffer_info.sharingMode = vk::SharingMode::eExclusive;
            
            vma::AllocationCreateInfo alloc_info{};
            alloc_info.usage = vma::MemoryUsage::eUnknown;
            alloc_info.requiredFlags = properties;

            std::tie(buf, allocation) = instance->allocator.createBuffer(buffer_info, alloc_info);
        }

        void clean(){
            if(!instance)
                return;
            if(!instance->allocator)
                return;
            if(!buf || !allocation)
                return;
            
            instance->allocator.destroyBuffer(buf, allocation);
        }

        void map(){
            if(internal_data)
                this->unmap();
            
            this->internal_data = instance->allocator.mapMemory(allocation);
        }

        void unmap(){
            if(!internal_data)
                return;
            
            instance->allocator.unmapMemory(allocation);
            this->internal_data = nullptr;
        }

        void* data(){
            return this->internal_data;
        }

        vk::Buffer& handle(){
            return buf;
        }

        vma::Allocation& allocation_handle(){
            return allocation;
        }

        size_t size(){
            return length;
        }

        private:
        size_t length;
        void* internal_data;
        Instance* instance;
        vk::Buffer buf;
        vma::Allocation allocation;
    };
} // namespace benzene::vulkan
