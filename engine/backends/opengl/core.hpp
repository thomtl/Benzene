#pragma once

#include "base.hpp"

#include <chrono>

#include "model/model.hpp"
#include "pipeline.hpp"
#include "framebuffer.hpp"

#include "../../libs/imgui/imgui.h"
#include "libs/imgui/imgui_impl_glfw.h"
#include "libs/imgui/imgui_impl_opengl3.h"

namespace benzene::opengl {
    class Backend : public IBackend {
        public:
        Backend(const char* application_name, GLFWwindow* window);
        ~Backend();

        void frame_update(std::unordered_map<ModelId, benzene::Model*>& models, benzene::FrameData& frame_data);
        void end_run();

        static void glfw_window_hints(){
            glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4); // TODO: Figure out the maximum version
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glfwWindowHint(GLFW_SAMPLES, 4);

            if constexpr (validation)
                glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
        }

        void set_property(BackendProperties property, glm::vec4 v);

        private:
        void framebuffer_resize_callback(int width, int height);
        void imgui_update();
        void draw_debug_window();

        #pragma region Handled by ImGui backend
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

        #pragma endregion

        void set_fps_cap(bool enabled, size_t fps){
            this->fps_cap_enabled = enabled;
            this->fps_cap = fps;
        }

        IRenderer* renderer;

        bool is_wireframe, fps_cap_enabled;
        float last_frame, frame_time, fps, min_frame_time, max_frame_time;
        uint64_t fps_cap;
        size_t frame_counter;
        std::chrono::time_point<std::chrono::high_resolution_clock> last_frame_timestamp;

        std::array<float, 100> last_frame_times;

        void show_extension_window(bool &opened);
        void show_driver_info_window(bool& opened);

        bool extension_window_is_showing, driver_info_window_is_showing;
    };
} // namespace benzene::opengl
