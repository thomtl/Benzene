#include "core.hpp"

#include <set>

using namespace benzene::vulkan;

#pragma region backend

backend::backend(const char* application_name, GLFWwindow* window): window{window}, current_frame{0} {
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
    info.pApplicationName = application_name;
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
            
    print("vulkan: Initialized instance with {} extension(s) and {} validation layer(s)\n", create_info.enabledExtensionCount, create_info.enabledLayerCount);

    if constexpr (enable_validation)
        this->init_debug_messenger();

    if((vk::Result)glfwCreateWindowSurface(this->instance, window, nullptr, (VkSurfaceKHR*)&this->surface) != vk::Result::eSuccess)
        throw std::runtime_error("Couldn't create window surface");

    this->init_physical_device();
    this->init_logical_device();

    vk::CommandPoolCreateInfo pool_info{};
    pool_info.queueFamilyIndex = graphics_queue_id;
    this->command_pool = this->logical_device.createCommandPool(pool_info);

    this->swapchain = swap_chain{&this->logical_device, &this->physical_device, &this->surface, this->graphics_queue_id, this->presentation_queue_id, this->window};
    this->pipeline = render_pipeline{this->logical_device, &this->swapchain};
    this->create_renderer();

    this->image_available.resize(max_frames_in_flight);
    this->render_finished.resize(max_frames_in_flight);
    this->in_flight_fences.resize(max_frames_in_flight);
    this->images_in_flight.resize(this->swapchain.get_images().size(), {nullptr});

    vk::SemaphoreCreateInfo semaphore_info{};
    vk::FenceCreateInfo fence_info{};
    fence_info.flags = vk::FenceCreateFlagBits::eSignaled;
    for(size_t i = 0; i < max_frames_in_flight; i++){
        this->image_available[i] = this->logical_device.createSemaphore(semaphore_info);
        this->render_finished[i] = this->logical_device.createSemaphore(semaphore_info);
        this->in_flight_fences[i] = this->logical_device.createFence(fence_info);
    }

    print("vulkan: Initialized Vulkan backend\n");
}

backend::~backend(){
    this->cleanup_renderer();

    this->pipeline.clean();

    for(size_t i = 0; i < max_frames_in_flight; i++){
        this->logical_device.destroySemaphore(this->render_finished[i]);
        this->logical_device.destroySemaphore(this->image_available[i]);
        this->logical_device.destroyFence(this->in_flight_fences[i]);
    }

    this->logical_device.destroyCommandPool(this->command_pool);

    
    this->logical_device.destroy();
    if constexpr (enable_validation)
        extra_api::DestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);
    this->instance.destroySurfaceKHR(this->surface);
    this->instance.destroy();
}

void backend::frame_update(){
    this->logical_device.waitForFences({this->in_flight_fences[this->current_frame]}, true, UINT64_MAX);
    vk::ResultValue<uint32_t> image_index{vk::Result::eSuccess, 0};
    try {
        image_index = this->logical_device.acquireNextImageKHR(this->swapchain.handle(), UINT64_MAX, image_available[this->current_frame], {nullptr});
    } catch(vk::OutOfDateKHRError& error) { 
        this->recreate_renderer();
        return;
    }
    if(image_index.result != vk::Result::eSuccess && image_index.result != vk::Result::eSuboptimalKHR)
        throw std::runtime_error("Failed to acquire swapchain image");
    
    if(images_in_flight[image_index.value] != vk::Fence{nullptr})
        this->logical_device.waitForFences({this->images_in_flight[image_index.value]}, true, UINT64_MAX);

    images_in_flight[image_index.value] = this->in_flight_fences[this->current_frame];

    vk::SubmitInfo submit_info{};

    vk::Semaphore wait_semaphores[] = {image_available[this->current_frame]};
    vk::PipelineStageFlags wait_stages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};

    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffers[image_index.value];

    vk::Semaphore signal_semaphores[] = {render_finished[this->current_frame]};
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;

    this->logical_device.resetFences({this->in_flight_fences[this->current_frame]});
    this->graphics_queue.submit({submit_info}, this->in_flight_fences[this->current_frame]);


    vk::PresentInfoKHR present_info{};
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;
    
    vk::SwapchainKHR swapchains{this->swapchain.handle()};
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchains;
    present_info.pImageIndices = &image_index.value;
    present_info.pResults = nullptr;

    vk::Result result;
    try {
        result = this->presentation_queue.presentKHR(present_info);
    } catch(vk::OutOfDateKHRError& error){
        this->recreate_renderer();
    }

    if(result == vk::Result::eSuboptimalKHR || framebuffer_resized){
        framebuffer_resized = false;
        this->recreate_renderer();
    } else if(result != vk::Result::eSuccess)
        throw std::runtime_error("Failed to acquire swapchain image");

    this->current_frame = (this->current_frame + 1) % max_frames_in_flight;
}

