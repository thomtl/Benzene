#pragma once

#include <vulkan/vulkan.hpp>
#include "base.hpp"

namespace benzene::vulkan::extra_api {
    void init_instance_level(vulkan::Instance* instance);
    void init_device_level(vulkan::Instance* instance);
}