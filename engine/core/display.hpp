#pragma once

#include <benzene/benzene.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace benzene
{
    class Display {
        public:
        static Display& instance(){
            static Display display;
            return display;
        }

        void set_hint(int hint, int value) {
            glfwWindowHint(hint, value);
        }

        void create_window(const std::string& title, size_t width, size_t height){
            this->window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
            assert(this->window);
            this->width = width;
            this->height = height;

            glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height){
                auto& self = *(Display*)glfwGetWindowUserPointer(window);
                self.backend->framebuffer_resize_callback(width, height);

                self.width = width;
                self.height = height;
            });

            glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, [[maybe_unused]] int mods){
                auto& self = *(Display*)glfwGetWindowUserPointer(window);
                self.backend->mouse_button_callback(button, action);
            });

            glfwSetCursorPosCallback(window, [](GLFWwindow* window, double x, double y){
                auto& self = *(Display*)glfwGetWindowUserPointer(window);
                self.backend->mouse_pos_callback(x, y);
            });

            glfwSetScrollCallback(window, [](GLFWwindow* window, double xoffset, double yoffset){
                auto& self = *(Display*)glfwGetWindowUserPointer(window);
                self.backend->mouse_scroll_callback(xoffset, yoffset);
            });

            glfwSetWindowUserPointer(window, this);
        }

        void clean(){
            glfwDestroyWindow(window);
        }

        GLFWwindow* operator()(){
            return window;
        }

        void set_window_backend(IBackend* backend){
            this->backend = backend;
        }
        
        void make_context_current() const {
            glfwMakeContextCurrent(window);
        }

        void swap_buffers() const {
            glfwSwapBuffers(window);
        }

        size_t get_width() const { return this->width; }
        size_t get_height() const { return this->height; }
        
        Display(Display const&) = delete;
        void operator=(Display const&) = delete;
        private:
        Display() {}

        size_t width, height;
        GLFWwindow* window;
        IBackend* backend;
    };
} // namespace benzene
