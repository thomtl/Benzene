#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <functional>
#include <memory>

namespace benzene
{
    class IBackend {
        public:
        virtual ~IBackend() {}
    };

    class instance {
        public:
        instance(size_t width, size_t height);
        ~instance();

        void run(std::function<void(void)> functor);

        private:
        GLFWwindow* window;
        std::unique_ptr<IBackend> backend; 
        size_t width, height;
    };
} // namespace benzene
