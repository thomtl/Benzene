#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>

#include <algorithm>
#include <vector>

namespace benzene::vulkan
{
    struct swapchain_support_details {
        swapchain_support_details(vk::PhysicalDevice* dev, vk::SurfaceKHR* surface){
            this->cap = dev->getSurfaceCapabilitiesKHR(*surface);
            this->formats = dev->getSurfaceFormatsKHR(*surface);
            this->present_modes = dev->getSurfacePresentModesKHR(*surface);
        }

        vk::SurfaceFormatKHR choose_format(){
            for(const auto& format : formats)
                if(format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
                    return format;

            return formats[0];
        }

        vk::PresentModeKHR choose_present_mode(){
            for(const auto& mode : present_modes)
                if(mode == vk::PresentModeKHR::eMailbox)
                    return mode;

            return vk::PresentModeKHR::eFifo; // Guaranteed to exist
        }

        vk::Extent2D choose_swap_extent(GLFWwindow* window){
            if(cap.currentExtent.width != UINT32_MAX)
                return cap.currentExtent;

            int width, height;
            glfwGetWindowSize(window, &width, &height);

            vk::Extent2D extent{0, 0};

            extent.width = std::max(cap.minImageExtent.width, std::min(cap.maxImageExtent.width, (uint32_t)width));
            extent.height = std::max(cap.minImageExtent.height, std::min(cap.maxImageExtent.height, (uint32_t)height));

            return extent;
        }

        vk::SurfaceCapabilitiesKHR cap;
        std::vector<vk::SurfaceFormatKHR> formats;
        std::vector<vk::PresentModeKHR> present_modes;   
    };

    class swap_chain {
        public:
        swap_chain(): chain{nullptr} {}
        swap_chain(vk::Device* dev, vk::PhysicalDevice* physical_dev, vk::SurfaceKHR* surface, uint32_t graphics_queue_id, uint32_t presentation_queue_id, GLFWwindow* win): dev{dev} {
            swapchain_support_details details{physical_dev, surface};

            auto surface_format = details.choose_format();
            this->format = surface_format.format;
            auto mode = details.choose_present_mode();
            this->extent = details.choose_swap_extent(win);

            auto image_count = details.cap.minImageCount + 1;
            if(details.cap.maxImageCount > 0 && image_count > details.cap.maxImageCount)
                image_count = details.cap.maxImageCount;

            vk::SwapchainCreateInfoKHR create_info{};
            create_info.minImageCount = image_count;
            create_info.surface = *surface;
            create_info.imageFormat = format;
            create_info.imageColorSpace = surface_format.colorSpace;
            create_info.imageExtent = extent;
            create_info.imageArrayLayers = 1;
            create_info.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

            uint32_t queue_family_indices[] = {graphics_queue_id, presentation_queue_id};

            if(graphics_queue_id != presentation_queue_id){
                create_info.imageSharingMode = vk::SharingMode::eConcurrent;
                create_info.queueFamilyIndexCount = 2;
                create_info.pQueueFamilyIndices = queue_family_indices;
            } else {
                create_info.imageSharingMode = vk::SharingMode::eExclusive;
            }

            create_info.preTransform = details.cap.currentTransform;
            create_info.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
            create_info.presentMode = mode;
            create_info.clipped = true;
            create_info.oldSwapchain = {nullptr};

            this->chain = dev->createSwapchainKHR(create_info);

            this->images = dev->getSwapchainImagesKHR(chain);

            this->image_views = std::vector<vk::ImageView>{};
            for(size_t i = 0; i < images.size(); i++){
                vk::ImageViewCreateInfo create_info{};
                create_info.image = images[i];
                create_info.viewType = vk::ImageViewType::e2D;
                create_info.format = this->get_format();
                create_info.components.r = vk::ComponentSwizzle::eIdentity;
                create_info.components.g = vk::ComponentSwizzle::eIdentity;
                create_info.components.b = vk::ComponentSwizzle::eIdentity;
                create_info.components.a = vk::ComponentSwizzle::eIdentity;

                create_info.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
                create_info.subresourceRange.baseMipLevel = 0;
                create_info.subresourceRange.levelCount = 1;
                create_info.subresourceRange.baseArrayLayer = 0;
                create_info.subresourceRange.layerCount = 1;

                this->image_views.push_back(this->dev->createImageView(create_info));
            }
        }

        void clean(){
            for(auto& view : image_views)
                dev->destroyImageView(view);

            dev->destroySwapchainKHR(this->chain);
        }

        vk::Format get_format() const {
            return format;
        }

        vk::Extent2D get_extent() const {
            return extent;
        }

        vk::SwapchainKHR& handle(){
            return chain;
        }

        std::vector<vk::Image>& get_images(){
            return images;
        }

        std::vector<vk::ImageView>& get_image_views(){
            return image_views;
        }


        private:
        std::vector<vk::ImageView> image_views;
        std::vector<vk::Image> images;
        vk::Format format;
        vk::Extent2D extent;
        vk::Device* dev;
        vk::SwapchainKHR chain;
    };
} // namespace benzene::vulkan
