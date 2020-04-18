#include "core.hpp"
#include <thread>
#include <set>

using namespace benzene::vulkan;

#pragma region backend

Backend::Backend(const char* application_name, GLFWwindow* window): is_wireframe{false}, current_frame{0} {
    frame_time = 0.0f;
    max_frame_time = 0.0f;
    min_frame_time = 9999.0f;
    last_frame_times = {};
    frame_counter = 0;
    fps = 0;
    framebuffer_resized = false;
    this->instance.window = window;
    if(enable_validation && !this->check_validation_layer_support())
        throw std::runtime_error("Wanted to enable validation layers but they are unsupported");

    if constexpr (debug) {
        auto supported_validation_layers = vk::enumerateInstanceLayerProperties();
        print("vulkan: Supported global validation layers: \n");
        for(const auto& layer : supported_validation_layers)
            print("\t - {}: {}, [Implementation version: {}, Spec version: {}]\n", layer.layerName, layer.description, layer.implementationVersion, spec_version{layer.specVersion});
    }

    auto extensions = vk::enumerateInstanceExtensionProperties();
    if constexpr (debug) {
        print("vulkan: Supported global extensions: \n");
        for(const auto& extension : extensions)
            print("\t - {} v{}\n", extension.extensionName, extension.specVersion);
    }

    vk::ApplicationInfo info{};
    info.pApplicationName = application_name;
    info.setApplicationVersion(VK_MAKE_VERSION(1, 0, 0));
    info.pEngineName = "Benzene";
    info.setEngineVersion(VK_MAKE_VERSION(1, 0, 0));
    info.apiVersion = VK_API_VERSION_1_2;

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

    if(vk::createInstance(&create_info, nullptr, &this->instance.instance) != vk::Result::eSuccess)
        throw std::runtime_error("Couldn't make instance");

    extra_api::init_instance_level(&instance);
            
    print("vulkan: Initialized instance with {} extension(s) and {} validation layer(s)\n", create_info.enabledExtensionCount, create_info.enabledLayerCount);

    if constexpr (enable_validation)
        this->init_debug_messenger();

    if((vk::Result)glfwCreateWindowSurface(this->instance.instance, window, nullptr, (VkSurfaceKHR*)&this->instance.surface) != vk::Result::eSuccess)
        throw std::runtime_error("Couldn't create window surface");

    this->init_physical_device();
    if constexpr (debug){
        auto supported_validation_layers = this->instance.gpu.enumerateDeviceLayerProperties();
        print("vulkan: Supported device validation layers: \n");
        for(const auto& layer : supported_validation_layers)
            print("\t - {}: {}, [Implementation version: {}, Spec version: {}]\n", layer.layerName, layer.description, layer.implementationVersion, spec_version{layer.specVersion});

        auto extensions = this->instance.gpu.enumerateDeviceExtensionProperties();
        print("vulkan: Supported device extensions: \n");
        for(const auto& extension : extensions)
            print("\t - {} v{}\n", extension.extensionName, extension.specVersion);
    }

    this->init_logical_device();

    extra_api::init_device_level(&instance);

    vma::AllocatorCreateInfo allocator_create_info{};
    allocator_create_info.device = this->instance.device;
    allocator_create_info.physicalDevice = this->instance.gpu;

    this->instance.allocator = vma::createAllocator(allocator_create_info);

    vk::CommandPoolCreateInfo pool_info{};
    pool_info.queueFamilyIndex = graphics_queue_id;
    pool_info.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    this->instance.command_pool = this->instance.device.createCommandPool(pool_info);

    this->swapchain = SwapChain{&this->instance, instance.graphics.family, instance.present.family};
    RenderPipeline::Options pipeline_opts{};
    pipeline_opts.shaders.emplace_back(&instance, benzene::read_binary_file("../engine/shaders/fragment.spv"), "main", vk::ShaderStageFlagBits::eFragment);
    pipeline_opts.shaders.emplace_back(&instance, benzene::read_binary_file("../engine/shaders/vertex.spv"), "main", vk::ShaderStageFlagBits::eVertex);
    pipeline_opts.polygon_mode = vk::PolygonMode::eFill;
    this->pipeline = RenderPipeline{&this->instance, &this->swapchain, pipeline_opts};
    instance.add_debug_tag(this->pipeline.get_pipeline(), "Main pipeline");

    if constexpr (enable_wireframe_outline){
        pipeline_opts.polygon_mode = vk::PolygonMode::eLine;
        this->wireframe_pipeline = RenderPipeline{&this->instance, &this->swapchain, pipeline_opts};
        instance.add_debug_tag(this->wireframe_pipeline.get_pipeline(), "Wireframe pipeline");
    }

    for(auto& shader : pipeline_opts.shaders)
        shader.clean();

    this->imgui_renderer = Imgui{&instance, &swapchain, &pipeline.get_render_pass(), this->swapchain.get_images().size()};
    this->texture = Texture{&instance, "../engine/resources/sample_texture.jpg"};

    this->create_renderer();

    this->image_available.resize(max_frames_in_flight);
    this->render_finished.resize(max_frames_in_flight);
    this->in_flight_fences.resize(max_frames_in_flight);
    this->images_in_flight.resize(this->swapchain.get_images().size(), {nullptr});

    vk::SemaphoreCreateInfo semaphore_info{};
    vk::FenceCreateInfo fence_info{};
    fence_info.flags = vk::FenceCreateFlagBits::eSignaled;
    for(size_t i = 0; i < max_frames_in_flight; i++){
        this->image_available[i] = this->instance.device.createSemaphore(semaphore_info);
        this->render_finished[i] = this->instance.device.createSemaphore(semaphore_info);
        this->in_flight_fences[i] = this->instance.device.createFence(fence_info);
    }

    print("vulkan: Initialized Vulkan backend\n");
}

