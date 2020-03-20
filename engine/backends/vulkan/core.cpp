#include "core.hpp"

using namespace benzene::vulkan;

#pragma region backend

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

    auto phys = this->choose_physical_device();
    this->device = logical_device{phys};
}

backend::~backend(){
    this->device.~logical_device();
    if constexpr (enable_validation)
        extra_api::DestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);

    this->instance.destroy();
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

physical_device backend::choose_physical_device(){
    std::vector<vk::PhysicalDevice> physical_devices = this->instance.enumeratePhysicalDevices();
    if(physical_devices.size() == 0)
        throw std::runtime_error("Couldn't find any GPUs with vulkan support");
    
    std::vector<physical_device> devices;
    for(const auto& dev: physical_devices){
        physical_device device{dev};

        if(device.suitability)
            devices.push_back(std::move(device)); // Make sure only suitable devices are pushed
    }

    std::sort(devices.begin(), devices.end(), [](auto& a, auto& b){
        return a.score < b.score;
    });

    if constexpr (debug) {
        print("vulkan: Physical GPUs: \n");
        for(const auto& gpu : devices)
            print("\t - Type: {}, Name: {} [DeviceID: {:#x}; VendorID: {:#x}; Driver version: {}] -> Score {}\n", vk::to_string(gpu.handle().getProperties().deviceType), gpu.handle().getProperties().deviceName, gpu.handle().getProperties().deviceID, gpu.handle().getProperties().vendorID, spec_version{gpu.handle().getProperties().driverVersion}, gpu.score);
    }

    return devices[0]; // Higest rated device
}

#pragma endregion

#pragma region physical_device

physical_device::physical_device(vk::PhysicalDevice device): score{-1}, suitability{false}, device{device}{
    this->score = this->calculate_score();
    this->queue_families = this->device.getQueueFamilyProperties();
    this->suitability = this->is_suitable();
}

int64_t physical_device::calculate_score(){
    const auto& properties = this->device.getProperties();
    const auto& features = this->device.getFeatures();

    int64_t score = 0;
        
    switch(properties.deviceType){
        case vk::PhysicalDeviceType::eDiscreteGpu:
            score += 1000;
            break;
        case vk::PhysicalDeviceType::eIntegratedGpu:
            score += 500;
            break;
        default:
            break;
    }

    score += properties.limits.maxImageDimension2D;

    if(!features.geometryShader)
        score = -1; // Invalid

    return score;
}

bool physical_device::is_suitable(){
    if(!this->get_queue_index(vk::QueueFlagBits::eGraphics).has_value())
        return false;

    return true;
}

std::optional<uint32_t> physical_device::get_queue_index(vk::QueueFlagBits flag){
    for(uint32_t i = 0; i < this->queue_families.size(); i++)
        if(this->queue_families[i].queueFlags & flag)
            return i;

    return {};
}

#pragma endregion

#pragma region logical_device

logical_device::logical_device(physical_device& physical_dev) {
    auto graphics_queue_id = physical_dev.get_queue_index(vk::QueueFlagBits::eGraphics);
    if(!graphics_queue_id)
        throw std::runtime_error("Didn't find graphics queue id");

    vk::DeviceQueueCreateInfo queue_create_info{};
    queue_create_info.queueFamilyIndex = *graphics_queue_id;
    queue_create_info.queueCount = 1;
    float priority = 1.0f;
    queue_create_info.pQueuePriorities = &priority;

    vk::PhysicalDeviceFeatures device_features{};
    
    vk::DeviceCreateInfo create_info{};
    create_info.pQueueCreateInfos = &queue_create_info;
    create_info.queueCreateInfoCount = 1;

    create_info.pEnabledFeatures = &device_features;

    if constexpr (enable_validation) {
        create_info.enabledLayerCount = validation_layers.size();
        create_info.ppEnabledLayerNames = validation_layers.data();
    }

    this->device = physical_dev.handle().createDevice(create_info);

    this->graphics_queue = this->device.getQueue(*graphics_queue_id, 0); // We only have 1 graphics queue
}

logical_device::~logical_device(){
    this->device.destroy();
    this->device = vk::Device{nullptr}; // Assign faux state so it won't segfault when called for the second time
}

#pragma endregion