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