Backend::~Backend(){
    for(auto& [id, model] : internal_models)
        model.clean();

    this->imgui_renderer.clean();
    this->cleanup_renderer();

    this->texture.clean();

    this->pipeline.clean();
    if constexpr (enable_wireframe_outline)
        this->wireframe_pipeline.clean();

    for(size_t i = 0; i < max_frames_in_flight; i++){
        this->instance.device.destroySemaphore(this->render_finished[i]);
        this->instance.device.destroySemaphore(this->image_available[i]);
        this->instance.device.destroyFence(this->in_flight_fences[i]);
    }

    this->instance.device.destroyCommandPool(this->instance.command_pool);

    this->instance.allocator.destroy();
    
    this->instance.device.destroy();
    if constexpr (enable_validation)
        this->instance.instance.destroyDebugUtilsMessengerEXT(debug_messenger);
    this->instance.instance.destroySurfaceKHR(this->instance.surface);
    this->instance.instance.destroy();
}

void Backend::frame_update(std::unordered_map<ModelId, Model*>& models){
    auto time_begin = std::chrono::high_resolution_clock::now();

    // First things first, create state of models that the backend understands
    for(auto& [id, model] : models){
        auto it = internal_models.find(id);
        if(it != internal_models.end())
            continue; // Already exists
        
        BackendModel item{};
        item.model = model;

        std::vector<Vertex> internal_vertices{};
        for(auto& vertex : model->mesh.vertices){
            auto& item = internal_vertices.emplace_back();
            item.pos = vertex.pos;
            item.colour = vertex.colour;
            item.tex_coord = vertex.tex_coord;
        }

        item.vertices = VertexBuffer{&this->instance, internal_vertices, vk::BufferUsageFlagBits::eVertexBuffer};
        item.indices = IndexBuffer{&this->instance, model->mesh.indices, vk::BufferUsageFlagBits::eIndexBuffer};
        item.pipeline = &pipeline; // TODO: Have the user control this based on shaders
        item.wireframe_pipeline = &wireframe_pipeline;

        internal_models[id] = std::move(item);
    }

    this->instance.device.waitForFences({this->in_flight_fences[this->current_frame]}, true, UINT64_MAX);
    vk::ResultValue<uint32_t> image_index{vk::Result::eSuccess, 0};
    try {
        image_index = this->instance.device.acquireNextImageKHR(this->swapchain.handle(), UINT64_MAX, image_available[this->current_frame], {nullptr});
    } catch(vk::OutOfDateKHRError& error) { 
        this->recreate_renderer();
        return;
    }
    if(image_index.result != vk::Result::eSuccess && image_index.result != vk::Result::eSuboptimalKHR)
        throw std::runtime_error("Failed to acquire swapchain image");

    
    
    if(images_in_flight[image_index.value] != vk::Fence{nullptr})
        this->instance.device.waitForFences({this->images_in_flight[image_index.value]}, true, UINT64_MAX);

    images_in_flight[image_index.value] = this->in_flight_fences[this->current_frame];

    auto update_uniform_buffer = [this, i = image_index.value](){
        ubos[i].map();

        auto& ubo = *(UniformBufferObject*)ubos[i].data();

        ubo.view = glm::lookAt(glm::vec3{2.0f, 2.0f, 2.0f}, glm::vec3{0.0f, 0.0f, 0.0f}, glm::vec3{0.0f, 1.0f, 0.0f});
        ubo.proj = glm::perspective(glm::radians(45.0f), this->swapchain.get_extent().width / (float)this->swapchain.get_extent().height, 0.1f, 10.0f);
        ubo.proj[1][1] *= -1; // GL compat

        ubos[i].unmap();
    };
    update_uniform_buffer();

    imgui_renderer.update_buffers(image_index.value);
    this->build_command_buffer(image_index.value);

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

    this->instance.device.resetFences({this->in_flight_fences[this->current_frame]});
    this->instance.graphics().submit({submit_info}, this->in_flight_fences[this->current_frame]);


    vk::PresentInfoKHR present_info{};
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;
    
    vk::SwapchainKHR swapchains{this->swapchain.handle()};
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchains;
    present_info.pImageIndices = &image_index.value;
    present_info.pResults = nullptr;

    vk::Result result = vk::Result::eSuccess;
    try {
        result = this->instance.present().presentKHR(present_info);
    } catch(vk::OutOfDateKHRError& error){
        this->recreate_renderer();
    }

    if(result == vk::Result::eSuboptimalKHR || framebuffer_resized){
        framebuffer_resized = false;
        this->recreate_renderer();
    } else if(result != vk::Result::eSuccess)
        throw std::runtime_error("Failed to acquire swapchain image");

    this->current_frame = (this->current_frame + 1) % max_frames_in_flight;
    this->frame_counter++;

    auto time_end = std::chrono::high_resolution_clock::now();
    frame_time = (float)(std::chrono::duration<double, std::milli>(time_end - time_begin).count());

    auto fps_timer = (float)std::chrono::duration<double, std::milli>(time_end - last_frame_timestamp).count();
    if(fps_timer > 1000.0f){
        fps = (uint32_t)((float)frame_counter * (1000.0f / fps_timer));
        frame_counter = 0;
        last_frame_timestamp = time_end;
    }

    if(fps_cap_enabled)
        std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(std::chrono::duration<double, std::milli>(1000 / this->fps_cap) - (time_end - time_begin)));
}

