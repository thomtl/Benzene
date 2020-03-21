#pragma once

#include <vulkan/vulkan.hpp>

#include "../../core/format.hpp"

namespace benzene::vulkan
{
    class shader {
        public:
        shader(vk::Device device, std::vector<std::byte> spirv_code): device{device} {
            vk::ShaderModuleCreateInfo create_info{};
            create_info.codeSize = spirv_code.size();
            create_info.pCode = (const uint32_t*)spirv_code.data();

            this->shader_module = this->device.createShaderModule(create_info);
        }

        void clean(){
            this->device.destroyShaderModule(this->shader_module);
        }

        vk::ShaderModule& handle(){
            return this->shader_module;
        }

        private:
        vk::Device device;
        vk::ShaderModule shader_module;
    };
} // namespace benzene::vulkan
