#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <utility>
#include <stdexcept>
#include <vector>
#include <array>
#include <algorithm>
#include <mutex>
#include <chrono>

#include <benzene/benzene.hpp>

#include "libs/vk_mem_alloc.hpp"
#include "../../core/format.hpp"
#include "../../core/utils.hpp"

#include "extra_api.hpp"

namespace benzene::vulkan
{
    constexpr bool enable_validation = true;
    constexpr bool debug = true;

    #define ENABLE_WIREFRAME_OUTLINE true // Define needed since we want to use it in #ifdef
    constexpr bool enable_wireframe_outline = ENABLE_WIREFRAME_OUTLINE;
    constexpr size_t max_frames_in_flight = 16;

    struct Instance {
        GLFWwindow* window;
        vk::Instance instance;
        vk::PhysicalDevice gpu;
        vk::Device device;
        vk::CommandPool command_pool;
        vk::SurfaceKHR surface;
        vma::Allocator allocator;

        vk::Format find_supported_format(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features){
            for(const auto& format : candidates){
                auto properties = this->gpu.getFormatProperties(format);

                if(tiling == vk::ImageTiling::eLinear && (properties.linearTilingFeatures & features) == features)
                    return format;
                else if(tiling == vk::ImageTiling::eOptimal && (properties.optimalTilingFeatures & features) == features)
                    return format;
            }

            throw std::runtime_error("vulkan: Couldn't find supported format");
        }

        vk::Format find_depth_format(){
            return find_supported_format({vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint}, vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
        }

        vk::SampleCountFlagBits find_max_msaa_samples(){
            auto limits = gpu.getProperties().limits;
            auto samples = limits.framebufferColorSampleCounts & limits.framebufferDepthSampleCounts;

            #define CHECK(sample) \
                if(samples & (sample)) { \
                    return (sample); \
                }

            CHECK(vk::SampleCountFlagBits::e64);
            CHECK(vk::SampleCountFlagBits::e32);
            CHECK(vk::SampleCountFlagBits::e16);
            CHECK(vk::SampleCountFlagBits::e8);
            CHECK(vk::SampleCountFlagBits::e4);
            CHECK(vk::SampleCountFlagBits::e2);

            #undef CHECK

            return vk::SampleCountFlagBits::e1;
        }

        template<typename T>
        void add_debug_tag([[maybe_unused]] T& item, [[maybe_unused]] const char* name){
            if constexpr (enable_validation){
                vk::DebugUtilsObjectNameInfoEXT name_info{};
                name_info.objectType = T::objectType;
                name_info.objectHandle = (uint64_t)(typename T::CType)item;
                name_info.pObjectName = name;
                extra_api::SetDebugUtilsObjectNameEXT(device, &name_info);
            }            
        }

        struct Queue {
            Queue(): family{0xFFFFFFFF}, handle{nullptr} {}
            Queue(uint32_t family, vk::Queue&& queue): family{family}, handle{std::move(queue)} {}

            uint32_t family;
            vk::Queue& operator()(){
                return handle;
            }
            private:
            vk::Queue handle;
        };
        Queue graphics, present;
    };

    struct CommandBuffer {
        CommandBuffer(Instance* instance, Instance::Queue* submit_queue): instance{instance}, submit_queue{submit_queue}, cmd{nullptr} {}

        void lock(){
            vk::CommandBufferAllocateInfo alloc_info{};
            alloc_info.level = vk::CommandBufferLevel::ePrimary;
            alloc_info.commandPool = instance->command_pool;
            alloc_info.commandBufferCount = 1;

            auto buffers = instance->device.allocateCommandBuffers(alloc_info);
            this->cmd = buffers[0];

            vk::CommandBufferBeginInfo begin_info{};
            begin_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

            cmd.begin(begin_info);
        }

        void unlock(){
            cmd.end();

            vk::SubmitInfo submit_info{};
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers = &cmd;
            (*submit_queue)().submit({submit_info}, {nullptr});
            (*submit_queue)().waitIdle();

            instance->device.freeCommandBuffers(instance->command_pool, {cmd});
        }

        vk::CommandBuffer& handle(){
            return cmd;
        }

        private:
        Instance* instance;
        Instance::Queue* submit_queue;
        vk::CommandBuffer cmd;
    };

    struct CommandBufferLabel {
        CommandBufferLabel(Instance* instance, vk::CommandBuffer buffer, const char* label, glm::vec3 colour): instance{instance}, buffer{buffer}, label{label}, colour{colour} {}

        void start(){
            if constexpr (enable_validation){
                vk::DebugUtilsLabelEXT label_info{};
                label_info.pLabelName = label;
                label_info.color[0] = colour.x;
                label_info.color[1] = colour.y;
                label_info.color[2] = colour.z;
                label_info.color[3] = 1.0f;

                extra_api::CmdBeginDebugUtilsLabelEXT(instance->device, buffer, &label_info);
            }
        }

        void stop(){
            if constexpr (enable_validation) {
                extra_api::CmdEndDebugUtilsLabelEXT(instance->device, buffer);
            }
        }

        void lock(){
            start();
        }

        void unlock(){
            stop();
        }

        private:
        Instance* instance;
        vk::CommandBuffer buffer;
        const char* label;
        glm::vec3 colour;
    };

} // namespace benzene::vulkan
