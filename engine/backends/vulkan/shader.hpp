#pragma once

#include "base.hpp"

namespace benzene::vulkan
{
    class Shader {
        public:
        Shader(Instance* instance, std::vector<std::byte> spirv_code): instance{instance} {
            vk::ShaderModuleCreateInfo create_info{};
            create_info.codeSize = spirv_code.size();
            create_info.pCode = (const uint32_t*)spirv_code.data();

            this->shader_module = this->instance->device.createShaderModule(create_info);
        }

        void clean(){
            this->instance->device.destroyShaderModule(this->shader_module);
        }

        vk::ShaderModule& handle(){
            return this->shader_module;
        }

        private:
        Instance* instance;
        vk::ShaderModule shader_module;
    };
} // namespace benzene::vulkan
