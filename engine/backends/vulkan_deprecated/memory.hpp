#pragma once

#include "base.hpp"
#include "../../libs/stb/stb_image.h"

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

    class Image {
        public:
        Image(): instance{nullptr}, image{nullptr}, allocation{nullptr} {}
        Image(Instance* instance, size_t width, size_t height, vk::Format format, vk::SampleCountFlagBits sample_count, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties): instance{instance} {
            vk::ImageCreateInfo image_info{};
            image_info.imageType = vk::ImageType::e2D;
            image_info.extent.width = width;
            image_info.extent.height = height;
            image_info.extent.depth = 1;
            image_info.mipLevels = 1;
            image_info.arrayLayers = 1;
            image_info.format = format;
            image_info.tiling = vk::ImageTiling::eOptimal;
            image_info.usage = usage;
            image_info.sharingMode = vk::SharingMode::eExclusive;
            image_info.samples = sample_count;
            
            vma::AllocationCreateInfo alloc_info{};
            alloc_info.usage = vma::MemoryUsage::eUnknown;
            alloc_info.requiredFlags = properties;

            std::tie(image, allocation) = instance->allocator.createImage(image_info, alloc_info);
        }

        void clean(){
            if(!instance)
                return;
            if(!instance->allocator)
                return;
            if(!image || !allocation)
                return;
            
            instance->allocator.destroyImage(image, allocation);
        }

        vk::Image& handle(){
            return image;
        }

        vma::Allocation& allocation_handle(){
            return allocation;
        }

        private:
        Instance* instance;
        
        vk::Image image;
        vma::Allocation allocation;
    };

    class ImageView {
        public:
        ImageView(): instance{nullptr}, view{nullptr} {}
        ImageView(Instance* instance, Image& image, vk::Format format, vk::ImageAspectFlags aspect): instance{instance} {
            vk::ImageViewCreateInfo view_info{};
            view_info.image = image.handle();
            view_info.format = format;
            view_info.viewType = vk::ImageViewType::e2D;
            view_info.subresourceRange.aspectMask = aspect;
            view_info.subresourceRange.baseMipLevel = 0;
            view_info.subresourceRange.levelCount = 1;
            view_info.subresourceRange.baseArrayLayer = 0;
            view_info.subresourceRange.layerCount = 1;

            view = instance->device.createImageView(view_info);
        }

        void clean(){
            //if(!instance || !view)
            //    return;
            if(instance && view)
                instance->device.destroyImageView(view);
        }

        vk::ImageView& handle(){
            return view;
        }

        private:
        Instance* instance;
        vk::ImageView view;
    };

    class Texture {
        public:
        Texture(): instance{nullptr}, image{}, view{}, sampler{nullptr} {}
        Texture(Instance* instance, size_t width, size_t height, const uint8_t* data, vk::Format format = vk::Format::eR8G8B8A8Srgb): instance{instance} {
            size_t size = width * height * 4;
            Buffer transfer{instance, size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent};

            transfer.map();
            memcpy(transfer.data(), data, size);
            transfer.unmap();

            image = Image{instance, (size_t)width, (size_t)height, format, vk::SampleCountFlagBits::e1, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal};

            auto transition_image_layout = [this]([[maybe_unused]] vk::Format format, vk::ImageLayout old_layout, vk::ImageLayout new_layout){
                CommandBuffer cmd{this->instance, &this->instance->graphics};
                {
                    std::lock_guard guard{cmd};

                    vk::ImageMemoryBarrier barrier{};
                    barrier.oldLayout = old_layout;
                    barrier.newLayout = new_layout;
                    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.image = image.handle();
                    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
                    barrier.subresourceRange.baseArrayLayer = 0;
                    barrier.subresourceRange.layerCount = 1;
                    barrier.subresourceRange.baseMipLevel = 0;
                    barrier.subresourceRange.levelCount = 1;

                    vk::PipelineStageFlags src_stage, dest_stage;

                    if(old_layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eTransferDstOptimal){
                        barrier.srcAccessMask = {};
                        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

                        src_stage = vk::PipelineStageFlagBits::eTopOfPipe;
                        dest_stage = vk::PipelineStageFlagBits::eTransfer;
                    } else if(old_layout == vk::ImageLayout::eTransferDstOptimal && new_layout == vk::ImageLayout::eShaderReadOnlyOptimal){
                        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
                        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

                        src_stage = vk::PipelineStageFlagBits::eTransfer;
                        dest_stage = vk::PipelineStageFlagBits::eFragmentShader;
                    } else 
                        throw std::invalid_argument("vulkan/texture: Unsupported layout transition");

                    cmd.handle().pipelineBarrier(src_stage, dest_stage, {}, {}, {}, {barrier});
                }
            };

            auto copy_buffer_to_image = [this](Buffer& buf, Image& img, size_t width, size_t height){
                CommandBuffer cmd{this->instance, &this->instance->graphics};
                {
                    std::lock_guard guard{cmd};

                    vk::BufferImageCopy region{};
                    region.bufferOffset = 0;
                    region.bufferRowLength = 0;
                    region.bufferImageHeight = 0;

                    region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
                    region.imageSubresource.mipLevel = 0;
                    region.imageSubresource.baseArrayLayer = 0;
                    region.imageSubresource.layerCount = 1;

                    region.imageOffset.x = 0;
                    region.imageOffset.y = 0;
                    region.imageOffset.z = 0;
                    region.imageExtent.width = width;
                    region.imageExtent.height = height;
                    region.imageExtent.depth = 1;

                    cmd.handle().copyBufferToImage(buf.handle(), img.handle(), vk::ImageLayout::eTransferDstOptimal, {region});
                }
            };

            transition_image_layout(format, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
            copy_buffer_to_image(transfer, image, width, height);
            transition_image_layout(format, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

            transfer.clean();

            view = ImageView{instance, image, format, vk::ImageAspectFlagBits::eColor};

            vk::SamplerCreateInfo sampler_info{};
            sampler_info.magFilter = vk::Filter::eLinear;
            sampler_info.minFilter = vk::Filter::eLinear;
            sampler_info.addressModeU = vk::SamplerAddressMode::eClampToBorder;
            sampler_info.addressModeV = vk::SamplerAddressMode::eClampToBorder;
            sampler_info.addressModeW = vk::SamplerAddressMode::eClampToBorder;
            
            sampler_info.anisotropyEnable = true;
            sampler_info.maxAnisotropy = 16;

            sampler_info.borderColor = vk::BorderColor::eIntOpaqueBlack;
            sampler_info.unnormalizedCoordinates = false;

            sampler_info.compareEnable = true;
            sampler_info.compareOp = vk::CompareOp::eAlways;

            sampler_info.mipmapMode = vk::SamplerMipmapMode::eLinear;
            sampler_info.mipLodBias = 0.0f;
            sampler_info.minLod = 0.0f;
            sampler_info.maxLod = 0.0f;

            sampler = instance->device.createSampler(sampler_info);
        }

        Texture(Instance* instance, benzene::Texture& tex): Texture{instance, (size_t)tex.dimensions().first, (size_t)tex.dimensions().second, tex.bytes().data()} {}

        void clean(){
            instance->device.destroySampler(sampler);
            view.clean();
            image.clean();
        }

        vk::ImageView& get_view(){
            return view.handle();
        }

        vk::Sampler& get_sampler(){
            return sampler;
        }

        private:
        Instance* instance;
        Image image;
        ImageView view;
        vk::Sampler sampler;

        int width, height, channels;
    };
} // namespace benzene::vulkan