void backend::end_run(){
    this->logical_device.waitIdle();
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

void backend::init_logical_device(){
    auto graphics_queue_id = this->get_queue_index(this->physical_device, [](const auto& family, [[maybe_unused]] const auto i){
        return family.queueFlags & vk::QueueFlagBits::eGraphics;
    });
    if(!graphics_queue_id)
        throw std::runtime_error("Didn't find graphics queue id");
    this->graphics_queue_id = *graphics_queue_id;

    auto presentation_queue_id = this->get_queue_index(this->physical_device, [this]([[maybe_unused]] const auto& family, const auto i){
        return this->physical_device.getSurfaceSupportKHR(i, this->surface);
    });
    if(!graphics_queue_id)
        throw std::runtime_error("Didn't find graphics queue id");
    this->presentation_queue_id = *presentation_queue_id;

    std::set<uint32_t> unique_queue_ids = {this->graphics_queue_id, this->presentation_queue_id};
    std::vector<vk::DeviceQueueCreateInfo> queue_creation_info{};

    float priority = 1.0f;
    for(const auto& queue_id : unique_queue_ids){
        vk::DeviceQueueCreateInfo queue_create_info{};
        queue_create_info.queueFamilyIndex = queue_id;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &priority;

        queue_creation_info.push_back(queue_create_info);
    }

    vk::PhysicalDeviceFeatures device_features{};
    
    vk::DeviceCreateInfo create_info{};
    create_info.pQueueCreateInfos = queue_creation_info.data();
    create_info.queueCreateInfoCount = queue_creation_info.size();

    create_info.pEnabledFeatures = &device_features;

    create_info.enabledExtensionCount = required_device_extensions.size();
    create_info.ppEnabledExtensionNames = required_device_extensions.data();

    if constexpr (enable_validation) {
        create_info.enabledLayerCount = validation_layers.size();
        create_info.ppEnabledLayerNames = validation_layers.data();
    }

    this->logical_device = this->physical_device.createDevice(create_info);
    this->graphics_queue = this->logical_device.getQueue(this->graphics_queue_id, 0); // We only have 1 graphics queue
    this->presentation_queue = this->logical_device.getQueue(this->presentation_queue_id, 0); // We only have 1 graphics queue
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

void backend::init_physical_device(){
    std::vector<vk::PhysicalDevice> physical_devices = this->instance.enumeratePhysicalDevices();
    if(physical_devices.size() == 0)
        throw std::runtime_error("Couldn't find any GPUs with vulkan support");
    
    std::vector<vk::PhysicalDevice> suitable_devices;
    for(const auto& dev: physical_devices){
        auto is_suitable = [this, dev]() -> bool {
            if(!this->get_queue_index(dev, [](const auto& family, [[maybe_unused]] const auto i){
                return family.queueFlags & vk::QueueFlagBits::eGraphics;
            }).has_value())
                return false; // Make sure we have a graphics queue

            if(!this->get_queue_index(dev, [this, &dev]([[maybe_unused]] const auto& family, const auto i){
                return dev.getSurfaceSupportKHR(i, this->surface);
            }).has_value())
                return false; // Make sure we have a presentation queue

            auto extensions = dev.enumerateDeviceExtensionProperties();
            for(const auto* a : required_device_extensions){
               bool found = false;
                for(const auto& b : extensions)
                    if(strcmp(a, b.extensionName) == 0)
                        found = true;

                if(!found)
                    return false;
            }

            return true;
        };
        
        if(is_suitable())
            suitable_devices.push_back(std::move(dev)); // Make sure only suitable devices are pushed
    }

    if(suitable_devices.size() == 0)
        throw std::runtime_error("Couldn't find any GPUs with suitable vulkan support");

    auto calculate_score = [](const auto& dev) -> int64_t {
        const auto& properties = dev.getProperties();
        const auto& features = dev.getFeatures();

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
    };

    std::sort(suitable_devices.begin(), suitable_devices.end(), [calculate_score](auto& a, auto& b){
        return calculate_score(a) < calculate_score(b);
    });

    if constexpr (debug) {
        print("vulkan: Physical GPUs: \n");
        for(const auto& gpu : suitable_devices)
            print("\t - Type: {}, Name: {} [DeviceID: {:#x}; VendorID: {:#x}; Driver version: {}] -> Score {}\n", vk::to_string(gpu.getProperties().deviceType), gpu.getProperties().deviceName, gpu.getProperties().deviceID, gpu.getProperties().vendorID, spec_version{gpu.getProperties().driverVersion}, calculate_score(gpu));
    }

    this->physical_device = suitable_devices[0]; // Higest rated device
}

void backend::cleanup_renderer(){
    for(auto& fb : framebuffers)
        logical_device.destroyFramebuffer(fb);

    this->logical_device.freeCommandBuffers(this->command_pool, this->command_buffers);

    this->swapchain.clean();
}

void backend::recreate_renderer(){
    int width = 0, height = 0;
    glfwGetFramebufferSize(this->window, &width, &height);
    while(!width || !height){
        glfwGetFramebufferSize(this->window, &width, &height);
        glfwWaitEvents();
    }

    this->logical_device.waitIdle();
    this->cleanup_renderer();

    this->swapchain = swap_chain{&this->logical_device, &this->physical_device, &this->surface, this->graphics_queue_id, this->presentation_queue_id, this->window};
    // Pipeline does not have to be recreated since its modified state is dynamic
    this->create_renderer();
}

void backend::create_renderer(){
    this->framebuffers.clear();
    for(size_t i = 0; i < this->swapchain.get_image_views().size(); i++){
        vk::ImageView attachments[] = {
            this->swapchain.get_image_views()[i]
        };
        
        vk::FramebufferCreateInfo framebuffer_create_info{};
        framebuffer_create_info.renderPass = this->pipeline.get_render_pass().handle();
        framebuffer_create_info.attachmentCount = 1;
        framebuffer_create_info.pAttachments = attachments;
        framebuffer_create_info.width = this->swapchain.get_extent().width;
        framebuffer_create_info.height = this->swapchain.get_extent().height;
        framebuffer_create_info.layers = 1;

        framebuffers.push_back(this->logical_device.createFramebuffer(framebuffer_create_info));
    }
    
    vk::CommandBufferAllocateInfo allocate_info{};
    allocate_info.commandPool = this->command_pool;
    allocate_info.level = vk::CommandBufferLevel::ePrimary;
    allocate_info.commandBufferCount = this->framebuffers.size();

    this->command_buffers = this->logical_device.allocateCommandBuffers(allocate_info);

    for(size_t i = 0; i < this->command_buffers.size(); i++){
        vk::CommandBufferBeginInfo begin_info{};
        
        this->command_buffers[i].begin(begin_info);

        vk::RenderPassBeginInfo render_info{};
        render_info.renderPass = this->pipeline.get_render_pass().handle();
        render_info.framebuffer = this->framebuffers[i];
        render_info.renderArea.offset = vk::Offset2D{0, 0};
        render_info.renderArea.extent = this->swapchain.get_extent();
        
        vk::ClearColorValue clear_colour{std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}};
        vk::ClearValue clear_value{};
        clear_value.setColor(clear_colour);
        render_info.clearValueCount = 1;
        render_info.pClearValues = &clear_value;

        this->command_buffers[i].beginRenderPass(render_info, vk::SubpassContents::eInline);
        this->command_buffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, this->pipeline.get_pipeline());

        vk::Viewport viewport{};
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = (float)swapchain.get_extent().width;
        viewport.height = (float)swapchain.get_extent().height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        vk::Rect2D scissor{};
        scissor.offset = vk::Offset2D{0, 0};
        scissor.extent = this->swapchain.get_extent();

        this->command_buffers[i].setViewport(0, {viewport});
        this->command_buffers[i].setScissor(0, {scissor});

        this->command_buffers[i].draw(3, 1, 0, 0);
        this->command_buffers[i].endRenderPass();
        this->command_buffers[i].end();
    }
}

#pragma endregion
