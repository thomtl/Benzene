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

        vk::Instance instance;
        vk::DebugUtilsMessengerEXT debug_messenger;
    };
} // namespace benzene::vulkan
