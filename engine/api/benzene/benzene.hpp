#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "../libs/imgui/imgui.h"

#include <functional>
#include <memory>
#include <string_view>


namespace benzene
{
    class IBackend {
        public:
        virtual ~IBackend() {}
        virtual void framebuffer_resize_callback(int width, int height) = 0;
        virtual void mouse_button_callback(int button, bool state) = 0;
        virtual void mouse_pos_callback(double x, double y) = 0;
        virtual void mouse_scroll_callback(double xoffset, double yoffset) = 0;
        virtual void frame_update() = 0;
        virtual void end_run() = 0;
        virtual void draw_debug_window() = 0;
    };

    struct FrameData {
        bool should_exit;
        IBackend* backend;
    };

    class Instance {
        public:
        Instance(const char* application_name, size_t width, size_t height);
        ~Instance();

        void run(std::function<void(FrameData&)> functor);

        private:
        GLFWwindow* window;
        std::unique_ptr<IBackend> backend; 
        size_t width, height;
    };
} // namespace benzene
