#pragma once

#include "libs/glad/include/glad/glad.h"
#include <GLFW/glfw3.h>

#include <benzene/benzene.hpp>

#include "model.hpp"
#include "pipeline.hpp"

#include "../../libs/imgui/imgui.h"
#include "libs/imgui/imgui_impl_glfw.h"
#include "libs/imgui/imgui_impl_opengl3.h"

namespace benzene::opengl
{
    class Backend : public IBackend {
        public:
        Backend(const char* application_name, GLFWwindow* window);
        ~Backend();

        void frame_update(std::unordered_map<ModelId, benzene::Model*>& models);
        void end_run();

        static void glfw_window_hints(){
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
            glfwWindowHint(GLFW_SAMPLES, 4);
        }

        private:
        void framebuffer_resize_callback(int width, int height);

        void mouse_button_callback(int button, bool state){
            (void)button;
            (void)state;
        }

        void mouse_pos_callback(double x, double y){
            (void)x;
            (void)y;
        }

        void mouse_scroll_callback(double xoffset, double yoffset){
            (void)xoffset;
            (void)yoffset;
        }

        void set_fps_cap(bool enabled, size_t fps){
            (void)enabled;
            (void)fps;
        }

        void imgui_update();


        void draw_debug_window(){
            
        }

        Program prog;
        std::unordered_map<ModelId, opengl::Model> internal_models;
    };
} // namespace benzene::opengl
