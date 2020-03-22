#pragma once

#include <vulkan/vulkan.hpp>

namespace benzene::vulkan
{
    class buffer {
        public:
        buffer(): buf{nullptr}, mem{nullptr} {}
        buffer(vk::Device dev, vk::PhysicalDevice physical_dev, size_t size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties): dev{dev} {
            vk::BufferCreateInfo buffer_info{};
            buffer_info.size = size;
            buffer_info.usage = usage;
            buffer_info.sharingMode = vk::SharingMode::eExclusive;

            this->buf = dev.createBuffer(buffer_info);

            auto requirements = dev.getBufferMemoryRequirements(this->buf);

            vk::MemoryAllocateInfo alloc_info{};
            auto find_memory_type = [&physical_dev](uint32_t type_filter, vk::MemoryPropertyFlags properties) -> uint32_t {
                auto mem_properties = physical_dev.getMemoryProperties();

                for(size_t i = 0; i < mem_properties.memoryTypeCount; i++)
                    if(type_filter & (1 << i) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
                        return i;

                throw std::runtime_error("Couldn't find suitable memory type");
            };

            alloc_info.allocationSize = requirements.size;
            alloc_info.memoryTypeIndex = find_memory_type(requirements.memoryTypeBits, properties);

            this->mem = dev.allocateMemory(alloc_info);

            dev.bindBufferMemory(buf, mem, 0);
        }

        void clean(){
            this->dev.destroyBuffer(this->buf);
            this->dev.freeMemory(this->mem);
        }

        vk::Buffer& handle(){
            return buf;
        }

        vk::DeviceMemory& memory_handle(){
            return mem;
        }

        private:
        vk::Device dev;
        vk::Buffer buf;
        vk::DeviceMemory mem;
    };
} // namespace benzene::vulkan
