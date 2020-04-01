#pragma once

#include <vulkan/vulkan.hpp>

namespace benzene::vulkan::extra_api {
    vk::Result CreateDebugUtilsMessengerEXT(vk::Instance instance, const vk::DebugUtilsMessengerCreateInfoEXT* create_info, const vk::AllocationCallbacks* p_allocator, vk::DebugUtilsMessengerEXT* debug_messenger);
    void DestroyDebugUtilsMessengerEXT(vk::Instance instance, vk::DebugUtilsMessengerEXT debug_messenger, vk::AllocationCallbacks* allocator);

    vk::Result SetDebugUtilsObjectNameEXT(vk::Device device, const vk::DebugUtilsObjectNameInfoEXT* name_info);
    void CmdBeginDebugUtilsLabelEXT(vk::Device dev, vk::CommandBuffer cmd, const vk::DebugUtilsLabelEXT* label);
    void CmdEndDebugUtilsLabelEXT(vk::Device dev, vk::CommandBuffer cmd);
}