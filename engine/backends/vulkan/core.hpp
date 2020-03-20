#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>

#include <stdexcept>
#include <vector>
#include <array>

#include <benzene/benzene.hpp>
#include "../../core/format.hpp"
#include "extra_api.hpp"
namespace benzene::vulkan
{
    struct spec_version {
        spec_version(uint32_t v): major{VK_VERSION_MAJOR(v)}, minor{VK_VERSION_MINOR(v)}, patch{VK_VERSION_PATCH(v)} {}
        uint16_t major, minor, patch;
    };
}

template<>
struct format::formatter<benzene::vulkan::spec_version> {
	template<typename OutputIt>
	static void format(format::format_output_it<OutputIt>& it, [[maybe_unused]] format::format_args args, benzene::vulkan::spec_version item){        
		formatter<uintptr_t>::format(it, {}, item.major);
        it.write('.');
        formatter<uintptr_t>::format(it, {}, item.minor);
        it.write('.');
        formatter<uintptr_t>::format(it, {}, item.patch);
    }
};

namespace benzene::vulkan {
    constexpr std::array<const char*, 1> validation_layers = {
        "VK_LAYER_KHRONOS_validation"
    };
    constexpr bool enable_validation = true;
    constexpr bool debug = true;

    class physical_device {
        public:
        physical_device(vk::PhysicalDevice device);

        std::optional<uint32_t> get_queue_index(vk::QueueFlagBits flag);
        int64_t score;
        bool suitability;

        const vk::PhysicalDevice& handle() const {
            return device;
        }
        private:
        int64_t calculate_score();
        bool is_suitable();
        vk::PhysicalDevice device;
        std::vector<vk::QueueFamilyProperties> queue_families;
    };

    class logical_device {
        public:
        logical_device(): device{nullptr} {};
        logical_device(physical_device& physical_dev);
        ~logical_device();

        logical_device& operator=(logical_device&& other){
            this->device = other.device;
            other.device = vk::Device{nullptr};
            return *this;
        }

        vk::Queue graphics_queue;

        logical_device(logical_device&& other) = delete;
        logical_device& operator=(const logical_device& other) = delete;
        logical_device(const logical_device& other) = delete;
        private:
        vk::Device device;
    };

    class backend : public IBackend {
        public:
        backend();
        ~backend();

        private:
        static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT _severity, VkDebugUtilsMessageTypeFlagsEXT _type, const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data);
        vk::DebugUtilsMessengerCreateInfoEXT make_debug_messenger_create_info();
        void init_debug_messenger();

        std::vector<const char*> get_extensions();

        bool check_validation_layer_support();

        physical_device choose_physical_device();

        logical_device device;
        vk::Instance instance;
        vk::DebugUtilsMessengerEXT debug_messenger;
    };
} // namespace benzene::vulkan
