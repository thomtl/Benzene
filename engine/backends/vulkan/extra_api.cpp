#include "extra_api.hpp"

static PFN_vkCmdPushDescriptorSetKHR _vkCmdPushDescriptorSetKHR;
extern "C" void vkCmdPushDescriptorSetKHR(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t set, uint32_t descriptorWriteCount, const VkWriteDescriptorSet* pDescriptorWrites){
    _vkCmdPushDescriptorSetKHR(commandBuffer, pipelineBindPoint, layout, set, descriptorWriteCount, pDescriptorWrites);
}

#pragma region VK_EXT_debug_utils

static PFN_vkSetDebugUtilsObjectNameEXT _vkSetDebugUtilsObjectNameEXT;
extern "C" VkResult vkSetDebugUtilsObjectNameEXT(VkDevice device, const VkDebugUtilsObjectNameInfoEXT* pNameInfo){
    return _vkSetDebugUtilsObjectNameEXT(device, pNameInfo);
}

static PFN_vkCmdBeginDebugUtilsLabelEXT _vkCmdBeginDebugUtilsLabelEXT;
extern "C" void vkCmdBeginDebugUtilsLabelEXT(VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT* pLabelInfo){
    return _vkCmdBeginDebugUtilsLabelEXT(commandBuffer, pLabelInfo);
}

static PFN_vkCmdEndDebugUtilsLabelEXT _vkCmdEndDebugUtilsLabelEXT;
extern "C" void vkCmdEndDebugUtilsLabelEXT(VkCommandBuffer commandBuffer){
    _vkCmdEndDebugUtilsLabelEXT(commandBuffer);
}

static PFN_vkCreateDebugUtilsMessengerEXT _vkCreateDebugUtilsMessengerEXT;
extern "C" VkResult vkCreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pMessenger){
    return _vkCreateDebugUtilsMessengerEXT(instance, pCreateInfo, pAllocator, pMessenger);
}

static PFN_vkDestroyDebugUtilsMessengerEXT _vkDestroyDebugUtilsMessengerEXT;
extern "C" void vkDestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks* pAllocator){
    return _vkDestroyDebugUtilsMessengerEXT(instance, messenger, pAllocator);
}

#pragma endregion

void benzene::vulkan::extra_api::init_instance_level(benzene::vulkan::Instance* instance){
    _vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)instance->instance.getProcAddr("vkSetDebugUtilsObjectNameEXT");
    _vkCmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)instance->instance.getProcAddr("vkCmdBeginDebugUtilsLabelEXT");
    _vkCmdEndDebugUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT)instance->instance.getProcAddr("vkCmdEndDebugUtilsLabelEXT");

    _vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)instance->instance.getProcAddr("vkCreateDebugUtilsMessengerEXT");
    _vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)instance->instance.getProcAddr("vkDestroyDebugUtilsMessengerEXT");
}

void benzene::vulkan::extra_api::init_device_level(benzene::vulkan::Instance* instance){
    _vkCmdPushDescriptorSetKHR = (PFN_vkCmdPushDescriptorSetKHR)instance->device.getProcAddr("vkCmdPushDescriptorSetKHR");
}