void Backend::end_run(){
    this->instance.device.waitIdle();
}

VKAPI_ATTR VkBool32 VKAPI_CALL Backend::debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT _severity, VkDebugUtilsMessageTypeFlagsEXT _type, const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data){
    [[maybe_unused]] auto& self = *(Backend*)user_data;
    auto severity = (vk::DebugUtilsMessageSeverityFlagBitsEXT)_severity;
    auto type = (vk::DebugUtilsMessageTypeFlagsEXT)_type;

    print("{} about {} for {}\n", vk::to_string(severity), vk::to_string(type), callback_data->pMessage ?: "No message");

    return false;
}

vk::DebugUtilsMessengerCreateInfoEXT Backend::make_debug_messenger_create_info(){
    vk::DebugUtilsMessengerCreateInfoEXT create_info{};
    create_info.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose; //  | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo
    create_info.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
    create_info.pfnUserCallback = debug_callback;
    create_info.pUserData = this;

    return create_info;
}

void Backend::init_debug_messenger(){
    auto create_info = this->make_debug_messenger_create_info();

    this->debug_messenger = this->instance.instance.createDebugUtilsMessengerEXT(create_info);
        throw std::runtime_error("Failed to create debug_messenger");
}

void Backend::init_logical_device(){
    auto graphics_queue_id = this->get_queue_index(this->instance.gpu, [](const auto& family, [[maybe_unused]] const auto i){
        return family.queueFlags & vk::QueueFlagBits::eGraphics;
    });
    if(!graphics_queue_id)
        throw std::runtime_error("Didn't find graphics queue id");
    this->graphics_queue_id = *graphics_queue_id;

    auto presentation_queue_id = this->get_queue_index(this->instance.gpu, [this]([[maybe_unused]] const auto& family, const auto i){
        return this->instance.gpu.getSurfaceSupportKHR(i, this->instance.surface);
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
    if constexpr(enable_wireframe_outline)
        device_features.fillModeNonSolid = true;

    device_features.samplerAnisotropy = true;
    
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

    this->instance.device = this->instance.gpu.createDevice(create_info);
    this->instance.graphics = {this->graphics_queue_id, this->instance.device.getQueue(this->graphics_queue_id, 0)}; // We only have 1 graphics queue
    this->instance.present = {this->graphics_queue_id, this->instance.device.getQueue(this->presentation_queue_id, 0)}; // We only have 1 graphics queue

    print("vulkan: Initialized device with {} extension(s) and {} validation layer(s)\n", create_info.enabledExtensionCount, create_info.enabledLayerCount);
}

std::vector<const char*> Backend::get_extensions(){
    uint32_t glfw_extension_count = 0;
    const auto** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    std::vector<const char*> extensions{glfw_extensions, glfw_extensions + glfw_extension_count};

    if constexpr (enable_validation)
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    return extensions;
}

bool Backend::check_validation_layer_support(){
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

void Backend::init_physical_device(){
    std::vector<vk::PhysicalDevice> physical_devices = this->instance.instance.enumeratePhysicalDevices();
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
                return dev.getSurfaceSupportKHR(i, this->instance.surface);
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

            if constexpr(enable_wireframe_outline)
                if(!dev.getFeatures().fillModeNonSolid) // When outline is enabled we need fillModeNonSolid
                    return false;
                
            if(!dev.getFeatures().samplerAnisotropy)
                return false;

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
            print("\t - Type: {}, Name: {} [DeviceID: {:#x}; VendorID: {:#x}; Driver version: {:x}] -> Score {}\n", vk::to_string(gpu.getProperties().deviceType), gpu.getProperties().deviceName, gpu.getProperties().deviceID, gpu.getProperties().vendorID, gpu.getProperties().driverVersion, calculate_score(gpu));
    }

    this->instance.gpu = suitable_devices[0]; // Higest rated device
}

void Backend::cleanup_renderer(){
    this->depth_image_view.clean();
    this->depth_image.clean();

    this->colour_image_view.clean();
    this->colour_image.clean();

    for(auto& ubo : ubos)
        ubo.clean();

    this->instance.device.destroyDescriptorPool(descriptor_pool);

    for(auto& fb : framebuffers)
        this->instance.device.destroyFramebuffer(fb);

    this->instance.device.freeCommandBuffers(this->instance.command_pool, this->command_buffers);

    this->swapchain.clean();
}

void Backend::recreate_renderer(){
    int width = 0, height = 0;
    glfwGetFramebufferSize(this->instance.window, &width, &height);
    while(!width || !height){
        glfwGetFramebufferSize(this->instance.window, &width, &height);
        glfwWaitEvents();
    }

    this->instance.device.waitIdle();
    this->cleanup_renderer();

    this->swapchain = SwapChain{&this->instance, this->graphics_queue_id, this->presentation_queue_id};
    ImGui::GetIO().DisplaySize = ImVec2{(float)swapchain.get_extent().width, (float)swapchain.get_extent().height};
    // Pipeline does not have to be recreated since its modified state(Viewport / Scissor) is dynamic
    this->create_renderer();
}

void Backend::create_renderer(){
    this->ubos.clear();
    for(size_t i = 0; i < this->swapchain.get_images().size(); i++)
        this->ubos.emplace_back(&instance, sizeof(UniformBufferObject), vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    
    std::array<vk::DescriptorPoolSize, 2> pool_sizes{};
    pool_sizes[0].type = vk::DescriptorType::eUniformBuffer;
    pool_sizes[0].descriptorCount = swapchain.get_images().size();

    pool_sizes[1].type = vk::DescriptorType::eCombinedImageSampler;
    pool_sizes[1].descriptorCount = swapchain.get_images().size();
    
    vk::DescriptorPoolCreateInfo pool_info{};
    pool_info.poolSizeCount = pool_sizes.size();
    pool_info.pPoolSizes = pool_sizes.data();
    pool_info.maxSets = swapchain.get_images().size();

    descriptor_pool = instance.device.createDescriptorPool(pool_info);

    std::vector<vk::DescriptorSetLayout> layouts{};
    layouts.resize(swapchain.get_images().size(), this->pipeline.get_descriptor_set_layout());

    vk::DescriptorSetAllocateInfo set_alloc_info{};
    set_alloc_info.descriptorPool = descriptor_pool;
    set_alloc_info.descriptorSetCount = layouts.size();
    set_alloc_info.pSetLayouts = layouts.data();

    descriptor_sets = instance.device.allocateDescriptorSets(set_alloc_info);

    for(size_t i = 0; i < swapchain.get_images().size(); i++){
        vk::DescriptorBufferInfo buf_info{};
        buf_info.buffer = ubos[i].handle();
        buf_info.offset = 0;
        buf_info.range = sizeof(UniformBufferObject);

        vk::DescriptorImageInfo image_info{};
        image_info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        image_info.imageView = texture.get_view();
        image_info.sampler = texture.get_sampler();

        std::array<vk::WriteDescriptorSet, 2> descriptor_writes = {};
        descriptor_writes[0].dstSet = descriptor_sets[i];
        descriptor_writes[0].dstBinding = 0;
        descriptor_writes[0].dstArrayElement = 0;
        descriptor_writes[0].descriptorType = vk::DescriptorType::eUniformBuffer;
        descriptor_writes[0].descriptorCount = 1;
        descriptor_writes[0].pBufferInfo = &buf_info;

        descriptor_writes[1].dstSet = descriptor_sets[i];
        descriptor_writes[1].dstBinding = 1;
        descriptor_writes[1].dstArrayElement = 0;
        descriptor_writes[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;
        descriptor_writes[1].descriptorCount = 1;
        descriptor_writes[1].pImageInfo = &image_info;

        instance.device.updateDescriptorSets(descriptor_writes, {});
    }

    this->depth_image = Image{&instance, swapchain.get_extent().width, swapchain.get_extent().height, instance.find_depth_format(), instance.find_max_msaa_samples(), vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal};
    this->depth_image_view = ImageView{&instance, this->depth_image, instance.find_depth_format(), vk::ImageAspectFlagBits::eDepth};

    this->colour_image = Image{&instance, swapchain.get_extent().width, swapchain.get_extent().height, swapchain.get_format(), instance.find_max_msaa_samples(), vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal};
    this->colour_image_view = ImageView{&instance, this->colour_image, swapchain.get_format(), vk::ImageAspectFlagBits::eColor};

    this->framebuffers.clear();
    for(size_t i = 0; i < this->swapchain.get_image_views().size(); i++){
        std::array<vk::ImageView, 3> attachments = {
            colour_image_view.handle(),
            depth_image_view.handle(),
            this->swapchain.get_image_views()[i]
        };
        
        vk::FramebufferCreateInfo framebuffer_create_info{};
        framebuffer_create_info.renderPass = this->pipeline.get_render_pass().handle();
        framebuffer_create_info.attachmentCount = attachments.size();
        framebuffer_create_info.pAttachments = attachments.data();
        framebuffer_create_info.width = this->swapchain.get_extent().width;
        framebuffer_create_info.height = this->swapchain.get_extent().height;
        framebuffer_create_info.layers = 1;

        framebuffers.push_back(this->instance.device.createFramebuffer(framebuffer_create_info));
    }
    
    vk::CommandBufferAllocateInfo allocate_info{};
    allocate_info.commandPool = this->instance.command_pool;
    allocate_info.level = vk::CommandBufferLevel::ePrimary;
    allocate_info.commandBufferCount = this->framebuffers.size();

    this->command_buffers = this->instance.device.allocateCommandBuffers(allocate_info);
}

void Backend::build_command_buffer(size_t i){
    auto& cmd = this->command_buffers[i];
    vk::CommandBufferBeginInfo begin_info{};
    cmd.begin(begin_info);

    vk::RenderPassBeginInfo render_info{};
    render_info.renderPass = this->pipeline.get_render_pass().handle();
    render_info.framebuffer = this->framebuffers[i];
    render_info.renderArea.offset = vk::Offset2D{0, 0};
    render_info.renderArea.extent = this->swapchain.get_extent();
        
    vk::ClearColorValue clear_colour{std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}};
    vk::ClearDepthStencilValue depth_colour{1.0f, 0};
    std::array<vk::ClearValue, 2> clear_values{};
    clear_values[0].setColor(clear_colour);
    clear_values[1].setDepthStencil(depth_colour);
    render_info.clearValueCount = clear_values.size();
    render_info.pClearValues = clear_values.data();

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

    auto draw_model = [this, &cmd](BackendModel& model){
        PushConstants pc{};
        pc.model = glm::translate(glm::mat4{1.0f}, model.model->pos);
        pc.model = glm::rotate(pc.model, glm::radians(model.model->rotation.x), glm::vec3{1.0f, 0.0f, 0.0f});
        pc.model = glm::rotate(pc.model, glm::radians(model.model->rotation.y), glm::vec3{0.0f, 1.0f, 0.0f});
        pc.model = glm::rotate(pc.model, glm::radians(model.model->rotation.z), glm::vec3{0.0f, 0.0f, 1.0f});
        pc.model = glm::scale(pc.model, model.model->scale);

        cmd.pushConstants<PushConstants>(this->pipeline.get_layout(), vk::ShaderStageFlagBits::eVertex, 0, {pc});
        cmd.drawIndexed(model.indices.size(), 1, 0, 0, 0);
    };

    cmd.setViewport(0, {viewport});
    cmd.setScissor(0, {scissor});
    cmd.beginRenderPass(render_info, vk::SubpassContents::eInline);

    {
        CommandBufferLabel label{&instance, cmd, "Main pass", glm::vec3{0.4f, 0.61f, 0.27f}};
        std::lock_guard label_gaurd{label};

        for(auto& [id, model] : internal_models){
            if constexpr (enable_wireframe_outline)
                cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, this->is_wireframe ? model.wireframe_pipeline->get_pipeline() : model.pipeline->get_pipeline());
            else
                cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, model.pipeline->get_pipeline());
            cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, model.pipeline->get_layout(), 0, {descriptor_sets[i]}, {});
        
            cmd.bindVertexBuffers(0, {model.vertices.handle()}, {0});
            cmd.bindIndexBuffer(model.indices.handle(), 0, vk::IndexType::eUint16);

            draw_model(model);
        }
    }

    {
        CommandBufferLabel label{&instance, cmd, "ImGui pass", glm::vec3{0.4f, 0.61f, 0.27f}};
        std::lock_guard label_gaurd{label};

        imgui_renderer.draw_frame(i, cmd);
    }

    cmd.endRenderPass();
    cmd.end();
}

