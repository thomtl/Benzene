#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "../libs/imgui/imgui.h"

#include <functional>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>


namespace benzene
{
    struct Mesh {
        struct Vertex {
            glm::vec3 pos, colour;
        };

        std::vector<Vertex> vertices;
        std::vector<uint16_t> indices;
    };

    struct Model {
        glm::vec3 pos;
        glm::vec3 rotation;
        glm::vec3 scale;
        Mesh mesh;
    };

    using ModelId = uint64_t;

    class IBackend {
        public:
        virtual ~IBackend() {}
        virtual void framebuffer_resize_callback(int width, int height) = 0;
        virtual void mouse_button_callback(int button, bool state) = 0;
        virtual void mouse_pos_callback(double x, double y) = 0;
        virtual void mouse_scroll_callback(double xoffset, double yoffset) = 0;
        virtual void frame_update(std::unordered_map<ModelId, Model*>& models) = 0;
        virtual void end_run() = 0;
        virtual void draw_debug_window() = 0;
    };

    struct FrameData {
        bool should_exit, display_debug_window;
        IBackend* backend;
    };

    class Instance {
        public:
        Instance(const char* application_name, size_t width, size_t height);
        ~Instance();

        void run(std::function<void(FrameData&)> functor);

        ModelId add_model(Model* model);

        private:
        GLFWwindow* window;
        std::unique_ptr<IBackend> backend; 
        size_t width, height;
        struct IdGen {
            IdGen(): curr{0} {}

            uint64_t next(){
                return curr++;
            }
            private:
            uint64_t curr;
        } id_gen;
        std::unordered_map<ModelId, Model*> render_models;
    };
} // namespace benzene
