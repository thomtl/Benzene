#pragma once

struct GLFWwindow; // Forward Decl

#include "../libs/imgui/imgui.h"
#include "../libs/stb/stb_image.h"
#include "../libs/tinyobjloader/tinyobjloader.h"

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
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>


namespace benzene
{
    struct Texture {
        enum class Gamut {
            Linear,
            Srgb
        };
        static const char* gamut_to_str(Gamut gamut){
            switch(gamut){
            case Gamut::Linear: return "linear";
            case Gamut::Srgb: return "srgb";
            }

            return "unknown";
        }
        static Texture load_from_file(const std::string& filename, const std::string& shader_name, Gamut gamut);
        static Texture load_from_colour(glm::vec3 colour, const std::string& shader_name);

        const std::vector<uint8_t>& bytes() const {
            return data;
        }

        std::pair<int, int> dimensions() const {
            return {width, height};
        }

        int get_channels() const {
            return channels;
        }

        const std::string& get_shader_name() const {
            return shader_name;
        }

        Gamut get_gamut() const {
            return gamut;
        }

        private:
        std::vector<uint8_t> data;
        int width, height, channels;
        std::string shader_name;
        Gamut gamut;
    };

    struct Mesh {
        struct Vertex {
            glm::vec3 pos, normal, tangent;
            glm::vec2 uv;

            bool operator==(const Vertex& other) const {
                return (pos == other.pos) && (normal == other.normal) && (tangent == other.tangent) && (uv == other.uv);
            }
        };

        struct Material {
            float shininess;
        };

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        std::vector<Texture> textures;
        Material material;
    };


    struct Model {
        void load_mesh_data_from_file(const std::string& folder, const std::string& file);
        void show_inspector();
        void update(){
            this->updated = true;
        }

        bool is_updated(){
            if(this->updated){
                this->updated = false;
                return true;
            } else {
                return false;
            }
        }

        glm::vec3 pos;
        glm::vec3 rotation;
        glm::vec3 scale;
        std::vector<Mesh> meshes;
        private:
        bool updated;
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

namespace std {
    template<>
    struct hash<benzene::Mesh::Vertex> {
        size_t operator()(benzene::Mesh::Vertex const& vertex) const {
            auto combine = []<typename T>(size_t& seed, const T a){
                seed ^= hash<T>()(a) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            };
            size_t seed = 0;
            combine(seed, vertex.pos);
            combine(seed, vertex.normal);
            combine(seed, vertex.tangent);
            combine(seed, vertex.uv);
            return seed;
        }
    };
}