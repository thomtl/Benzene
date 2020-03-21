#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <functional>
#include <memory>
#include <string_view>


namespace benzene
{
    class IBackend {
        public:
        virtual ~IBackend() {}
        virtual void frame_update() = 0;
        virtual void end_run() = 0;
    };

    class instance {
        public:
        instance(const char* application_name, size_t width, size_t height);
        ~instance();

        void run(std::function<void(void)> functor);

        private:
        GLFWwindow* window;
        std::unique_ptr<IBackend> backend; 
        size_t width, height;
    };
} // namespace benzene
