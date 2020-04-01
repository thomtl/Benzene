#include "extra_api.hpp"

vk::Result benzene::vulkan::extra_api::CreateDebugUtilsMessengerEXT(vk::Instance instance, const vk::DebugUtilsMessengerCreateInfoEXT* create_info, const vk::AllocationCallbacks* allocator, vk::DebugUtilsMessengerEXT* debug_messenger){
    auto f = (PFN_vkCreateDebugUtilsMessengerEXT)instance.getProcAddr("vkCreateDebugUtilsMessengerEXT");
    if(f)
        return (vk::Result)f((VkInstance)instance, (const VkDebugUtilsMessengerCreateInfoEXT*)create_info, (VkAllocationCallbacks*)allocator, (VkDebugUtilsMessengerEXT*)debug_messenger);
    else
        return vk::Result::eErrorExtensionNotPresent;
}

void benzene::vulkan::extra_api::DestroyDebugUtilsMessengerEXT(vk::Instance instance, vk::DebugUtilsMessengerEXT debug_messenger, vk::AllocationCallbacks* allocator){
    auto f = (PFN_vkDestroyDebugUtilsMessengerEXT)instance.getProcAddr("vkDestroyDebugUtilsMessengerEXT");
    if(f)
        f((VkInstance)instance, (VkDebugUtilsMessengerEXT)debug_messenger, (VkAllocationCallbacks*)allocator);
}

vk::Result benzene::vulkan::extra_api::SetDebugUtilsObjectNameEXT(vk::Device device, const vk::DebugUtilsObjectNameInfoEXT* name_info){
    auto f = (PFN_vkSetDebugUtilsObjectNameEXT)device.getProcAddr("vkSetDebugUtilsObjectNameEXT");
    if(f)
        return (vk::Result)f((VkDevice)device, (const VkDebugUtilsObjectNameInfoEXT*)name_info);

    return vk::Result::eErrorExtensionNotPresent;
}

void benzene::vulkan::extra_api::CmdBeginDebugUtilsLabelEXT(vk::Device dev, vk::CommandBuffer cmd, const vk::DebugUtilsLabelEXT* label){
    auto f = (PFN_vkCmdBeginDebugUtilsLabelEXT)dev.getProcAddr("vkCmdBeginDebugUtilsLabelEXT");
    if(f)
        f((VkCommandBuffer)cmd, (const VkDebugUtilsLabelEXT*)label);
}

void benzene::vulkan::extra_api::CmdEndDebugUtilsLabelEXT(vk::Device dev, vk::CommandBuffer cmd){
    auto f = (PFN_vkCmdEndDebugUtilsLabelEXT)dev.getProcAddr("vkCmdEndDebugUtilsLabelEXT");
    if(f)
        f((VkCommandBuffer)cmd);
}