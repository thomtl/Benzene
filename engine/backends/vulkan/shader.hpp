#pragma once

#include "base.hpp"

namespace benzene::vulkan
{
    class Shader {
        public:
        Shader(Instance* instance, std::vector<std::byte> spirv_code, const std::string& entry, vk::ShaderStageFlagBits stage): instance{instance}, stage{stage}, entry_point{entry} {
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

        vk::PipelineShaderStageCreateInfo get_stage_create_info(){
            vk::PipelineShaderStageCreateInfo info{};
            info.stage = stage;
            info.module = shader_module;
            info.pName = entry_point.c_str();

            return info;
        }

        private:
        Instance* instance;
        vk::ShaderStageFlagBits stage;
        const std::string entry_point;
        vk::ShaderModule shader_module;
    };
} // namespace benzene::vulkan
