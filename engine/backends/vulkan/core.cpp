#include "core.hpp"

using namespace benzene::vulkan;

backend::backend(){
    if(enable_validation && !this->check_validation_layer_support())
        throw std::runtime_error("Wanted to enable validation layer but unsupported");

    if constexpr (debug) {
        auto supported_validation_layers = vk::enumerateInstanceLayerProperties();
        print("vulkan: Supported validation layers: \n");
        for(const auto& layer : supported_validation_layers)
            print("\t - {}: {}, [Implementation version: {}, Spec version: {}]\n", layer.layerName, layer.description, layer.implementationVersion, spec_version{layer.specVersion});
    }

    auto extensions = vk::enumerateInstanceExtensionProperties();
    if constexpr (debug) {
        print("vulkan: Supported extensions: \n");
        for(const auto& extension : extensions)
            print("\t - {} v{}\n", extension.extensionName, spec_version{extension.specVersion});
    }

    vk::ApplicationInfo info{};
    info.pApplicationName = "Application";
    info.setApplicationVersion(VK_MAKE_VERSION(1, 0, 0));
    info.pEngineName = "Benzene";
    info.setEngineVersion(VK_MAKE_VERSION(1, 0, 0));
    info.apiVersion = VK_API_VERSION_1_0;

    vk::InstanceCreateInfo create_info{};
    create_info.pApplicationInfo = &info;

    auto needed_extensions = this->get_extensions();
    create_info.enabledExtensionCount = needed_extensions.size();
    create_info.ppEnabledExtensionNames = needed_extensions.data();

    auto instance_debug_info = this->make_debug_messenger_create_info();
    if constexpr (enable_validation){
        create_info.enabledLayerCount = validation_layers.size();
        create_info.ppEnabledLayerNames = validation_layers.data();
        
        create_info.pNext = &instance_debug_info;
    }


    if(vk::createInstance(&create_info, nullptr, &this->instance) != vk::Result::eSuccess)
        throw std::runtime_error("Couldn't make instance");
            
    print("vulkan: Initialized Vulkan with {} extension(s) and {} validation layer(s)\n", create_info.enabledExtensionCount, create_info.enabledLayerCount);

    if constexpr (enable_validation)
        this->init_debug_messenger();
}

backend::~backend(){
    if constexpr (enable_validation)
        extra_api::DestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);

    vkDestroyInstance(this->instance, nullptr);
}

VKAPI_ATTR VkBool32 VKAPI_CALL backend::debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT _severity, VkDebugUtilsMessageTypeFlagsEXT _type, const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data){
    [[maybe_unused]] auto& self = *(backend*)user_data;
    auto severity = (vk::DebugUtilsMessageSeverityFlagBitsEXT)_severity;
    auto type = (vk::DebugUtilsMessageTypeFlagsEXT)_type;

    print("{} about {} for {}\n", vk::to_string(severity), vk::to_string(type), callback_data->pMessage ?: "No message");

    return VK_FALSE;
}

vk::DebugUtilsMessengerCreateInfoEXT backend::make_debug_messenger_create_info(){
    vk::DebugUtilsMessengerCreateInfoEXT create_info{};
    create_info.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose; //  | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo
    create_info.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
    create_info.pfnUserCallback = debug_callback;
    create_info.pUserData = this;

    return create_info;
}

void backend::init_debug_messenger(){
    auto create_info = this->make_debug_messenger_create_info();

    if(extra_api::CreateDebugUtilsMessengerEXT(instance, &create_info, nullptr, &debug_messenger) != vk::Result::eSuccess)
        throw std::runtime_error("Failed to create debug_messenger");
}

std::vector<const char*> backend::get_extensions(){
    uint32_t glfw_extension_count = 0;
    const auto** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    std::vector<const char*> extensions{glfw_extensions, glfw_extensions + glfw_extension_count};

    if constexpr (enable_validation)
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    return extensions;
}

bool backend::check_validation_layer_support(){
    auto supported_layers = vk::enumerateInstanceLayerProperties();
            
    for(const auto* layer : validation_layers){
        bool found = false;
        for(const auto& layer_properties : supported_layers)
            if(strcmp(layer_properties.layerName, layer) == 0)
                found = true;

        if(!found)
            return false;
    }
    return true;
}