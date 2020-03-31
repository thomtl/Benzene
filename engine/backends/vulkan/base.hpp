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

namespace benzene::vulkan
{
    struct Instance {
        GLFWwindow* window;
        vk::Instance instance;
        vk::PhysicalDevice gpu;
        vk::Device device;
        vk::CommandPool command_pool;
        vk::SurfaceKHR surface;
        vma::Allocator allocator;

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

    constexpr bool enable_validation = true;
    constexpr bool debug = true;

    #define ENABLE_OUTLINE true // Define needed since we want to use it in #ifdef
    constexpr bool enable_outline = ENABLE_OUTLINE;
    constexpr size_t max_frames_in_flight = 16;
} // namespace benzene::vulkan