void Backend::draw_debug_window(){
    ImGui::SetNextWindowSize(ImVec2{200, 200}, ImGuiCond_FirstUseEver);
    ImGui::Begin("Benzene");
    auto name = format_to_str("Device: {:s} ({:s}) [{:#x}:{:#x}]", instance.gpu.getProperties().deviceName, vk::to_string(instance.gpu.getProperties().deviceType), instance.gpu.getProperties().vendorID, instance.gpu.getProperties().deviceID);
    ImGui::TextUnformatted(name.c_str());

    auto chain = instance.gpu.getProperties2<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceDriverProperties>();
    [[maybe_unused]] auto& prop2 = chain.get<vk::PhysicalDeviceProperties2>();
    auto& driver = chain.get<vk::PhysicalDeviceDriverProperties>();

    name = format_to_str("Driver name: {:s}", strlen(driver.driverName) != 0 ? driver.driverName : "Unknown");
    ImGui::TextUnformatted(name.c_str());

    name = format_to_str("Driver info: {:s}", strlen(driver.driverInfo) != 0 ? driver.driverInfo : "Unknown");
    ImGui::TextUnformatted(name.c_str());

    if constexpr (enable_wireframe_outline)
        ImGui::Checkbox("Wireframe", &this->is_wireframe);

    std::rotate(last_frame_times.begin(), last_frame_times.begin() + 1, last_frame_times.end());
    last_frame_times.back() = this->frame_time;

    if(frame_time < min_frame_time)
        min_frame_time = frame_time;

    if(frame_time > max_frame_time)
        max_frame_time = frame_time;

    ImGui::PlotLines("Frame times (ms)", last_frame_times.data(), last_frame_times.size(), 0, "", min_frame_time, max_frame_time, ImVec2{0, 80});
    auto fps_str = format_to_str("FPS: {:d}", fps);
    ImGui::TextUnformatted(fps_str.c_str());

    ImGui::End();
}

#pragma endregion
