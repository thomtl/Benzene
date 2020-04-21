#pragma once

struct GLFWwindow; // Forward Decl

#include "../libs/imgui/imgui.h"
#include "../libs/stb/stb_image.h"

#include <functional>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <cstddef>
#include <cstdint>

#define GLM_FORCE_RADIANS

#ifdef BENZENE_VULKAN
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>


namespace benzene
{
    struct Mesh {
        struct Vertex {
            glm::vec3 pos, colour;
            glm::vec2 uv;
        };

        std::vector<Vertex> vertices;
        std::vector<uint16_t> indices;
    };

    struct Texture {
        static Texture load_from_file(const std::string& filename);

        const std::vector<uint8_t>& bytes(){
            return data;
        }

        std::pair<int, int> dimensions(){
            return {width, height};
        }

        private:
        std::vector<uint8_t> data;
        int width, height, channels;
    };

    struct Model {
        glm::vec3 pos;
        glm::vec3 rotation;
        glm::vec3 scale;
        Mesh mesh;
        Texture texture;
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
        virtual void set_fps_cap(bool enabled, size_t fps = 60) = 0;
        virtual void imgui_update() = 0;
    };

    struct FrameData {
        bool should_exit, display_debug_window;
    };

    class Instance {
        public:
        Instance(const char* application_name, size_t width, size_t height);
        ~Instance();

        void run(std::function<void(FrameData&)> functor);
        IBackend& get_backend(){
            return *backend;
        }

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
